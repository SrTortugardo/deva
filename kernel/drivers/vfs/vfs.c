#include <vfs.h>
#include <devafs.h>

static int vfs_initialized = 0;

int vfs_init(void) {
  if (!vfs_initialized) {
    devafs_init();
    vfs_initialized = 1;
  }
  return vfs_initialized;
}

int vfs_read(const char *path, char *buffer, uint32_t max_size) {
  if (!vfs_initialized || !path || !buffer || max_size == 0)
    return -1;
  return devafs_read_file(path, buffer, max_size);
}

int vfs_write(const char *path, const char *content, uint32_t size) {
  if (!vfs_initialized || !path || !content || size == 0)
    return -1;
  if (devafs_file_exists(path)) {
    if (devafs_delete_file(path) != 0)
      return -1;
  }
  return devafs_create_file(path, content);
}

int vfs_create(const char *path, int is_dir) {
  if (!vfs_initialized || !path)
    return -1;
  if (is_dir)
    return devafs_create_dir(path);
  else
    return devafs_create_file(path, "");
}

int vfs_delete(const char *path) {
  if (!vfs_initialized || !path)
    return -1;
  if (devafs_file_exists(path))
    return devafs_delete_file(path);
  return -1;
}

void vfs_list(const char *path) {
  devafs_list_dir(path);
}

int vfs_exists(const char *path) {
  if (!vfs_initialized || !path)
    return 0;
  return devafs_file_exists(path);
}

void vfs_save(void) {
  devafs_save();
}
