#ifndef APP_EVENT_GROUP_H
#define APP_EVENT_GROUP_H

#include "freertos/event_groups.h"
#include "freertos/task.h"

#define WIFI_INIT_BIT BIT0
#define WIFI_CONNECTED_BIT BIT1
#define WIFI_FAIL_BIT BIT2
#define MQTT_CONNECTED_BIT BIT3

extern EventGroupHandle_t app_event_group;
extern TaskHandle_t led_task_handle;

#endif