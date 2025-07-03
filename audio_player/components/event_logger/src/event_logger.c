#include "event_logger.h"
#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "tiempo_embebido.h"

static entrada_log_t buffer_log[LOG_CAPACIDAD];
static uint8_t log_index = 0;

// Guarda buffer circular en NVS
static void guardar_buffer_nvs(void) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err != ESP_OK) return;

    err = nvs_set_blob(handle, "log_buffer", buffer_log, sizeof(buffer_log));
    if (err != ESP_OK) {
        nvs_close(handle);
        return;
    }
    err = nvs_set_u8(handle, "log_index", log_index);
    if (err != ESP_OK) {
        nvs_close(handle);
        return;
    }
    nvs_commit(handle);
    nvs_close(handle);
}

// Carga el buffer desde NVS
static void cargar_buffer_nvs(void) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &handle);
    if (err != ESP_OK) return;

    size_t size = sizeof(buffer_log);
    err = nvs_get_blob(handle, "log_buffer", buffer_log, &size);
    if (err != ESP_OK) {
        memset(buffer_log, 0, sizeof(buffer_log));
    }
    err = nvs_get_u8(handle, "log_index", &log_index);
    if (err != ESP_OK) {
        log_index = 0;
    }
    nvs_close(handle);
}

// Inicializa logger y carga desde NVS
void event_logger_init(void) {
    cargar_buffer_nvs();
}

// Registra acción con timestamp actual y nombre de canción
void event_logger_log_action(const char *accion, const char *cancion) {
    char timestamp[32] = "Sin hora";

    if (!tiempo_embebido_obtener_timestamp(timestamp, sizeof(timestamp))) {
        strncpy(timestamp, "Hora no disponible", sizeof(timestamp));
        timestamp[sizeof(timestamp) - 1] = '\0';
    }

    strncpy(buffer_log[log_index].accion, accion, sizeof(buffer_log[log_index].accion) - 1);
    buffer_log[log_index].accion[sizeof(buffer_log[log_index].accion) - 1] = '\0';

    strncpy(buffer_log[log_index].cancion, cancion, sizeof(buffer_log[log_index].cancion) - 1);
    buffer_log[log_index].cancion[sizeof(buffer_log[log_index].cancion) - 1] = '\0';

    strncpy(buffer_log[log_index].timestamp, timestamp, sizeof(buffer_log[log_index].timestamp) - 1);
    buffer_log[log_index].timestamp[sizeof(buffer_log[log_index].timestamp) - 1] = '\0';

    log_index = (log_index + 1) % LOG_CAPACIDAD;
    guardar_buffer_nvs();
}

// Muestra todos los eventos del log
void event_logger_print(void) {
    printf("===== LOG DE EVENTOS =====\n");
    for (int i = 0; i < LOG_CAPACIDAD; i++) {
        if (strlen(buffer_log[i].accion) > 0) {
            printf("[%02d] Acción: %s | Canción: %s | Hora: %s\n",
                   i,
                   buffer_log[i].accion,
                   buffer_log[i].cancion,
                   buffer_log[i].timestamp);
        }
    }
}