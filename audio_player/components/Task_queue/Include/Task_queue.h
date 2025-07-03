#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>
#include "Task_queue.h"

// Definición del tipo de comando musical
typedef enum {
    CMD_VOLUME_UP,
    CMD_VOLUME_DOWN,
    CMD_QUEUE_SONG,
    CMD_NEXT_SONG,
    CMD_PAUSE,
    CMD_PLAY,
    CMD_INVALID
} music_command_t;

// Prototipos de funciones de la librería
void push_command(music_command_t cmd);
music_command_t pop_command(void);

music_command_t parse_command(const char *str);

void user_input_task(void *pvParameters);
void music_player_task(void *pvParameters);

void volume_up(void);
void volume_down(void);
void queue_song(void);
void next_song(void);
void pause_song(void);
void play_song(void);

// Funciones de utilidad
void print_command(music_command_t cmd) {
    switch (cmd) {
        case CMD_VOLUME_UP: printf("Comando: Subir Volumen\n"); break;
        case CMD_VOLUME_DOWN: printf("Comando: Bajar Volumen\n"); break;
        case CMD_QUEUE_SONG: printf("Comando: Agregar Canción a la Cola\n"); break;
        case CMD_NEXT_SONG: printf("Comando: Siguiente Canción\n"); break;
        case CMD_PAUSE: printf("Comando: Pausar Reproducción\n"); break;
        case CMD_PLAY: printf("Comando: Reproducir Música\n"); break;
        default: printf("Comando Inválido\n");
    }
}