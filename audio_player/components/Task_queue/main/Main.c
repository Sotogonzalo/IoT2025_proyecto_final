#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Task_queue.h"  
void app_main() {
    // Crea las tasks de prueba de la librería
    xTaskCreate(user_input_task, "UserInput", 2048, NULL, 5, NULL);
    xTaskCreate(music_player_task, "MusicPlayer", 2048, NULL, 5, NULL);

    // El loop de main queda vacío porque FreeRTOS ejecuta las tasks
}
