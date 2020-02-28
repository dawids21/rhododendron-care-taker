#ifndef APP_EVENT_GROUP_H
#define APP_EVENT_GROUP_H

#include "freertos/event_groups.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define WIFI_DICONNECTED_BIT BIT2
#define MQTT_CONNECTED_BIT BIT3
#define MQTT_DISCONNECTED_BIT BIT4
#define LED_CHANGE_BIT BIT5

extern EventGroupHandle_t app_event_group;

#endif