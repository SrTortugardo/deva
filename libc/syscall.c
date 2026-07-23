#include <syscall.h>

/* ABI en deva: eax = numero, ebx, ecx, edx, esi, edi = 5 args.
 * Es la misma que usa el kernel en int $0x80. */
long syscall(long num, long a1, long a2, long a3, long a4, long a5) {
  long ret;
  /* interrupción 0x80: pasa al kernel */
  __asm__ volatile("int $0x80"
                   : "=a"(ret) /* ret = eax después de la llamada */
                   : "a"(num), /* eax = número de syscall */
                     "b"(a1),  /* ebx = arg1 */
                     "c"(a2),  /* ecx = arg2 */
                     "d"(a3),  /* edx = arg3 */
                     "S"(a4),  /* esi = arg4 */
                     "D"(a5)   /* edi = arg5 */
                   : "memory");
  return ret;
}
