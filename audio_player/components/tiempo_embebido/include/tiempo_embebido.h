#ifndef TIEMPO_EMBEDIDO_H
#define TIEMPO_EMBEDIDO_H

#include <stdbool.h>
#include <stddef.h>

void tiempo_embebido_iniciar(void);

bool tiempo_embebido_esta_sincronizado(void);

bool tiempo_embebido_obtener_timestamp(char *buffer, size_t max_len);

#endif // TIEMPO_EMBEDIDO_H
