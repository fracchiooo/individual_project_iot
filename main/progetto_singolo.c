#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "esp_adc/adc_cali.h"
#include "read_continuous.h"
#include "fft_wrapper.h"
#include "wifi_wrapper.h"
#include "mqtt_wrapper.h"
#include "freertos/queue.h"
#define MAX_SAMPLING_FREQUENCY          SOC_ADC_SAMPLE_FREQ_THRES_HIGH

 #define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })
     
#define TIME_WINDOW 3.5

QueueHandle_t queue;

float x1[READ_LEN];
float x2[READ_LEN];
float wind[READ_LEN];
float y_cf[READ_LEN * 2];



void app_main(void)
{
     
  queue = xQueueCreate(5, sizeof(bool));
  
  xTaskCreatePinnedToCore(wifi_start_connection, "WiFi Task", 4096, queue, 0, NULL, 1);
  //wifi_start_connection(queue);
  
  adc_cali_handle_t adc1_cali_chan0_handle = NULL; //ADC calibration handle
    bool do_calibration1_chan0 = adc_calibration_init(ADC_UNIT, CHANNEL, ADC_ATTEN, &adc1_cali_chan0_handle, ADC_BIT_WIDTH);
    if(!do_calibration1_chan0){
      printf("calibration for converting raw data to MilliVolts has an error");
      return;
    }
    

//i use the SOC_ADC_SAMPLE_FREQ_THRES_HIGH instead of the max_sample_freq, because is more precise for calculating the fft, also if that frequency probabily cannot be used in mean
 uint32_t* sampled_signal  = read_and_get_data_fixed_samples(SOC_ADC_SAMPLE_FREQ_THRES_HIGH, adc1_cali_chan0_handle);
  if(sampled_signal == NULL){
    printf("the sampling function returns NULL value\n");
    return;
  }
  


  float max_freq=get_max_frequency_fft(sampled_signal, READ_LEN, SOC_ADC_SAMPLE_FREQ_THRES_HIGH);
  free(sampled_signal);
  
  uint32_t new_sampling_freq=2* ((uint32_t)max_freq); //approximation to uint32

  printf("the sampling frequency result of fft*2: %ld\n", new_sampling_freq);

//uint32_t new_sampling_freq=SOC_ADC_SAMPLE_FREQ_THRES_HIGH;

  uint32_t mean= get_mean_window_time(TIME_WINDOW, adc1_cali_chan0_handle, new_sampling_freq);

  adc_calibration_deinit(adc1_cali_chan0_handle); // deinit of the calibration

  printf("the mean is %ld\n", mean);
  
    // connecting the esp to the broker
  esp_mqtt_client_handle_t client= mqtt_app_start("mqtts://cb8e2462247645a3823484e7851a5d68.s1.eu.hivemq.cloud", "object1", "Progettoiot1", queue);



  // sending the mean to the broker
  char mess[sizeof(uint32_t)];
  sprintf(mess, "%ld", mean);
  int msg_id=mqtt_publish_message(client, mess, "mean", 1);
  printf("sent message to broker, msg_id=%d\n", msg_id);
  
  
  //deinit wifi, mqtt and the deallocating the queue
  disconnect_mqtt_client(client);
  disconnect_wifi(); 
  vQueueDelete(queue);
  
  return;
    
    ////////////////////////////////////////////////////////////
 
}

