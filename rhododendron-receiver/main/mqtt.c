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

static void mqtt_task(void* data);
static void mqtt_init(void);
static void mqtt_publish_task(void* data);
static void mqtt_event_handler_cb(void *handler_args, esp_event_base_t base,
                                    int32_t event_id, void *event_data);

static const char *TAG = "MQTT_CLIENT";
static esp_mqtt_client_handle_t client;
static TaskHandle_t mqtt_task_handle;
static TaskHandle_t mqtt_publish_task_handle;
static QueueHandle_t mqtt_queue;

void mqtt_app_start(void)
{
    ESP_LOGI(TAG, "Creating mqtt task");
    xTaskCreate(mqtt_task, "MQTT Task", 4096, NULL, 1, &mqtt_task_handle);
}

void add_mqtt_msg(mqtt_msg_t msg)
{
    ESP_LOGI(TAG, "Got data in MQTT queue");
    xQueueSend(mqtt_queue, &msg, 0);
}

void mqtt_notify(void)
{
    xTaskNotify(mqtt_task_handle, 0, eNoAction);
}

static void mqtt_task(void* data)
{
    while (true)
    {
        if (xTaskNotifyWait(pdFALSE, pdFALSE, NULL, portMAX_DELAY))
        {
            EventBits_t wifi_state = xEventGroupGetBits(wifi_state_machine);
            EventBits_t mqtt_state = xEventGroupGetBits(mqtt_state_machine);
            if (wifi_state == WIFI_NOT_INIT && mqtt_state == MQTT_NOT_INIT)
            {
                ESP_LOGE(TAG, "Wifi is not initiated");
            }
            else if (wifi_state == WIFI_INITIATED && mqtt_state == MQTT_NOT_INIT)
            {
                ESP_LOGE(TAG, "Wifi is not connected");
            }
            else if (wifi_state == WIFI_CONNECTED && mqtt_state == MQTT_NOT_INIT)
            {
                ESP_LOGI(TAG, "Init MQTT");
                mqtt_init();
            }
            else if (wifi_state == WIFI_CONNECTED && mqtt_state == MQTT_INITIATED)
            {
                ESP_LOGI(TAG, "Connecting MQTT");
                esp_mqtt_client_start(client);
            }
            else if (wifi_state == WIFI_CONNECTED && mqtt_state == MQTT_CONNECTED)
            {
                xTaskCreate(mqtt_publish_task, "MQTT Publish task", 2048,
                            NULL, 1, &mqtt_publish_task_handle);
                xTaskNotify(led_task_handle, 0, eNoAction);
            }
            else if (wifi_state == WIFI_DISCONNECTED && mqtt_state == MQTT_CONNECTED)
            {
                ESP_LOGE(TAG, "No WiFi, stopping MQTT");
                esp_mqtt_client_stop(client);
                xEventGroupClearBits(mqtt_state_machine, MQTT_ALL_BITS);
                xEventGroupSetBits(mqtt_state_machine, MQTT_STOPPED);
                xTaskNotify(led_task_handle, 0, eNoAction);
            }
            else if (wifi_state == WIFI_CONNECTED && mqtt_state == MQTT_STOPPED)
            {
                ESP_LOGI(TAG, "WIFI reconnected, restoring MQTT");
                esp_mqtt_client_start(client);
                esp_mqtt_client_reconnect(client);
                xEventGroupClearBits(mqtt_state_machine, MQTT_ALL_BITS);
                xEventGroupSetBits(mqtt_state_machine, MQTT_RECONNECTING);
            }
            else if (wifi_state == WIFI_CONNECTED && mqtt_state == MQTT_RECONNECTED)
            {
                ESP_LOGI(TAG, "MQTT reconnected");
                xTaskNotify(led_task_handle, 1, eNoAction);
                xEventGroupClearBits(mqtt_state_machine, MQTT_ALL_BITS);
                xEventGroupSetBits(mqtt_state_machine, MQTT_CONNECTED);
            }
        }
    }
}

static void mqtt_init(void)
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
    xEventGroupClearBits(mqtt_state_machine, MQTT_ALL_BITS);
    xEventGroupSetBits(mqtt_state_machine, MQTT_INITIATED);
    mqtt_notify();
}

static void mqtt_publish_task(void* data)
{
    mqtt_msg_t buffer;
    while (true)
    {
        if (xQueueReceive(mqtt_queue, &buffer, portMAX_DELAY))
        {
            if (xEventGroupWaitBits(wifi_state_machine, WIFI_CONNECTED,
                                        pdFALSE, pdTRUE, portMAX_DELAY)
                && xEventGroupWaitBits(mqtt_state_machine, MQTT_CONNECTED,
                                        pdFALSE, pdTRUE, portMAX_DELAY))
            {
                esp_mqtt_client_publish(client, buffer.topic, buffer.payload, 0, 1, pdTRUE);
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
            EventBits_t bits = xEventGroupWaitBits(mqtt_state_machine, MQTT_INITIATED|MQTT_RECONNECTING,
                                                    pdFALSE, pdFALSE, portMAX_DELAY);
            xEventGroupClearBits(mqtt_state_machine, MQTT_ALL_BITS);
            if (bits == MQTT_INITIATED)
            {
                xEventGroupSetBits(mqtt_state_machine, MQTT_CONNECTED);
            }
            else if (bits == MQTT_RECONNECTING)
            {
                xEventGroupSetBits(mqtt_state_machine, MQTT_RECONNECTED);
            }
            mqtt_notify();
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