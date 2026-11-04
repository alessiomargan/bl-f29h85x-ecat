#include <string.h>

#include <soes/esc.h>
#include <soes/esc_coe.h>
#include <soes/esc_foe.h>
#include <soes/hal/advr_esc/soes.h>
#include <ecat_options.h>
#include <globals.h>


/* Forward declaration of the default fault handlers. */
void Default_Object_Handler(uint8_t) __attribute__((weak));

extern void Handle_0x8001(uint8_t) __attribute__((weak, alias("Default_Object_Handler")));

void pre_state_change_hook (uint8_t * as, uint8_t * an);
void post_state_change_hook (uint8_t * as, uint8_t * an);
uint32_t ESC_App_objecthandler (uint16_t index, uint8_t subindex, uint16_t flags);

/* Setup config hooks */
const esc_cfg_t config =
{
    .user_arg					= "hello_world",
    .use_interrupt				= 1,
    .watchdog_cnt				= 0,
	.skip_default_initialization = true,
    .set_defaults_hook 			= NULL,
    .pre_state_change_hook 		= pre_state_change_hook,
    .post_state_change_hook 	= post_state_change_hook,
    .application_hook 			= NULL,
    .safeoutput_override 		= NULL,
    .pre_object_download_hook 	= NULL,
    .post_object_download_hook 	= ESC_App_objecthandler,
	.pre_object_upload_hook		= NULL,
	.post_object_upload_hook	= NULL,
    .rxpdo_override				= NULL,
    .txpdo_override				= NULL,
    .esc_hw_interrupt_enable	= NULL,
    .esc_hw_interrupt_disable	= NULL,
    .esc_hw_eep_handler			= NULL,
	.esc_check_dc_handler		= NULL,
	.get_device_id				= NULL,
};

void Default_Object_Handler(uint8_t subindex) {
	DPRINT("%s %d ... not handled\n", __FUNCTION__, subindex);
}

/** Mandatory: Hook called from the slave stack SDO Download handler to act on
 * user specified Index and Sub-index.
 *
 * @param[in] index      = index of SDO download request to handle
 * @param[in] sub-index  = sub-index of SDO download request to handle
 */
uint32_t ESC_App_objecthandler (uint16_t index, uint8_t subindex, uint16_t flags)
{
	switch ( index ) {
    	case 0x8001:
 			Handle_0x8001(subindex);
 			break;
         default:
             DPRINT("SDO 0x%04X %d NOT Handled\n", index, subindex);
             break;
    }

	// TODO handle sub-func returns
	return 0;
}

/** Optional: Hook called BEFORE state change for application
 * specific actions for specific state changes. 
 */
void pre_state_change_hook (uint8_t * as, uint8_t * an)
{
    DPRINT ("pre_state_change_hook 0x%02X %d\n", *as, *an);
	/* Add specific step change hooks here */
    
}

/** Optional: Hook called AFTER state change for application
 * specific actions for specific state changes. 
 */
void post_state_change_hook (uint8_t * as, uint8_t * an)
{
    DPRINT ("post_state_change_hook 0x%02X %d\n", *as, *an);
    /* Add specific step change hooks here */
}


/*
 * cb_set_outputs();
 * cb_get_inputs();
 * is equal to call ecat_process_pdo() without RXPDO_update TXPDO_update();
 */

void cb_set_outputs() {

}

void cb_get_inputs() {

}

