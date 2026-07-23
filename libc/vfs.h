#ifndef VFS_H
#define VFS_H

#include <stdint.h>

/* estructura con informacion de un archivo */
typedef struct {
  int is_dir;    /* 1 si es directorio, 0 si es archivo */
  uint32_t size; /* tamano en bytes */
  char name[13]; /* nombre (hasta 12 chars + nulo) */
} vfs_stat_t;

/* operaciones del sistema de archivos */
int vfs_stat(const char *path, vfs_stat_t *out);
int vfs_read(const char *path, char *buf, int len);
int vfs_pread(const char *path, char *buf, int len, int offset);
int vfs_write(const char *path, const char *buf, int len);
int vfs_create(const char *path, int is_dir);
int vfs_delete(const char *path);
int vfs_exists(const char *path);
void vfs_list(const char *path);

#endif
