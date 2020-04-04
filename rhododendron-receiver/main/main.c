#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "wifi.h"
#include "mqtt.h"
#include "ble.h"
#include "global.h"

#define MIN_VALUE_MOISTURE 0
#define MAX_VALUE_MOISTURE 330

static states_t program_state;
static TimerHandle_t ble_failed_timer;

static const char *PROGRAM_TAG = "PROGRAM";

static TaskHandle_t main_task_handle;
static TaskHandle_t program_task_handle;
static QueueHandle_t states_queue;
static EventGroupHandle_t program_event_group;
#define ACTIVE_BIT BIT0

static void main_task(void *data);
static void program_task(void *data);
static void program_procedure(void);
static int convert_value(int value);
static esp_err_t user_gpio_config();
static void ble_timer_callback(TimerHandle_t timer);

#define WIFI_LED GPIO_NUM_21
#define BLE_LED GPIO_NUM_23

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
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	program_state = PROGRAM_START;
	ESP_ERROR_CHECK(user_gpio_config());
	ble_failed_timer = xTimerCreate("BLE timer",
									30000 / portTICK_PERIOD_MS,
									pdFALSE,
									(void *)1,
									ble_timer_callback);
	xTaskCreate(main_task, "MAIN task", 4096, NULL, 1, &main_task_handle);
	states_queue = xQueueCreate(10, sizeof(states_t));
	program_event_group = xEventGroupCreate();
	xTaskCreate(program_task, "PROGRAM task", 4096, NULL, 1, &program_task_handle);
	set_program_state(WIFI_INIT);
}

void main_task(void *data)
{
	while (true)
	{
		if (xQueueReceive(states_queue, &program_state, portMAX_DELAY))
		{
			switch (get_program_state())
			{
			case PROGRAM_START:
				break;
			case WIFI_INIT:
				wifi_init();
				break;
			case WIFI_FAILED:
				gpio_set_level(WIFI_LED, 1);
				break;
			case MQTT_INIT:
				gpio_set_level(WIFI_LED, 0);
				mqtt_init();
				break;
			case BLE_INIT:
				//TODO add timer that switch off scanning
				//after 30 sec
				xTimerStart(ble_failed_timer, 2000 / portTICK_PERIOD_MS);
				ble_init();
				//xTimerStop(ble_failed_timer, 2000 / portTICK_PERIOD_MS);
				//xTimerDelete(ble_failed_timer, 2000 / portTICK_PERIOD_MS);
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
				gpio_set_level(WIFI_LED, 1);
				break;
			case WIFI_RECONNECTED:
				mqtt_reopen();
				gpio_set_level(WIFI_LED, 0);
				break;
			}
		}
	}
}

static void program_task(void *data)
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
	if (ble_value != -1)
	{
		gpio_set_level(BLE_LED, 0);
		int moisture = convert_value(ble_value);
		ESP_LOGI(PROGRAM_TAG, "Value: %d Moisture: %d%%", ble_value, moisture);
		mqtt_msg_t msg;
		strcpy(msg.topic, "home/garden/rhododendrons/moisture");
		sprintf(msg.payload, "%d", moisture);
		add_mqtt_msg(msg);
	}
	else
	{
		gpio_set_level(BLE_LED, 1);
	}
}

static int convert_value(int value)
{
	return (value - MIN_VALUE_MOISTURE) * 100 / (MAX_VALUE_MOISTURE - MIN_VALUE_MOISTURE);
}

static esp_err_t user_gpio_config()
{
	const uint64_t PIN_BIT_MASK = ((1ULL << WIFI_LED) | (1ULL << BLE_LED));
	gpio_config_t config;
	config.pin_bit_mask = PIN_BIT_MASK;
	config.intr_type = GPIO_INTR_DISABLE;
	config.mode = GPIO_MODE_OUTPUT;
	config.pull_up_en = GPIO_PULLUP_DISABLE;
	config.pull_down_en = GPIO_PULLDOWN_DISABLE;

	return gpio_config(&config);
}

static void ble_timer_callback(TimerHandle_t timer)
{
	ESP_LOGI("BLE", "BLE failed");
	gpio_set_level(BLE_LED, 1);
	//xTimerStop(ble_failed_timer, 2000 / portTICK_PERIOD_MS);
}