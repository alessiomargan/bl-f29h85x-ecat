--entry_point="code_start"

MEMORY
{
    SRAM_LDAx        : o=0x200E0000, l=0x1FFF8
    BOOT_REQUEST     : o=0x200FFFF8, l=0x00008
    SRAM_LPAx        : o=0x20100000, l=0x10000
    SRAM_CPAx        : o=0x20110000, l=0x10000
    SRAM_CDAx        : o=0x20120000, l=0x30000

    CERT             : o=0x10000000, l=0x01000
    CPU1_FLASH_SBL   : o=0x10001000, l=0x3F000

    FLASH_PARAMS     : o=0x10C00000, l=0x00800
    FLASH_CALIB      : o=0x10C00800, l=0x00800
}

SECTIONS
{
    cert          : {} > CERT,            palign(8)
    codestart     : {} > 0x10001000,      palign(8)

    .text         : {} > CPU1_FLASH_SBL,  palign(8)
    .binit        : {} > CPU1_FLASH_SBL,  palign(8)
    .cinit        : {} > CPU1_FLASH_SBL,  palign(8)
    .const        : {} > CPU1_FLASH_SBL,  palign(8)
    .rodata       : {} > CPU1_FLASH_SBL,  palign(8)
    .init_array   : {} > CPU1_FLASH_SBL,  palign(8)

    .TI.ramfunc   : { -l F29H85x_NWFlashAPI_v21.00.00.00.lib }
                    LOAD=CPU1_FLASH_SBL, RUN=SRAM_LPAx,
                    table(BINIT), palign(8)

    .data         : {} > SRAM_LDAx
    .bss          : {} > SRAM_LDAx
    .stack        : {} > SRAM_LDAx
    .sysmem       : {} > SRAM_LDAx
    .cio          : {} > SRAM_LDAx
    .boot_request : {} > BOOT_REQUEST, type=NOINIT

    .params       : {} > FLASH_PARAMS, palign(8), type=NOLOAD
    .calib        : {} > FLASH_CALIB,  palign(8), type=NOLOAD
}
