#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "wifi.h"
#include "mqtt.h"
#include "ble.h"
#include "global.h"

#define MIN_VALUE_MOISTURE 0
#define MAX_VALUE_MOISTURE 330

static states_t program_state;

static const char* LED_TAG = "LED";
static const char* PROGRAM_TAG = "PROGRAM";

static TaskHandle_t main_task_handle;
static TaskHandle_t program_task_handle;
static QueueHandle_t states_queue;
static EventGroupHandle_t program_event_group;
#define ACTIVE_BIT BIT0

static void main_task(void* data);
static void program_task(void* data);
static void program_procedure(void);
static int convert_value(int value);
void led_task(void* data);

void set_program_state(states_t state)
{
	ESP_LOGI(PROGRAM_TAG, "Setting state: %d", state);	
	xQueueSend(states_queue, &state, 5000 / portTICK_PERIOD_MS);
}

states_t get_program_state(void)
{
	return program_state;
}

void app_main()
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    program_state = PROGRAM_START;
	xTaskCreate(main_task, "MAIN task", 4096, NULL, 1, &main_task_handle);
	states_queue = xQueueCreate(10, sizeof(states_t));
	program_event_group = xEventGroupCreate();
	xTaskCreate(program_task, "PROGRAM task", 4096, NULL, 1, &program_task_handle);
	set_program_state(WIFI_INIT);
	//xTaskCreate(led_task, "LED task", 2048, NULL, 1, &led_task_handle);
}

void main_task(void* data)
{
	while (true)
	{
		if(xQueueReceive(states_queue, &program_state, portMAX_DELAY))
		{
			switch (get_program_state())
			{
				case PROGRAM_START:
					break;
				case WIFI_INIT:
					wifi_init();
					break;
				case WIFI_FAILED:
					//TODO turn wifi led on
					break;
				case MQTT_INIT:
					//TODO turn wifi led off
					mqtt_init();
					break;
				case BLE_INIT:
					ble_init();
					break;
				case BLE_INITIATED:
					vTaskDelay(1000 / portTICK_PERIOD_MS);
					ESP_LOGI(PROGRAM_TAG, "Program active");
					ble_close_connection();
					break;
				case ACTIVE:
					xEventGroupSetBits(program_event_group, ACTIVE_BIT);
					break;
				case WIFI_RECONNECT:
					xEventGroupClearBits(program_event_group, ACTIVE_BIT);
					mqtt_stop();
					//TODO turn wifi led on
					break;
				case WIFI_RECONNECTED:
					mqtt_reopen();
					//TODO turn wifi led off
					break;
				case MQTT_REOPEN:
					//TODO
					break;
			}
		}
	}
}

void led_task(void* data)
{
	uint32_t notification_value = 0;
	while (true)
	{
		if (xTaskNotifyWait(pdFALSE, pdFALSE, &notification_value, portMAX_DELAY) == pdTRUE)
		{
			if (notification_value == 1)
			{
				ESP_LOGI(LED_TAG, "Turned the led on");
				//TODO turn led on
			}
			else
			{
				ESP_LOGI(LED_TAG, "Turned the led off");
				//TODO turn led on
			}
		}
	}
}

static void program_task(void* data)
{
	while (true)
	{
		if (xEventGroupWaitBits(program_event_group, ACTIVE_BIT, pdFALSE, pdTRUE, portMAX_DELAY))
		{
			program_procedure();
			//vTaskDelay(21600000 / portTICK_PERIOD_MS);
			vTaskDelay(60000 / portTICK_PERIOD_MS);
		}
	}
}

static void program_procedure(void)
{
	int ble_value = ble_get_data();
	int moisture = convert_value(ble_value);
	ESP_LOGI(PROGRAM_TAG, "Value: %d Moisture: %d%%", ble_value, moisture);
	mqtt_msg_t msg;
	strcpy(msg.topic, "home/garden/rhododendrons/moisture");
	sprintf(msg.payload, "%d", moisture);
	add_mqtt_msg(msg);
}

static int convert_value(int value)
{
	return (value - MIN_VALUE_MOISTURE) * 100 / (MAX_VALUE_MOISTURE - MIN_VALUE_MOISTURE);
}