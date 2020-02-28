#include "mqtt.h"
#include "mqtt_client.h"
#include "esp_log.h"

#define BROKER_HOST "192.168.8.5"
#define BROKER_PORT 1883
#define BROKER_USERNAME "***REMOVED***"
#define BROKER_PASSWORD "***REMOVED***"


static void mqtt_event_handler_cb(  void *handler_args, 
                                    esp_event_base_t base,
                                    int32_t event_id,
                                    void *event_data);

static const char *TAG = "MQTT_EXAMPLE";

void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = 
    {
        .host = BROKER_HOST,
        .port = BROKER_PORT,
        .username = BROKER_USERNAME,
        .password = BROKER_PASSWORD
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler_cb, client);
    esp_mqtt_client_start(client);
}

static void mqtt_event_handler_cb(  void *handler_args, 
                                    esp_event_base_t base,
                                    int32_t event_id,
                                    void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    /* esp_mqtt_client_handle_t client = event->client;
    int msg_id; */
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}