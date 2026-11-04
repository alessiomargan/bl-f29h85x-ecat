#include <soes/esc.h>
#include <soes/esc_foe.h>

#include "boot_f29h85x.h"
#include "boot_image.h"
#include "flash_utils.h"
#include "globals.h"

static bool firmware_write_open;
static uint32_t programmed_octets;

static uint32_t foe_write_flash(foe_file_cfg_t *cfg, uint8_t *data,
                                size_t length);
static uint32_t foe_read_flash(foe_file_cfg_t *cfg, uint8_t *data,
                               size_t length);
static uint32_t on_foe_open(uint8_t op);
static uint32_t on_foe_close(void);

static foe_file_cfg_t firmware_files[] = {
    {
        .name = "f29h85x.bin",
        .max_data = BOOT_WIRE_OCTETS,
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
    const uint32_t address = cfg->dest_start_address + cfg->address_offset;

    if (!firmware_write_open || ESCvar.ALstatus != ESCboot ||
        cfg != &firmware_files[0] || data == NULL || count == 0U ||
        (count % FLASH_PROGRAM_SIZE_BYTES) != 0U ||
        (cfg->address_offset % FLASH_PROGRAM_SIZE_BYTES) != 0U ||
        cfg->address_offset != programmed_octets ||
        cfg->address_offset > BOOT_WIRE_OCTETS ||
        count > BOOT_WIRE_OCTETS - cfg->address_offset) {
        firmware_write_open = false;
        return 1U;
    }

    if (Program_flash(address, data, count) != Fapi_Status_Success) {
        firmware_write_open = false;
        return 1U;
    }

    programmed_octets += count;
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
    programmed_octets = 0U;

    if (op != FOE_OP_WRQ || ESCvar.ALstatus != ESCboot ||
        gFOE_config.buffer_size == 0U ||
        gFOE_config.buffer_size > sizeof(foe_buffer) ||
        (gFOE_config.buffer_size % FLASH_PROGRAM_SIZE_BYTES) != 0U ||
        foe_update_prepare() != 0U) {
        return 1U;
    }

    if (Erase_flash(BOOT_APP_ADDR,
                    BOOT_SLOT_OCTETS - BOOT_SECTOR_OCTETS) !=
        Fapi_Status_Success) {
        return 1U;
    }

    firmware_write_open = true;
    return 0U;
}

static uint32_t on_foe_close(void)
{
    const bool was_open = firmware_write_open;
    const uint32_t received = firmware_files[0].total_size;
    firmware_write_open = false;

    if (!was_open || ESCvar.ALstatus != ESCboot ||
        received != BOOT_WIRE_OCTETS || programmed_octets < received ||
        programmed_octets - received >= gFOE_config.buffer_size) {
        return 1U;
    }

    return foe_update_validate_and_stage(BOOT_APP_ADDR, received);
}
