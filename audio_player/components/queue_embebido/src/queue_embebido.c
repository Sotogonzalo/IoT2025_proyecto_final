#include "queue_embebido.h"
#include "audio_embebido.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include <string.h>

#define TAG "QUEUE_EMBEBIDO"

#define MAX_COMMANDS 10

static music_command_t command_queue[MAX_COMMANDS];
static int command_count = 0;
static SemaphoreHandle_t mutex_cola = NULL;

void queue_embebido_init(void) {
    if (mutex_cola == NULL) {
        mutex_cola = xSemaphoreCreateMutex();
        if (mutex_cola == NULL) {
            ESP_LOGE(TAG, "Error al crear el mutex de la cola");
        } else {
            ESP_LOGI(TAG, "Mutex de la cola creado correctamente");
        }
    }
}

void print_command_queue(void) {
    if (!mutex_cola) return;

    xSemaphoreTake(mutex_cola, portMAX_DELAY);
    ESP_LOGI(TAG, "Estado de la cola (%d comandos):", command_count);
    for (int i = 0; i < command_count; i++) {
        ESP_LOGI(TAG, " [%d]: %d", i, command_queue[i]);
    }
    xSemaphoreGive(mutex_cola);
}


void push_command(music_command_t cmd) {
    bool new_command = false;

    if (!mutex_cola) return;

    xSemaphoreTake(mutex_cola, portMAX_DELAY);

    if (command_count < MAX_COMMANDS) {
        command_queue[command_count++] = cmd;
        ESP_LOGI(TAG, "Comando agregado: %d", cmd);
        new_command = true;
    } else {
        ESP_LOGW(TAG, "Cola llena, comando no agregado");
    }

    xSemaphoreGive(mutex_cola);

    if (new_command) {
        print_command_queue();
    }
}


music_command_t pop_command(void) {
    if (!mutex_cola) return CMD_INVALID;

    xSemaphoreTake(mutex_cola, portMAX_DELAY);

    music_command_t cmd = CMD_INVALID;
    if (command_count > 0) {
        cmd = command_queue[0];
        for (int i = 1; i < command_count; i++) {
            command_queue[i - 1] = command_queue[i];
        }
        command_count--;
    }

    xSemaphoreGive(mutex_cola);
    return cmd;
}

void volume_up(void) {
    ESP_LOGI(TAG, "Subiendo volumen");
    audio_embebido_volup();
}

void volume_down(void) {
    ESP_LOGI(TAG, "Bajando volumen");
    audio_embebido_voldown();
}

void next_song(void) {
    ESP_LOGI(TAG, "Siguiente canción");
    audio_embebido_next();
}

void prev_song(void) {
    ESP_LOGI(TAG, "Canción anterior");
    audio_embebido_prev();
}

void pause_song(void) {
    ESP_LOGI(TAG, "Pausando");
    audio_embebido_pause();
}

void play_song(void) {
    ESP_LOGI(TAG, "Reproduciendo");
    audio_embebido_play();
}

void stop_song(void) {
    ESP_LOGI(TAG, "Deteniendo reproducción");
    audio_embebido_stop();
}

music_command_t parse_command(const char *str) {
    if (strcmp(str, "volup") == 0) return CMD_VOLUME_UP;
    if (strcmp(str, "voldown") == 0) return CMD_VOLUME_DOWN;
    if (strcmp(str, "next") == 0) return CMD_NEXT_SONG;
    if (strcmp(str, "prev") == 0) return CMD_PREV_SONG;
    if (strcmp(str, "pause") == 0) return CMD_PAUSE;
    if (strcmp(str, "play") == 0) return CMD_PLAY;
    if (strcmp(str, "stop") == 0) return CMD_STOP;
    return CMD_INVALID;
}

// Tarea que procesa comandos en la cola
void music_player_task(void *pvParameters) {
    while(1) {
        if (command_count > 0) {
            music_command_t cmd = pop_command();
            ESP_LOGI(TAG, "Ejecutando comando: %d", cmd);
            switch(cmd) {
                case CMD_VOLUME_UP:    volume_up(); break;
                case CMD_VOLUME_DOWN:  volume_down(); break;
                case CMD_NEXT_SONG:    next_song(); break;
                case CMD_PREV_SONG:    prev_song(); break;
                case CMD_PAUSE:        pause_song(); break;
                case CMD_PLAY:         play_song(); break;
                case CMD_STOP:         stop_song(); break;
                default:
                    ESP_LOGW(TAG, "Comando inválido");
                    break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

