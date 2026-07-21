#ifndef DEVAFS_H
#define DEVAFS_H

#include <stdint.h>

#define DEVAFS_MAGIC 0x61766564
#define MAX_FILES 64
#define MAX_FILENAME 32
#define MAX_FILE_SIZE 8192
#define SECTOR_SIZE 512
#define FS_START_SECTOR 2048

typedef struct __attribute__((packed)) {
  char name[MAX_FILENAME];
  uint32_t size;
  uint32_t offset;
  uint8_t used;
  uint8_t is_dir;
  uint32_t parent;
} file_entry_t;

typedef struct __attribute__((packed)) {
  uint32_t magic;
  uint32_t file_count;
  file_entry_t files[MAX_FILES];
} devafs_header_t;

void devafs_init(void);
int devafs_create_file(const char *name, const char *content);
int devafs_create_dir(const char *name);
int devafs_read_file(const char *name, char *buffer, uint32_t max_size);
int devafs_delete_file(const char *name);
void devafs_list_files(void);
void devafs_list_dir(const char *path);
int devafs_file_exists(const char *name);
void devafs_save(void);

#endif
