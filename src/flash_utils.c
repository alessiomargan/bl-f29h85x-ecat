#include "flash_utils.h"

#include "boot_image.h"
#include "board.h"
#include <stdio.h>

void Disable_interrupt(void) { DINT; }
void Enable_interrupt(void)  { EINT; }

static uint32_t user_flash_config;
static bool flash_api_configured;

static bool range_inside(uint32_t address, uint32_t size,
                         uint32_t begin, uint32_t end_plus_one)
{
    return size != 0U && address >= begin && address < end_plus_one &&
           size <= end_plus_one - address;
}

static bool is_application_range(uint32_t address, uint32_t size)
{
    return range_inside(address, size, BOOT_APP_ADDR,
                        BOOT_APP_ADDR + BOOT_SLOT_OCTETS);
}

static bool is_data_range(uint32_t address, uint32_t size)
{
    return range_inside(address, size, C29FLASHBANKFR4RP0STARTADDRESS,
                        C29FLASHBANKFR4RP0ENDADDRESSPLUS1);
}

static inline __attribute__((always_inline))
Fapi_StatusType release_semaphore(Fapi_StatusType status)
{
    return SSU_releaseFlashSemaphore() ? status : Fapi_Error_Fail;
}

static __attribute__((section(".TI.ramfunc")))
Fapi_StatusType clear_fsm_status(uint32_t address)
{
    Fapi_StatusType status;

    while (Fapi_checkFsmForReady(address, user_flash_config) !=
           Fapi_Status_FsmReady) {}

    if (Fapi_getFsmStatus(address, user_flash_config) == 0U) {
        return Fapi_Status_Success;
    }

    status = Fapi_issueAsyncCommand(address, user_flash_config,
                                    Fapi_ClearStatus);
    while (Fapi_getFsmStatus(address, user_flash_config) != 0U) {}
    return status;
}

__attribute__((section(".TI.ramfunc")))
Fapi_StatusType Configure_flashAPI(void)
{
    Fapi_StatusType status;
    Fapi_LibraryInfoType info;

    flash_api_configured = false;
    Flash_initModule(3);

    if (!SSU_claimFlashSemaphore()) {
        return Fapi_Error_Fail;
    }

    user_flash_config = Fapi_getUserConfiguration(C29Bank, Active_Bank);
    status = Fapi_SetFlashCPUConfiguration(Mode1);
    if (status != Fapi_Status_Success) {
        return release_semaphore(status);
    }

    status = Fapi_initializeAPI((Fapi_FmcRegistersType *)FLASHCONTROLLER1_BASE,
                                DEVICE_SYSCLK_FREQ / 1000000U);
    if (status != Fapi_Status_Success) {
        return release_semaphore(status);
    }

    if (!SSU_releaseFlashSemaphore()) {
        return Fapi_Error_Fail;
    }

    flash_api_configured = true;
    info = Fapi_getLibraryInfo();
    printf("Flash lib info %u.%u %u\n", info.u8ApiMajorVersion,
           info.u8ApiMinorVersion, info.u8ApiRevision);
    return Fapi_Status_Success;
}

__attribute__((section(".TI.ramfunc")))
Fapi_StatusType Erase_flash(uint32_t address, uint32_t size)
{
    Fapi_FlashStatusWordType flash_status_word;
    Fapi_StatusType status;
    const bool code_flash = is_application_range(address, size);
    const bool data_flash = is_data_range(address, size);
    const uint32_t sector_size = code_flash ? CODE_FLASH_SECTOR_SIZE_BYTES :
                                             DATA_FLASH_SECTOR_SIZE_BYTES;
    const uint32_t blank_words = code_flash ? CodeFlashSector4KB_u32length :
                                             DataBankSector2KB_u32length;

    if (!flash_api_configured || (!code_flash && !data_flash) ||
        (address % sector_size) != 0U) {
        return Fapi_Error_InvalidAddress;
    }

    const uint32_t sectors = (size + sector_size - 1U) / sector_size;
    if (!SSU_claimFlashSemaphore()) {
        return Fapi_Error_Fail;
    }

    for (uint32_t current = address;
         current < address + sectors * sector_size;
         current += sector_size) {
        const uint8_t first_iterator = code_flash ? 1U : 0U;
        const uint8_t last_iterator = code_flash ? 2U : 0U;

        for (uint8_t iterator = first_iterator; iterator <= last_iterator;
             ++iterator) {
            status = clear_fsm_status(current);
            if (status != Fapi_Status_Success) {
                return release_semaphore(status);
            }

            Fapi_setupBankSectorEnable((uint32_t *)current, user_flash_config,
                                       FLASH_NOWRAPPER_O_CMDWEPROTA, 0U);
            Fapi_setupBankSectorEnable((uint32_t *)current, user_flash_config,
                                       FLASH_NOWRAPPER_O_CMDWEPROTB, 0U);
            status = Fapi_issueAsyncCommandWithAddress(
                Fapi_EraseSector, (uint32_t *)current, iterator,
                user_flash_config);
            while (Fapi_checkFsmForReady(current, user_flash_config) !=
                   Fapi_Status_FsmReady) {}
            if (status != Fapi_Status_Success ||
                Fapi_getFsmStatus(current, user_flash_config) != 3U) {
                return release_semaphore(status == Fapi_Status_Success ?
                                         Fapi_Error_Fail : status);
            }
        }

        status = Fapi_doBlankCheck((uint32_t *)current, blank_words,
                                   &flash_status_word, 0U,
                                   user_flash_config);
        if (status != Fapi_Status_Success) {
            return release_semaphore(status);
        }
    }

    return release_semaphore(Fapi_Status_Success);
}

__attribute__((section(".TI.ramfunc")))
Fapi_StatusType Program_flash(uint32_t address, const uint8_t *src,
                              uint32_t size)
{
    Fapi_FlashStatusWordType flash_status_word;
    uint8_t program_data[FLASH_PROGRAM_SIZE_BYTES];
    Fapi_StatusType status;

    if (!flash_api_configured || src == NULL ||
        (!is_application_range(address, size) && !is_data_range(address, size)) ||
        (address % FLASH_PROGRAM_SIZE_BYTES) != 0U) {
        return Fapi_Error_InvalidAddress;
    }

    if (!SSU_claimFlashSemaphore()) {
        return Fapi_Error_Fail;
    }

    for (uint32_t offset = 0U; offset < size;
         offset += FLASH_PROGRAM_SIZE_BYTES) {
        uint32_t chunk = size - offset;
        const uint32_t current = address + offset;
        if (chunk > FLASH_PROGRAM_SIZE_BYTES) {
            chunk = FLASH_PROGRAM_SIZE_BYTES;
        }

        for (uint32_t i = 0U; i < FLASH_PROGRAM_SIZE_BYTES; ++i) {
            program_data[i] = (i < chunk) ? src[offset + i] : 0xFFU;
        }

        status = clear_fsm_status(current);
        if (status != Fapi_Status_Success) {
            return release_semaphore(status);
        }

        Fapi_setupBankSectorEnable((uint32_t *)current, user_flash_config,
                                   FLASH_NOWRAPPER_O_CMDWEPROTA, 0U);
        Fapi_setupBankSectorEnable((uint32_t *)current, user_flash_config,
                                   FLASH_NOWRAPPER_O_CMDWEPROTB, 0U);
        status = Fapi_issueProgrammingCommand(
            (uint32_t *)current, program_data, FLASH_PROGRAM_SIZE_BYTES,
            0, 0, Fapi_AutoEccGeneration, user_flash_config);
        while (Fapi_checkFsmForReady(current, user_flash_config) ==
               Fapi_Status_FsmBusy) {}
        if (status != Fapi_Status_Success ||
            Fapi_getFsmStatus(current, user_flash_config) != 3U) {
            return release_semaphore(status == Fapi_Status_Success ?
                                     Fapi_Error_Fail : status);
        }

        status = Fapi_doVerifyByByte((uint8_t *)current,
                                     FLASH_PROGRAM_SIZE_BYTES,
                                     program_data, &flash_status_word, 0U,
                                     user_flash_config);
        if (status != Fapi_Status_Success) {
            return release_semaphore(status);
        }
    }

    return release_semaphore(Fapi_Status_Success);
}

Fapi_StatusType Erase_dataFlashSector(uint32_t address, uint32_t size)
{
    return Erase_flash(address, size);
}

Fapi_StatusType Program_dataFlashSector(const uint8_t *src, uint32_t address,
                                        uint32_t size)
{
    return Program_flash(address, src, size);
}

bool Write_flash(uint32_t flash_addr, const void *src, uint32_t size)
{
    return Erase_flash(flash_addr, size) == Fapi_Status_Success &&
           Program_flash(flash_addr, (const uint8_t *)src, size) ==
               Fapi_Status_Success;
}
