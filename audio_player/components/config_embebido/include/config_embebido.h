#ifndef CONFIG_EMBEBIDO_H
#define CONFIG_EMBEBIDO_H
#include <stdbool.h>

#define CONFIG_MAX_STR 64

typedef struct {
    char ssid[CONFIG_MAX_STR];
    char password[CONFIG_MAX_STR];
    char mqtt_uri[CONFIG_MAX_STR];
    int mqtt_port;
} configuracion_t;

bool config_guardar(const configuracion_t *cfg);
bool config_cargar(configuracion_t *cfg);
void config_borrar_todo(void);

#endif