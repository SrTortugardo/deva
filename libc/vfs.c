#include <syscall.h>
#include <vfs.h>

int vfs_read(const char *path, char *buf, int len) {
  /* lee un archivo completo */
  return (int)syscall(SYS_VFS_READ, (long)path, (long)buf, len, 0, 0);
}

int vfs_pread(const char *path, char *buf, int len, int offset) {
  /* lee desde una posición específica */
  return (int)syscall(SYS_VFS_READ, (long)path, (long)buf, len, offset, 0);
}

int vfs_write(const char *path, const char *buf, int len) {
  /* escribe o sobrescribe un archivo */
  return (int)syscall(SYS_VFS_WRITE, (long)path, (long)buf, len, 0, 0);
}

int vfs_exists(const char *path) {
  /* consulta si existe */
  return (int)syscall(SYS_VFS_EXISTS, (long)path, 0, 0, 0, 0);
}

int vfs_create(const char *path, int is_dir) {
  /* crea archivo o directorio */
  return (int)syscall(SYS_VFS_CREATE, (long)path, is_dir, 0, 0, 0);
}

int vfs_delete(const char *path) {
  /* elimina archivo o directorio */
  return (int)syscall(SYS_VFS_DELETE, (long)path, 0, 0, 0, 0);
}

int vfs_stat(const char *path, vfs_stat_t *out) {
  /* obtiene metadatos */
  return (int)syscall(SYS_VFS_STAT, (long)path, (long)out, 0, 0, 0);
}

void vfs_list(const char *path) {
  /* imprime el contenido del directorio */
  syscall(SYS_VFS_LIST, (long)path, 0, 0, 0, 0);
}
