#include "boot_image.h"

uint32_t boot_crc_octet(uint32_t state, uint8_t octet)
{
    state ^= octet;
    for (uint16_t bit = 0U; bit < 8U; ++bit) {
        state = (state >> 1U) ^ ((state & 1U) ? 0xEDB88320UL : 0UL);
    }
    return state;
}

static uint32_t bytes_u32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8U) |
           ((uint32_t)data[2] << 16U) |
           ((uint32_t)data[3] << 24U);
}

bool boot_manifest_decode(const uint8_t manifest[BOOT_MANIFEST_OCTETS],
                          uint32_t *payload_octets)
{
    uint32_t crc = 0xFFFFFFFFUL;
    const uint32_t length = bytes_u32(&manifest[12]);

    for (uint32_t i = 0UL; i < 28UL; ++i) {
        crc = boot_crc_octet(crc, manifest[i]);
    }
    crc ^= 0xFFFFFFFFUL;

    if (bytes_u32(&manifest[0]) != BOOT_IMAGE_MAGIC ||
        bytes_u32(&manifest[4]) != 1UL ||
        bytes_u32(&manifest[8]) != BOOT_IMAGE_TARGET ||
        length < 8UL || length > BOOT_PAYLOAD_OCTETS ||
        bytes_u32(&manifest[16]) != BOOT_APP_ADDR ||
        bytes_u32(&manifest[24]) != 0UL ||
        bytes_u32(&manifest[28]) != crc) {
        return false;
    }

    if (payload_octets != 0) {
        *payload_octets = length;
    }
    return true;
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
    uint8_t manifest[BOOT_MANIFEST_OCTETS];
    uint32_t length;
    uint32_t header_crc;

    for (uint32_t i = 0UL; i < BOOT_MANIFEST_OCTETS; ++i) {
        manifest[i] = io->read_octet(m + i);
    }
    if (!boot_manifest_decode(manifest, &length)) {
        return false;
    }
    header_crc = read_u32(io, m + 28UL);

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

    return true;
}

bool boot_image_prepare(const boot_flash_io_t *io)
{
    return io->erase_octets(BOOT_COMMIT_ADDR, BOOT_SECTOR_OCTETS);
}

bool boot_image_finish(const boot_flash_io_t *io, uint32_t wire_octets)
{
    uint8_t marker[16];
    uint8_t manifest[BOOT_MANIFEST_OCTETS];
    uint32_t payload_octets;
    const uint32_t header_crc = read_u32(io, BOOT_MANIFEST_ADDR + 28UL);
    const uint32_t values[4] = {
        BOOT_COMMIT_MAGIC,
        BOOT_COMMIT_MAGIC ^ 0xFFFFFFFFUL,
        header_crc,
        header_crc ^ 0xFFFFFFFFUL,
    };

    for (uint32_t i = 0UL; i < BOOT_MANIFEST_OCTETS; ++i) {
        manifest[i] = io->read_octet(BOOT_MANIFEST_ADDR + i);
    }
    if (!boot_manifest_decode(manifest, &payload_octets) ||
        wire_octets != BOOT_MANIFEST_OCTETS + payload_octets ||
        !boot_image_valid(io, false)) {
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
