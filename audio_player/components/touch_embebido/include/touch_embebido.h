#ifndef TOUCH_EMBEBIDO_H
#define TOUCH_EMBEBIDO_H

#include "esp_err.h"
#include <stdbool.h>

esp_err_t touch_embebido_iniciar(void);
bool touch_embebido_fue_tocado(int canal);

#endif // TOUCH_EMBEBIDO_H