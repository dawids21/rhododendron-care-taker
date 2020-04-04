#ifndef GLOBAL_H
#define GLOBAL_H

#include "freertos/event_groups.h"
#include "freertos/task.h"

typedef enum states
{
	PROGRAM_START = 0,
	WIFI_INIT,
	WIFI_FAILED,
	MQTT_INIT,
	BLE_INIT,
	BLE_INITIATED,
	ACTIVE,
	WIFI_RECONNECT,
	WIFI_RECONNECTED
} states_t;

void set_program_state(states_t state);
states_t get_program_state(void);

#endif