#ifndef WIFI_EMBEBIDO_H
#define WIFI_EMBEBIDO_H

#include <stdbool.h>

void iniciar_wifi_sta(const char *ssid, const char *password);
void iniciar_wifi_ap(const char *ssid, const char *password);
bool wifi_esta_conectado(void);

#endif // WIFI_EMBEBIDO_H
