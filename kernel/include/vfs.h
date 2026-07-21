#ifndef VFS_H
#define VFS_H

#include <stdint.h>

#define VFS_READ_BUFFER_SIZE 4096

int vfs_init(void);
int vfs_read(const char *path, char *buffer, uint32_t max_size);
int vfs_write(const char *path, const char *content, uint32_t size);
int vfs_create(const char *path, int is_dir);
int vfs_delete(const char *path);
void vfs_list(const char *path);
int vfs_exists(const char *path);
void vfs_save(void);

#endif
