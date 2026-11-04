#include "ethercat_subdevice_cpu1_hal.h"
#include "board.h"

#include <soes/ecat_slv.h>
#include "boot_f29h85x.h"
#include "flash_utils.h"
#include "globals.h"

extern esc_cfg_t config;
static uint8_t log_buff[128];

int dbg_printf(const char *fmt, ...)
{
    va_list args;
    int written;

    va_start(args, fmt);
    written = vsnprintf((char *)log_buff, sizeof(log_buff), fmt, args);
    UART_writeCharArray(dbg_UART0_BASE, log_buff, strlen((char *)log_buff));
    va_end(args);
    return written;
}

__attribute__((interrupt))
void INT_myCPUTIMER0_ISR(void)
{
    GPIO_writePin(dbg_1, 1U);
    if (!ESC_SYNCactivation()) {
        ecat_slv();
    }
    GPIO_writePin(dbg_1, 0U);
}

void Sync0_Isr(void)
{
    GPIO_writePin(dbg_2, 1U);
    ecat_slv();
    GPIO_writePin(dbg_2, 0U);
}

int main(void)
{
    const bool force_bootloader = boot_capture_request();
    uint16_t init_status;

    /* Preserve the integrated ESC during a warm application handoff. */
    init_status = ESC_initHWMode(force_bootloader);
    Board_init();
    print_build_info();

    if (init_status == ESC_HW_INIT_FAIL) {
        DPRINT("FAIL ESC_initHWMode\n");
        for (;;) {}
    }

    if (Configure_flashAPI() != Fapi_Status_Success) {
        DPRINT("FAIL Configure_flashAPI\n");
        for (;;) {}
    }

    DPRINT("Boot request = %s\n", force_bootloader ? "FORCED" : "NORMAL");
    boot_try_start_application();

    esc_cfg_t boot_config = config;
    boot_config.post_state_change_hook = boot_post_state_change;
    ecat_slv_init(&boot_config);

    CPUTimer_startTimer(myCPUTIMER0_BASE);
    EINT;

    for (;;) {
        GPIO_togglePin(DEVICE_GPIO_PIN_LED1);
        DEVICE_DELAY_US((uint32_t)500000U);
    }
}
