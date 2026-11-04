#include "boot_image.h"

uint32_t boot_crc_octet(uint32_t state, uint8_t octet)
{
    state ^= octet;
    for (uint16_t bit = 0U; bit < 8U; ++bit) {
        state = (state >> 1U) ^ ((state & 1U) ? 0xEDB88320UL : 0UL);
    }
    return state;
}

static uint32_t read_u32(const boot_flash_io_t *io, uint32_t address)
{
    return (uint32_t)io->read_octet(address) |
           ((uint32_t)io->read_octet(address + 1UL) << 8U) |
           ((uint32_t)io->read_octet(address + 2UL) << 16U) |
           ((uint32_t)io->read_octet(address + 3UL) << 24U);
}

static uint32_t flash_crc(const boot_flash_io_t *io, uint32_t address,
                          uint32_t octets)
{
    uint32_t crc = 0xFFFFFFFFUL;
    for (uint32_t i = 0UL; i < octets; ++i) {
        crc = boot_crc_octet(crc, io->read_octet(address + i));
    }
    return crc ^ 0xFFFFFFFFUL;
}

bool boot_image_valid(const boot_flash_io_t *io, bool require_commit)
{
    const uint32_t m = BOOT_MANIFEST_ADDR;
    const uint32_t length = read_u32(io, m + 12UL);
    const uint32_t header_crc = read_u32(io, m + 28UL);

    if (read_u32(io, m) != BOOT_IMAGE_MAGIC ||
        read_u32(io, m + 4UL) != 1UL ||
        read_u32(io, m + 8UL) != BOOT_IMAGE_TARGET ||
        length < 8UL || length > BOOT_PAYLOAD_OCTETS ||
        read_u32(io, m + 16UL) != BOOT_APP_ADDR ||
        read_u32(io, m + 24UL) != 0UL ||
        flash_crc(io, m, 28UL) != header_crc) {
        return false;
    }

    if (require_commit &&
        (read_u32(io, BOOT_COMMIT_ADDR) != BOOT_COMMIT_MAGIC ||
         read_u32(io, BOOT_COMMIT_ADDR + 4UL) != (BOOT_COMMIT_MAGIC ^ 0xFFFFFFFFUL) ||
         read_u32(io, BOOT_COMMIT_ADDR + 8UL) != header_crc ||
         read_u32(io, BOOT_COMMIT_ADDR + 12UL) != (header_crc ^ 0xFFFFFFFFUL))) {
        return false;
    }

    if (read_u32(io, BOOT_APP_ADDR) == 0xFFFFFFFFUL ||
        read_u32(io, BOOT_APP_ADDR) == 0UL ||
        flash_crc(io, BOOT_APP_ADDR, length) != read_u32(io, m + 20UL)) {
        return false;
    }

    for (uint32_t i = length; i < BOOT_PAYLOAD_OCTETS; ++i) {
        if (io->read_octet(BOOT_APP_ADDR + i) != 0xFFU) {
            return false;
        }
    }
    return true;
}

bool boot_image_prepare(const boot_flash_io_t *io)
{
    return io->erase_octets(BOOT_COMMIT_ADDR, BOOT_SECTOR_OCTETS);
}

bool boot_image_finish(const boot_flash_io_t *io, uint32_t wire_octets)
{
    uint8_t marker[16];
    const uint32_t header_crc = read_u32(io, BOOT_MANIFEST_ADDR + 28UL);
    const uint32_t values[4] = {
        BOOT_COMMIT_MAGIC,
        BOOT_COMMIT_MAGIC ^ 0xFFFFFFFFUL,
        header_crc,
        header_crc ^ 0xFFFFFFFFUL,
    };

    if (wire_octets != BOOT_WIRE_OCTETS || !boot_image_valid(io, false)) {
        return false;
    }

    for (uint16_t i = 0U; i < 4U; ++i) {
        marker[i * 4U] = (uint8_t)values[i];
        marker[i * 4U + 1U] = (uint8_t)(values[i] >> 8U);
        marker[i * 4U + 2U] = (uint8_t)(values[i] >> 16U);
        marker[i * 4U + 3U] = (uint8_t)(values[i] >> 24U);
    }

    return io->program_octets(marker, BOOT_COMMIT_ADDR, sizeof(marker)) &&
           boot_image_valid(io, true);
}
