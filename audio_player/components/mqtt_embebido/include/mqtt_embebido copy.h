#ifndef MQTT_EMBEBIDO_H
#define MQTT_EMBEBIDO_H

#include "mqtt_client.h"

void mqtt_embebido_start(void);
void mqtt_embebido_publicar_json(const char *topico, const char *mensaje);
esp_mqtt_client_handle_t mqtt_embebido_get_client(void);

#endif