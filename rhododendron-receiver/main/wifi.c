#include "wifi.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "global.h"
#include "mqtt.h"

static const char *TAG = "wifi station";

static void wifi_task(void* data);
static void wifi_init(void);
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);

#define ESP_WIFI_SSID      "***REMOVED***"
#define ESP_WIFI_PASS      "***REMOVED***"
#define ESP_MAXIMUM_RETRY  5

static int s_retry_num = 0;
static TaskHandle_t wifi_task_handle;

void wifi_start(void)
{
    xTaskCreate(wifi_task, "WiFi Task", 4096, NULL, 1, &wifi_task_handle);
}

void wifi_notify(void)
{
    xTaskNotify(wifi_task_handle, 0, eNoAction);
}

static void wifi_task(void* data)
{
    while (true)
    {
        if(xTaskNotifyWait(pdFALSE, pdFALSE, NULL, portMAX_DELAY))
        {
            EventBits_t wifi_state = xEventGroupGetBits(wifi_state_machine);
            if (wifi_state == WIFI_NOT_INIT)
            {
                wifi_init();
            }
            else if (wifi_state == WIFI_INITIATED)
            {
                ESP_LOGI(TAG, "WiFi initiated");
            }
            else if (wifi_state == WIFI_CONNECTED)
            {
                ESP_LOGI(TAG, "WiFi connected");
                mqtt_notify();
            }
        }
    }
}

static void wifi_init(void)
{
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");

    tcpip_adapter_init();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = ESP_WIFI_SSID,
            .password = ESP_WIFI_PASS
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    xEventGroupSetBits(wifi_state_machine, WIFI_INITIATED);
    wifi_notify();
}

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) 
    {
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        //TODO disconnected state
        if (s_retry_num < ESP_MAXIMUM_RETRY) 
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } 
        else 
        {
            //TODO failed state
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:%s",
                 ip4addr_ntoa(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(wifi_state_machine, WIFI_CONNECTED);
        wifi_notify();
    }
}