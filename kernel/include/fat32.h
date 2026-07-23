#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>

/* Informacion de un archivo/directorio */
typedef struct {
  int is_dir;
  uint32_t size;
  char name[13];
} fat32_stat_t;

void fat32_init(void);

int fat32_read(const char *path, uint8_t *buf, uint32_t max_size,
               uint32_t offset);               /* lee bytes de un archivo */
int fat32_write(const char *path, const uint8_t *buf, uint32_t size); /* escribe un archivo */
int fat32_create(const char *path, int is_dir); /* crea archivo o directorio */
int fat32_delete(const char *path);             /* elimina un archivo */
int fat32_delete_recursive(const char *path);   /* elimina directorio + contenido */
int fat32_exists(const char *path);             /* verifica si existe */
void fat32_list(const char *path);              /* lista directorio por pantalla */
int fat32_stat(const char *path, fat32_stat_t *out); /* informacion del path */

#endif
