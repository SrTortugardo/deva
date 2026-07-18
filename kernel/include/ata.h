#ifndef ATA_H
#define ATA_H

#include <stdint.h>

#define ATA_PRIMARY_IO 0x1F0
#define ATA_PRIMARY_CONTROL 0x3F6
#define ATA_SECONDARY_IO 0x170
#define ATA_SECONDARY_CONTROL 0x376

#define ATA_REG_DATA 0
#define ATA_REG_ERROR 1
#define ATA_REG_FEATURES 1
#define ATA_REG_SECCOUNT 2
#define ATA_REG_LBA_LO 3
#define ATA_REG_LBA_MID 4
#define ATA_REG_LBA_HI 5
#define ATA_REG_DRIVE 6
#define ATA_REG_STATUS 7
#define ATA_REG_COMMAND 7

#define ATA_CMD_READ_PIO 0x20
#define ATA_CMD_WRITE_PIO 0x30
#define ATA_CMD_IDENTIFY 0xEC

#define ATA_SR_BSY 0x80
#define ATA_SR_DRDY 0x40
#define ATA_SR_DF 0x20
#define ATA_SR_DSC 0x10
#define ATA_SR_DRQ 0x08
#define ATA_SR_CORR 0x04
#define ATA_SR_IDX 0x02
#define ATA_SR_ERR 0x01

#define SECTOR_SIZE 512
#define FS_START_SECTOR 2048

#define MAX_DRIVES 4

typedef struct {
  uint16_t io_base;
  uint8_t drive;
  int present;
  char model[41];
  uint32_t size_mb;
  int is_removable;
  uint32_t sectors;
} ata_device_t;

void ata_init(void);
int ata_detect_drives(void);
void ata_print_drives(void);
int ata_select_drive(int drive_num);
int ata_read_sector(uint32_t lba, uint8_t *buffer);
int ata_write_sector(uint32_t lba, const uint8_t *buffer);
int ata_get_drive_count(void);
ata_device_t *ata_get_drive(int index);
int ata_is_selected(void);

#endif
