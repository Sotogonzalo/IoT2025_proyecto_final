#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>
#include "Task_queue.h"
#include "esp_log.h"

static const char *TAG = "TaskQueue";
void task_queue_init(task_queue_t *queue) {
    memset(queue, 0, sizeof(task_queue_t));
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
    }
}
