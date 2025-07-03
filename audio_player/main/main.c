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

void app_main(void)
{
    montar_spiffs();

    audio_embebido_iniciar();

    iniciar_wifi_embebido();

    iniciar_wifi_sta("Fraga", "Fraga188");

    xTaskCreate(music_player_task, "player_task", 4096, NULL, 5, NULL);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}