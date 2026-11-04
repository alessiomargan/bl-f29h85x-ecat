#include "boot_image.h"

#include <stdint.h>
#include <string.h>

static uint8_t flash_image[BOOT_SLOT_OCTETS];

static uint8_t read_octet(uint32_t address)
{
    return flash_image[address - BOOT_APP_ADDR];
}

static bool program_octets(const uint8_t *data, uint32_t address, uint32_t count)
{
    uint32_t offset = address - BOOT_APP_ADDR;
    if ((address % 16U) != 0U || offset + count > sizeof(flash_image)) {
        return false;
    }
    for (uint32_t i = 0U; i < count; ++i) {
        if ((flash_image[offset + i] & data[i]) != data[i]) {
            return false;
        }
        flash_image[offset + i] = data[i];
    }
    return true;
}

static bool erase_octets(uint32_t address, uint32_t count)
{
    uint32_t offset = address - BOOT_APP_ADDR;
    if ((address % BOOT_SECTOR_OCTETS) != 0U ||
        offset + count > sizeof(flash_image)) {
        return false;
    }
    memset(&flash_image[offset], 0xFF, count);
    return true;
}

static const boot_flash_io_t io = { read_octet, program_octets, erase_octets };

void test_reset(void)
{
    memset(flash_image, 0xFF, sizeof(flash_image));
}

int test_load(const uint8_t *image, uint32_t count)
{
    uint32_t payload_octets;

    if (count < BOOT_MANIFEST_OCTETS ||
        !boot_manifest_decode(image, &payload_octets) ||
        count != BOOT_MANIFEST_OCTETS + payload_octets) {
        return 0;
    }
    memcpy(flash_image, image + BOOT_MANIFEST_OCTETS, payload_octets);
    memcpy(&flash_image[BOOT_MANIFEST_ADDR - BOOT_APP_ADDR], image,
           BOOT_MANIFEST_OCTETS);
    return 1;
}

int test_valid(int require_commit)
{
    return boot_image_valid(&io, require_commit != 0);
}

int test_finish(uint32_t count)
{
    return boot_image_finish(&io, count);
}

void test_corrupt(uint32_t offset)
{
    flash_image[offset] ^= 1U;
}
