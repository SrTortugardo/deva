#ifndef TASK_H
#define TASK_H

#include <paging/paging.h>
#include <stdint.h>

#define STACK_SIZE 16384
#define MAX_TASKS 8

typedef enum {
  TASK_RUNNING = 0,
  TASK_WAITING = 1, /* bloqueado esperando a un hijo (waitpid) */
} task_state_t;

typedef struct task {
  uint32_t id;
  char name[16];

  uint32_t esp;
  uint8_t stack[STACK_SIZE];

  page_directory_t *cr3;
  uint32_t private_page_phys;

  task_state_t state;
  int waiting_for_pid; /* pid del hijo que esperamos, -1 si ninguno */

  struct task *next;
} task_t;

/* lifecycle */
void init_multitasking(void);
void create_process(void (*func)(void), char *name);
void kill_task(int pid);
int spawn_elf(const char *path, int argc, const char **argv);

/* scheduling */
void yield(void);
void schedule(void);
task_t *get_current_task(void);
task_t *find_task_by_pid(int pid);
void unblock_waiter(int pid);

#endif
