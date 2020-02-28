#ifndef APP_EVENT_GROUP_H
#define APP_EVENT_GROUP_H

#include "freertos/event_groups.h"
#include "freertos/task.h"

#define WIFI_INIT_BIT BIT0
#define WIFI_CONNECTED_BIT BIT1
#define WIFI_FAIL_BIT BIT2

enum wifi_states {
    WIFI_NOT_INIT = 0,
    WIFI_INITIATED = WIFI_INIT_BIT,
    WIFI_CONNECTED = WIFI_INIT_BIT|WIFI_CONNECTED_BIT,
};

#define MQTT_INIT_BIT BIT0
#define MQTT_CONNECTED_BIT BIT1

enum mqtt_states {
    MQTT_NOT_INIT = 0,
    MQTT_INITIATED = MQTT_INIT_BIT,
    MQTT_CONNECTED = MQTT_INIT_BIT|MQTT_CONNECTED_BIT,
};

extern EventGroupHandle_t wifi_state_machine;
extern EventGroupHandle_t mqtt_state_machine;
extern TaskHandle_t led_task_handle;

#endif