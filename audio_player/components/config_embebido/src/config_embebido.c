#include "config_embebido.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

#define CONFIG_NAMESPACE "app_cfg"
static const char *TAG = "CONFIG_EMBEBIDO";

esp_err_t config_embebido_init(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Reinicializando NVS...");
        ret = nvs_flash_erase();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error borrando NVS: %s", esp_err_to_name(ret));
            return ret;
        }
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error inicializando NVS: %s", esp_err_to_name(ret));
    }
    return ret;
}

// Guardar la configuración en NVS
bool config_guardar(const configuracion_t *cfg) {
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(CONFIG_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error abriendo NVS: %s", esp_err_to_name(err));
        return false;
    }

    // Guardar los campos principales
    if (nvs_set_str(nvs, "ssid", cfg->ssid) != ESP_OK ||
        nvs_set_str(nvs, "pass", cfg->password) != ESP_OK ||
        nvs_set_str(nvs, "mqtt_uri", cfg->mqtt_uri) != ESP_OK ||
        nvs_set_i32(nvs, "mqtt_port", cfg->mqtt_port) != ESP_OK ||
        nvs_set_i32(nvs, "cancion_idx", cfg->cancion_idx) != ESP_OK ||
        nvs_set_i32(nvs, "estado_audio", cfg->estado_audio) != ESP_OK ||
        nvs_set_i32(nvs, "offset_actual", cfg->offset_actual) != ESP_OK) {
        ESP_LOGE(TAG, "Error guardando configuración base en NVS");
        nvs_close(nvs);
        return false;
    }

    // Guardar canciones
    for (int i = 0; i < MAX_CANCIONES; i++) {
        char key[16];
        snprintf(key, sizeof(key), "song_%d", i);
        if (nvs_set_str(nvs, key, cfg->canciones_actuales[i]) != ESP_OK) {
            ESP_LOGW(TAG, "No se pudo guardar %s", key);
        }
    }

    err = nvs_commit(nvs);
    nvs_close(nvs);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Configuración guardada en NVS");
        return true;
    } else {
        ESP_LOGE(TAG, "Error al hacer commit: %s", esp_err_to_name(err));
        return false;
    }
}


// Cargar configuración desde NVS
bool config_cargar(configuracion_t *cfg) {

    memset(cfg, 0, sizeof(configuracion_t)); // Limpia toda la estructura

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
    
    for (int i = 0; i < MAX_CANCIONES; i++) {
        char key[16];
        snprintf(key, sizeof(key), "song_%d", i);
        size_t len = CONFIG_MAX_STR;
        
        err = nvs_get_str(nvs, key, cfg->canciones_actuales[i], &len);

        if (err == ESP_ERR_NVS_NOT_FOUND) {
            cfg->canciones_actuales[i][0] = '\0';
        } else if (err != ESP_OK) {
            ESP_LOGW(TAG, "No se pudo leer %s: %s", key, esp_err_to_name(err));
            cfg->canciones_actuales[i][0] = '\0';
        }
    }

    ESP_LOGI(TAG, "Configuración cargada correctamente:");
    ESP_LOGI(TAG, "  SSID: %s", cfg->ssid);
    ESP_LOGI(TAG, "  MQTT URI: %s", cfg->mqtt_uri);
    ESP_LOGI(TAG, "  MQTT Port: %d", cfg->mqtt_port);
    ESP_LOGI(TAG, "  Canción actual: %d, Estado: %d, Offset: %d",
             cfg->cancion_idx, cfg->estado_audio, cfg->offset_actual);

    nvs_close(nvs);
    return true;

fail:
    ESP_LOGW(TAG, "Error al cargar configuración desde NVS: %s", esp_err_to_name(err));
    nvs_close(nvs);
    return false;
}
