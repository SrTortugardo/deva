#include <ata.h>
#include <cpu.h>

static ata_device_t drives[MAX_DRIVES];
static int drive_count = 0;
static int selected_drive = -1;

static int ata_wait_bsy(uint16_t io_base) {
  int timeout = 100000;
  while ((inb(io_base + ATA_REG_STATUS) & ATA_SR_BSY) && timeout > 0) {
    timeout--;
  }
  return timeout > 0 ? 0 : -1;
}

static int ata_wait_drq(uint16_t io_base) {
  int timeout = 100000;
  while (!(inb(io_base + ATA_REG_STATUS) & ATA_SR_DRQ) && timeout > 0) {
    timeout--;
  }
  return timeout > 0 ? 0 : -1;
}

static void ata_400ns_delay(uint16_t io_base) {
  for (int i = 0; i < 4; i++) {
    inb(io_base + ATA_REG_STATUS);
  }
}

static int ata_identify(uint16_t io_base, uint8_t drive, ata_device_t *device) {
  device->io_base = io_base;
  device->drive = drive;
  device->present = 0;

  outb(io_base + ATA_REG_DRIVE, 0xA0 | (drive << 4));
  ata_400ns_delay(io_base);

  outb(io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
  ata_400ns_delay(io_base);

  uint8_t status = inb(io_base + ATA_REG_STATUS);
  if (status == 0) {
    return 0;
  }

  int timeout = 10000;
  while (timeout--) {
    status = inb(io_base + ATA_REG_STATUS);
    if (status & ATA_SR_ERR) {
      return 0;
    }
    if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) {
      break;
    }
  }

  if (timeout <= 0) {
    return 0;
  }

  uint16_t identify[256];
  for (int i = 0; i < 256; i++) {
    identify[i] = inw(io_base + ATA_REG_DATA);
  }

  for (int i = 0; i < 40; i += 2) {
    device->model[i] = identify[27 + i / 2] >> 8;
    device->model[i + 1] = identify[27 + i / 2] & 0xFF;
  }
  device->model[40] = '\0';

  for (int i = 39; i >= 0; i--) {
    if (device->model[i] == ' ') {
      device->model[i] = '\0';
    } else {
      break;
    }
  }

  if (identify[83] & (1 << 10)) {
    uint32_t sectors_lo = ((uint32_t)identify[101] << 16) | identify[100];
    uint32_t sectors_hi = ((uint32_t)identify[103] << 16) | identify[102];
    device->sectors = sectors_hi ? 0xFFFFFFFFUL : sectors_lo;
  } else {
    device->sectors = ((uint32_t)identify[61] << 16) | identify[60];
  }
  device->size_mb = device->sectors / 2048;
  device->is_removable = (identify[0] & 0x80) ? 1 : 0;

  device->present = 1;
  return 1;
}

void ata_init(void) {
  for (int i = 0; i < MAX_DRIVES; i++) {
    drives[i].present = 0;
    drives[i].io_base = 0;
  }

  drive_count = 0;
  selected_drive = -1;
}

int ata_detect_drives(void) {
  drive_count = 0;

  if (ata_identify(ATA_PRIMARY_IO, 0, &drives[drive_count])) {
    drive_count++;
  }

  if (ata_identify(ATA_PRIMARY_IO, 1, &drives[drive_count])) {
    drive_count++;
  }

  if (ata_identify(ATA_SECONDARY_IO, 0, &drives[drive_count])) {
    drive_count++;
  }

  if (ata_identify(ATA_SECONDARY_IO, 1, &drives[drive_count])) {
    drive_count++;
  }

  return drive_count;
}

void ata_print_drives(void) {}

int ata_select_drive(int drive_num) {
  if (drive_num < 0 || drive_num >= drive_count) {
    return -1;
  }

  if (!drives[drive_num].present) {
    return -1;
  }

  ata_device_t *dev = &drives[drive_num];
  uint16_t io_base = dev->io_base;

  outb(io_base + ATA_REG_DRIVE, 0xA0 | (dev->drive << 4));
  ata_400ns_delay(io_base);

  if (ata_wait_bsy(io_base) < 0) {
    return -1;
  }

  uint8_t status = inb(io_base + ATA_REG_STATUS);
  if (!(status & ATA_SR_DRDY)) {
    return -1;
  }

  selected_drive = drive_num;
  return 0;
}

int ata_is_selected(void) { return selected_drive >= 0; }

int ata_get_drive_count(void) { return drive_count; }

ata_device_t *ata_get_drive(int index) {
  if (index < 0 || index >= drive_count) {
    return 0;
  }
  return &drives[index];
}

int ata_write_sector(uint32_t lba, const uint8_t *buffer) {
  if (selected_drive < 0 || selected_drive >= drive_count) {
    return -1;
  }

  ata_device_t *dev = &drives[selected_drive];

  if (!dev->present || dev->io_base == 0) {
    return -1;
  }

  uint16_t io_base = dev->io_base;

  if (ata_wait_bsy(io_base) < 0) {
    return -1;
  }

  outb(io_base + ATA_REG_DRIVE,
       0xE0 | (dev->drive << 4) | ((lba >> 24) & 0x0F));
  ata_400ns_delay(io_base);

  outb(io_base + ATA_REG_SECCOUNT, 1);
  outb(io_base + ATA_REG_LBA_LO, (uint8_t)lba);
  outb(io_base + ATA_REG_LBA_MID, (uint8_t)(lba >> 8));
  outb(io_base + ATA_REG_LBA_HI, (uint8_t)(lba >> 16));

  outb(io_base + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

  if (ata_wait_drq(io_base) < 0) {
    return -1;
  }

  const uint16_t *buf16 = (const uint16_t *)buffer;
  for (int i = 0; i < 256; i++) {
    outw(io_base + ATA_REG_DATA, buf16[i]);
  }

  if (ata_wait_bsy(io_base) < 0) {
    return -1;
  }

  return 0;
}

int ata_read_sector(uint32_t lba, uint8_t *buffer) {
  if (selected_drive < 0 || selected_drive >= drive_count) {
    return -1;
  }

  ata_device_t *dev = &drives[selected_drive];

  if (!dev->present || dev->io_base == 0) {
    return -1;
  }

  uint16_t io_base = dev->io_base;

  if (ata_wait_bsy(io_base) < 0) {
    return -1;
  }

  outb(io_base + ATA_REG_DRIVE,
       0xE0 | (dev->drive << 4) | ((lba >> 24) & 0x0F));
  ata_400ns_delay(io_base);

  outb(io_base + ATA_REG_SECCOUNT, 1);
  outb(io_base + ATA_REG_LBA_LO, (uint8_t)lba);
  outb(io_base + ATA_REG_LBA_MID, (uint8_t)(lba >> 8));
  outb(io_base + ATA_REG_LBA_HI, (uint8_t)(lba >> 16));

  outb(io_base + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

  if (ata_wait_drq(io_base) < 0) {
    return -1;
  }

  uint16_t *buf16 = (uint16_t *)buffer;
  for (int i = 0; i < 256; i++) {
    buf16[i] = inw(io_base + ATA_REG_DATA);
  }

  return 0;
}
