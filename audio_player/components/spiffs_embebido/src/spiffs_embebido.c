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
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    limpiar_spiffs();

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

void limpiar_spiffs() {
    const char *archivos_a_preservar[] = {
        "canciones.txt",
        "index.html",
        NULL             
    };

    DIR *dir = opendir("/spiffs");
    if (!dir) {
        ESP_LOGW(TAG, "No se pudo abrir /spiffs para limpiar");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        bool preservar = false;
        for (int i = 0; archivos_a_preservar[i] != NULL; i++) {
            if (strcmp(entry->d_name, archivos_a_preservar[i]) == 0) {
                preservar = true;
                break;
            }
        }

        if (preservar) {
            ESP_LOGI(TAG, "Preservando archivo: %s", entry->d_name);
            continue;
        }

        char path[272];
        snprintf(path, sizeof(path), "/spiffs/%s", entry->d_name);
        ESP_LOGI(TAG, "Borrando archivo: %s", path);
        unlink(path);
    }

    closedir(dir);

    FILE *index = fopen("/spiffs/canciones.txt", "w");
    if (index) {
        fclose(index);
        ESP_LOGI(TAG, "Archivo canciones.txt limpiado");
    }
}

