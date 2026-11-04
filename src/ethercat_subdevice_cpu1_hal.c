//###########################################################################
//
// FILE:   ethercat_slave_cpu1_hal.c
//
// TITLE:  C29.CPU1 EtherCAT HAL Driver.
//
//###########################################################################
// $TI Release: F2838x EtherCAT Software v2.02.00.00 $
// $Release Date: Mon Aug 12 15:29:33 IST 2024 $
// $Copyright:
// Copyright (C) 2024 Texas Instruments Incorporated - http://www.ti.com/
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions 
// are met:
// 
//   Redistributions of source code must retain the above copyright 
//   notice, this list of conditions and the following disclaimer.
// 
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the 
//   documentation and/or other materials provided with the   
//   distribution.
// 
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// $
//###########################################################################

//
// Included Files
//
#include "ethercat_subdevice_cpu1_hal.h"
#include "clockconfig.h"
#include "gpio.h"
#include "sysctl.h"

extern void Sync0_Isr(void);

#ifdef PDI_HAL_TEST

//
// HAL Test Application - Register Debug Structure
//
ESC_DebugRegisters ESC_escRegs[ESC_HAL_TEST_DEBUG_REGS_LENGTH];

//
// HAL Block Data Arrays
//
static uint32_t ESC_writeBlockData[ESC_HAL_TEST_BLOCK_LENGTH / 4U];
static uint32_t ESC_readBlockData[ESC_HAL_TEST_BLOCK_LENGTH / 4U];

#endif // PDI_HAL_TEST

//*****************************************************************************
//
// ESC_getTimer
//
//*****************************************************************************
uint32_t
ESC_getTimer(void)
{
    //
    // Return 1's compliment of the CPU timer
    //
    return(~((uint32_t)CPUTimer_getTimerCount(CPUTIMER0_BASE)));
}

//*****************************************************************************
//
// ESC_clearTimer
//
//*****************************************************************************
void
ESC_clearTimer(void)
{
    //
    // Set the timer period count
    //
    CPUTimer_setPeriod(CPUTIMER0_BASE, 0xFFFFFFFFUL);

    //
    // Reload timer with the period count value
    //
    CPUTimer_reloadTimerCounter(CPUTIMER0_BASE);
}

//*****************************************************************************
//
// ESC_timerIncPerMilliSec
//
//*****************************************************************************
uint32_t
ESC_timerIncPerMilliSec(void)
{
    //
    // Returns value based on core frequency of 200MHz.
    //
    return((uint32_t)200000UL);
}

//*****************************************************************************
//
// ESC_readBlockISR
//
//*****************************************************************************
void
ESC_readBlockISR(ESCMEM_ADDR *pData, uint16_t address, uint16_t len)
{
    //
    // Create pointer to ESC Address
    //
    const void * escSource = (void *)(ESC_BASE + (uint32_t)(address));
    //
    // Copy ESC data to buffer. Convert lengths to words. 
    //
    memcpy(pData, escSource, (size_t)len);
}

//*****************************************************************************
//
// ESC_readBlock
//
//*****************************************************************************
void
ESC_readBlock(ESCMEM_ADDR *pData, uint16_t address, uint16_t len)
{
    //
    // Disables interrupts
    //
    DINT;

    //
    // Perform copy of ESC data to buffer
    //
    ESC_readBlockISR(pData, address, len);

    //
    // Enable interrupts
    //
    EINT;
}

//*****************************************************************************
//
// ESC_readDWordISR
//
//*****************************************************************************
uint32_t
ESC_readDWordISR(uint16_t address)
{
    //
    // Read 32-bit address from ESC memory
    //
    return(HWREG(ESC_BASE + address));
}

//*****************************************************************************
//
// ESC_readDWord
//
//*****************************************************************************
uint32_t
ESC_readDWord(uint16_t address)
{
    uint32_t dWordValue;

    //
    // Disables interrupts
    //
    DINT;

    //
    // Read 32-bit address from ESC memory
    //
    dWordValue = ESC_readDWordISR(address);

    //
    // Enable interrupts
    //
    EINT;

    //
    // Return value
    //
    return(dWordValue);
}

//*****************************************************************************
//
// ESC_readWordISR
//
//*****************************************************************************
uint16_t
ESC_readWordISR(uint16_t address)
{
    //
    // Read 16-bit address from ESC memory
    //
    return(HWREGH(ESC_BASE + address));
}

//*****************************************************************************
//
// ESC_readWord
//
//*****************************************************************************
uint16_t
ESC_readWord(uint16_t address)
{
    uint16_t wordValue;

    //
    // Disables interrupts
    //
    DINT;

    //
    // Read 16-bit address from ESC memory
    //
    wordValue = ESC_readWordISR(address);

    //
    // Enable interrupts
    //
    EINT;

    //
    // Return value
    //
    return(wordValue);
}

//*****************************************************************************
//
// ESC_readByteISR
//
//*****************************************************************************
uint8_t
ESC_readByteISR(uint16_t address)
{
    //
    // Read 8-bit address from ESC memory
    //
    return(HWREGB(ESC_BASE + address));
}

//*****************************************************************************
//
// ESC_readByte
//
//*****************************************************************************
uint8_t
ESC_readByte(uint16_t address)
{
    uint8_t byteValue;

    //
    // Disables interrupts
    //
    DINT;

    //
    // Read 8-bit address from ESC memory
    //
    byteValue = ESC_readByteISR(address);

    //
    // Enable interrupts
    //
    EINT;

    //
    // Return Value
    //
    return(byteValue);
}


//*****************************************************************************
//
// ESC_writeBlockISR
//
//*****************************************************************************
void
ESC_writeBlockISR(ESCMEM_ADDR *pData, uint16_t address, uint16_t len)
{
    uint16_t wordValue;

    //
    // Create pointer to ESC Address
    //
    void * escDest = (void *)(ESC_BASE + (uint32_t)(address));

    //
    // copy buffer data to ESC Memory
    //
    memcpy(escDest, pData, (size_t)len);
}

//*****************************************************************************
//
// ESC_writeBlock
//
//*****************************************************************************
void
ESC_writeBlock(ESCMEM_ADDR *pData, uint16_t address, uint16_t len)
{
    //
    // Disables interrupts
    //
    DINT;

    //
    // Write data buffer into ESC memory
    //
    ESC_writeBlockISR(pData, address, len);

    //
    // Enable interrupts
    //
    EINT;
}

//*****************************************************************************
//
// ESC_writeDWordISR
//
//*****************************************************************************
void
ESC_writeDWordISR(uint32_t dWordValue, uint16_t address)
{
    //
    // Write two 16-bit words to ESC memory
    //
    HWREG(ESC_BASE + (address)) = dWordValue;
}

//*****************************************************************************
//
// ESC_writeDWord
//
//*****************************************************************************
void
ESC_writeDWord(uint32_t dWordValue, uint16_t address)
{
    //
    // Disables interrupts
    //
    DINT;

    //
    // Write two 16-words into ESC memory
    //
    ESC_writeDWordISR(dWordValue, address);

    //
    // Enable interrupts
    //
    EINT;
}

//*****************************************************************************
//
// ESC_writeWordISR
//
//*****************************************************************************
void
ESC_writeWordISR(uint16_t wordValue, uint16_t address)
{
    //
    // Write 16-bit word into ESC memory
    //
    HWREGH(ESC_BASE + (address)) = wordValue;
}

//*****************************************************************************
//
// ESC_writeWord
//
//*****************************************************************************
void
ESC_writeWord(uint16_t wordValue, uint16_t address)
{
    //
    // Disables interrupts
    //
    DINT;

    //
    // Write one 16-word into ESC memory
    //
    ESC_writeWordISR(wordValue, address);

    //
    // Enable interrupts
    //
    EINT;
}

//*****************************************************************************
//
// ESC_writeByteISR
//
//*****************************************************************************
void
ESC_writeByteISR(uint8_t byteValue, uint16_t address)
{
    //
    // Write 8-bit data into ESC memory
    //
    HWREGB(ESC_BASE + address) = byteValue;
}

//*****************************************************************************
//
// ESC_writeByte
//
//*****************************************************************************
void
ESC_writeByte(uint8_t byteValue, uint16_t address)
{
    //
    // Disables interrupts
    //
    Interrupt_disableGlobal();

    //
    // Write one byte into ESC memory
    //
    ESC_writeByteISR(byteValue, address);

    //
    // Enable interrupts
    //
    Interrupt_enableGlobal();
}

//*****************************************************************************
//
// ESC_setLed
//
//*****************************************************************************
void
ESC_setLed(uint16_t runLed, uint16_t errLed)
{
    //
    // Set/Clear Run LED
    //
    GPIO_writePin(ESC_RUN_LED_GPIO, (uint32_t)(runLed));

    //
    // Set/Clear Error LED
    //
    GPIO_writePin(ESC_ERR_LED_GPIO, (uint32_t)(errLed));
}

#ifdef PDI_HAL_TEST
//*****************************************************************************
//
// HAL_initWriteBlockData
//
//*****************************************************************************
void HAL_initWriteBlockData(void)
{
    uint16_t i;

    //
    // Initialize write block data array
    //
    for(i = 0U; i < (ESC_HAL_TEST_BLOCK_LENGTH / 4U); i++)
    {
        ESC_writeBlockData[i] = 0x1234ABCDUL;
    }
}

//*****************************************************************************
//
// HAL_clearReadBlockData
//
//*****************************************************************************
void HAL_clearReadBlockData(void)
{
    uint16_t i;

    //
    // Clear read block data array
    //
    for(i = 0U; i < (ESC_HAL_TEST_BLOCK_LENGTH / 4U); i++)
    {
        ESC_readBlockData[i] = 0x0UL;
    }
}

//*****************************************************************************
//
// HAL_testBlockData
//
//*****************************************************************************
uint16_t HAL_testBlockData(uint16_t length)
{
    uint16_t i;
    uint8_t * writeData = (uint8_t *)(&ESC_writeBlockData[0]);
    uint8_t * readData = (uint8_t *)(&ESC_readBlockData[0]);

    //
    // Check that data written and data read match
    //
    for(i = 0U; i < length; i++)
    {
        if(*writeData != *readData)
        {
            return(ESC_HAL_BLOCK_TEST_FAIL);
        }

        writeData++;
        readData++;
    }
    return(ESC_HAL_BLOCK_TEST_PASS);
}

//*****************************************************************************
//
// ESC_setupPDITestInterface
//
//*****************************************************************************
void
ESC_setupPDITestInterface(void)
{
    uint16_t currentAddress = 0x0U;
    uint32_t doubleWordData = 0x0UL;
    uint16_t wordData = 0x0U;
    uint8_t byteData = 0x0U;

    //
    // Setup and add various ESC registers to monitor
    //
    ESC_debugInitESCRegLogs();
    ESC_debugAddESCRegsAddress(0x0U);     // Type, Revision
    ESC_debugAddESCRegsAddress(0x2U);     // Build
    ESC_debugAddESCRegsAddress(0x4U);     // FMMUs, SyncManagers Supported
    ESC_debugAddESCRegsAddress(0x8U);     // ESC Supported Features
    ESC_debugAddESCRegsAddress(0x100U);   // DL Control
    ESC_debugAddESCRegsAddress(0x102U);   // DL Control (Extended)
    ESC_debugAddESCRegsAddress(0x110U);   // DL Status
    ESC_debugAddESCRegsAddress(0x310U);   // Lost Link Counter 1
    ESC_debugAddESCRegsAddress(0x312U);   // Lost Link Counter 2
    ESC_debugAddESCRegsAddress(0x510U);   // MII Control/Status
    ESC_debugAddESCRegsAddress(0x512U);   // PHY Address
    ESC_debugAddESCRegsAddress(0x514U);   // PHY Data
    ESC_debugAddESCRegsAddress(0x516U);   // MII ECAT Access State
    ESC_debugAddESCRegsAddress(0x518U);   // PHY Port Status (1)
    ESC_debugAddESCRegsAddress(0x51AU);   // PHY Port Status (2)
    ESC_debugAddESCRegsAddress(0x1000U);  // First data word of ESC RAM

    //
    // Check that correct PDI type (ASYNC16) is configured
    //
    if(ESC_readByte(ESC_O_PDI_CONTROL) != ESC_PDI_CONTROL_ASYNC16)
    {
        //
        // PDI not operational or incorrect
        //
        while(1)
        {
            //
            // Toggle Error
            //
            ESC_signalFail();
        }
    }

    //
    // Perform PDI read and write tests for entire ESC PDI RAM range
    //
    for(currentAddress = ESC_PDI_RAM_START_ADDRESS_OFFSET;
            currentAddress <= ESC_PDI_RAM_END_ADDRESS_OFFSET;
            currentAddress += 4U)
    {
        //
        // Read DWord and Write DWord API Tests
        //
        doubleWordData = ESC_readDWord(currentAddress);
        ESC_writeDWord(0xABCD1234UL, currentAddress);
        doubleWordData = ESC_readDWord(currentAddress);

        if(doubleWordData != 0xABCD1234UL)
        {
            while(1)
            {
                //
                // Toggle Error
                //
                ESC_signalFail();
            }
        }

        doubleWordData = ESC_readDWordISR(currentAddress);
        ESC_writeDWordISR(0x1A2B3C4DUL, currentAddress);
        doubleWordData = ESC_readDWordISR(currentAddress);

        if(doubleWordData != 0x1A2B3C4DUL)
        {
            while(1)
            {
                //
                // Toggle Error
                //
                ESC_signalFail();
            }
        }

        //
        // Read Word and Write Word API Tests
        //
        wordData = ESC_readWord(currentAddress);
        ESC_writeWord(0x6789U, currentAddress);
        wordData = ESC_readWord(currentAddress);

        if(wordData != 0x6789U)
        {
            while(1)
            {
                //
                // Toggle Error
                //
                ESC_signalFail();
            }
        }

        wordData = ESC_readWordISR(currentAddress);
        ESC_writeWordISR(0x5A5AU, currentAddress);
        wordData = ESC_readWordISR(currentAddress);

        if(wordData != 0x5A5AU)
        {
            while(1)
            {
                //
                // Toggle Error
                //
                ESC_signalFail();
            }
        }

        //
        // Read Byte and Write Byte API Tests
        //
        byteData = ESC_readByte(currentAddress);
        ESC_writeByte(0xEFU, currentAddress);
        byteData = ESC_readByte(currentAddress);

        if(byteData != 0xEFU)
        {
            while(1)
            {
                //
                // Toggle Error
                //
                ESC_signalFail();
            }
        }

        byteData = ESC_readByteISR(currentAddress);
        ESC_writeByteISR(0xBCU, currentAddress);
        byteData = ESC_readByteISR(currentAddress);

        if(byteData != 0xBCU)
        {
            while(1)
            {
                //
                // Toggle Error
                //
                ESC_signalFail();
            }
        }
    }

    //
    // Setup for read/write block API Tests
    //
    HAL_initWriteBlockData();

    //
    // Perform even length write and read block (non-ISR) API Test
    //
    ESC_writeBlock((uint8_t *)(&ESC_writeBlockData[0]),
                   ESC_PDI_RAM_START_ADDRESS_OFFSET,
                   ESC_HAL_TEST_BLOCK_LENGTH);
    ESC_readBlock((uint8_t *)(&ESC_readBlockData[0]),
                  ESC_PDI_RAM_START_ADDRESS_OFFSET,
                  ESC_HAL_TEST_BLOCK_LENGTH);

    if(HAL_testBlockData(ESC_HAL_TEST_BLOCK_LENGTH) != ESC_HAL_BLOCK_TEST_PASS)
    {
        while(1)
        {
            //
            // Toggle Error
            //
            ESC_signalFail();
        }
    }

    HAL_clearReadBlockData();

    //
    // Perform even length write and read block API Test
    //
    ESC_writeBlockISR((uint8_t *)(&ESC_writeBlockData[0]),
                      (ESC_PDI_RAM_START_ADDRESS_OFFSET + 0x1000U),
                      ESC_HAL_TEST_BLOCK_LENGTH);
    ESC_readBlockISR((uint8_t *)(&ESC_readBlockData[0]),
                     (ESC_PDI_RAM_START_ADDRESS_OFFSET + 0x1000U),
                     ESC_HAL_TEST_BLOCK_LENGTH);

    if(HAL_testBlockData(ESC_HAL_TEST_BLOCK_LENGTH) != ESC_HAL_BLOCK_TEST_PASS)
    {
        while(1)
        {
            //
            // Toggle Error
            //
            ESC_signalFail();
        }
    }

    HAL_clearReadBlockData();

    //
    // Perform odd length write and read block (non-ISR) API Test
    //
    ESC_writeBlock((uint8_t *)(&ESC_writeBlockData[0]),
                   (ESC_PDI_RAM_START_ADDRESS_OFFSET + 0x2000U),
                   (ESC_HAL_TEST_BLOCK_LENGTH - 1U));
    ESC_readBlock((uint8_t *)(&ESC_readBlockData[0]),
                  (ESC_PDI_RAM_START_ADDRESS_OFFSET + 0x2000U),
                  (ESC_HAL_TEST_BLOCK_LENGTH - 1U));

    if(HAL_testBlockData((ESC_HAL_TEST_BLOCK_LENGTH - 1U)) !=
            ESC_HAL_BLOCK_TEST_PASS)
    {
        while(1)
        {
            //
            // Toggle Error
            //
            ESC_signalFail();
        }
    }

    HAL_clearReadBlockData();

    //
    // Perform odd length write and read block API Test
    //
    ESC_writeBlockISR((uint8_t *)(&ESC_writeBlockData[0]),
                      (ESC_PDI_RAM_START_ADDRESS_OFFSET + 0x3000U),
                      (ESC_HAL_TEST_BLOCK_LENGTH - 1U));
    ESC_readBlockISR((uint8_t *)(&ESC_readBlockData[0]),
                     (ESC_PDI_RAM_START_ADDRESS_OFFSET + 0x3000U),
                     (ESC_HAL_TEST_BLOCK_LENGTH - 1U));

    if(HAL_testBlockData((ESC_HAL_TEST_BLOCK_LENGTH - 1U)) !=
            ESC_HAL_BLOCK_TEST_PASS)
    {
        while(1)
        {
            //
            // Toggle Error
            //
            ESC_signalFail();
        }
    }

    //
    // Pass
    //
    ESC_signalPass();
}

void
ESC_debugUpdateESCRegLogs(void)
{
    uint16_t i = 0U;

    //
    // Update ESC register data into debug array
    //
    while(ESC_escRegs[i].address != 0xFFFFU)
    {
        ESC_escRegs[i].data = ESC_readWord(ESC_escRegs[i].address);
        i++;
    }
}

//*****************************************************************************
//
// ESC_debugAddESCRegsAddress
//
//*****************************************************************************
void
ESC_debugAddESCRegsAddress(uint16_t address)
{
    uint16_t i = 0U;

    //
    // Add ESC Register Address to debug array
    //
    for(i = 0U; i < ESC_HAL_TEST_DEBUG_REGS_LENGTH; i++)
    {
        if(ESC_escRegs[i].address == 0xFFFFU)
        {
            //
            // Assign byte address to struct
            //
            ESC_escRegs[i].address = address;
            return;
        }
    }
}

//*****************************************************************************
//
// ESC_debugInitESCRegLogs
//
//*****************************************************************************
void
ESC_debugInitESCRegLogs(void)
{
    uint16_t i = 0U;

    //
    // Initialize ESC register debug array
    //
    for(i = 0U; i < ESC_HAL_TEST_DEBUG_REGS_LENGTH; i++)
    {
        ESC_escRegs[i].address = 0xFFFFU;
        ESC_escRegs[i].data = 0xFFFFU;
    }
}

//*****************************************************************************
//
// ESC_signalPass
//
//*****************************************************************************
void
ESC_signalPass(void)
{
    //Turn on LEDs
    GPIO_writePin(DEVICE_GPIO_PIN_LED1, 0);
    GPIO_writePin(DEVICE_GPIO_PIN_LED2, 0);
}

//*****************************************************************************
//
// ESC_signalFail
//
//*****************************************************************************
void
ESC_signalFail(void)
{
    //
        // Set LED GPIOs to output mode
        //
        GPIO_setDirectionMode(DEVICE_GPIO_PIN_LED1, GPIO_DIR_MODE_OUT);
        GPIO_setDirectionMode(DEVICE_GPIO_PIN_LED2, GPIO_DIR_MODE_OUT);
    //
    // Toggle controlCARD LEDs and delay
    //

    //Turn on LEDs
    GPIO_writePin(DEVICE_GPIO_PIN_LED1, 0);
    GPIO_writePin(DEVICE_GPIO_PIN_LED2, 0);

    // wait for 500ms
    DEVICE_DELAY_US((uint32_t)(500000)); 

    // Turn off LEDs
    GPIO_writePin(DEVICE_GPIO_PIN_LED1, 1);
    GPIO_writePin(DEVICE_GPIO_PIN_LED2, 1);

    // Wait for 500ms
    DEVICE_DELAY_US((uint32_t)(500000)); 
}

//*****************************************************************************
//
// ESC_passFailSignalSetup
//
//*****************************************************************************
void
ESC_passFailSignalSetup(void)
{
    //
        // Set LED GPIOs to output mode
        //
        GPIO_setDirectionMode(DEVICE_GPIO_PIN_LED1, GPIO_DIR_MODE_OUT);
        GPIO_setDirectionMode(DEVICE_GPIO_PIN_LED2, GPIO_DIR_MODE_OUT);
    //
    // Set LED GPIOs to output mode
    //
    GPIO_setDirectionMode(CCARD_LED_1_GPIO, GPIO_DIR_MODE_OUT);
    GPIO_setDirectionMode(CCARD_LED_2_GPIO, GPIO_DIR_MODE_OUT);

    //
    // Turn off controlCARD LEDs
    //
    GPIO_writePin(CCARD_LED_1_GPIO, 1UL);
    GPIO_writePin(CCARD_LED_2_GPIO, 1UL);
}
#endif // PDI_HAL_TEST

//*****************************************************************************
//
// ESC_loadedCheckEEPROM
//
//*****************************************************************************
uint16_t
ESC_loadedCheckEEPROM(void)
{
    uint16_t status;

    //
    // Get ESC DL Status register
    //
    status = ESC_readWord(ESC_O_DL_STATUS);

    //
    // Check if EEPROM is loaded
    //
    if((status & ESC_DL_STATUS_EEPROMLOAD_M) == ESC_DL_STATUS_EEPROMLOAD_M)
    {
        //
        // Get ESC EEPROM Status register
        //
        status = ESC_readWord(ESC_O_EEPROM_STATUS);

        //
        // Check if EEPROM is loaded and device information is okay
        //
        if((status & ESC_EEPROM_STATUS_EEPROMLOAD_M) == ESC_EEPROM_SUCCESS)
        {
            return(ESC_EEPROM_SUCCESS);
        }
        else
        {
            return(ESC_EEPROM_LOAD_ERROR);
        }
    }
    else
    {
        //
        // DL Status indicated EEPROM is not loaded
        //
        return(ESC_EEPROM_NOT_LOADED);
    }
}

//*****************************************************************************
//
// ESC_resetESC
//
//*****************************************************************************
void
ESC_resetESC(void)
{
    SysCtl_resetPeripheral(SYSCTL_PERIPH_CLK_ETHERCAT);
}

//****************************************************************************
// ESC_enableESC
//****************************************************************************

void
ESC_EnableESC(void)
{
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_ETHERCAT);
}


//*****************************************************************************
//
// ESC_holdESCInReset
//
//*****************************************************************************
void
ESC_holdESCInReset(void)
{
    //
    // Put ESC in reset
    //
    HWREGH(DEVCFG_BASE + SYSCTL_O_SOFTPRES23) |= SYSCTL_SOFTPRES23_ETHERCAT;
}

//*****************************************************************************
//
// ESC_releaseESCReset
//
//*****************************************************************************
void
ESC_releaseESCReset(void)
{
    //
    // Release from reset
    //
    HWREGH(DEVCFG_BASE + SYSCTL_O_SOFTPRES23) &= (~SYSCTL_SOFTPRES23_ETHERCAT);
}



//*****************************************************************************
//
// ESC_initHW
//
//*****************************************************************************
uint16_t
ESC_initHWMode(bool preserveESC)
{
    //
    // Set application-specific timeout for waiting for EEPROM to load
    // and one for waiting for memory initialization
    // (End user can adjust as needed)
    //
    uint16_t eepromTimeOut = 0x1000U;
    uint16_t memoryTimeOut = 0x300U;

    //
    // Disable the watchdog
    //
    SysCtl_disableWatchdog();

    //
    // Copy time critical code and flash setup code to RAM. This includes the
    // following functions: Flash_initModule();
    //
    // The RamfuncsLoadStart, RamfuncsLoadSize, and RamfuncsRunStart symbols
    // are created by the linker. Refer to the device .cmd file.
    //
    // memcpy(&RamfuncsRunStart, &RamfuncsLoadStart, (size_t)&RamfuncsLoadSize);

    //
    // Call Flash Initialization to setup flash waitstates. This function must
    // reside in RAM.
    //
    // Flash_initModule(FLASH0CTRL_BASE, FLASH0ECC_BASE, DEVICE_FLASH_WAITSTATES);

    //
    // Verify the XTAL crystal frequency.
    //
    if(!Device_verifyXTAL(DEVICE_OSCSRC_FREQ / 1000000.0F))
    {
        //
        // The actual XTAL frequency does not match DEVICE_OSCSRC_FREQ!!
        // Please check the XTAL frequency used.
        //
        // Note that the latest F2838x controlCARDs (Rev.B and later) have been
        // updated to use 25MHz XTAL by default. If you have an older 20MHz XTAL
        // controlCARD (E1, E2, or Rev.A), refer to the controlCARD
        // documentation on steps to reconfigure the controlCARD from 20MHz to
        // 25MHz.
        //
        // If XTAL verrification is failing, make sure that XTAL is being used as clock source, in Device.h verify that #USE_PLL_SRC_XTAL is defined 
        ESTOP0;

        //
        // Set LED GPIOs to output mode
        //
        GPIO_setDirectionMode(DEVICE_GPIO_PIN_LED1, GPIO_DIR_MODE_OUT);
        GPIO_setDirectionMode(DEVICE_GPIO_PIN_LED2, GPIO_DIR_MODE_OUT);

        while(1)
        {
            //
            // Toggle controlCARD LEDs and delay
            //

           //Turn on LEDs
            GPIO_writePin(DEVICE_GPIO_PIN_LED1, 0);
            GPIO_writePin(DEVICE_GPIO_PIN_LED2, 0);

            // wait for 500ms
            DEVICE_DELAY_US((uint32_t)(500000)); 

            // Turn off LEDs
            GPIO_writePin(DEVICE_GPIO_PIN_LED1, 1);
            GPIO_writePin(DEVICE_GPIO_PIN_LED2, 1);

            // Wait for 500ms
            DEVICE_DELAY_US((uint32_t)(500000)); 
        }
    }

    //
    // Set up device clock
    // Set device clock to 200MHz. 
	SysCtl_setClock(SYSCTL_OSCSRC_XTAL_SE, SYSCTL_PLL_ENABLE, SYSCTL_REFDIV_2, 
					SYSCTL_IMULT_32, SYSCTL_ODIV_2, SYSCTL_SYSDIV_1, DCC1_BASE);
    
    //Device clock freq can verify by checking the output of SysCtl_getClock(DEVICE_OSCSRC_FREQ);

    //
    // Turn on all peripherals and initialize GPIOs
    //
    Device_enableAllPeripherals();
    Device_initGPIO();

    //
    // Configure GPIOs for ECAT
    //


    //
    // PHY Reset
    //

    GPIO_setPinConfig(GPIO_245_ESC_PHY_RESETN);
    GPIO_setQualificationMode(245,GPIO_QUAL_ASYNC);


    //
    // I2C for EEPROM
    //
    GPIO_setPinConfig(GPIO_29_ESC_I2C_SDA);
    GPIO_setQualificationMode(29, GPIO_QUAL_ASYNC);
    GPIO_setPadConfig(29, GPIO_PIN_TYPE_PULLUP);

    GPIO_setPinConfig(GPIO_30_ESC_I2C_SCL);
    GPIO_setQualificationMode(30, GPIO_QUAL_ASYNC);
    GPIO_setPadConfig(30, GPIO_PIN_TYPE_PULLUP);

    //
    // P0 TX and RX DATA
    //
    GPIO_setPinConfig(GPIO_87_ESC_TX0_DATA0);
    GPIO_setQualificationMode(87, GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_11_ESC_TX0_DATA1);
    GPIO_setQualificationMode(11, GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_89_ESC_TX0_DATA2);
    GPIO_setQualificationMode(89, GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_90_ESC_TX0_DATA3);
    GPIO_setQualificationMode(90, GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_80_ESC_RX0_DATA0);
    GPIO_setQualificationMode(80, GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_38_ESC_RX0_DATA1);
    GPIO_setQualificationMode(38, GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_82_ESC_RX0_DATA2);
    GPIO_setQualificationMode(82, GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_83_ESC_RX0_DATA3);
    GPIO_setQualificationMode(83, GPIO_QUAL_ASYNC);

    //
    // P0 TX Enable, RX DV, RX ERR
    //
    GPIO_setPinConfig(GPIO_84_ESC_TX0_ENA);
    GPIO_setQualificationMode(84, GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_25_ESC_RX0_DV);
    GPIO_setQualificationMode(25, GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_79_ESC_RX0_ERR);
    GPIO_setPadConfig(79, GPIO_PIN_TYPE_STD);

    //
    // P0 TX and RX Clk
    //
    GPIO_setPinConfig(GPIO_85_ESC_TX0_CLK);
    GPIO_setQualificationMode(85, GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_24_ESC_RX0_CLK);
    GPIO_setQualificationMode(24, GPIO_QUAL_ASYNC);

    //
    // P0 Linkstatus and Link Active LED
    //
    GPIO_setPinConfig(GPIO_55_ESC_PHY0_LINKSTATUS);
    GPIO_setPadConfig(55, GPIO_PIN_TYPE_INVERT);
    GPIO_setQualificationMode(55, GPIO_QUAL_ASYNC);

    GPIO_setPinConfig(GPIO_243_ESC_LED_LINK0_ACTIVE);
    GPIO_setQualificationMode(243, GPIO_QUAL_ASYNC);

    //
    // P0+P1 MDIO CLK and Data
    //
    GPIO_setPinConfig(GPIO_62_ESC_MDIO_CLK);
    GPIO_setQualificationMode(62, GPIO_QUAL_ASYNC);

    GPIO_setPinConfig(GPIO_39_ESC_MDIO_DATA);
    GPIO_setQualificationMode(39, GPIO_QUAL_ASYNC);

    //
    // P1 TX and RX DATA
    //
    GPIO_setPinConfig(GPIO_75_ESC_TX1_DATA0);
    GPIO_setQualificationMode(75, GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_74_ESC_TX1_DATA1);
    GPIO_setQualificationMode(74, GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_73_ESC_TX1_DATA2);
    GPIO_setQualificationMode(73, GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_72_ESC_TX1_DATA3);
    GPIO_setQualificationMode(72, GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_63_ESC_RX1_DATA0);
    GPIO_setQualificationMode(63, GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_64_ESC_RX1_DATA1);
    GPIO_setQualificationMode(64, GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_65_ESC_RX1_DATA2);
    GPIO_setQualificationMode(65, GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_66_ESC_RX1_DATA3);
    GPIO_setQualificationMode(66, GPIO_QUAL_ASYNC);

    //
    // P1 TX Enable, RX DV, RX ERR
    //
    GPIO_setPinConfig(GPIO_45_ESC_TX1_ENA);
    GPIO_setQualificationMode(45, GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_70_ESC_RX1_DV);
    GPIO_setQualificationMode(70, GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_71_ESC_RX1_ERR);
    GPIO_setPadConfig(71, GPIO_PIN_TYPE_STD);

    //
    // P1 TX and RX Clk
    //
    GPIO_setPinConfig(GPIO_44_ESC_TX1_CLK);
    GPIO_setQualificationMode(44, GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_69_ESC_RX1_CLK);
    GPIO_setQualificationMode(69, GPIO_QUAL_ASYNC);

    //
    // P1 Linkstatus and Link Active LED
    //
    // GPIO233 is an AGPIO by default. Need to disable Analog for using it for Digital Signals
    GPIO_setAnalogMode(233, GPIO_ANALOG_DISABLED);
    GPIO_setPinConfig(GPIO_233_ESC_PHY1_LINKSTATUS);
    GPIO_setPadConfig(233, GPIO_PIN_TYPE_INVERT);
    GPIO_setQualificationMode(233,GPIO_QUAL_ASYNC);

    GPIO_setPinConfig(GPIO_244_ESC_LED_LINK1_ACTIVE);
    GPIO_setQualificationMode(244, GPIO_QUAL_ASYNC);

    //
    // Sync and Latch
    //
    // Note these GPIOs aren't required for EtherCAT
    // operation and are configured here for demonstration,
    // analysis, and debug purposes
   //
 /*   GPIO_setPinConfig(GPIO_29_ESC_LATCH0);
    GPIO_setQualificationMode(29,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_30_ESC_LATCH1);
    GPIO_setQualificationMode(30,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_238_ESC_SYNC0);
    GPIO_setQualificationMode(238,GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_239_ESC_SYNC1);
    GPIO_setQualificationMode(239,GPIO_QUAL_ASYNC);
 */   
    //
    // Set GPIO direction for ECAT RUN/ERR LEDs
    //
    
    GPIO_setDirectionMode(ESC_RUN_LED_GPIO, GPIO_DIR_MODE_OUT);
    GPIO_setDirectionMode(ESC_ERR_LED_GPIO, GPIO_DIR_MODE_OUT);

    GPIO_setPinConfig(GPIO_100_ESC_GPI0);
    GPIO_setQualificationMode(100,GPIO_QUAL_ASYNC);
 

    //
    // Initialize PIE and clear PIE registers. Disables CPU interrupts.
    //
    Interrupt_initModule();

    //
    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    //
    Interrupt_initVectorTable();

    //
    // Register EtherCAT Interrupt Handlers
    //
    Interrupt_register(INT_ECAT, &ESC_applicationLayerHandler);
    Interrupt_register(INT_ECATSYNC0, &ESC_applicationSync0Handler);
    Interrupt_register(INT_ECATSYNC1, &ESC_applicationSync1Handler);

    //
    // Enable ECAT/PDI, Sync0, and Sync1 interrupts in CPU
    //
    Interrupt_enable(INT_ECAT);
    Interrupt_enable(INT_ECATSYNC0);
    Interrupt_enable(INT_ECATSYNC1);

    //
    // Configure and Start timer
    //
    CPUTimer_setPeriod(CPUTIMER0_BASE, 0xFFFFFFFFUL);
    CPUTimer_setPreScaler(CPUTIMER0_BASE, 0U);
    CPUTimer_startTimer(CPUTIMER0_BASE);

#ifdef PDI_HAL_TEST
    //
    // Enable Pass, fail signals
    //
    ESC_passFailSignalSetup();
#endif // PDI_HAL_TEST

    if (!preserveESC)
    {
        //
        // Sys PLL = 200MHz and use /2 to get 100MHz for ECAT IP
        // (There is a built in /4 to get 25MHz for PHY when using
        //  internal clocking for PHY)
        SysCtl_setECatClock(SYSCTL_ECATCLKOUT_DIV_2, SYSCTL_ECAT_PHYCLK_ENABLE);

        // Configure EEPROM Size for 16K bits or less
        ESCSS_configureEEPROMSize(ESC_SS_CONFIG_BASE, ESCSS_LESS_THAN_16K);

        // Reset ESC
        ESC_resetESC();

#ifdef PDI_HAL_TEST
    //
    // Enable the debug access
    //
    while((HWREGH(ESC_SS_BASE + ECAT_SS_O_ESCSS_ACCESS_CTRL) &
           ECAT_SS_ESCSS_ACCESS_CTRL_ENABLE_DEBUG_ACCESS) !=
          ECAT_SS_ESCSS_ACCESS_CTRL_ENABLE_DEBUG_ACCESS)
    {
        ESCSS_enableDebugAccess(ESC_SS_BASE);
    }
#endif // PDI_HAL_TEST

        // Initialize ESCSS Memory
        ESCSS_initMemory(ESC_SS_BASE);

    //
    // Wait for ESCSS memory initialization to complete
    //
        if(ESCSS_getMemoryInitDoneStatusBlocking(ESC_SS_BASE, memoryTimeOut) !=
           ESCSS_API_SUCCESS)
        {
            return(ESC_HW_INIT_FAIL);
        }

    //
    // Wait (with time out) for EEPROM to be loaded
    //
        while(ESC_loadedCheckEEPROM() != ESC_EEPROM_SUCCESS)
        {
            eepromTimeOut--;

        //
        // On time out, return fail
        //
            if(eepromTimeOut == 0U)
            {
                return(ESC_HW_INIT_FAIL);
            }
        }

    //
    // Initialize AL Event Mask to zero to prevent ESC interrupts until
    // set during stack state transition to SAFEOP
    //
        ESC_writeDWord(0UL, ESC_O_AL_EVENTMASK);
    }

    //
    // Enable ECAT/PDI, Sync0, and Sync1 interrupts in ESCSS
    //
    ESCSS_setMaskedInterruptStatus(ESC_SS_BASE,
                                   (ECAT_SS_ESCSS_INTR_MASK_IRQ_MASK |
                                    ECAT_SS_ESCSS_INTR_MASK_SYNC0_MASK |
                                    ECAT_SS_ESCSS_INTR_MASK_SYNC1_MASK));

    //
    // Connect ESCSS Sync0/1 signals to respective interrupts in PIE
    //
    ESCSS_configureSync0Connections(ESC_SS_BASE, ECAT_SS_ESCSS_SYNC0_CONFIG_CPUX_INT_EN,
                                    ESCSS_VALID_KEY_VALUE);
    ESCSS_configureSync1Connections(ESC_SS_BASE, ECAT_SS_ESCSS_SYNC1_CONFIG_CPUX_INT_EN,
                                    ESCSS_VALID_KEY_VALUE);

    //
    // Enable interrupts to CPU
    //
    EINT;

    return(ESC_HW_INIT_SUCCESS);
}

uint16_t
ESC_initHW(void)
{
    return ESC_initHWMode(false);
}

//*****************************************************************************
//
// ESC_releaseHW
//
//*****************************************************************************
void
ESC_releaseHW(void)
{
    //
    // Intentionally empty - Implementation is left to the end user
    //
}

//*****************************************************************************
//
// ESC_applicationLayerHandler
//
//*****************************************************************************
__attribute__((interrupt ))void
ESC_applicationLayerHandler(void)
{
#ifdef ETHERCAT_STACK
#if AL_EVENT_ENABLED
    //
    // When stack is included and application event enabled, call stack PDI ISR
    //
    PDI_Isr();
#endif  // AL_EVENT_ENABLED
#endif  // ETHERCAT_STACK

    //
    // Acknowledge and clear interrupt in ESCSS
    //
    ESCSS_clearRawInterruptStatus(ESC_SS_BASE, ECAT_SS_ESCSS_INTR_CLR_IRQ_CLR);
}

//*****************************************************************************
//
// ESC_applicationSync0Handler
//
//*****************************************************************************
__attribute__((interrupt )) void
ESC_applicationSync0Handler(void)
{
#ifdef ETHERCAT_STACK
#if DC_SUPPORTED
    //
    // When stack is included and DC is enabled, call stack Sync0 ISR
    //
    Sync0_Isr();
#endif  // DC_SUPPORTED
#endif  // ETHERCAT_STACK
    
    Sync0_Isr();
    
    //
    // Acknowledge and clear interrupt in ESCSS
    //
    ESCSS_clearRawInterruptStatus(ESC_SS_BASE, ECAT_SS_ESCSS_INTR_CLR_SYNC0_CLR);
}

//*****************************************************************************
//
// ESC_applicationSync1Handler
//
//*****************************************************************************
__attribute__((interrupt )) void
ESC_applicationSync1Handler(void)
{
#ifdef ETHERCAT_STACK
#if DC_SUPPORTED
    //
    // When stack is included and DC is enabled, call stack Sync1 ISR
    //
    Sync1_Isr();
#endif  // DC_SUPPORTED
#endif  // ETHERCAT_STACK

    //
    // Acknowledge and clear interrupt in ESCSS
    //
    ESCSS_clearRawInterruptStatus(ESC_SS_BASE, ECAT_SS_ESCSS_INTR_CLR_SYNC1_CLR);
}

//
// End of File
//
