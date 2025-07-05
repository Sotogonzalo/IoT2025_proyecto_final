#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

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
    montar_spiffs();
    audio_embebido_iniciar();

    led_embebido_iniciar();

    // Cargar configuración desde NVS
    configuracion_t cfg;
    if (!config_cargar(&cfg)) {
        ESP_LOGW(TAG, "No se pudo cargar configuración con los valores guardados.");

        snprintf(cfg.ssid, CONFIG_MAX_STR, "Nombre_red");
        snprintf(cfg.password, CONFIG_MAX_STR, "COntraseña_red");
        snprintf(cfg.mqtt_uri, CONFIG_MAX_STR, "mqtt://broker.hivemq.com"); // Ejemplo de broker público HiveMQ
        cfg.mqtt_port = 1883; // puerto por defecto del MQTT.
        config_guardar(&cfg);

    } else {
        ESP_LOGI(TAG, "Configuración cargada: SSID=%s, MQTT=%s:%d", cfg.ssid, cfg.mqtt_uri, cfg.mqtt_port);
    }

    iniciar_wifi_embebido();
    iniciar_wifi_sta(cfg.ssid, cfg.password);
    while(!wifi_esta_conectado()) {
        ESP_LOGI(TAG, "Esperando conexión WiFi...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    audio_embebido_cargar_estado();

    tiempo_embebido_iniciar();

    xTaskCreate(music_player_task, "player_task", 4096, NULL, 5, NULL);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
