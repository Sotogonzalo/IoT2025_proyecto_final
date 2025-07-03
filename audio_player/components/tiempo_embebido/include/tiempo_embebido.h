#ifndef TIEMPO_EMBEDIDO_H
#define TIEMPO_EMBEDIDO_H

#include <stdbool.h>
#include <stddef.h>

// Inicia la sincronización NTP con pool.ntp.org y configura la zona horaria Uruguay
void tiempo_embebido_iniciar(void);

// Retorna true si la hora ya fue sincronizada correctamente
bool tiempo_embebido_esta_sincronizado(void);

// Imprime en log la hora actual formateada (o advierte si no está sincronizada)
void tiempo_embebido_mostrar_hora_actual(void);

// Obtiene la hora actual formateada "YYYY-MM-DD HH:MM:SS" en buffer.
// Retorna true si la hora es válida (sincronizada), false si no lo está.
bool tiempo_embebido_obtener_timestamp(char *buffer, size_t max_len);

#endif // TIEMPO_EMBEDIDO_H
