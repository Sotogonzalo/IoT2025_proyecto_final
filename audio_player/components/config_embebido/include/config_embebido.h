#ifndef CONFIG_EMBEBIDO_H
#define CONFIG_EMBEBIDO_H
#include <stdbool.h>
#include "esp_err.h"

#define CONFIG_MAX_STR 64
#define MAX_CANCIONES 5

typedef struct {
    char ssid[CONFIG_MAX_STR];
    char password[CONFIG_MAX_STR];
    char mqtt_uri[CONFIG_MAX_STR];
    int mqtt_port;
    int cancion_idx;
    int estado_audio;
    int offset_actual;
    char canciones_actuales[MAX_CANCIONES][64];
} configuracion_t;

bool config_guardar(const configuracion_t *cfg);
bool config_cargar(configuracion_t *cfg);
esp_err_t config_embebido_init(void);

#endif