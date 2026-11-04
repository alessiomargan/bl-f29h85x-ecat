#ifndef FLASH_UTILS_H
#define FLASH_UTILS_H

#include "flash.h"
#include "FlashTech_F29H85x_C29x.h"

#define FLASH_PARAM_ADDR                 C29FLASHBANKFR4RP0STARTADDRESS
#define DATA_FLASH_SECTOR_SIZE_BYTES     0x800U
#define CODE_FLASH_SECTOR_SIZE_BYTES     0x1000U
#define FLASH_PROGRAM_SIZE_BYTES         16U
#define DATA_FLASH_PROGRAM_SIZE_BYTES    FLASH_PROGRAM_SIZE_BYTES
#define DataBankSector2KB_u32length      0x200U
#define CodeFlashSector4KB_u32length     0x400U

Fapi_StatusType Configure_flashAPI(void);
Fapi_StatusType Erase_flash(uint32_t address, uint32_t size);
Fapi_StatusType Program_flash(uint32_t address, const uint8_t *src, uint32_t size);

/* Compatibility API used by the shared parameter module. */
Fapi_StatusType Erase_dataFlashSector(uint32_t address, uint32_t size);
Fapi_StatusType Program_dataFlashSector(const uint8_t *src, uint32_t address,
                                        uint32_t size);
bool Write_flash(uint32_t flash_addr, const void *src, uint32_t size);

#endif
