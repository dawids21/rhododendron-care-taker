#include "mqtt.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define BROKER_HOST "192.168.8.5"
#define BROKER_PORT 1883
#define BROKER_USERNAME "***REMOVED***"
#define BROKER_PASSWORD "***REMOVED***"

static void mqtt_init_task(void* data);
static void mqtt_event_handler_cb(  void *handler_args, 
                                    esp_event_base_t base,
                                    int32_t event_id,
                                    void *event_data);

static const char *TAG = "MQTT_CLIENT";
static esp_mqtt_client_handle_t client;
static TaskHandle_t mqtt_init;
static QueueHandle_t mqtt_queue;

void mqtt_app_start(void)
{
    ESP_LOGI(TAG, "Creating app init task");
    xTaskCreate(mqtt_init_task, "MQTT Init Task", 2048, NULL, 5, &mqtt_init);
}

static void mqtt_init_task(void* data)
{
    while (true)
    {
        if (xTaskNotifyWait(0, 0, 0, portMAX_DELAY) == pdPASS)
        {
            esp_mqtt_client_config_t mqtt_cfg = 
            {
                .host = BROKER_HOST,
                .port = BROKER_PORT,
                .username = BROKER_USERNAME,
                .password = BROKER_PASSWORD
            };
            client = esp_mqtt_client_init(&mqtt_cfg);
            esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler_cb, NULL);
            mqtt_queue = xQueueCreate(10, sizeof(mqtt_msg_t));
            esp_mqtt_client_start(client);
            vTaskDelete(NULL);
        }
    }
}

void mqtt_task_notify()
{
    ESP_LOGI(TAG, "Get notify from WiFi to MQTT init");
    xTaskNotify(mqtt_init, 0, eNoAction);
}

void add_mqtt_msg(mqtt_msg_t msg)
{
    ESP_LOGI(TAG, "Get data to MQTT queue");
    xQueueSend(mqtt_queue, msg, 0);
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