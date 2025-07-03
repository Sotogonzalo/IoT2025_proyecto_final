#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>
#include "Task_queue.h"
typedef enum {
    CMD_VOLUME_UP,
    CMD_VOLUME_DOWN,
    CMD_QUEUE_SONG,
    CMD_NEXT_SONG,
    CMD_PAUSE,
    CMD_PLAY,
    CMD_INVALID
} music_command_t;

#define MAX_COMMANDS 10
music_command_t command_queue[MAX_COMMANDS];
int command_count = 0;

void push_command(music_command_t cmd) {
    if (command_count < MAX_COMMANDS) {
        command_queue[command_count++] = cmd;
    }
}

music_command_t pop_command(void) {
    if (command_count == 0) return CMD_INVALID;
    music_command_t cmd = command_queue[0];
    memmove(command_queue, command_queue + 1, (command_count - 1) * sizeof(music_command_t));
    command_count--;
    return cmd;
}

void volume_up(void) { printf("Subiendo volumen\n"); }
void volume_down(void) { printf("Bajando volumen\n"); }
void queue_song(void) { printf("Canción agregada a la cola\n"); }
void next_song(void) { printf("Siguiente canción\n"); }
void pause_song(void) { printf("Pausando\n"); }
void play_song(void) { printf("Reproduciendo\n"); }

// Función para convertir string de usuario a comando
music_command_t parse_command(const char *str) {
    if (strcmp(str, "up") == 0) return CMD_VOLUME_UP;
    if (strcmp(str, "down") == 0) return CMD_VOLUME_DOWN;
    if (strcmp(str, "queue") == 0) return CMD_QUEUE_SONG;
    if (strcmp(str, "next") == 0) return CMD_NEXT_SONG;
    if (strcmp(str, "pause") == 0) return CMD_PAUSE;
    if (strcmp(str, "play") == 0) return CMD_PLAY;
    return CMD_INVALID;
}

void user_input_task(void *pvParameters) {
    char input[32];
    while (1) {
        printf("Ingrese comando (up, down, queue, next, pause, play): ");
        if (scanf("%31s", input) == 1) {
            music_command_t cmd = parse_command(input);
            if (cmd != CMD_INVALID) {
                push_command(cmd);
            } else {
                printf("Comando inválido\n");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Para RTOS, en PC se puede omitir
    }
}

void music_player_task(void *pvParameters) {
    while (1) {
        if (command_count > 0) {
            music_command_t cmd = pop_command();
            switch (cmd) {
                case CMD_VOLUME_UP:    volume_up(); break;
                case CMD_VOLUME_DOWN:  volume_down(); break;
                case CMD_QUEUE_SONG:   queue_song(); break;
                case CMD_NEXT_SONG:    next_song(); break;
                case CMD_PAUSE:        pause_song(); break;
                case CMD_PLAY:         play_song(); break;
                default: break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_main() {
    xTaskCreate(user_input_task, "UserInput", 2048, NULL, 5, NULL);
    xTaskCreate(music_player_task, "MusicPlayer", 2048, NULL, 5, NULL);
}
