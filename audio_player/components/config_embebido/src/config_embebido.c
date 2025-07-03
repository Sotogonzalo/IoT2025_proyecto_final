#include "config_embebido.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

#define CONFIG_NAMESPACE "app_cfg"
static const char *TAG = "Config";

// Guardar la configuración en NVS
bool config_guardar(const configuracion_t *cfg) {
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(CONFIG_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK) return false;

    err = nvs_set_str(nvs, "ssid", cfg->ssid);
    err |= nvs_set_str(nvs, "pass", cfg->password);
    err |= nvs_set_str(nvs, "mqtt_uri", cfg->mqtt_uri);
    err |= nvs_set_i32(nvs, "mqtt_port", cfg->mqtt_port);

    err |= nvs_set_i32(nvs, "cancion_idx", cfg->cancion_idx);
    err |= nvs_set_i32(nvs, "estado_audio", cfg->estado_audio);
    err |= nvs_set_i32(nvs, "offset_actual", cfg->offset_actual);

    err = nvs_commit(nvs);
    nvs_close(nvs);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Configuración guardada en NVS");
        return true;
    } else {
        ESP_LOGE(TAG, "Error guardando configuración: %d", err);
        return false;
    }
}

// Cargar configuración desde NVS
bool config_cargar(configuracion_t *cfg) {
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(CONFIG_NAMESPACE, NVS_READONLY, &nvs);
    if (err != ESP_OK) return false;

    size_t len = CONFIG_MAX_STR;

    err = nvs_get_str(nvs, "ssid", cfg->ssid, &len);
    if (err != ESP_OK) goto fail;

    len = CONFIG_MAX_STR;
    err = nvs_get_str(nvs, "pass", cfg->password, &len);
    if (err != ESP_OK) goto fail;

    len = CONFIG_MAX_STR;
    err = nvs_get_str(nvs, "mqtt_uri", cfg->mqtt_uri, &len);
    if (err != ESP_OK) goto fail;

    int32_t mqtt_port = 0;
    err = nvs_get_i32(nvs, "mqtt_port", &mqtt_port);
    if (err != ESP_OK) goto fail;
    cfg->mqtt_port = mqtt_port;

    int32_t cancion_idx = 0;
    err = nvs_get_i32(nvs, "cancion_idx", &cancion_idx);
    if (err != ESP_OK) goto fail;
    cfg->cancion_idx = cancion_idx;

    int32_t estado_audio = 0;
    err = nvs_get_i32(nvs, "estado_audio", &estado_audio);
    if (err != ESP_OK) goto fail;
    cfg->estado_audio = estado_audio;

    int32_t offset = 0;
    err = nvs_get_i32(nvs, "offset_actual", &offset);
    cfg->offset_actual = offset;

    nvs_close(nvs);
    return true;

fail:
    nvs_close(nvs);
    return false;
}

void config_borrar_todo(void) {
    ESP_LOGI(TAG, "Borrando toda la configuración NVS");
    nvs_flash_erase();
}
