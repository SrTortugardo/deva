#ifndef SYSCALL_H
#define SYSCALL_H

/* Numeros de syscall (espejo de kernel/include/syscall/syscall.h).
 * Mantener sincronizado con el kernel. */
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
  SYS_EXIT,
  SYS_GETPID,
  SYS_WRITE,
  SYS_READ,
  SYS_YIELD,
  SYS_EXEC,
  SYS_DRAW_PIXEL,
  SYS_PUTCHAR,
  SYS_SPAWN,
  SYS_WAITPID,
  SYS_MALLOC,
  SYS_FREE,
  SYS_CLEAR,
  SYS_GET_CPU_CYCLES,
};

/* ABI deva (i386, int $0x80):
 *   eax = numero, ebx..edi = hasta 5 args, retorno en eax. */
long syscall(long num, long a1, long a2, long a3, long a4, long a5);

#endif
