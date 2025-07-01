#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_TASKS 10

typedef void (*task_function_t)(void *);

typedef struct {
    task_function_t function;
    void *arg;
} task_t;

typedef struct {
    task_t tasks[MAX_TASKS];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} task_queue_t;

void task_queue_init(task_queue_t *queue);
bool task_queue_push(task_queue_t *queue, task_function_t function, void *arg);
bool task_queue_pop(task_queue_t *queue, task_t *task);
bool task_queue_is_empty(task_queue_t *queue);
bool task_queue_is_full(task_queue_t *queue);

#endif // TASK_QUEUE_H