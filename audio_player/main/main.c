#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "wifi_embebido.h"
#include "servidor_embebido.h"
#include "mqtt_embebido.h"
#include "event_logger.h"
#include "tiempo_embebido.h"

#define WIFI_SSID "Wifi"         // nombre de la red
#define WIFI_PASS "diegosan5677" // contraseña de la red
#define MI_ID "ESP32S2_A"
#define TOPIC_TX "iot/demo/" MI_ID "/tx"
#define TAG "MQTT_MAIN"

static void publisher_task(void *arg)
{
    int contador = 0;

    while (1)
    {
        char mensaje[64];
        snprintf(mensaje, sizeof(mensaje),
                 "Mensaje #%d desde %s", ++contador, MI_ID);

        mqtt_embebido_publicar_json(TOPIC_TX, mensaje);
        ESP_LOGI(TAG, "Publicado: %s", mensaje);

        // Obtener hora actual
        char timestamp[32];
        obtener_timestamp_actual(timestamp, sizeof(timestamp));
        event_logger_add("Mensaje MQTT publicado", timestamp);

        // // Obtener hora actual
        // time_t now;
        // struct tm timeinfo;
        // char timestamp[32];

        // time(&now);
        // localtime_r(&now, &timeinfo);
        // strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);

        // // Loggear la acción
        // event_logger_add("Mensaje MQTT publicado", timestamp);

        vTaskDelay(pdMS_TO_TICKS(30000)); // 30s
    }
}

void app_main(void)
{
    // Inicialización del Wifi
    iniciar_wifi_sta(WIFI_SSID, WIFI_PASS);

    // inicialización del logger
    event_logger_init();
    event_logger_print(); // Mostrar los logs guardados antes del reinicio

    // Inicialización del servidor web
    iniciar_servidor_web();

    // Inicialización del cliente MQTT
    mqtt_embebido_start();

    xTaskCreate(publisher_task, "publisher", 4096, NULL, 5, NULL);
}
