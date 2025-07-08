#ifndef WIFI_EMBEBIDO_H
#define WIFI_EMBEBIDO_H

#include <stdbool.h>
#include "esp_err.h"

esp_err_t iniciar_wifi_embebido(void);
void iniciar_wifi_sta(const char *ssid, const char *password);
void iniciar_wifi_ap(const char *ssid, const char *password);
bool wifi_esta_conectado(void);
void reiniciar_wifi_sta(const char *ssid, const char *password);
void tarea_reiniciar_wifi(void *param);

#endif // WIFI_EMBEBIDO_H
