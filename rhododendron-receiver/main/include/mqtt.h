#ifndef MQTT_H
#define MQTT_H

#include "mqtt_client.h"

typedef struct s_mqtt_msg_t {
    char topic[60];
    char payload[20];
} mqtt_msg_t;

void mqtt_init(void);
void add_mqtt_msg(mqtt_msg_t msg);
void mqtt_stop(void);
void mqtt_reopen(void);

#endif