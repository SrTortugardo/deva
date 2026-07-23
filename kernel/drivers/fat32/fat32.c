#include <ata.h>
#include <colors.h>
#include <fat32.h>
#include <klib/string.h>
#include <stdint.h>
#include <term.h>

#define FAT_EOC 0x0FFFFFF8
#define FAT_MASK 0x0FFFFFFF
#define ATTR_LFN 0x0F
#define ATTR_DIR 0x10
#define ATTR_VOLUME 0x08
#define ATTR_ARCHIVE 0x20
#define MAX_CLUSTER_SECTORS 8

static struct {
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint16_t reserved_sectors;
  uint8_t num_fats;
  uint32_t fat_size_sectors;
  uint32_t root_cluster;
  uint32_t fat_start;
  uint32_t data_start;
  uint32_t cluster_bytes;
} fs;

static uint8_t fat_cache[512];
static uint32_t fat_cache_sector;
static uint8_t cluster_buf[MAX_CLUSTER_SECTORS * 512];

void fat32_init(void) {
  ata_init();
  ata_detect_drives();
  ata_select_drive(0);

  uint8_t boot[512];
  ata_read_sector(0, boot);

  fs.bytes_per_sector = *(uint16_t *)(boot + 11);
  fs.sectors_per_cluster = boot[13];
  fs.reserved_sectors = *(uint16_t *)(boot + 14);
  fs.num_fats = boot[16];
  fs.fat_size_sectors = *(uint32_t *)(boot + 36);
  fs.root_cluster = *(uint32_t *)(boot + 44);

  fs.fat_start = fs.reserved_sectors;
  fs.data_start =
      fs.reserved_sectors + (uint32_t)fs.num_fats * fs.fat_size_sectors;
  fs.cluster_bytes = (uint32_t)fs.sectors_per_cluster * fs.bytes_per_sector;

  fat_cache_sector = 0xFFFFFFFF;
}

static uint32_t cluster_to_lba(uint32_t cluster) {
  return fs.data_start + (cluster - 2) * fs.sectors_per_cluster;
}

static int read_cluster(uint32_t cluster, uint8_t *buf) {
  uint32_t lba = cluster_to_lba(cluster);
  for (uint32_t i = 0; i < fs.sectors_per_cluster; i++)
    if (ata_read_sector(lba + i, buf + i * fs.bytes_per_sector) < 0)
      return -1;
  return 0;
}

static int write_cluster(uint32_t cluster, const uint8_t *buf) {
  uint32_t lba = cluster_to_lba(cluster);
  for (uint32_t i = 0; i < fs.sectors_per_cluster; i++)
    if (ata_write_sector(lba + i, buf + i * fs.bytes_per_sector) < 0)
      return -1;
  return 0;
}

static int is_eoc(uint32_t cluster) { return cluster >= FAT_EOC; }

static uint32_t fat_read(uint32_t cluster) {
  uint32_t fat_offset = cluster * 4;
  uint32_t sector = fs.fat_start + fat_offset / fs.bytes_per_sector;
  uint32_t off = fat_offset % fs.bytes_per_sector;

  if (sector != fat_cache_sector) {
    ata_read_sector(sector, fat_cache);
    fat_cache_sector = sector;
  }
  return *(uint32_t *)(fat_cache + off) & FAT_MASK;
}

static void fat_write(uint32_t cluster, uint32_t value) {
  uint32_t fat_offset = cluster * 4;
  uint32_t sector = fs.fat_start + fat_offset / fs.bytes_per_sector;
  uint32_t off = fat_offset % fs.bytes_per_sector;

  uint8_t buf[512];
  ata_read_sector(sector, buf);
  *(uint32_t *)(buf + off) = value;
  ata_write_sector(sector, buf);

  for (int c = 1; c < fs.num_fats; c++)
    ata_write_sector(sector + c * fs.fat_size_sectors, buf);

  if (sector == fat_cache_sector)
    *(uint32_t *)(fat_cache + off) = value;
}

static uint32_t fat_find_free(void) {
  uint32_t total = (fs.fat_size_sectors * fs.bytes_per_sector) / 4;
  for (uint32_t c = 2; c < total; c++)
    if (fat_read(c) == 0)
      return c;
  return 0;
}

static uint32_t fat_alloc(void) {
  uint32_t c = fat_find_free();
  if (c)
    fat_write(c, FAT_EOC);
  return c;
}

static void fat_free_chain(uint32_t cluster) {
  while (!is_eoc(cluster) && cluster >= 2) {
    uint32_t next = fat_read(cluster);
    fat_write(cluster, 0);
    cluster = next;
  }
}

static void to_83(const char *name, uint8_t out[11]) {
  memset(out, ' ', 11);
  int i;
  for (i = 0; i < 8 && name[i] && name[i] != '.'; i++) {
    char c = name[i];
    if (c >= 'a' && c <= 'z')
      c -= 32;
    out[i] = (uint8_t)c;
  }
  const char *dot = name;
  for (; *dot && *dot != '.'; dot++)
    ;
  if (*dot == '.')
    dot++;
  for (i = 0; i < 3 && dot[i]; i++) {
    char c = dot[i];
    if (c >= 'a' && c <= 'z')
      c -= 32;
    out[8 + i] = (uint8_t)c;
  }
}

static void from_83(const uint8_t *entry, char *out) {
  int j = 0;
  for (int i = 0; i < 8 && entry[i] != ' '; i++)
    out[j++] = (char)entry[i];
  if (entry[8] != ' ') {
    out[j++] = '.';
    for (int i = 0; i < 3 && entry[8 + i] != ' '; i++)
      out[j++] = (char)entry[8 + i];
  }
  out[j] = '\0';
}

static uint32_t find_in_dir(uint32_t dir_cluster, const char *name,
                            uint32_t *size_out) {
  uint8_t target[11];
  to_83(name, target);

  uint32_t cluster = dir_cluster;
  while (!is_eoc(cluster) && cluster >= 2) {
    if (read_cluster(cluster, cluster_buf) < 0)
      return 0;

    for (uint32_t i = 0; i < fs.cluster_bytes; i += 32) {
      uint8_t *e = cluster_buf + i;
      if (e[0] == 0x00)
        return 0;
      if (e[0] == 0xE5)
        continue;
      if ((e[11] & ATTR_LFN) == ATTR_LFN)
        continue;

      if (memcmp(e, target, 11) == 0) {
        uint32_t hi = *(uint16_t *)(e + 20);
        uint32_t lo = *(uint16_t *)(e + 26);
        if (size_out)
          *size_out = *(uint32_t *)(e + 28);
        return (hi << 16) | lo;
      }
    }
    cluster = fat_read(cluster);
  }
  return 0;
}

static int find_entry(uint32_t dir_cluster, const char *name,
                      uint32_t *out_cluster, uint32_t *out_offset,
                      uint32_t *size_out) {
  uint8_t target[11];
  to_83(name, target);

  uint32_t cluster = dir_cluster;
  while (!is_eoc(cluster) && cluster >= 2) {
    if (read_cluster(cluster, cluster_buf) < 0)
      return 0;

    for (uint32_t i = 0; i < fs.cluster_bytes; i += 32) {
      uint8_t *e = cluster_buf + i;
      if (e[0] == 0x00)
        return 0;
      if (e[0] == 0xE5)
        continue;
      if ((e[11] & ATTR_LFN) == ATTR_LFN)
        continue;

      if (memcmp(e, target, 11) == 0) {
        if (out_cluster)
          *out_cluster = cluster;
        if (out_offset)
          *out_offset = i;
        if (size_out)
          *size_out = *(uint32_t *)(e + 28);
        return 1;
      }
    }
    cluster = fat_read(cluster);
  }
  return 0;
}

static uint32_t resolve_path(const char *path, uint32_t *size_out) {
  if (!path)
    return 0;
  if (path[0] != '/') {
    char normalized[256];
    normalized[0] = '/';
    uint32_t plen = strlen(path);
    if (plen + 1 >= 256)
      return 0;
    memcpy(normalized + 1, path, plen + 1);
    return resolve_path(normalized, size_out);
  }
  path++;
  if (*path == '\0') {
    if (size_out)
      *size_out = 0;
    return fs.root_cluster;
  }

  uint32_t dir_cluster = fs.root_cluster;
  char component[64];

  while (*path) {
    int len = 0;
    while (path[len] && path[len] != '/' && len < 63) {
      component[len] = path[len];
      len++;
    }
    component[len] = '\0';
    path += len;
    if (*path == '/')
      path++;
    if (len == 0)
      continue;
    if (component[0] == '.' && component[1] == '\0') {
      if (*path == '\0')
        return dir_cluster;
      continue;
    }

    uint32_t size;
    uint32_t next;

    if (*path == '\0') {
      uint32_t ec, eo;
      if (!find_entry(dir_cluster, component, &ec, &eo, &size))
        return 0;
      uint8_t *ep = cluster_buf + eo;
      uint32_t hi = *(uint16_t *)(ep + 20);
      uint32_t lo = *(uint16_t *)(ep + 26);
      next = (hi << 16) | lo;
    } else {
      next = find_in_dir(dir_cluster, component, &size);
      if (next == 0)
        return 0;
    }

    if (*path == '\0') {
      if (size_out)
        *size_out = size;
      return next;
    }
    dir_cluster = next;
  }
  return 0;
}

/* Separa un path en directorio padre + nombre de archivo */
static uint32_t resolve_parent(const char *path, char *leaf, int leaf_max) {
  if (!path)
    return 0;
  if (path[0] != '/') {
    char normalized[256];
    normalized[0] = '/';
    uint32_t plen = strlen(path);
    if (plen + 1 >= 256)
      return 0;
    memcpy(normalized + 1, path, plen + 1);
    return resolve_parent(normalized, leaf, leaf_max);
  }

  char tmp[256];
  uint32_t len = (uint32_t)strlen(path);
  if (len >= 256)
    return 0;
  memcpy(tmp, path, len + 1);

  while (len > 1 && tmp[len - 1] == '/')
    tmp[--len] = '\0';

  int last_sep = -1;
  for (uint32_t i = 0; i < len; i++)
    if (tmp[i] == '/')
      last_sep = (int)i;

  if (last_sep < 0) {
    strcpy(leaf, tmp);
    return fs.root_cluster;
  }

  strcpy(leaf, tmp + last_sep + 1);

  if (last_sep == 0)
    return fs.root_cluster;

  tmp[last_sep] = '\0';
  return resolve_path(tmp, 0);
}

/* Encuentra un slot libre (0x00 o 0xE5) en un directorio */
static int find_free_dir_slot(uint32_t dir_cluster, uint32_t *out_cluster,
                              uint32_t *out_offset) {
  uint32_t cluster = dir_cluster;
  uint32_t last = 0;

  while (!is_eoc(cluster) && cluster >= 2) {
    if (read_cluster(cluster, cluster_buf) < 0)
      return 0;
    for (uint32_t i = 0; i < fs.cluster_bytes; i += 32) {
      if (cluster_buf[i] == 0x00 || cluster_buf[i] == 0xE5) {
        *out_cluster = cluster;
        *out_offset = i;
        return 1;
      }
    }
    last = cluster;
    cluster = fat_read(cluster);
  }

  if (last == 0)
    return 0;

  uint32_t new_cl = fat_alloc();
  if (new_cl == 0)
    return 0;
  fat_write(last, new_cl);
  memset(cluster_buf, 0, fs.cluster_bytes);
  write_cluster(new_cl, cluster_buf);
  *out_cluster = new_cl;
  *out_offset = 0;
  return 1;
}

static int modify_dir_entry(uint32_t cluster, uint32_t offset,
                            const uint8_t entry[32]) {
  if (read_cluster(cluster, cluster_buf) < 0)
    return -1;
  memcpy(cluster_buf + offset, entry, 32);
  return write_cluster(cluster, cluster_buf);
}

static int dir_add_entry(uint32_t dir_cluster, const char *name, uint8_t attr,
                         uint32_t first_cluster, uint32_t size) {
  uint32_t ec, eo;
  if (!find_free_dir_slot(dir_cluster, &ec, &eo))
    return -1;

  uint8_t entry[32];
  memset(entry, 0, 32);
  to_83(name, entry);
  entry[11] = attr;
  entry[26] = (uint8_t)(first_cluster & 0xFF);
  entry[27] = (uint8_t)((first_cluster >> 8) & 0xFF);
  entry[20] = (uint8_t)((first_cluster >> 16) & 0xFF);
  entry[21] = (uint8_t)((first_cluster >> 24) & 0xFF);
  entry[28] = (uint8_t)(size & 0xFF);
  entry[29] = (uint8_t)((size >> 8) & 0xFF);
  entry[30] = (uint8_t)((size >> 16) & 0xFF);
  entry[31] = (uint8_t)((size >> 24) & 0xFF);

  return modify_dir_entry(ec, eo, entry);
}

static int dir_init(uint32_t dir_cluster, uint32_t parent_cluster) {
  memset(cluster_buf, 0, fs.cluster_bytes);

  memset(cluster_buf, ' ', 11);
  cluster_buf[0] = '.';
  cluster_buf[11] = ATTR_DIR;
  *(uint16_t *)(cluster_buf + 26) = (uint16_t)(dir_cluster & 0xFFFF);
  *(uint16_t *)(cluster_buf + 20) = (uint16_t)((dir_cluster >> 16) & 0xFFFF);

  memset(cluster_buf + 32, ' ', 11);
  cluster_buf[32] = '.';
  cluster_buf[33] = '.';
  cluster_buf[43] = ATTR_DIR;
  *(uint16_t *)(cluster_buf + 58) = (uint16_t)(parent_cluster & 0xFFFF);
  *(uint16_t *)(cluster_buf + 52) = (uint16_t)((parent_cluster >> 16) & 0xFFFF);

  return write_cluster(dir_cluster, cluster_buf);
}

static int delete_entry_in_dir(uint32_t dir_cluster, const char *name) {
  uint32_t ec, eo;
  if (!find_entry(dir_cluster, name, &ec, &eo, 0))
    return -1;

  if (read_cluster(ec, cluster_buf) < 0)
    return -1;
  cluster_buf[eo] = 0xE5;
  return write_cluster(ec, cluster_buf);
}

/* --- Public API --- */

int fat32_read(const char *path, uint8_t *buf, uint32_t max_size,
               uint32_t offset) {
  uint32_t size;
  uint32_t cluster = resolve_path(path, &size);
  if (cluster == 0) {
    char leaf[64];
    uint32_t parent = resolve_parent(path, leaf, 64);
    if (!parent)
      return -1;
    uint32_t ec, eo;
    if (!find_entry(parent, leaf, &ec, &eo, &size))
      return -1;
    return 0;
  }
  if (offset >= size)
    return 0;
  if (size > max_size + offset)
    size = max_size + offset;

  uint32_t skip = offset;
  while (!is_eoc(cluster) && cluster >= 2 && skip > 0) {
    if (skip >= fs.cluster_bytes) {
      skip -= fs.cluster_bytes;
      cluster = fat_read(cluster);
    } else {
      if (read_cluster(cluster, cluster_buf) < 0)
        return -1;
      break;
    }
  }

  uint32_t read_bytes = 0;
  while (!is_eoc(cluster) && cluster >= 2 && read_bytes + offset < size) {
    if (read_cluster(cluster, cluster_buf) < 0)
      return -1;
    uint32_t avail = fs.cluster_bytes - skip;
    uint32_t remain = size - (offset + read_bytes);
    uint32_t to_copy = avail < remain ? avail : remain;
    memcpy(buf + read_bytes, cluster_buf + skip, to_copy);
    read_bytes += to_copy;
    skip = 0;
    cluster = fat_read(cluster);
  }
  return (int)read_bytes;
}

/* Escribe `size` bytes de `buf` en `path` */
int fat32_write(const char *path, const uint8_t *buf, uint32_t size) {
  if (!path || !buf)
    return -1;

  char leaf[64];
  uint32_t parent = resolve_parent(path, leaf, 64);
  if (parent == 0)
    return -1;

  uint32_t old_cluster = resolve_path(path, 0);
  if (old_cluster != 0)
    fat_free_chain(old_cluster);

  delete_entry_in_dir(parent, leaf);

  if (size == 0)
    return 0;

  uint32_t needed = (size + fs.cluster_bytes - 1) / fs.cluster_bytes;
  uint32_t first = fat_alloc();
  if (first == 0)
    return -1;

  uint32_t prev = first;
  for (uint32_t c = 1; c < needed; c++) {
    uint32_t next = fat_alloc();
    if (next == 0) {
      fat_free_chain(first);
      return -1;
    }
    fat_write(prev, next);
    prev = next;
  }

  uint32_t written = 0;
  uint32_t cl = first;
  while (written < size) {
    uint32_t to_write = fs.cluster_bytes;
    if (to_write > size - written)
      to_write = size - written;
    memset(cluster_buf, 0, fs.cluster_bytes);
    memcpy(cluster_buf, buf + written, to_write);
    if (write_cluster(cl, cluster_buf) < 0) {
      fat_free_chain(first);
      return -1;
    }
    written += to_write;
    cl = fat_read(cl);
  }

  if (dir_add_entry(parent, leaf, ATTR_ARCHIVE, first, size) < 0) {
    fat_free_chain(first);
    return -1;
  }

  return (int)size;
}

int fat32_create(const char *path, int is_dir) {
  if (!path)
    return -1;

  char leaf[64];
  uint32_t parent = resolve_parent(path, leaf, 64);
  if (parent == 0)
    return -1;
  if (resolve_path(path, 0) != 0)
    return -2;

  if (is_dir) {
    uint32_t cl = fat_alloc();
    if (cl == 0)
      return -1;
    if (dir_init(cl, parent) < 0) {
      fat_free_chain(cl);
      return -1;
    }
    return dir_add_entry(parent, leaf, ATTR_DIR, cl, 0);
  }

  return dir_add_entry(parent, leaf, ATTR_ARCHIVE, 0, 0);
}

int fat32_delete(const char *path) {
  if (!path)
    return -1;

  char leaf[64];
  uint32_t parent = resolve_parent(path, leaf, 64);
  if (parent == 0)
    return -1;

  uint32_t ec, eo;
  if (!find_entry(parent, leaf, &ec, &eo, 0))
    return -1;

  uint32_t cluster = resolve_path(path, 0);
  if (cluster >= 2)
    fat_free_chain(cluster);
  return delete_entry_in_dir(parent, leaf);
}

static int delete_recursive(uint32_t dir_cluster) {
  if (dir_cluster < 2)
    return 0;

  uint32_t cluster = dir_cluster;
  while (!is_eoc(cluster) && cluster >= 2) {
    if (read_cluster(cluster, cluster_buf) < 0)
      return -1;
    for (uint32_t i = 0; i < fs.cluster_bytes; i += 32) {
      uint8_t *e = cluster_buf + i;
      if (e[0] == 0x00)
        goto done_del;
      if (e[0] == 0xE5)
        continue;
      if ((e[11] & ATTR_LFN) == ATTR_LFN)
        continue;
      if (e[0] == '.' && (e[1] == ' ' || e[1] == '.'))
        continue;

      uint32_t hi = *(uint16_t *)(e + 20);
      uint32_t lo = *(uint16_t *)(e + 26);
      uint32_t sub = (hi << 16) | lo;

      if (e[11] & ATTR_DIR) {
        delete_recursive(sub);
      }
      fat_free_chain(sub);
      e[0] = 0xE5;
    }
    cluster = fat_read(cluster);
  }
done_del:
  return write_cluster(dir_cluster, cluster_buf);
}

int fat32_delete_recursive(const char *path) {
  if (!path)
    return -1;

  char leaf[64];
  uint32_t parent = resolve_parent(path, leaf, 64);
  if (parent == 0)
    return -1;

  uint32_t cluster = resolve_path(path, 0);
  if (cluster < 2) {
    uint32_t ec, eo;
    if (!find_entry(parent, leaf, &ec, &eo, 0))
      return -1;
    return delete_entry_in_dir(parent, leaf);
  }

  delete_recursive(cluster);
  fat_free_chain(cluster);
  return delete_entry_in_dir(parent, leaf);
}

int fat32_exists(const char *path) {
  char leaf[64];
  uint32_t parent = resolve_parent(path, leaf, 64);
  if (!parent)
    return 0;
  uint32_t ec, eo;
  return find_entry(parent, leaf, &ec, &eo, 0);
}

void fat32_list(const char *path) {
  uint32_t dir_cluster;

  if (!path || !*path || (path[0] == '/' && path[1] == '\0'))
    dir_cluster = fs.root_cluster;
  else
    dir_cluster = resolve_path(path, 0);

  if (dir_cluster == 0) {
    term_write("(directorio no encontrado)\n", COLOR_RED);
    return;
  }

  uint32_t cluster = dir_cluster;
  int found = 0;

  while (!is_eoc(cluster) && cluster >= 2) {
    if (read_cluster(cluster, cluster_buf) < 0)
      return;

    for (uint32_t i = 0; i < fs.cluster_bytes; i += 32) {
      uint8_t *e = cluster_buf + i;
      if (e[0] == 0x00)
        goto done;
      if (e[0] == 0xE5)
        continue;
      if ((e[11] & ATTR_LFN) == ATTR_LFN)
        continue;
      if (e[11] & ATTR_VOLUME)
        continue;

      char name[13];
      from_83(e, name);
      found++;

      if (e[11] & ATTR_DIR) {
        term_write(name, COLOR_CYAN);
        term_write("/\n", COLOR_CYAN);
      } else {
        term_write(name, COLOR_TEXT);
        term_write("  (", COLOR_GRAY);
        char num[16];
        itoa((int)*(uint32_t *)(e + 28), num);
        term_write(num, COLOR_GRAY);
        term_write(" B)\n", COLOR_GRAY);
      }
    }
    cluster = fat_read(cluster);
  }

done:
  if (!found)
    term_write("(vacio)\n", COLOR_GRAY);
}

int fat32_stat(const char *path, fat32_stat_t *out) {
  if (!path || !out)
    return -1;

  uint32_t size;
  uint32_t cluster = resolve_path(path, &size);
  if (cluster == 0) {
    char leaf[64];
    uint32_t parent = resolve_parent(path, leaf, 64);
    if (!parent)
      return -1;
    uint32_t ec, eo;
    if (!find_entry(parent, leaf, &ec, &eo, &size))
      return -1;
  }

  out->size = size;
  out->is_dir = 0;

  if (cluster >= 2 && !is_eoc(cluster)) {
    if (read_cluster(cluster, cluster_buf) == 0) {
      uint8_t dot[11];
      memset(dot, ' ', 11);
      dot[0] = '.';
      if (memcmp(cluster_buf, dot, 11) == 0)
        out->is_dir = 1;
    }
  }

  const char *slash = path;
  for (const char *p = path; *p; p++)
    if (*p == '/')
      slash = p + 1;

  int i;
  for (i = 0; i < 12 && slash[i]; i++)
    out->name[i] = slash[i];
  out->name[i] = '\0';

  return 0;
}
