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

    nvs_set_str(nvs, "ssid", cfg->ssid);
    nvs_set_str(nvs, "pass", cfg->password);
    nvs_set_str(nvs, "mqtt_uri", cfg->mqtt_uri);
    nvs_set_i32(nvs, "mqtt_port", cfg->mqtt_port);

    err = nvs_commit(nvs);
    nvs_close(nvs);
    ESP_LOGI(TAG, "Configuración guardada en NVS");
    return err == ESP_OK;
}

// Cargar configuración desde NVS
bool config_cargar(configuracion_t *cfg) {
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(CONFIG_NAMESPACE, NVS_READONLY, &nvs);
    if (err != ESP_OK) return false;

    size_t len = CONFIG_MAX_STR;

    err |= nvs_get_str(nvs, "ssid", cfg->ssid, &len);
    len = CONFIG_MAX_STR;
    err |= nvs_get_str(nvs, "pass", cfg->password, &len);
    len = CONFIG_MAX_STR;
    err |= nvs_get_str(nvs, "mqtt_uri", cfg->mqtt_uri, &len);
    int32_t puerto;
    err |= nvs_get_i32(nvs, "mqtt_port", &puerto);
    cfg->mqtt_port = puerto;

    nvs_close(nvs);
    return err == ESP_OK;
}

void config_borrar_todo(void) {
    nvs_flash_erase();
}
