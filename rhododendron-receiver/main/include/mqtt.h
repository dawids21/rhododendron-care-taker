#ifndef MQTT_H
#define MQTT_H

#include "mqtt_client.h"

typedef struct s_mqtt_msg_t {
    char topic[60];
    char payload[100];
} mqtt_msg_t;


void mqtt_app_start(void);
void add_mqtt_msg(mqtt_msg_t msg);
void mqtt_notify(void);

#endif