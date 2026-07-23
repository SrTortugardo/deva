#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>

typedef struct {
  int is_dir;
  uint32_t size;
  char name[13];
} fat32_stat_t;

void fat32_init(void);

int fat32_read(const char *path, uint8_t *buf, uint32_t max_size,
               uint32_t offset);
int fat32_write(const char *path, const uint8_t *buf, uint32_t size);
int fat32_create(const char *path, int is_dir);
int fat32_delete(const char *path);
int fat32_delete_recursive(const char *path);
int fat32_exists(const char *path);
void fat32_list(const char *path);
int fat32_stat(const char *path, fat32_stat_t *out);

#endif
