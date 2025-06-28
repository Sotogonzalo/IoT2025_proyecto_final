#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"
#include "wifi_embebido.h"
#include "servidor_embebido.h"
#include "mqtt_embebido.h"
#include "audio_embebido.h"
#include <math.h>

// #define WIFI_SSID  "Fraga" // nombre de la red
// #define WIFI_PASS  "Fraga188" // contraseña de la red
// #define MI_ID      "ESP32S2_A"
// #define TOPIC_TX   "iot/demo/" MI_ID "/tx"
#define TAG        "MAIN"
#define BUFFER_TAM 512

extern const uint8_t music_pcm_start[] asm("_binary_canon_pcm_start");
extern const uint8_t music_pcm_end[]   asm("_binary_canon_pcm_end");

// static void publisher_task(void *arg)
// {
//     int contador = 0;

//     while (1) {
//         char mensaje[64];
//         snprintf(mensaje, sizeof(mensaje),
//                  "Mensaje #%d desde %s", ++contador, MI_ID);

//         mqtt_embebido_publicar_json(TOPIC_TX, mensaje);
//         ESP_LOGI(TAG, "Publicado: %s", mensaje);

//         vTaskDelay(pdMS_TO_TICKS(30000)); //30s
//     }
// }

// En main.c o archivo propio

static void audio_embebido_task(void *args)
{
    extern const uint8_t music_pcm_start[] asm("_binary_canon_pcm_start");
    extern const uint8_t music_pcm_end[]   asm("_binary_canon_pcm_end");

    const uint8_t *data_ptr = music_pcm_start;
    size_t music_len = music_pcm_end - music_pcm_start;
    size_t bytes_written = 0;

    ESP_ERROR_CHECK(i2s_channel_disable(audio_embebido_get_tx_handle()));
    ESP_ERROR_CHECK(i2s_channel_preload_data(audio_embebido_get_tx_handle(), data_ptr, music_len, &bytes_written));
    data_ptr += bytes_written;

    ESP_ERROR_CHECK(i2s_channel_enable(audio_embebido_get_tx_handle()));

    while (1) {
        esp_err_t ret = i2s_channel_write(audio_embebido_get_tx_handle(), data_ptr, music_len - bytes_written, &bytes_written, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE("TASK", "[audio] Fallo al escribir en I2S: %d", ret);
            abort();
        }

        ESP_LOGI("TASK", "[audio] Reproducción OK: %d bytes", (int)bytes_written);
        data_ptr = music_pcm_start;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    vTaskDelete(NULL);
}


void app_main(void)
{
    ESP_LOGI("MAIN", "Inicializando audio_embebido...");
    if (audio_embebido_iniciar() == ESP_OK) {
        ESP_LOGI("MAIN", "Inicialización exitosa. Reproduciendo...");
        xTaskCreate(audio_embebido_task, "audio_task", 4096, NULL, 5, NULL);
    } else {
        ESP_LOGE("MAIN", "Error al inicializar el sistema de audio.");
    }
    // // Inicialización del Wifi
    // iniciar_wifi_sta(WIFI_SSID, WIFI_PASS);

    // if (!wifi_esta_conectado()) {
    //     ESP_LOGE(TAG, "No se pudo conectar al WiFi");
    //     return;
    // }
    // // // Inicialización del servidor web
    // // iniciar_servidor_web();

    // // Inicialización del cliente MQTT
    // mqtt_embebido_start();

    // xTaskCreate(publisher_task, "publisher", 2048, NULL, 5, NULL);
}
