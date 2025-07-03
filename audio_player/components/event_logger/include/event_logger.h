// para que el archivo del encabezado se incluya solo una vez
#ifndef EVENT_LOGGER_H
#define EVENT_LOGGER_H

#include "event_logger.h"
#include <time.h>

#define LOG_CAPACIDAD 20

typedef struct
{
    char accion[32];
    char cancion[64];
    char timestamp[32];
} entrada_log_t;

void event_logger_init(void);
void event_logger_log_action(const char *accion, const char *cancion);
void event_logger_print(void);

#endif