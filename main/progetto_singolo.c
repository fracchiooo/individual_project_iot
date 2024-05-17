#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_adc/adc_continuous.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "read_continuous.h"
#include "fft_wrapper.h"
#include <math.h>
#include "wifi_wrapper.h"
#include "mqtt_wrapper.h"
#include "freertos/queue.h"
#define ADC_UNIT                    ADC_UNIT_1
#define ADC_ATTEN                   ADC_ATTEN_DB_12
#define ADC_BIT_WIDTH               SOC_ADC_DIGI_MAX_BITWIDTH
#define ADC_OUTPUT_TYPE             ADC_DIGI_OUTPUT_FORMAT_TYPE2
#define READ_LEN CONFIG_READ_LEN
#define CHANNEL                     ADC_CHANNEL_0
#define ADC_CONV_MODE               ADC_CONV_SINGLE_UNIT_1
#define SAMPLING_FREQUENCY          SOC_ADC_SAMPLE_FREQ_THRES_HIGH

QueueHandle_t queue;

float x1[READ_LEN];
float x2[READ_LEN];
float wind[READ_LEN];
float y_cf[READ_LEN * 2];


void print_array_int(uint32_t* array, size_t size){
  printf("[");
  for(int i=0;i<size; i++){
      printf("%ld, ", array[i]);
  }
  printf("]");
  printf("\n");
}

void print_array_float(float* array, size_t size){
  printf("[");
  for(int i=0;i<size; i++){
      printf("%f, ", array[i]);
  }
  printf("]");
  printf("\n\n\n");
}

void print_array_double(double* array, size_t size){
  for(int i=0;i<size; i++){
      printf("%f - ", array[i]);
  }
  printf("\n");
}




uint32_t do_read_mean_stuff(uint32_t sampling_freq, float time_int_seconds){

  adc_oneshot_unit_handle_t handle= configure_read_oneshot(ADC_ATTEN, CHANNEL, ADC_UNIT, ADC_BIT_WIDTH);

        //-------------ADC1 Calibration Init---------------//
    adc_cali_handle_t adc1_cali_chan0_handle = NULL; //ADC calibration handle
    bool do_calibration1_chan0 = adc_calibration_init(ADC_UNIT, CHANNEL, ADC_ATTEN, &adc1_cali_chan0_handle, ADC_BIT_WIDTH);
    if(!do_calibration1_chan0){
      printf("calibration for converting raw data to MilliVolts has an error");
      return 0;
    }
    
    uint32_t mean= get_mean_window_time(handle, time_int_seconds, adc1_cali_chan0_handle, CHANNEL, sampling_freq);
    
    ESP_ERROR_CHECK(adc_oneshot_del_unit(handle));

    adc_calibration_deinit(adc1_cali_chan0_handle);
    
   return mean; 
  
}





uint32_t* do_read_continuous_stuff(uint32_t sampling_freq){
  printf(" sampling frequence is: %ld and bit width is: %d\n\n",sampling_freq, ADC_BIT_WIDTH);
  fflush(stdout);
  int array_lenght= 32/8*READ_LEN;
  uint32_t* converted_result= (uint32_t*) calloc(READ_LEN, 32);
  adc_continuous_handle_t handle = configure_read_continuous(4*READ_LEN,array_lenght, ADC_ATTEN, CHANNEL, ADC_UNIT, ADC_BIT_WIDTH, sampling_freq, ADC_CONV_MODE, ADC_OUTPUT_TYPE);
      //-------------ADC1 Calibration Init---------------//
  adc_cali_handle_t adc1_cali_chan0_handle = NULL; //ADC calibration handle
  bool do_calibration1_chan0 = adc_calibration_init(ADC_UNIT, CHANNEL, ADC_ATTEN, &adc1_cali_chan0_handle, ADC_BIT_WIDTH);
  if(!do_calibration1_chan0){
    printf("calibration for converting raw data to MilliVolts has an error");
    free(converted_result);
    return NULL;
  }
  ESP_ERROR_CHECK(adc_continuous_start(handle));
  read_and_get_data_fixed_samples(handle, array_lenght, converted_result, adc1_cali_chan0_handle, ADC_UNIT);
  //print_array_int(converted_result, READ_LEN);

  ESP_ERROR_CHECK(adc_continuous_stop(handle));
  ESP_ERROR_CHECK(adc_continuous_deinit(handle));
  adc_calibration_deinit(adc1_cali_chan0_handle);
  return converted_result;
}






void app_main(void)
{
     
  queue = xQueueCreate(5, sizeof(bool));

     
  xTaskCreatePinnedToCore(wifi_start_connection, "WiFi Task", 4096, queue, 0, NULL, 1);

  uint32_t* sampled_signal=do_read_continuous_stuff(SAMPLING_FREQUENCY);
  if(sampled_signal==NULL){
    printf("the sampling function returns NULL value\n");
    return;
  }
  

  //print_array_int(sampled_signal, READ_LEN);



  float max_freq=get_max_frequency_fft(sampled_signal, READ_LEN, SAMPLING_FREQUENCY);
  float new_sampling_freq=2*max_freq;
  free(sampled_signal);
  
  printf("the sampling frequency to use is: %f\n", new_sampling_freq);
  

  uint32_t mean = do_read_mean_stuff(new_sampling_freq, 3.5);



  printf("the mean is %ld\n", mean);
  

    // connecting the esp to the broker
    esp_mqtt_client_handle_t client= mqtt_app_start("mqtts://cb8e2462247645a3823484e7851a5d68.s1.eu.hivemq.cloud", "object1", "Progettoiot1", queue);


    
    // sending the mean to the broker
    char mess[sizeof(uint32_t)];
    sprintf(mess, "%ld", mean);
    int msg_id=mqtt_publish_message(client, mess, "mean", 1);
    
    ////////////////////////////////////////////////////////////
 
}

