#ifndef MQTT_EMBEBIDO_H
#define MQTT_EMBEBIDO_H

#include "mqtt_client.h"
#include <stdbool.h>

// Inicializa cliente MQTT con URI dinámico (ej: "mqtt://192.168.1.100:1883")
void mqtt_embebido_start(const char *uri);

// Publica un mensaje JSON en un tópico con QoS 1
void mqtt_embebido_publicar_json(const char *topico, const char *json);

// Para uso interno o diagnóstico
// esp_mqtt_client_handle_t mqtt_embebido_get_client(void);

// Llamada al conectarse para enviar logger
// void mqtt_embebido_enviar_logger_pendiente(void);

// Saber si el cliente está conectado
bool mqtt_embebido_esta_conectado(void);


#endif