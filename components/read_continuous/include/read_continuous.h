#ifndef READ_CONTINUOUS_H
#define READ_CONTINUOUS_H

#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_adc/adc_continuous.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_timer.h"
#include <rom/ets_sys.h>
#include "esp_heap_caps.h"
#include <math.h>
#define MAX_SAMPLING_FREQUENCY          SOC_ADC_SAMPLE_FREQ_THRES_HIGH

#define ADC_UNIT                    ADC_UNIT_1
#define ADC_ATTEN                   ADC_ATTEN_DB_12
#define ADC_BIT_WIDTH               SOC_ADC_DIGI_MAX_BITWIDTH
#define ADC_OUTPUT_TYPE             ADC_DIGI_OUTPUT_FORMAT_TYPE2
#define READ_LEN                    CONFIG_READ_LEN
#define CHANNEL                     ADC_CHANNEL_0
#define ADC_CONV_MODE               ADC_CONV_SINGLE_UNIT_1


 #define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })


#define ADC_GET_CHANNEL(p_data)     ((p_data)->type2.channel)
#define ADC_GET_DATA(p_data)        ((p_data)->type2.data)


adc_oneshot_unit_handle_t configure_read_oneshot();
static TaskHandle_t s_task_handle;
  

static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    BaseType_t mustYield = pdFALSE;
    //Notify that ADC continuous driver has done enough number of conversions
    vTaskNotifyGiveFromISR(s_task_handle, &mustYield);

    return (mustYield == pdTRUE);
}


uint32_t get_time_window_max_available_frequency(float time_interval_s, uint32_t* time_for_read_us){

    adc_oneshot_unit_handle_t handle= configure_read_oneshot();

    printf("the max sampling frequency due to adc limitations is %d\n", MAX_SAMPLING_FREQUENCY);

    uint32_t free_size=heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    printf("the free size of the largest block in heap is %ld (bytes)\n", free_size);
    uint32_t max_freq_memory=(uint32_t) ((free_size/2)*1.0f/time_interval_s);
    printf("the max sampling frequency due to memory limitations is %ld\n", max_freq_memory);
    int tempo;
    
    uint64_t m=0;
    ESP_ERROR_CHECK(adc_oneshot_read(handle, CHANNEL, &tempo)); //the first takes longer time
    
    int num_average=20;
    
    for(int i=0; i<num_average; i++){
    uint64_t a=esp_timer_get_time();
    ESP_ERROR_CHECK(adc_oneshot_read(handle, CHANNEL, &tempo));
    uint64_t aa=esp_timer_get_time();
    m= m + (aa-a);
    }
    
    if(time_for_read_us!=NULL){
        *time_for_read_us=(uint32_t) m / num_average;
    }

    uint32_t max_freq_read= 1000000/(*time_for_read_us);
    
    printf("the max sampling frequency due to read time limitations is %ld\n", max_freq_read);
    
    uint32_t min_temp= min((uint32_t) MAX_SAMPLING_FREQUENCY, max_freq_memory);
    ESP_ERROR_CHECK(adc_oneshot_del_unit(handle));
    
    return min(max_freq_read, min_temp);
    
    
}

void print_array_uint16(){}


float exp_weigh_moving_avg_filter(uint16_t* data, size_t t, adc_cali_handle_t calibration){
  
  int i=1;
  float alpha=0.5f;
  
  float mean=0.0f;
  
  int temp_value;
  
  while(i<=t){
    
    uint32_t d= (uint32_t) data[i-1];
  
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(calibration, d, &temp_value));
  
    mean+= pow(alpha, t-i)* (1-alpha) * temp_value;
  
    i++;
  }
  
  return mean;


}




uint32_t get_mean_window_time(float time_interval_s, adc_cali_handle_t adc1_cali_chan0_handle, uint32_t new_sampling_freq){

    uint32_t num_to_del;
    
    uint32_t max_sample_freq = get_time_window_max_available_frequency(time_interval_s, &num_to_del); // I obtain the bottleneck contraint max frequency
    
    printf("the bottleneck frequency is %ld\n", max_sample_freq);
      
    uint32_t effective_freq=min(max_sample_freq, new_sampling_freq);
    if(effective_freq!=new_sampling_freq) ESP_LOGI("READ CONTINUOUS", "sampling with a lower frequency wrt requested!");
    
    printf("I am sampling at frequency %ld for calculating the mean\n", effective_freq);
      
  
    uint32_t sampl_period_us=(uint32_t) (1000000.0f/(effective_freq*1.0f)) - num_to_del;
    if(sampl_period_us<0){
      sampl_period_us=0;
      printf("sampl_period_us is %ld\n", sampl_period_us);
      // this could happen if it is using maximum read time frequency and there is an error in float approximation
    }
    
    int num_samples = effective_freq*time_interval_s;
    
    
    printf("the number of samples I am trying to allocate is %d\n", num_samples);


    uint16_t* raw_values=(uint16_t*) malloc(num_samples*sizeof(uint16_t));
    
    if(raw_values==NULL){
      printf("unable to calculate mean in a time window: memory issue\n");
      return -1;
    }
    
    adc_oneshot_unit_handle_t handle= configure_read_oneshot();
    
    int i=0;
    uint64_t time_really_past1=esp_timer_get_time();
    while(i<num_samples){
    ESP_ERROR_CHECK(adc_oneshot_read(handle, CHANNEL, &raw_values[i]));
    i++;
    ets_delay_us(sampl_period_us); 
    }
    int64_t time_really_past2=esp_timer_get_time();
    

    
    printf(" il tempo veramente passato a samplare è: %lld\n", time_really_past2-time_really_past1);
    fflush(stdout);
    
    float result = exp_weigh_moving_avg_filter(raw_values, num_samples, adc1_cali_chan0_handle);
    
    free(raw_values);
    ESP_ERROR_CHECK(adc_oneshot_del_unit(handle));
    return result;
}


adc_oneshot_unit_handle_t configure_read_oneshot(){

  adc_oneshot_unit_handle_t adc1_handle;
  adc_oneshot_unit_init_cfg_t init_config1 = {
      .unit_id = ADC_UNIT,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

  //-------------ADC1 Config---------------//
  adc_oneshot_chan_cfg_t config = {
      .bitwidth = ADC_BIT_WIDTH,
      .atten = ADC_ATTEN,
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, CHANNEL, &config));

  
  return adc1_handle;

}





adc_continuous_handle_t configure_read_continuous(uint32_t max_store_buf_size, uint32_t conv_frame_size, uint32_t sample_freq_hz){
  adc_continuous_handle_t handle =NULL;

  adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = max_store_buf_size,
        .conv_frame_size = conv_frame_size,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));
    adc_digi_pattern_config_t adc_pattern = {
      .atten= ADC_ATTEN,
      .channel= CHANNEL,
      .unit= ADC_UNIT,
      .bit_width= ADC_BIT_WIDTH,
    };

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = sample_freq_hz,
        .conv_mode = ADC_CONV_MODE,
        .format = ADC_OUTPUT_TYPE,
        .adc_pattern=&adc_pattern,
        .pattern_num=1,
    };

    ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));
    
    ////////////////////////////////////////////
    s_task_handle = xTaskGetCurrentTaskHandle();
    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = s_conv_done_cb,
    };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(handle, &cbs, NULL));
    
    
    
    ///////////////////////////////////////////////
    return handle;

}




uint32_t* read_and_get_data_fixed_samples(uint32_t sampling_freq, adc_cali_handle_t adc1_cali_chan0_handle){


  int array_lenght= 32/8*READ_LEN; // each sample is saved in 4 bytes
  
  uint32_t* converted_result= (uint32_t*) calloc(READ_LEN, 32); // array with the samples
  
  adc_continuous_handle_t handle = configure_read_continuous(array_lenght, array_lenght, sampling_freq);

  ESP_ERROR_CHECK(adc_continuous_start(handle));




  esp_err_t ret;
  uint32_t ret_num = 0;
  uint8_t result[array_lenght];
  memset(result, 0x00, array_lenght);
  while(1){
      ret = adc_continuous_read(handle, result, array_lenght, &ret_num, 0);
      if (ret == ESP_OK) {

        int ii=0;
        for (int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES) {
            adc_digi_output_data_t *p = (adc_digi_output_data_t*)&result[i];
            uint32_t chan_num = ADC_GET_CHANNEL(p);
            uint32_t data = ADC_GET_DATA(p);
            if (chan_num < SOC_ADC_CHANNEL_NUM(unit)) {
              ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan0_handle, data, &converted_result[ii])); //converts raw data to calibrated voltage


            } else {
              printf("error in reading sampled raw data");
            }
            ii++;
        }
      
      vTaskDelay(1);
      break;
      } else if (ret == ESP_ERR_TIMEOUT) {
              printf(" err esp timeout\n");
              vTaskDelay(10);
              continue;
      }
  }
  
  
  ESP_ERROR_CHECK(adc_continuous_stop(handle));
  ESP_ERROR_CHECK(adc_continuous_deinit(handle));

  
  return converted_result;
}  


/*---------------------------------------------------------------
        ADC Calibration
---------------------------------------------------------------*/
static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle, uint8_t bitWidth)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
//this is supported by esp32S3
//printf("ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED \n");
    if (!calibrated) {
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = ADC_UNIT,
            .chan = CHANNEL,
            .atten = ADC_ATTEN,
            .bitwidth = ADC_BIT_WIDTH,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
//printf("ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED \n");
    if (!calibrated) {
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = ADC_UNIT,
            .atten = ADC_ATTEN,
            .bitwidth = ADC_BIT_WIDTH,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
    } else {
    }

    return calibrated;
}

static void adc_calibration_deinit(adc_cali_handle_t handle)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}

#endif
