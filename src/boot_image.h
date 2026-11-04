#ifndef F29_BOOT_IMAGE_H
#define F29_BOOT_IMAGE_H

#include <stdbool.h>
#include <stdint.h>

#define BOOT_LOADER_ENTRY_ADDR 0x10001000UL
#define BOOT_APP_ADDR          0x10040000UL
#define BOOT_SLOT_OCTETS       0x001C0000UL
#define BOOT_SECTOR_OCTETS     0x00001000UL
#define BOOT_PAYLOAD_OCTETS    (BOOT_SLOT_OCTETS - 2UL * BOOT_SECTOR_OCTETS)
#define BOOT_MANIFEST_ADDR     (BOOT_APP_ADDR + BOOT_PAYLOAD_OCTETS)
#define BOOT_COMMIT_ADDR       (BOOT_APP_ADDR + BOOT_SLOT_OCTETS - BOOT_SECTOR_OCTETS)
#define BOOT_WIRE_OCTETS       (BOOT_PAYLOAD_OCTETS + 32UL)

#define BOOT_IMAGE_MAGIC  0x46323948UL
#define BOOT_IMAGE_TARGET 0xF29A8501UL
#define BOOT_COMMIT_MAGIC 0xB007CAFEUL

typedef struct {
    uint8_t (*read_octet)(uint32_t address);
    bool (*program_octets)(const uint8_t *data, uint32_t address, uint32_t count);
    bool (*erase_octets)(uint32_t address, uint32_t count);
} boot_flash_io_t;

uint32_t boot_crc_octet(uint32_t state, uint8_t octet);
bool boot_image_valid(const boot_flash_io_t *io, bool require_commit);
bool boot_image_prepare(const boot_flash_io_t *io);
bool boot_image_finish(const boot_flash_io_t *io, uint32_t wire_octets);

#endif
