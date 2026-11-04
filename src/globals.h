#ifndef __GLOBALS_H__
#define __GLOBALS_H__


/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>
#include <cc.h>

#include <soes/esc_foe.h>

#define FLASH_SIGN_VALID		0xCAFE
#define FLASH_SIGN_DEFAULT		0xDEFA

#define FATAL_ERROR_FLASH_API       1U
#define FATAL_ERROR_DEFAULT_PARAMS  2U
#define FATAL_ERROR_ESCSS_HW_INIT   3U

typedef struct {
	// empty
} tx_pdo_t;

typedef struct {
	// empty
} rx_pdo_t;

typedef struct {
    // 0x8001 ram param
    char 		bl_ver[8];
	uint16_t 	ctrl_cmd;
	uint16_t 	ctrl_cmd_ack;
	uint16_t 	flash_cmd;
	uint16_t 	flash_cmd_ack;
	uint32_t 	crc_app;
	uint32_t 	crc_cal;
} ram_sdo_t;

typedef struct {
    //flash_sdo_t	flash;
    ram_sdo_t	ram;
} sdo_t;

extern tx_pdo_t 			tx_pdo;
extern rx_pdo_t 			rx_pdo;
extern uint8_t *            rxpdo;
extern uint8_t *            txpdo;
extern sdo_t 				sdo;

extern const char       git_hash[];
extern const char       git_branch[];
extern const char       git_tag[];
extern const char       build_ts[];

extern foe_cfg_t 	gFOE_config;
extern uint8_t		foe_buffer[0x400];
extern foe_file_cfg_t	gFOE_firmware_files[];

void print_sdo(const ram_sdo_t *);
void print_build_info(void);

#endif
