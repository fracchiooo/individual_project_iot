#ifndef FFT_WRAPPER_H
#define FFT_WRAPPER_H

#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "esp_dsp.h"
#include <math.h>
#define READ_LEN CONFIG_READ_LEN

__attribute__((aligned(16)))
extern float x1[READ_LEN];
__attribute__((aligned(16)))
extern float x2[READ_LEN];
// Window coefficients
__attribute__((aligned(16)))
extern float wind[READ_LEN];
// working complex array
__attribute__((aligned(16)))
extern float y_cf[READ_LEN * 2];
// Pointer to result arrays
extern float *y1_cf = &y_cf[0];




void normalize(float array[], int N, uint32_t max_val) {
    // Define the minimum and maximum values in the original range
    float min_value = 0.0;
    float max_value = (float)(max_val*1.0f);

    // Define the minimum and maximum values in the target range
    float target_min = -0.99;
    float target_max = 0.99;

    // Calculate the scaling factor and shift
    float scale = (target_max - target_min) / (max_value - min_value);
    float shift = target_min - scale * min_value;

    // Apply normalization to each element in the array
    for (int i = 0; i < N; i++) {
        array[i] = scale * array[i] + shift;
    }
}


void std_deviation_and_mean(float* data, size_t size, float* std_dev, float* mean){

  float sum = 0.0f;
  float quad_sum = 0.0f;
  
  for(int i=0; i< size; i++){
    sum += data[i];
    quad_sum += pow(data[i], 2.0f);
  }
  
  *mean = sum/ size;
  float variance = (quad_sum / size) - pow(*mean, 2.0f);
  *std_dev = sqrtf(variance);
  return;
}


float get_max_frequency_fft(uint32_t* samples, size_t N, uint32_t sample_frequency, uint32_t max_value){

    esp_err_t ret;
    ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    if (ret  != ESP_OK)
    {
        printf("Not possible to initialize FFT. Error = %i", ret);
        return 0.0f;
    }
    
   // Generate hann window
    dsps_wind_hann_f32(wind, N);

    
    for(int j=0; j<N; j++){
    
      x1[j]=samples[j]*1.0f;
    
    }

    


    x1[0]=x1[1];
    normalize(x1, N, max_value);
 

    for (int i=0 ; i< N ; i++)
    {
        y_cf[i*2 + 0] = x1[i] * wind[i]; // Real part is your signal multiply with window
        y_cf[i*2 + 1] = 0; // Imag part is 0
    }


    dsps_fft2r_fc32(y_cf, N);
    // Bit reverse 
    dsps_bit_rev_fc32(y_cf, N);
    // Convert one complex vector to two complex vectors
    dsps_cplx2reC_fc32(y_cf, N);

     for (int i = 0 ; i < N/2 ; i++) {
     
      y1_cf[i]=sqrtf(pow(y1_cf[i * 2 + 0],2.0f) + pow(y1_cf[i * 2 + 1],2.0f));
     
     }
     
     float std_dev;
     float mean;
     
     std_deviation_and_mean(y1_cf, N/2, &std_dev, &mean);     
     float threshold = 3.0f;
     
     int idx=0;
     
     
     for(int j=N/2; j>=0; j--){
      float dat = y1_cf[j];
      float z= (dat - mean) / std_dev;
      if(z > threshold){
        idx=j;
        break;
      }
     }
     
    
    // Show power spectrum in 64x10 window from -60 to 0 dB from 0..N/2 samples
    //printf("Signal x1 in log scale");
    //dsps_view(y1_cf, N/2, 64, 10,  -60, 40, '|');
    printf("Signal x1 in absolute scale");
    dsps_view(y1_cf, N/2, 64, 10,  0, 2, '|');


    float max_frequency=(idx*sample_frequency)*1.0f/N*1.0f;

    
    printf("the max frequency of the signal is: %f\n", max_frequency);

    
  return max_frequency;
}


#endif
