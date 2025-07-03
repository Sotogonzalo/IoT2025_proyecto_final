#include "spiffs_embebido.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "esp_err.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define TAG "SPIFFS_EMBEBIDO"
void limpiar_spiffs(void);

void montar_spiffs(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 8,
        .format_if_mount_failed = false
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error al montar SPIFFS (%s)", esp_err_to_name(ret));
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo obtener info de SPIFFS (%s)", esp_err_to_name(ret));
        return;
    }


    ESP_LOGI(TAG, "Montado SPIFFS con éxito. Total: %d bytes, Usado: %d bytes", total, used);
}
