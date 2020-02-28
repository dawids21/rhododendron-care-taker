#ifndef MQTT_H
#define MQTT_H

typedef struct s_mqtt_msg_t {
    char topic[60];
    char payload[100];
} *mqtt_msg_t;


void mqtt_app_init(void);
void mqtt_app_start(void);
void add_mqtt_msg(mqtt_msg_t msg);

#endif