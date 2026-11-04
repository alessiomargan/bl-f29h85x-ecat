#include <soes/esc.h>
#include <soes/esc_foe.h>

#include "boot_f29h85x.h"
#include "boot_image.h"
#include "flash_utils.h"
#include "globals.h"

static bool firmware_write_open;
static bool manifest_ready;
static uint8_t wire_manifest[BOOT_MANIFEST_OCTETS];
static uint32_t manifest_octets;
static uint32_t expected_payload_octets;
static uint32_t expected_wire_octets;
static uint32_t programmed_octets;
static uint32_t callback_octets;

static uint32_t foe_write_flash(foe_file_cfg_t *cfg, uint8_t *data,
                                size_t length);
static uint32_t foe_read_flash(foe_file_cfg_t *cfg, uint8_t *data,
                               size_t length);
static uint32_t on_foe_open(uint8_t op);
static uint32_t on_foe_close(void);

static foe_file_cfg_t firmware_files[] = {
    {
        .name = "f29h85x.bin",
        .max_data = BOOT_MAX_WIRE_OCTETS,
        .dest_start_address = BOOT_APP_ADDR,
        .address_offset = 0U,
        .filepass = 0xF290U,
        .write_only_in_boot = true,
        .write_function = foe_write_flash,
        .read_function = foe_read_flash,
        .on_foe_open = on_foe_open,
        .on_foe_close = on_foe_close,
    },
    { 0 }
};

void bootstrap_foe_init(void)
{
    gFOE_config.fbuffer = foe_buffer;
    gFOE_config.buffer_size = sizeof(foe_buffer);
    gFOE_config.n_files = 1U;
    gFOE_config.files = firmware_files;
    FOE_config(&gFOE_config);

    DPRINT("foe_file %s addr 0x%08" PRIX32 "\n",
           firmware_files[0].name, firmware_files[0].dest_start_address);
}

static uint32_t foe_write_flash(foe_file_cfg_t *cfg, uint8_t *data,
                                size_t length)
{
    const uint32_t count = (uint32_t)length;
    const uint32_t callback_limit = BOOT_MAX_WIRE_OCTETS +
                                    gFOE_config.buffer_size - 1U;
    uint32_t position = 0U;

    DPRINT("%s offset=%" PRIu32 " callback=%" PRIu32 " bytes\n",
           __FUNCTION__, cfg != NULL ? cfg->address_offset : 0U, count);

    if (!firmware_write_open || ESCvar.ALstatus != ESCboot ||
        cfg != &firmware_files[0] || data == NULL || count == 0U ||
        (count % FLASH_PROGRAM_SIZE_BYTES) != 0U ||
        (cfg->address_offset % FLASH_PROGRAM_SIZE_BYTES) != 0U ||
        cfg->address_offset != callback_octets ||
        cfg->address_offset > BOOT_MAX_WIRE_OCTETS ||
        count > callback_limit - cfg->address_offset) {
        DPRINT("%s rejected callback state\n", __FUNCTION__);
        firmware_write_open = false;
        return 1U;
    }

    while (manifest_octets < BOOT_MANIFEST_OCTETS && position < count) {
        wire_manifest[manifest_octets++] = data[position++];
    }

    if (manifest_octets == BOOT_MANIFEST_OCTETS && !manifest_ready) {
        uint32_t erase_octets;

        if (!boot_manifest_decode(wire_manifest, &expected_payload_octets)) {
            DPRINT("%s invalid manifest\n", __FUNCTION__);
            firmware_write_open = false;
            return 1U;
        }
        expected_wire_octets = BOOT_MANIFEST_OCTETS +
                               expected_payload_octets;
        erase_octets = ((expected_payload_octets + BOOT_SECTOR_OCTETS - 1U) /
                        BOOT_SECTOR_OCTETS) * BOOT_SECTOR_OCTETS;

        DPRINT("%s manifest payload=%" PRIu32 " wire=%" PRIu32
               " erase=%" PRIu32 " bytes\n", __FUNCTION__,
               expected_payload_octets, expected_wire_octets, erase_octets);

        if (Erase_flash(BOOT_APP_ADDR, erase_octets) != Fapi_Status_Success ||
            Erase_flash(BOOT_MANIFEST_ADDR, BOOT_SECTOR_OCTETS) !=
                Fapi_Status_Success) {
            DPRINT("%s payload/manifest erase failed\n", __FUNCTION__);
            firmware_write_open = false;
            return 1U;
        }
        manifest_ready = true;
    }

    if (manifest_ready && position < count) {
        const uint32_t remaining = expected_payload_octets - programmed_octets;
        uint32_t payload_count = count - position;
        const uint32_t address = BOOT_APP_ADDR + programmed_octets;

        if (payload_count > remaining) {
            payload_count = remaining;
        }
        if (payload_count != 0U && payload_count != remaining &&
            (payload_count % FLASH_PROGRAM_SIZE_BYTES) != 0U) {
            DPRINT("%s unaligned intermediate payload\n", __FUNCTION__);
            firmware_write_open = false;
            return 1U;
        }

        if (payload_count != 0U) {
            DPRINT("%s FlashAddr 0x%08" PRIX32 " with %" PRIu32
                   " bytes\n", __FUNCTION__, address, payload_count);
            if (Program_flash(address, &data[position], payload_count) !=
                Fapi_Status_Success) {
                DPRINT("%s flash program failed\n", __FUNCTION__);
                firmware_write_open = false;
                return 1U;
            }
            programmed_octets += payload_count;
            position += payload_count;
        }

        /* SOES pads its final partial staging buffer with erased octets. */
        while (position < count) {
            if (data[position++] != 0xFFU) {
                DPRINT("%s data exceeds manifest length\n", __FUNCTION__);
                firmware_write_open = false;
                return 1U;
            }
        }
    }

    callback_octets += count;
    return 0U;
}

static uint32_t foe_read_flash(foe_file_cfg_t *cfg, uint8_t *data,
                               size_t length)
{
    (void)cfg;
    (void)data;
    (void)length;
    return 1U;
}

static uint32_t on_foe_open(uint8_t op)
{
    firmware_write_open = false;
    manifest_ready = false;
    manifest_octets = 0U;
    expected_payload_octets = 0U;
    expected_wire_octets = 0U;
    programmed_octets = 0U;
    callback_octets = 0U;

    DPRINT("%s op=%u state=0x%02x\n", __FUNCTION__, op, ESCvar.ALstatus);

    if (op != FOE_OP_WRQ || ESCvar.ALstatus != ESCboot ||
        gFOE_config.buffer_size == 0U ||
        gFOE_config.buffer_size > sizeof(foe_buffer) ||
        (gFOE_config.buffer_size % FLASH_PROGRAM_SIZE_BYTES) != 0U ||
        foe_update_prepare() != 0U) {
        DPRINT("%s rejected or commit erase failed\n", __FUNCTION__);
        return 1U;
    }

    DPRINT("%s ready; commit invalidated\n", __FUNCTION__);
    firmware_write_open = true;
    return 0U;
}

static uint32_t on_foe_close(void)
{
    const bool was_open = firmware_write_open;
    const uint32_t received = firmware_files[0].total_size;
    firmware_write_open = false;

    DPRINT("%s received=%" PRIu32 " expected=%" PRIu32
           " programmed=%" PRIu32 "\n", __FUNCTION__, received,
           expected_wire_octets, programmed_octets);

    if (!was_open || ESCvar.ALstatus != ESCboot ||
        !manifest_ready || received != expected_wire_octets ||
        programmed_octets != expected_payload_octets) {
        DPRINT("%s incomplete update\n", __FUNCTION__);
        return 1U;
    }

    if (Program_flash(BOOT_MANIFEST_ADDR, wire_manifest,
                      BOOT_MANIFEST_OCTETS) != Fapi_Status_Success) {
        DPRINT("%s manifest program failed\n", __FUNCTION__);
        return 1U;
    }

    const uint32_t status =
        foe_update_validate_and_stage(BOOT_APP_ADDR, received);
    DPRINT("%s validation %s\n", __FUNCTION__, status == 0U ? "OK" : "FAIL");
    return status;
}
