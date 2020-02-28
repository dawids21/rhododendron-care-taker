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
#include "app_event_group.h"

void led_task(void* data);

EventGroupHandle_t app_event_group;
static const char* LED_TAG = "LED";

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
    app_event_group = xEventGroupCreate();
    wifi_init_sta();
    mqtt_app_init();
	xTaskCreate(led_task, "LED task", 2048, NULL, 1, NULL);
}

void led_task(void* data)
{
	while (true)
	{
		if (xEventGroupWaitBits(app_event_group, 
								LED_CHANGE_BIT,
								pdTRUE,
								pdTRUE,
								portMAX_DELAY))
		{
			EventBits_t bits = xEventGroupGetBits(app_event_group);
			if (bits & (WIFI_CONNECTED_BIT|MQTT_CONNECTED_BIT))
			{
				ESP_LOGI(LED_TAG, "LED turned off");
				//TODO turn led off
			}
			else
			{
				ESP_LOGI(LED_TAG, "LED turned on");
				//TODO turn led on
			}
		}
		
	}
}