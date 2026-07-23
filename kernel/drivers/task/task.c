#include <elf/elf.h>
#include <klib/string.h>
#include <stdint.h>
#include <task/task.h>
#include <vfs.h>

/* Rutina de ASM: guarda el contexto actual y cambia al nuevo ESP. */
extern void context_switch(uint32_t *old_esp, uint32_t new_esp);

static task_t task_table[MAX_TASKS]; /* Tabla con las tareas */
static int task_count = 0;           /* Cuantas tareas hay vivas */
static int next_pid = 1;             /* Proximo PID a asignar */
static task_t *current_task = 0;     /* Tarea que se esta ejecutando */

static void build_initial_stack(task_t *t, void (*entry)(void)) {
  uint32_t *stack =
      (uint32_t *)&t->stack[STACK_SIZE]; /* arrancamos al final del stack */

  *(--stack) =
      (uint32_t)entry; /* EIP: context_switch saltara aqui al retornar */

  for (int j = 0; j < 8;
       j++) /* pusha: edi, esi, ebp, esp, ebx, edx, ecx, eax */
    *(--stack) = 0;

  *(--stack) = 0x10; /* gs */ /* segmentos de datos del kernel */
  *(--stack) = 0x10;          /* fs */
  *(--stack) = 0x10;          /* es */
  *(--stack) = 0x10;          /* ds */

  t->esp = (uint32_t)stack; /* context_switch restaurara este ESP */
}

void init_multitasking(void) {
  task_count = 0;
  next_pid = 1;

  current_task = &task_table[0]; /* la tarea inicial es el propio kernel */

  current_task->id = next_pid++;
  current_task->esp =
      0; /* el kernel ya esta corriendo, no necesita stack fake */
  current_task->cr3 = get_kernel_directory();
  current_task->private_page_phys = 0;
  current_task->state = TASK_RUNNING;
  current_task->waiting_for_pid = -1;
  current_task->next = current_task; /* un solo elemento: apunta a si mismo */

  for (int i = 0; i < 16; i++) {
    current_task->name[i] = "kernel"[i];
  }
  task_count = 1;
}

static task_t *task_alloc(const char *name) {
  if (task_count >= MAX_TASKS)
    return 0;

  task_t *t = &task_table[task_count++];
  t->id = next_pid++;
  t->cr3 = 0;
  t->private_page_phys = 0;
  t->state = TASK_RUNNING;
  t->waiting_for_pid = -1;

  int i = 0;
  for (; i < 15 && name[i]; i++)
    t->name[i] = name[i];
  t->name[i] = 0;

  t->next = current_task->next; /* insertamos en la lista circular */
  current_task->next = t;       /* la nueva tarea va detras de la actual */
  return t;
}

void create_process(void (*entry)(void), char *name) {
  task_t *t = task_alloc(name);
  if (!t)
    return;

  build_initial_stack(
      t, entry); /* preparamos el stack como si hubiera sido context-switched */

  t->cr3 = create_page_directory(); /* directorio de paginas propio */
  t->private_page_phys = alloc_frame();
  if (t->private_page_phys) {
    volatile uint32_t *p = (volatile uint32_t *)t->private_page_phys;
    p[0] = t->id; /* guardamos el PID en la pagina privada */
    p[1] = 0;
    map_page(
        t->cr3, TASK_PRIVATE_VADDR, t->private_page_phys,
        PTE_RW |
            PTE_USER); /* mapeamos pagina privada en el espacio del proceso */
  }
}

static void build_user_stack(task_t *t, void (*entry)(void), int argc,
                             const char **argv) {
  uint32_t *stack = (uint32_t *)&t->stack[STACK_SIZE];

  uint32_t total_str = 0;
  for (int i = 0; i < argc; i++)
    total_str +=
        strlen(argv[i]) + 1; /* calculamos espacio total para los strings */

  char *string_ptr = (char *)stack - total_str; /* reservamos espacio al tope */
  stack = (uint32_t *)string_ptr;
  char *p = string_ptr;
  for (int i = 0; i < argc; i++) {
    uint32_t len = strlen(argv[i]) + 1;
    memcpy(p, argv[i], len); /* copiamos cada argumento al stack */
    p += len;
  }

  for (int i = argc; i >= 0; i--)
    *(--stack) = 0; /* espacio para argv[] (punteros null-terminated) */
  uint32_t *argv_array = stack;

  p = string_ptr;
  for (int i = 0; i < argc; i++) {
    uint32_t len = strlen(argv[i]) + 1;
    argv_array[i] =
        (uint32_t)p; /* cada entrada apunta a su string en el stack */
    p += len;
  }

  *(--stack) = (uint32_t)argc;  /* argc en el stack (convencion de llamada C) */
  *(--stack) = (uint32_t)entry; /* EIP: punto de entrada del ELF */
  for (int j = 0; j < 8; j++)   /* pusha: edi..eax, todos cero */
    *(--stack) = 0;
  *(--stack) = 0x10; /* gs */ /* segmentos de datos del kernel */
  *(--stack) = 0x10;          /* fs */
  *(--stack) = 0x10;          /* es */
  *(--stack) = 0x10;          /* ds */

  t->esp = (uint32_t)stack;
}

int spawn_elf(const char *path, int argc, const char **argv) {
  static uint8_t buf[32768];

  int len = vfs_read(path, (char *)buf, sizeof(buf) - 1,
                     0); /* leemos el binario del disco */
  if (len <= 0)
    return -1;

  const char *slash = path;
  const char *p = path;
  for (; *p; p++)
    if (*p == '/')
      slash = p + 1; /* extraemos el nombre del archivo (basename) */
  if (!*slash)
    slash = path;

  page_directory_t *dir =
      create_page_directory(); /* directorio de paginas para el nuevo proceso */
  uint32_t entry = elf_load(dir, buf); /* cargamos el ELF en ese espacio */
  if (!entry)
    return -1;

  task_t *t = task_alloc(slash);
  if (!t)
    return -1;

  build_user_stack(t, (void (*)(void))entry, argc,
                   argv); /* stack con argc, argv, y contexto */

  t->cr3 = dir;
  t->private_page_phys = alloc_frame();
  if (t->private_page_phys) {
    volatile uint32_t *p = (volatile uint32_t *)t->private_page_phys;
    p[0] = t->id;
    p[1] = 0;
    map_page(t->cr3, TASK_PRIVATE_VADDR, t->private_page_phys,
             PTE_RW |
                 PTE_USER); /* pagina privada con PID para identificacion */
  }

  return (int)t->id;
}

static void remove_task(task_t *t) {
  task_t *prev = current_task;

  while (prev->next != t) {
    prev =
        prev->next; /* recorremos la lista circular hasta encontrar la tarea */
  }

  prev->next = t->next; /* saltamos la tarea eliminada */

  if (t == current_task) {
    current_task = t->next; /* si eliminamos la actual, movemos current */
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
    return; /* una sola tarea, no hay a quien cambiar */
  }

  task_t *old = current_task;
  task_t *new_task = current_task->next;

  while (new_task->state == TASK_WAITING && new_task != current_task)
    new_task = new_task->next; /* saltamos tareas bloqueadas */

  if (new_task == old)
    return; /* todas las demas estan bloqueadas */

  current_task = new_task;

  if (new_task->cr3 && new_task->cr3 != get_current_directory()) {
    switch_page_directory(
        new_task->cr3); /* cambiamos el espacio de direcciones */
  }

  context_switch(&old->esp,
                 new_task->esp); /* guardamos old-ESP y cargamos new-ESP */
}

void yield(void) { schedule(); }

task_t *get_current_task(void) { return current_task; }

task_t *find_task_by_pid(int pid) {
  if (!current_task)
    return 0;

  task_t *t = current_task;
  for (int i = 0; i < task_count; i++) {
    if (t->id == (uint32_t)pid)
      return t;
    t = t->next;
  }
  return 0;
}

void unblock_waiter(int pid) {
  if (!current_task)
    return;

  task_t *t = current_task;
  for (int i = 0; i < task_count; i++) {
    if (t->state == TASK_WAITING && t->waiting_for_pid == pid) {
      t->state =
          TASK_RUNNING; /* despertamos la tarea que esperaba por este PID */
      t->waiting_for_pid = -1;
    }
    t = t->next;
  }
}
