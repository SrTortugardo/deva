/* Capa VFS: delega al driver FAT32 (lectura + escritura). */

#include <fat32.h>
#include <klib/string.h>
#include <stdint.h>
#include <vfs.h>

static int vfs_initialized = 0;

int vfs_init(void) {
  if (!vfs_initialized) {
    fat32_init(); /* inicializamos el unico filesystem soportado */
    vfs_initialized = 1;
  }
  return vfs_initialized;
}

int vfs_read(const char *path, char *buffer, uint32_t max_size,
             uint32_t offset) {
  if (!vfs_initialized || !path || !buffer || max_size == 0)
    return -1;
  return fat32_read(path, (uint8_t *)buffer, max_size,
                    offset); /* delegamos al driver FAT32 */
}

int vfs_write(const char *path, const char *content, uint32_t size) {
  if (!vfs_initialized || !path || !content)
    return -1;
  return fat32_write(path, (const uint8_t *)content,
                     size); /* delegamos al driver FAT32 */
}

int vfs_create(const char *path, int is_dir) {
  if (!vfs_initialized || !path)
    return -1;
  return fat32_create(path, is_dir); /* crea archivo o directorio en FAT32 */
}

int vfs_delete(const char *path) {
  if (!vfs_initialized || !path)
    return -1;
  return fat32_delete(path); /* elimina archivo en FAT32 */
}

int vfs_delete_recursive(const char *path) {
  if (!vfs_initialized || !path)
    return -1;
  return fat32_delete_recursive(path); /* elimina directorio y su contenido */
}

void vfs_list(const char *path) {
  if (!vfs_initialized)
    return;
  fat32_list(path); /* lista el contenido del directorio via FAT32 */
}

int vfs_exists(const char *path) {
  if (!vfs_initialized || !path)
    return 0;
  return fat32_exists(path); /* verifica si el path existe en FAT32 */
}

int vfs_stat(const char *path, vfs_stat_t *out) {
  if (!vfs_initialized || !path || !out)
    return -1;

  fat32_stat_t fs_stat;
  if (fat32_stat(path, &fs_stat) != 0)
    return -1;

  out->is_dir = fs_stat.is_dir; /* traducimos stat FAT32 al formato VFS */
  out->size = fs_stat.size;
  memcpy(out->name, fs_stat.name, sizeof(out->name));
  return 0;
}
