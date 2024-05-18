#ifndef MQTT_WRAPPER_H
#define MQTT_WRAPPER_H
#include "mqtt_client.h"
static const char *TAG_mqtt = "mqtt_example";


/*extern const uint8_t client_cert_start[] asm("_binary_object_project_cert_pem_start");
extern const uint8_t client_cert_end[] asm("_binary_object_project_cert_pem_end");
extern const uint8_t client_key_start[] asm("_binary_object_project_private_key_start");
extern const uint8_t client_key_end[] asm("_binary_object_project_private_key_end");
extern const uint8_t server_cert_start[] asm("_binary_root_CA_crt_start");
extern const uint8_t server_cert_end[] asm("_binary_root_CA_crt_end");
*/

extern const uint8_t server_cert_start[] asm("_binary_isrgrootx1_pem_start");
extern const uint8_t server_cert_end[] asm("_binary_isrgrootx1_pem_end");



static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG_mqtt, "Last error %s: 0x%x", message, error_code);
    }
}


static int mqtt_publish_message(esp_mqtt_client_handle_t client, char* message, char* topic, int qos){
  int msg_id;
  msg_id = esp_mqtt_client_publish(client, topic, message, 0, qos, 0);
  ESP_LOGI(TAG_mqtt, "sent publish successful, msg_id=%d", msg_id);
  return msg_id;
}


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG_mqtt, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    //esp_mqtt_client_handle_t client = event->client;
    int msg_id=-1000;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
    //una volta connessso al broker mi sottoscrivo al topic "mean" con qos 1
        ESP_LOGI(TAG_mqtt, "MQTT_EVENT_CONNECTED");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG_mqtt, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG_mqtt, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        //msg_id = esp_mqtt_client_publish(client, "mean", "mi sono sottoscritto eh", 0, 1, 0);
        ESP_LOGI(TAG_mqtt, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG_mqtt, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG_mqtt, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG_mqtt, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG_mqtt, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG_mqtt, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG_mqtt, "Other event id:%d", event->event_id);
        break;
    }
}


static esp_mqtt_client_handle_t mqtt_app_start(char* broker_url, char* username, char* password, QueueHandle_t queue){

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = broker_url,
        .broker.address.port = 8883,
        .broker.verification.certificate=(const char *)server_cert_start,
        .credentials.username = username,
        .credentials.authentication.password= password,
    };
    
    bool value;
    
    while(1){
      if(xQueueReceive(queue, &value, (TickType_t)5)){
        if(value==true){
          printf("mqtt could proceed, connection established and ip address received\n");
          fflush(stdout);
          break;
        }
      }
    
      vTaskDelay(400/ portTICK_PERIOD_MS);
    }
    
    
  

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
    
    return client;
}


#endif
