#ifndef TIEMPO_EMBEBIDO_H
#define TIEMPO_EMBEBIDO_H
#include <stdio.h>
// obtiene la fecha del server usando NTP
void obtener_timestamp_actual(char *destino, size_t max_len);

#endif