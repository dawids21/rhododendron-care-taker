#ifndef APP_EVENT_GROUP_H
#define APP_EVENT_GROUP_H

#include "freertos/event_groups.h"
#include "freertos/task.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define MQTT_CONNECTED_BIT BIT2

extern EventGroupHandle_t app_event_group;
extern TaskHandle_t led_task_handle;

#endif