#include "boot_f29h85x.h"

#include "boot_image.h"
#include "ethercat_subdevice_cpu1_hal.h"
#include "flash_utils.h"
#include <soes/esc.h>

#define BOOT_REQUEST_MAGIC 0xB007B007UL

__attribute__((section(".boot_request")))
static volatile struct {
    uint32_t magic;
    uint32_t magic_inverse;
} boot_request;

static bool force_recovery;

static uint8_t read_octet(uint32_t address)
{
    return *(const volatile uint8_t *)address;
}

static bool program_octets(const uint8_t *data, uint32_t address, uint32_t count)
{
    return Program_flash(address, data, count) == Fapi_Status_Success;
}

static bool erase_octets(uint32_t address, uint32_t count)
{
    return Erase_flash(address, count) == Fapi_Status_Success;
}

static const boot_flash_io_t flash_io = {
    read_octet,
    program_octets,
    erase_octets,
};

bool boot_capture_request(void)
{
    force_recovery = (boot_request.magic == BOOT_REQUEST_MAGIC) &&
                     (boot_request.magic_inverse == ~BOOT_REQUEST_MAGIC);
    boot_request.magic = 0UL;
    boot_request.magic_inverse = 0UL;
    return force_recovery;
}

uint32_t foe_update_prepare(void)
{
    return boot_image_prepare(&flash_io) ? 0UL : 1UL;
}

uint32_t foe_update_validate_and_stage(uint32_t address, uint32_t octets)
{
    return (address == BOOT_APP_ADDR && boot_image_finish(&flash_io, octets)) ?
           0UL : 1UL;
}

void boot_try_start_application(void)
{
    if (!force_recovery && boot_image_valid(&flash_io, true)) {
        void (*application_entry)(void) = (void (*)(void))BOOT_APP_ADDR;
        DINT;
        application_entry();
        for (;;) {}
    }
}

void boot_post_state_change(uint8_t *as, uint8_t *an)
{
    if (*as == BOOT_TO_INIT && *an == ESCinit) {
        void (*boot_entry)(void) = (void (*)(void))BOOT_LOADER_ENTRY_ADDR;
        DINT;
        boot_entry();
        for (;;) {}
    }
}
