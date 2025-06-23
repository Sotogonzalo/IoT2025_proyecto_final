#ifndef MQTT_EMBEBIDO_H
#define MQTT_EMBEBIDO_H

#include "mqtt_client.h"

/* Inicializa y arranca el cliente MQTT */
void mqtt_embebido_start(void);

/* Publica un mensaje JSON (QoS 1) */
void mqtt_embebido_publicar_json(const char *topico, const char *mensaje);

/* Devuelve el handle al cliente MQTT, si es necesario */
esp_mqtt_client_handle_t mqtt_embebido_get_client(void);

#endif