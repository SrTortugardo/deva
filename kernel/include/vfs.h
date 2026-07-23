#ifndef VFS_H
#define VFS_H

#include <stdint.h>

typedef struct {
  int is_dir;
  uint32_t size;
  char name[13];
} vfs_stat_t;

int vfs_init(void);
int vfs_read(const char *path, char *buffer, uint32_t max_size, uint32_t offset);
int vfs_write(const char *path, const char *content, uint32_t size);
int vfs_create(const char *path, int is_dir);
int vfs_delete(const char *path);
int vfs_delete_recursive(const char *path);
void vfs_list(const char *path);
int vfs_exists(const char *path);
int vfs_stat(const char *path, vfs_stat_t *out);

#endif
