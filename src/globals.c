#include "globals.h"
#include "flash_utils.h"
#define BUILD_INFO_IMPLEMENTATION
#include "build_info.h"

#include <stdio.h>

/************************************************************************************
 * Ethercat PDOs SDOs
 */
rx_pdo_t    	rx_pdo;
tx_pdo_t    	tx_pdo;
uint8_t *       rxpdo = (uint8_t*)&rx_pdo;
uint8_t *       txpdo = (uint8_t*)&tx_pdo;

sdo_t	sdo = {
	.ram.bl_ver = BUILD_SHORT_TS
};


void print_sdo(const ram_sdo_t *s) {

	DPRINT("sdo.ram.bl_ver=%s\n", s->bl_ver);
    DPRINT("sdo.ram.crc_app=%" PRIX32 "\n", s->crc_app);
    DPRINT("sdo.ram.crc_cal=%" PRIX32 "\n", s->crc_cal);
}


/************************************************************************************
 * FOE files
 */
foe_cfg_t 	gFOE_config = { 0, 0, 0 ,0 };
#pragma DATA_SECTION(foe_buffer, "ramgs1")
uint8_t		foe_buffer[0x400];
