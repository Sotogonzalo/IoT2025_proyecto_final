// para que el archivo del encabezado se incluya solo una vez
#pragma once

#define LOG_CAPACIDAD 20

typedef struct
{
    char accion[32];
    char timestamp[32];
} entrada_log_t;

// Inicializa y carga buffer desde NVS
void event_logger_init(void);

// Agrega una entrada al buffer y la guarda
void event_logger_add(const char *accion, const char *timestamp);

// Muestra en consola todo el buffer
void event_logger_print(void);
