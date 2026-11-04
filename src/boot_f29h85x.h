#ifndef BOOT_F29H85X_H
#define BOOT_F29H85X_H

#include <stdbool.h>
#include <stdint.h>

bool boot_capture_request(void);
void boot_try_start_application(void);
void boot_post_state_change(uint8_t *as, uint8_t *an);
uint32_t foe_update_prepare(void);
uint32_t foe_update_validate_and_stage(uint32_t address, uint32_t octets);

#endif
