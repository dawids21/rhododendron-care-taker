#include "mqtt.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "global.h"

#define BROKER_HOST "192.168.8.5"
#define BROKER_PORT 1883
#define BROKER_USERNAME "***REMOVED***"
#define BROKER_PASSWORD "***REMOVED***"

static void mqtt_publish_task(void* data);
static void mqtt_event_handler_cb(void *handler_args, esp_event_base_t base,
                                    int32_t event_id, void *event_data);

static const char *TAG = "MQTT_CLIENT";
static esp_mqtt_client_handle_t client;
static TaskHandle_t mqtt_publish_task_handle;
static QueueHandle_t mqtt_queue;

void mqtt_init(void)
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
    xTaskCreate(mqtt_publish_task, "MQTT Publish Task", 4096, NULL, 1, mqtt_publish_task_handle);
    esp_mqtt_client_start(client);
}

void add_mqtt_msg(mqtt_msg_t msg)
{
    ESP_LOGI(TAG, "Got data in MQTT queue");
    xQueueSend(mqtt_queue, &msg, 0);
}

static void mqtt_publish_task(void* data)
{
    mqtt_msg_t buffer;
    while (true)
    {
        if (xQueueReceive(mqtt_queue, &buffer, portMAX_DELAY))
        {
            if(get_program_state() == ACTIVE)
            {
                esp_mqtt_client_publish(client, buffer.topic, buffer.payload, 0, 1, pdTRUE);
            }
            else
            {
                vTaskDelay(5000);
            }
        }
    }
}

static void mqtt_event_handler_cb(  void *handler_args, 
                                    esp_event_base_t base,
                                    int32_t event_id,
                                    void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            set_program_state(BLE_INIT);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}