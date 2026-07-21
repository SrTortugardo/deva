#include <stdint.h>
#include <task/task.h>

/* Rutina de ASM: guarda el contexto actual y cambia al nuevo ESP. */
extern void context_switch(uint32_t *old_esp, uint32_t new_esp);

static task_t task_table[MAX_TASKS]; /* Tabla con las tareas */
static int task_count = 0;           /* Cuantas tareas hay vivas */
static int next_pid = 1;             /* Proximo PID a asignar */
static task_t *current_task = 0;     /* Tarea que se esta ejecutando */

void init_multitasking(void) {
  task_count = 0;
  next_pid = 1;

  current_task = &task_table[0];

  current_task->id = next_pid++;
  current_task->esp = 0;
  current_task->next = current_task;

  for (int i = 0; i < 16; i++) {
    current_task->name[i] = "kernel"[i];
  }
  task_count = 1;
}

void create_process(void (*entry)(void), char *name) {
  if (task_count >= MAX_TASKS) {
    return;
  }

  task_t *t = &task_table[task_count++];
  t->id = next_pid++;

  int i = 0;
  for (; i < 15 && name[i]; i++) {
    t->name[i] = name[i];
  }
  t->name[i] = 0;

  uint32_t *stack = (uint32_t *)&t->stack[STACK_SIZE];

  *(--stack) = (uint32_t)entry; /* EIP de retorno: punto de entrada */

  /* pusha: edi, esi, ebp, esp, ebx, edx, ecx, eax */
  for (int j = 0; j < 8; j++) {
    *(--stack) = 0;
  }

  *(--stack) = 0x10; /* gs */
  *(--stack) = 0x10; /* fs */
  *(--stack) = 0x10; /* es */
  *(--stack) = 0x10; /* ds */

  t->esp = (uint32_t)stack;

  /* Lista circular */
  t->next = current_task->next;
  current_task->next = t;
}

static void remove_task(task_t *t) {
  task_t *prev = current_task;

  while (prev->next != t) {
    prev = prev->next;
  }

  prev->next = t->next;

  if (t == current_task) {
    current_task = t->next;
  }

  task_count--;
}

void kill_task(int pid) {
  task_t *t = current_task;

  for (int i = 0; i < task_count; i++) {
    if (t->id == pid) {
      remove_task(t);
      return;
    }
    t = t->next;
  }
}

void schedule(void) {
  if (!current_task || current_task->next == current_task) {
    return;
  }

  task_t *old = current_task;
  task_t *new_task = current_task->next;

  current_task = new_task;

  context_switch(&old->esp, new_task->esp);
}

void yield(void) { schedule(); }

task_t *get_current_task(void) { return current_task; }
