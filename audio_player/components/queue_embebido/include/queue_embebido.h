#ifndef QUEUE_EMBEBIDO_H
#define QUEUE_EMBEBIDO_H

typedef enum {
    CMD_VOLUME_UP,
    CMD_VOLUME_DOWN,
    CMD_NEXT_SONG,
    CMD_PREV_SONG,
    CMD_PAUSE,
    CMD_PLAY,
    CMD_STOP,
    CMD_INVALID
} music_command_t;

void push_command(music_command_t cmd);
music_command_t pop_command(void);
music_command_t parse_command(const char *str);

void volume_up(void);
void volume_down(void);
void next_song(void);
void prev_song(void);
void pause_song(void);
void play_song(void);
void stop_song(void);

// Tarea que procesa los comandos
void music_player_task(void *pvParameters);

#endif // QUEUE_EMBEBIDO_H
