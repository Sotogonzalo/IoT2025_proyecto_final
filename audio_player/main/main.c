#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "wifi_embebido.h"
#include "servidor_embebido.h"
#include "spiffs_embebido.h"
#include "audio_embebido.h"
#include "mqtt_embebido.h"
#include "queue_embebido.h"
#include "config_embebido.h"
#include "tiempo_embebido.h"
#include "led_embebido.h"

#define TAG "MAIN"

void app_main(void)
{
    if (config_embebido_init() != ESP_OK) {
        ESP_LOGW(TAG, "No se pudo inicializar NVS");
    }

    led_embebido_iniciar();
    vTaskDelay(pdMS_TO_TICKS(1000));
    montar_spiffs();
    vTaskDelay(pdMS_TO_TICKS(1000));
    audio_embebido_iniciar();
    vTaskDelay(pdMS_TO_TICKS(1000));


    // Cargar configuración desde NVS
    configuracion_t cfg;
    memset(&cfg, 0, sizeof(cfg));

    if (!config_cargar(&cfg)) {
        ESP_LOGW(TAG, "No se pudo cargar configuración desde NVS, inicializando con valores por defecto.");

        memset(&cfg, 0, sizeof(cfg));
        strncpy(cfg.ssid, "caliope", CONFIG_MAX_STR - 1);
        cfg.ssid[CONFIG_MAX_STR - 1] = '\0';
        strncpy(cfg.password, "sinlugar", CONFIG_MAX_STR - 1);
        cfg.password[CONFIG_MAX_STR - 1] = '\0';

        // También dejá vacía la URI MQTT para evitar confusiones
        cfg.mqtt_uri[0] = '\0';
        cfg.mqtt_port = 0;

        config_guardar(&cfg);
    } else {
        ESP_LOGI(TAG, "Configuración cargada: SSID=%s, MQTT=%s:%d", cfg.ssid, cfg.mqtt_uri, cfg.mqtt_port);
    }



    iniciar_wifi_sta(cfg.ssid, cfg.password);
    while(!wifi_esta_conectado()) {
        ESP_LOGI(TAG, "Esperando conexión WiFi...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    if (strlen(cfg.mqtt_uri) > 0 && cfg.mqtt_port > 0) {
        char mqtt_uri_full[128];
        snprintf(mqtt_uri_full, sizeof(mqtt_uri_full), "mqtt://%s:%d", cfg.mqtt_uri, cfg.mqtt_port);
        mqtt_embebido_start(mqtt_uri_full);
        ESP_LOGI(TAG, "Cliente MQTT iniciado con URI %s y puerto %d", cfg.mqtt_uri, cfg.mqtt_port);
    }


    audio_embebido_cargar_estado();

    iniciar_servidor_web();
    vTaskDelay(pdMS_TO_TICKS(1000));

    tiempo_embebido_iniciar();

    queue_embebido_init();
    xTaskCreate(music_player_task, "player_task", 4096, NULL, 5, NULL);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
