#ifndef ATA_H
#define ATA_H

#include <stdint.h>

/*
 * Driver ATA/ATAPI (PIO) - acceso a discos IDE en modo LBA28.
 *
 * El bus IDE clasico tiene 2 canales (primary/secondary) con 2 discos
 * cada uno (master/slave), hasta 4 dispositivos en total. Hablamos con
 * ellos mediante puertos de E/S: cada canal tiene un bloque de 8 registros
 * "IO" mas un registro de control/adress compartido.
 *
 * Usamos PIO (no DMA): el CPU copia los datos palabra a palabra via
 * ata_wait_drq + inw/outw del puerto DATA. Es lento pero trivial.
 */

/* Bases de los bloques de registros IO de cada canal. */
#define ATA_PRIMARY_IO 0x1F0
#define ATA_PRIMARY_CONTROL 0x3F6
#define ATA_SECONDARY_IO 0x170
#define ATA_SECONDARY_CONTROL 0x376

/*
 * Offsets (relativos a io_base) de cada registro del bloque IO.
 * El mismo offset puede tener significado distinto segun si se lee o
 * se escribe (p.ej. ERROR vs FEATURES, STATUS vs COMMAND).
 */
#define ATA_REG_DATA 0       /* R/W:  puerto de datos de 16 bits */
#define ATA_REG_ERROR 1      /* R:    registro de error */
#define ATA_REG_FEATURES 1   /* W:    registro de features */
#define ATA_REG_SECCOUNT 2   /* W:    cantidad de sectores a transferir */
#define ATA_REG_LBA_LO 3     /* W:    bits 0-7 del LBA */
#define ATA_REG_LBA_MID 4    /* W:    bits 8-15 del LBA */
#define ATA_REG_LBA_HI 5     /* W:    bits 16-23 del LBA */
#define ATA_REG_DRIVE 6      /* W:    bits 24-27 del LBA + selector master/slave + modo LBA */
#define ATA_REG_STATUS 7     /* R:    registro de estado */
#define ATA_REG_COMMAND 7    /* W:    registro de comando */

/* Comandos ATA que usamos. */
#define ATA_CMD_READ_PIO 0x20   /* READ SECTORS, PIO mode, con retry */
#define ATA_CMD_WRITE_PIO 0x30  /* WRITE SECTORS, PIO mode, con retry */
#define ATA_CMD_IDENTIFY 0xEC   /* IDENTIFY DEVICE: devuelve 256 words con info del disco */

/* Bits del registro STATUS (offset 7). */
#define ATA_SR_BSY 0x80   /* BUSY: el disco esta ocupado, no acepta mas comandos */
#define ATA_SR_DRDY 0x40  /* DRIVE READY: listo para recibir un comando */
#define ATA_SR_DF 0x20    /* DRIVE WRITE FAULT */
#define ATA_SR_DSC 0x10   /* DRIVE SEEK COMPLETE */
#define ATA_SR_DRQ 0x08   /* DATA REQUEST: hay datos listos en el puerto DATA */
#define ATA_SR_CORR 0x04  /* CORRECTED DATA */
#define ATA_SR_IDX 0x02   /* INDEX */
#define ATA_SR_ERR 0x01   /* ERROR: se setea si hubo error en el ultimo comando */

#define SECTOR_SIZE 512     /* tamano de un sector ATA estandar */
#define FS_START_SECTOR 2048 /* primer sector donde vive devafs en el disco (1 MiB) */

#define MAX_DRIVES 4 /* primary/secondary x master/slave */

/* Representacion de un dispositivo ATA detectado. */
typedef struct {
  uint16_t io_base;     /* base del bloque IO del canal al que pertenece */
  uint8_t drive;        /* 0 = master, 1 = slave */
  int present;          /* 1 si el IDENTIFY encontro un dispositivo */
  char model[41];       /* modelo reportado por IDENTIFY (string de 40 chars) */
  uint32_t size_mb;     /* capacidad en MiB */
  int is_removable;     /* 1 si el dispositivo es removible */
  uint32_t sectors;     /* cantidad total de sectores (LBA28 o LBA48 si soporta) */
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
