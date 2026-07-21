#include <ata.h>
#include <devafs.h>
#include <klib/string.h> /* sabra dios porque el LSP dice que no se usa, cuando si se usa XD */
#include <stdint.h>

static devafs_header_t fs_header;
static uint8_t fs_data[MAX_FILES * MAX_FILE_SIZE];
static int fs_initialized = 0;
static int fs_persistent = 0;

#define HEADER_SECTORS                                                         \
  ((sizeof(devafs_header_t) + SECTOR_SIZE - 1) / SECTOR_SIZE)
#define DATA_SECTORS ((sizeof(fs_data) + SECTOR_SIZE - 1) / SECTOR_SIZE)
#define DEVAFS_PATH_MAX 128

static int fs_read_sector_direct(uint32_t lba, uint8_t *buffer) {
  return ata_read_sector(lba, buffer);
}

static int fs_write_sector_direct(uint32_t lba, const uint8_t *buffer) {
  return ata_write_sector(lba, (uint8_t *)buffer);
}

static int disk_available(void) { return ata_is_selected(); }

static int fs_is_sep(char c) { return c == '/' || c == '\\'; }

static int fs_has_sep(const char *s) {
  if (!s)
    return 0;
  for (uint32_t i = 0; s[i]; i++) {
    if (fs_is_sep(s[i]))
      return 1;
  }
  return 0;
}

static int fs_char_eq_ci(char a, char b) {
  if (a >= 'a' && a <= 'z')
    a -= 32;
  if (b >= 'a' && b <= 'z')
    b -= 32;
  return a == b;
}

static int fs_name_eq_ci(const char *a, const char *b) {
  uint32_t i = 0;
  if (!a || !b)
    return 0;

  while (a[i] && b[i]) {
    if (!fs_char_eq_ci(a[i], b[i]))
      return 0;
    i++;
  }

  return a[i] == '\0' && b[i] == '\0';
}

static void fs_copy_name(char *dst, const char *src) {
  uint32_t len = (uint32_t)strlen(src);
  if (len >= MAX_FILENAME)
    len = MAX_FILENAME - 1;
  memcpy(dst, src, len);
  dst[len] = '\0';
}

static int fs_find_child(uint32_t parent, const char *name, int want_dir) {
  for (int i = 0; i < MAX_FILES; i++) {
    if (!fs_header.files[i].used)
      continue;
    if (fs_header.files[i].parent != parent)
      continue;
    if (!fs_name_eq_ci(fs_header.files[i].name, name))
      continue;
    if (want_dir >= 0 && fs_header.files[i].is_dir != (uint8_t)want_dir)
      continue;
    return i;
  }
  return -1;
}

static int fs_find_any_file(const char *name) {
  for (int i = 0; i < MAX_FILES; i++) {
    if (!fs_header.files[i].used)
      continue;
    if (fs_header.files[i].is_dir)
      continue;
    if (fs_name_eq_ci(fs_header.files[i].name, name))
      return i;
  }
  return -1;
}

static int fs_resolve_path(const char *path, int require_dir) {
  if (!path)
    return -1;

  while (fs_is_sep(*path))
    path++;
  if (*path == '\0')
    return 0; /* root logico */

  char tmp[DEVAFS_PATH_MAX];
  uint32_t len = (uint32_t)strlen(path);
  if (len >= sizeof(tmp))
    return -1;

  memcpy(tmp, path, len + 1);

  while (len > 1 && fs_is_sep(tmp[len - 1])) {
    tmp[--len] = '\0';
  }

  int last_sep = -1;
  for (uint32_t i = 0; i < len; i++) {
    if (fs_is_sep(tmp[i]))
      last_sep = (int)i;
  }

  if (last_sep < 0) {
    int slot = fs_find_child(0, tmp, -1);
    if (slot < 0)
      return -1;
    if (require_dir >= 0 &&
        fs_header.files[slot].is_dir != (uint8_t)require_dir)
      return -1;
    return slot;
  }

  char *leaf = &tmp[last_sep + 1];
  tmp[last_sep] = '\0';

  while (fs_is_sep(*leaf))
    leaf++;
  if (*leaf == '\0')
    return -1;

  int parent_slot = fs_resolve_path(tmp, 1);
  if (parent_slot < 0)
    return -1;

  int slot = fs_find_child((uint32_t)parent_slot, leaf, -1);
  if (slot < 0)
    return -1;

  if (require_dir >= 0 && fs_header.files[slot].is_dir != (uint8_t)require_dir)
    return -1;

  return slot;
}

static int fs_split_parent(const char *path, uint32_t *parent_out,
                           char leaf_out[MAX_FILENAME]) {
  if (!path || !parent_out || !leaf_out)
    return -1;

  char tmp[DEVAFS_PATH_MAX];
  uint32_t len = (uint32_t)strlen(path);
  if (len == 0 || len >= sizeof(tmp))
    return -1;

  memcpy(tmp, path, len + 1);

  while (len > 1 && fs_is_sep(tmp[len - 1])) {
    tmp[--len] = '\0';
  }

  int last_sep = -1;
  for (uint32_t i = 0; i < len; i++) {
    if (fs_is_sep(tmp[i]))
      last_sep = (int)i;
  }

  if (last_sep < 0) {
    *parent_out = 0;
    fs_copy_name(leaf_out, tmp);
    return 0;
  }

  char *leaf = &tmp[last_sep + 1];
  tmp[last_sep] = '\0';

  while (fs_is_sep(*leaf))
    leaf++;
  if (*leaf == '\0')
    return -1;

  int parent_slot = fs_resolve_path(tmp, 1);
  if (parent_slot < 0)
    return -1;

  *parent_out = (uint32_t)parent_slot;
  fs_copy_name(leaf_out, leaf);
  return 0;
}

void devafs_load_from_disk(void) {
  uint8_t sector_buffer[SECTOR_SIZE];

  uint8_t *header_ptr = (uint8_t *)&fs_header;
  for (uint32_t i = 0; i < HEADER_SECTORS; i++) {
    if (fs_read_sector_direct(FS_START_SECTOR + i, sector_buffer) == 0) {
      memcpy(header_ptr + (i * SECTOR_SIZE), sector_buffer, SECTOR_SIZE);
    }
  }

  if (fs_header.magic != DEVAFS_MAGIC) {
    fs_header.magic = DEVAFS_MAGIC;
    fs_header.file_count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
      fs_header.files[i].used = 0;
      fs_header.files[i].name[0] = '\0';
      fs_header.files[i].size = 0;
      fs_header.files[i].offset = 0;
      fs_header.files[i].is_dir = 0;
      fs_header.files[i].parent = 0;
    }
    memset(fs_data, 0, sizeof(fs_data));
    fs_persistent = 1;
    devafs_save();
    return;
  }

  for (uint32_t i = 0; i < DATA_SECTORS; i++) {
    if (fs_read_sector_direct(FS_START_SECTOR + HEADER_SECTORS + i,
                              sector_buffer) == 0) {
      memcpy(fs_data + (i * SECTOR_SIZE), sector_buffer, SECTOR_SIZE);
    }
  }

  fs_persistent = 1;
}

void devafs_save(void) {
  if (!disk_available()) {
    fs_persistent = 0;
    return;
  }

  uint8_t sector_buffer[SECTOR_SIZE];

  uint8_t *header_ptr = (uint8_t *)&fs_header;
  for (uint32_t i = 0; i < HEADER_SECTORS; i++) {
    memset(sector_buffer, 0, SECTOR_SIZE);
    memcpy(sector_buffer, header_ptr + (i * SECTOR_SIZE), SECTOR_SIZE);
    fs_write_sector_direct(FS_START_SECTOR + i, sector_buffer);
  }

  for (uint32_t i = 0; i < DATA_SECTORS; i++) {
    memset(sector_buffer, 0, SECTOR_SIZE);
    uint32_t copy_size = SECTOR_SIZE;
    if ((i + 1) * SECTOR_SIZE > sizeof(fs_data)) {
      copy_size = sizeof(fs_data) - (i * SECTOR_SIZE);
    }
    memcpy(sector_buffer, fs_data + (i * SECTOR_SIZE), copy_size);
    fs_write_sector_direct(FS_START_SECTOR + HEADER_SECTORS + i, sector_buffer);
  }
}

void devafs_init(void) {
  fs_header.magic = DEVAFS_MAGIC;
  fs_header.file_count = 0;

  for (int i = 0; i < MAX_FILES; i++) {
    fs_header.files[i].used = 0;
    fs_header.files[i].name[0] = '\0';
    fs_header.files[i].size = 0;
    fs_header.files[i].offset = 0;
    fs_header.files[i].is_dir = 0;
    fs_header.files[i].parent = 0;
  }

  memset(fs_data, 0, sizeof(fs_data));
  fs_initialized = 1;

  ata_init();
  ata_detect_drives();

  if (ata_get_drive_count() > 0) {
    ata_select_drive(0);
  }

  if (disk_available()) {
    devafs_load_from_disk();
  }
}

static int fs_find_free_slot(void) {
  for (int i = 0; i < MAX_FILES; i++) {
    if (!fs_header.files[i].used) {
      return i;
    }
  }
  return -1;
}

int devafs_create_file(const char *name, const char *content) {
  if (!fs_initialized || !name || !content)
    return -1;

  uint32_t parent = 0;
  char leaf[MAX_FILENAME];

  if (fs_split_parent(name, &parent, leaf) < 0) {
    return -1;
  }

  if (fs_find_child(parent, leaf, -1) >= 0) {
    return -2;
  }

  int slot = fs_find_free_slot();
  if (slot < 0) {
    return -3;
  }

  uint32_t content_len = (uint32_t)strlen(content);
  if (content_len > MAX_FILE_SIZE) {
    return -4;
  }

  fs_copy_name(fs_header.files[slot].name, leaf);
  fs_header.files[slot].size = content_len;
  fs_header.files[slot].offset = slot * MAX_FILE_SIZE;
  fs_header.files[slot].used = 1;
  fs_header.files[slot].is_dir = 0;
  fs_header.files[slot].parent = parent;

  memcpy(fs_data + fs_header.files[slot].offset, content, content_len);

  fs_header.file_count++;
  devafs_save();
  return 0;
}

int devafs_create_dir(const char *name) {
  if (!fs_initialized || !name)
    return -1;

  uint32_t parent = 0;
  char leaf[MAX_FILENAME];

  if (fs_split_parent(name, &parent, leaf) < 0) {
    return -1;
  }

  if (fs_find_child(parent, leaf, -1) >= 0) {
    return -2;
  }

  int slot = fs_find_free_slot();
  if (slot < 0)
    return -3;

  fs_copy_name(fs_header.files[slot].name, leaf);
  fs_header.files[slot].size = 0;
  fs_header.files[slot].offset = 0;
  fs_header.files[slot].used = 1;
  fs_header.files[slot].is_dir = 1;
  fs_header.files[slot].parent = parent;

  fs_header.file_count++;
  devafs_save();
  return 0;
}

int devafs_read_file(const char *name, char *buffer, uint32_t max_size) {
  if (!fs_initialized || !name || !buffer || max_size == 0)
    return -1;

  int slot;

  if (fs_has_sep(name)) {
    slot = fs_resolve_path(name, 0);
  } else {
    slot = fs_find_any_file(name);
  }

  if (slot < 0 || fs_header.files[slot].is_dir) {
    return -1;
  }

  uint32_t size = fs_header.files[slot].size;
  if (size > max_size - 1) {
    size = max_size - 1;
  }

  memcpy(buffer, fs_data + fs_header.files[slot].offset, size);
  buffer[size] = '\0';

  return (int)size;
}

int devafs_delete_file(const char *name) {
  if (!fs_initialized || !name)
    return -1;

  int slot;

  if (fs_has_sep(name)) {
    slot = fs_resolve_path(name, 0);
  } else {
    slot = fs_find_any_file(name);
  }

  if (slot < 0 || fs_header.files[slot].is_dir) {
    return -1;
  }

  fs_header.files[slot].used = 0;
  fs_header.files[slot].name[0] = '\0';
  fs_header.files[slot].size = 0;
  fs_header.files[slot].offset = 0;
  fs_header.files[slot].parent = 0;
  fs_header.file_count--;

  devafs_save();
  return 0;
}

void devafs_list_files(void) {
  if (!fs_initialized)
    return;
  if (fs_header.file_count == 0)
    return;

  for (int i = 0; i < MAX_FILES; i++) {
    if (!fs_header.files[i].used)
      continue;

    /* aquí podrías imprimir:
       - name
       - si es dir o file
       - parent
       - size
     */
  }
}

int devafs_file_exists(const char *name) {
  if (!fs_initialized || !name)
    return 0;

  if (fs_has_sep(name)) {
    return fs_resolve_path(name, -1) >= 0;
  }

  return fs_find_any_file(name) >= 0;
}

void devafs_list_dir(const char *path) {
  if (!fs_initialized)
    return;
  if (fs_header.file_count == 0)
    return;

  int dir_slot = 0;
  int is_root = 1;

  if (path && path[0] && !(path[0] == '/' && path[1] == '\0')) {
    dir_slot = fs_resolve_path(path, 1);
    if (dir_slot < 0)
      return;
    is_root = 0;
  }

  for (int i = 0; i < MAX_FILES; i++) {
    if (!fs_header.files[i].used)
      continue;

    if (is_root) {
      if (fs_header.files[i].parent != 0)
        continue;
    } else {
      if (fs_header.files[i].parent != (uint32_t)dir_slot)
        continue;
    }

    /* imprime o lista aquí */
  }
}
