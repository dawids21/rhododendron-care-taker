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
#include "global.h"

void led_task(void* data);

EventGroupHandle_t mqtt_state_machine;
EventGroupHandle_t wifi_state_machine;
static const char* LED_TAG = "LED";

TaskHandle_t led_task_handle; 

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
    mqtt_state_machine = xEventGroupCreate();
	wifi_state_machine = xEventGroupCreate();
	wifi_start();
	xTaskCreate(led_task, "LED task", 2048, NULL, 1, &led_task_handle);
    mqtt_app_start();
	wifi_notify();
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