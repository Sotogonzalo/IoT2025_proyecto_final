#ifndef MQTT_EMBEBIDO_H
#define MQTT_EMBEBIDO_H

#include "mqtt_client.h"
#include <stdbool.h>

// Inicia el cliente MQTT con la URI del broker
void mqtt_embebido_start(const char *uri);

// Publica un mensaje JSON en el tópico MQTT indicado
void mqtt_embebido_publicar_json(const char *topico, const char *mensaje);

// Indica si el cliente MQTT está conectado al broker
bool mqtt_embebido_esta_conectado(void);

void mqtt_event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data);

#endif // MQTT_EMBEBIDO_H