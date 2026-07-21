#ifndef TASK_H
#define TASK_H

#include <stdint.h>

#define STACK_SIZE 16384
#define MAX_TASKS 4

typedef struct task {
  uint32_t id;
  char name[16];

  uint32_t esp;
  uint8_t stack[STACK_SIZE];

  struct task *next;
} task_t;

/* lifecycle */
void init_multitasking(void);
void create_process(void (*func)(void), char *name);
void kill_task(int pid);

/* scheduling */
void yield(void);
void schedule(void);
task_t *get_current_task(void);

#endif
