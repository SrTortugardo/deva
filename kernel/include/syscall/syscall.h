#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

struct regs; /* forward declaration, definido en interrupts.h */

/* Numeros de syscall (van en eax al hacer int $0x80). */
enum {
  SYS_TEST = 0,
  SYS_VFS_EXISTS,
  SYS_VFS_READ,
  SYS_VFS_WRITE,
  SYS_VFS_CREATE,
  SYS_VFS_DELETE,
  SYS_VFS_DELETE_REC,
  SYS_VFS_LIST,
  SYS_VFS_STAT,
  SYS_VFS_APPEND,
  SYS_VFS_RENAME,
  SYS_VFS_TRUNCATE,

  /* Procesos */
  SYS_EXIT,   /* exit(status)                              */
  SYS_GETPID, /* getpid()                                  */
  SYS_WRITE,  /* write(fd, buf, len) -> stdout por ahora   */
  SYS_READ,   /* read(fd, buf, len) -> stdin teclado,      */
              /*   bloqueante hasta \n, con eco            */
  SYS_YIELD,  /* yield()                                   */
  SYS_EXEC,   /* exec(path) -> reemplazar imagen por ELF   */

  SYS_DRAW_PIXEL, /* draw_pixel(x, y, color_rgb)            */
  SYS_PUTCHAR,

  SYS_SPAWN,   /* spawn(path, argc, argv) -> pid del hijo   */
  SYS_WAITPID, /* waitpid(pid) -> bloquea hasta que pid exit*/

  /* Memoria */
  SYS_MALLOC, /* malloc(size) -> void* */
  SYS_FREE,   /* free(ptr)             */

  /* Pantalla */
  SYS_CLEAR,  /* clear()               */

  /* CPU */
  SYS_GET_CPU_CYCLES, /* get_cpu_cycles() -> contador RDTSC */
};

static inline int32_t syscall(uint32_t num, uint32_t a1, uint32_t a2,
                              uint32_t a3, uint32_t a4, uint32_t a5) {
  int32_t ret;
  __asm__ volatile("int $0x80"
                   : "=a"(ret)
                   : "a"(num), "b"(a1), "c"(a2), "d"(a3), "S"(a4), "D"(a5)
                   : "memory");
  return ret;
}

uint32_t syscall_test(void);

/* Wrappers comodos para userspace/kernel-mode */
static inline int32_t sys_exit(int status) {
  return syscall(SYS_EXIT, (uint32_t)status, 0, 0, 0, 0);
}
static inline int32_t sys_getpid(void) {
  return syscall(SYS_GETPID, 0, 0, 0, 0, 0);
}
static inline int32_t sys_write(int fd, const char *buf, uint32_t len) {
  return syscall(SYS_WRITE, (uint32_t)fd, (uint32_t)(uintptr_t)buf, len, 0, 0);
}
static inline int32_t sys_read(int fd, char *buf, uint32_t len) {
  return syscall(SYS_READ, (uint32_t)fd, (uint32_t)(uintptr_t)buf, len, 0, 0);
}
static inline int32_t sys_yield(void) {
  return syscall(SYS_YIELD, 0, 0, 0, 0, 0);
}
static inline int32_t sys_exec(const char *path, int argc, const char **argv) {
  return syscall(SYS_EXEC, (uint32_t)(uintptr_t)path, (uint32_t)argc,
                 (uint32_t)(uintptr_t)argv, 0, 0);
}
static inline int32_t sys_draw_pixel(uint32_t x, uint32_t y, uint32_t rgb) {
  return syscall(SYS_DRAW_PIXEL, x, y, rgb, 0, 0);
}
static inline uint64_t sys_get_cpu_cycles(void) {
  uint64_t ret;
  __asm__ volatile("rdtsc" : "=A"(ret));
  return ret;
}

/* Handler C invocado desde isr128. */
void syscall_handler_c(struct regs *r);

#endif
