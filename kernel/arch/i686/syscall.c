#include <arch/i686.h>
#include <colors.h>
#include <framebuffer.h>
#include <interrupts.h>
#include <mm/mm.h>
#include <stdint.h>
#include <syscall/syscall.h>
#include <task/task.h>
#include <term.h>
#include <tty.h>
#include <vfs.h>

void syscall_handler_c(struct regs *r) {
  switch (r->eax) {
  case SYS_TEST:
    term_write("syscall\n", COLOR_CYAN);
    r->eax = 0;
    break;

  case SYS_VFS_EXISTS:
    r->eax = (uint32_t)vfs_exists((const char *)(uintptr_t)r->ebx);
    break;
  case SYS_VFS_READ:
    r->eax = (uint32_t)vfs_read((const char *)(uintptr_t)r->ebx,
                                (char *)(uintptr_t)r->ecx, r->edx, r->esi);
    break;
  case SYS_VFS_WRITE:
    r->eax = (uint32_t)vfs_write((const char *)(uintptr_t)r->ebx,
                                 (const char *)(uintptr_t)r->ecx, r->edx);
    break;
  case SYS_VFS_CREATE:
    r->eax = (uint32_t)vfs_create((const char *)(uintptr_t)r->ebx, (int)r->ecx);
    break;
  case SYS_VFS_DELETE:
    r->eax = (uint32_t)vfs_delete((const char *)(uintptr_t)r->ebx);
    break;
  case SYS_VFS_DELETE_REC:
    r->eax = (uint32_t)vfs_delete_recursive((const char *)(uintptr_t)r->ebx);
    break;
  case SYS_VFS_LIST:
    vfs_list((const char *)(uintptr_t)r->ebx);
    r->eax = 0;
    break;
  case SYS_VFS_STAT:
    r->eax = (uint32_t)vfs_stat((const char *)(uintptr_t)r->ebx,
                                (vfs_stat_t *)(uintptr_t)r->ecx);
    break;
  case SYS_VFS_APPEND:
    r->eax = (uint32_t)-1; /* FAT32 read-only */
    break;
  case SYS_VFS_RENAME:
    r->eax = (uint32_t)-1;
    break;
  case SYS_VFS_TRUNCATE:
    r->eax = (uint32_t)-1;
    break;

  /* --- procesos --- */
  case SYS_EXIT: {
    task_t *me = get_current_task();
    unblock_waiter((int)me->id);
    kill_task(me->id);
    r->eax = 0;

    task_t *next = get_current_task();
    task_t *start = next;
    do {
      if (next->state == TASK_RUNNING)
        break;
      next = next->next;
    } while (next != start);

    if (next->state == TASK_RUNNING) {
      extern void context_switch(uint32_t *old_esp, uint32_t new_esp);
      if (next->cr3 && next->cr3 != get_current_directory())
        switch_page_directory(next->cr3);
      context_switch(&me->esp, next->esp);
    }
    break;
  }
  case SYS_GETPID:
    r->eax = (uint32_t)get_current_task()->id;
    break;
  case SYS_WRITE: {
    const char *buf = (const char *)(uintptr_t)r->ecx;
    uint32_t len = r->edx;
    /* fd ignorado por ahora: siempre va a la terminal */
    for (uint32_t i = 0; i < len; i++)
      term_putchar(buf[i], COLOR_TEXT);
    r->eax = len;
    break;
  }
  case SYS_READ: {
    char *buf = (char *)(uintptr_t)r->ecx;
    int max = (int)r->edx;
    enable_interrupts();
    r->eax = (uint32_t)tty_readline(&tty0, buf, max);
    break;
  }
  case SYS_YIELD:
    yield();
    r->eax = 0;
    break;
  case SYS_EXEC: {
    const char *path = (const char *)(uintptr_t)r->ebx;
    int argc = (int)r->ecx;
    const char **argv = (const char **)(uintptr_t)r->edx;
    int pid = spawn_elf(path, argc, argv);
    if (pid < 0) {
      r->eax = (uint32_t)-1;
      break;
    }
    /* Reemplazar la tarea actual: matar la vieja y saltar a la nueva. */
    task_t *me = get_current_task();
    if (me->id != pid) {
      kill_task(me->id);
    }
    r->eax = 0;
    yield();
    break;
  }
  case SYS_DRAW_PIXEL:
    /* color en 0x00RRGGBB (igual que el framebuffer). */
    video_put_pixel((int)r->ebx, (int)r->ecx, r->edx);
    r->eax = 0;
    break;
  case SYS_PUTCHAR: {
    uint32_t color = r->ebx;
    const char *buf = (const char *)(uintptr_t)r->ecx;
    uint32_t len = r->edx;
    for (uint32_t i = 0; i < len; i++)
      term_putchar(buf[i], color);
    r->eax = len;
    break;
  }
  case SYS_SPAWN: {
    const char *path = (const char *)(uintptr_t)r->ebx;
    int argc = (int)r->ecx;
    const char **argv = (const char **)(uintptr_t)r->edx;
    r->eax = (uint32_t)spawn_elf(path, argc, argv);
    break;
  }
  case SYS_WAITPID: {
    int pid = (int)r->ebx;
    if (!find_task_by_pid(pid)) {
      r->eax = (uint32_t)-1;
      break;
    }
    task_t *me = get_current_task();
    me->state = TASK_WAITING;
    me->waiting_for_pid = pid;
    yield(); /* el scheduler nos skipea hasta que el hijo haga exit */
    me->state = TASK_RUNNING;
    r->eax = 0;
    break;
  }
  case SYS_CLEAR:
    term_clear();
    r->eax = 0;
    break;

  /* --- memoria --- */
  case SYS_MALLOC:
    r->eax = (uint32_t)malloc((size_t)r->ebx);
    break;
  case SYS_FREE:
    free((void *)(uintptr_t)r->ebx);
    r->eax = 0;
    break;

  default:
    r->eax = (uint32_t)-1;
    break;
  }
}

uint32_t syscall_test(void) {
  return (uint32_t)syscall(SYS_TEST, 0, 0, 0, 0, 0);
}
