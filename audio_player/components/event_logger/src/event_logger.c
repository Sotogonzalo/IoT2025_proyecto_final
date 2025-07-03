#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "nvs.h"
#include <time.h>
#include "event_logger.h"
#include "tiempo_embebido.h"

static entrada_log_t buffer_log[LOG_CAPACIDAD];
static uint8_t log_index = 0;

// guarda buffer circular en la flash
static void guardar_buffer_nvs()
{
    nvs_handle_t handle;
    if (nvs_open("storage", NVS_READWRITE, &handle) != ESP_OK)
        return;
    nvs_set_blob(handle, "log_buffer", buffer_log, sizeof(buffer_log));
    nvs_set_u8(handle, "log_index", log_index);
    nvs_commit(handle);
    nvs_close(handle);
}
// carga el buffer ya guardado de la flash
static void cargar_buffer_nvs()
{
    nvs_handle_t handle;
    if (nvs_open("storage", NVS_READONLY, &handle) != ESP_OK)
        return;

    size_t tamaño = sizeof(buffer_log);
    nvs_get_blob(handle, "log_buffer", buffer_log, &tamaño);
    nvs_get_u8(handle, "log_index", &log_index);
    nvs_close(handle);
}
// establece los elementos del buffer en 0 y lo carga a la flash
void event_logger_init(void)
{
    memset(buffer_log, 0, sizeof(buffer_log));
    cargar_buffer_nvs();
}
// añade a una celda del buffer la accion realizada y la hora actual
void event_logger_add(const char *accion, const char *timestamp)
{
    strncpy(buffer_log[log_index].accion, accion, sizeof(buffer_log[log_index].accion));
    strncpy(buffer_log[log_index].timestamp, timestamp, sizeof(buffer_log[log_index].timestamp));
    log_index = (log_index + 1) % LOG_CAPACIDAD;
    guardar_buffer_nvs();
}
// printea el contenido del buffer circular
void event_logger_print(void)
{
    printf("===== LOG DE EVENTOS =====\n");
    for (int i = 0; i < LOG_CAPACIDAD; i++)
    {
        if (strlen(buffer_log[i].accion) > 0)
        {
            printf("[%02d] Acción: %s - Hora: %s\n", i, buffer_log[i].accion, buffer_log[i].timestamp);
        }
    }
}
