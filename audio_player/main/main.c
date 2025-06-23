#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "wifi_embebido.h"
#include "servidor_embebido.h"
#include "mqtt_embebido.h"

#define WIFI_SSID  "host" // nombre de la red
#define WIFI_PASS  "pass" // contraseña de la red
#define MI_ID      "ESP32S2_A"
#define TOPIC_TX   "iot/demo/" MI_ID "/tx"
#define TAG        "MQTT_MAIN"

static void publisher_task(void *arg)
{
    int contador = 0;

    while (1) {
        char mensaje[64];
        snprintf(mensaje, sizeof(mensaje),
                 "Mensaje #%d desde %s", ++contador, MI_ID);

        mqtt_embebido_publicar_json(TOPIC_TX, mensaje);
        ESP_LOGI(TAG, "Publicado: %s", mensaje);

        vTaskDelay(pdMS_TO_TICKS(30000)); //30s
    }
}

void app_main(void)
{
    // Inicialización del Wifi
    iniciar_wifi_sta(WIFI_SSID, WIFI_PASS);

    // Inicialización del servidor web
    iniciar_servidor_web();

    // Inicialización del cliente MQTT
    mqtt_embebido_start();

    xTaskCreate(publisher_task, "publisher", 4096, NULL, 5, NULL);
}
