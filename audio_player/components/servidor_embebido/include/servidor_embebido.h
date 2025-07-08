#ifndef SERVIDOR_EMBEBIDO_H
#define SERVIDOR_EMBEBIDO_H

#include "esp_http_server.h"
#include <stdbool.h>

httpd_handle_t iniciar_servidor_web(void);
void detener_servidor_web(void);
bool servidor_web_esta_activo(void);

#endif
