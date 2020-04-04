#include "mqtt.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "global.h"
#include "secrets.h"

#define LWT_TOPIC "home/garden/rhododendrons/state"
#define LWT_MSG "offline"

static void mqtt_publish_task(void *data);
static void mqtt_event_handler_cb(void *handler_args, esp_event_base_t base,
                                  int32_t event_id, void *event_data);

static const char *TAG = "MQTT_CLIENT";
static esp_mqtt_client_handle_t client;
static TaskHandle_t mqtt_publish_task_handle;
static QueueHandle_t mqtt_queue;
static EventGroupHandle_t mqtt_event_group;
#define REOPEN_BIT BIT0

void mqtt_init(void)
{
    esp_mqtt_client_config_t mqtt_cfg =
        {
            .host = BROKER_HOST,
            .port = BROKER_PORT,
            .username = BROKER_USERNAME,
            .password = BROKER_PASSWORD,
            .lwt_topic = LWT_TOPIC,
            .lwt_msg = LWT_MSG,
            .lwt_qos = 1,
            .lwt_retain = pdTRUE,
        };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler_cb, NULL);
    mqtt_queue = xQueueCreate(10, sizeof(mqtt_msg_t));
    xTaskCreate(mqtt_publish_task, "MQTT Publish Task", 4096, NULL, 1, mqtt_publish_task_handle);
    esp_mqtt_client_start(client);
    mqtt_event_group = xEventGroupCreate();
}

void add_mqtt_msg(mqtt_msg_t msg)
{
    ESP_LOGI(TAG, "Got data in MQTT queue");
    xQueueSend(mqtt_queue, &msg, 0);
}

void mqtt_stop(void)
{
    ESP_LOGI(TAG, "Stopping MQTT client");
    esp_mqtt_client_stop(client);
}

void mqtt_reopen(void)
{
    ESP_LOGI(TAG, "Reconnecting MQTT client");
    esp_mqtt_client_start(client);
}

static void mqtt_publish_task(void *data)
{
    mqtt_msg_t buffer;
    while (true)
    {
        if (xQueueReceive(mqtt_queue, &buffer, portMAX_DELAY))
        {
            while (get_program_state() != ACTIVE)
            {
                vTaskDelay(5000 / portTICK_PERIOD_MS);
            }
            esp_mqtt_client_publish(client, buffer.topic, buffer.payload, 0, 1, pdTRUE);
        }
    }
}

static void mqtt_event_handler_cb(void *handler_args,
                                  esp_event_base_t base,
                                  int32_t event_id,
                                  void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
    {
        states_t state = get_program_state();
        if (state == MQTT_INIT)
        {
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            esp_mqtt_client_publish(client, "home/garden/rhododendrons/state", "online", 0, 1, pdTRUE);
            set_program_state(BLE_INIT);
        }
        else if (state == WIFI_RECONNECTED)
        {
            esp_mqtt_client_publish(client, "home/garden/rhododendrons/state", "online", 0, 1, pdTRUE);
            set_program_state(ACTIVE);
        }
        break;
    }
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