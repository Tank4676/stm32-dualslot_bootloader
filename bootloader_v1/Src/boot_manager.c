/*
 * boot_manager.c
 *
 *  Created on: Mar 8, 2026
 *      Author: tanmay
 */

#include "boot_manager.h"
#include "boot_config.h"
#include "app_header.h"
#include "flash_layout.h"
#include "stm32f4xx_hal.h"

void BootManager_JumpToApp(uint8_t slot)
{
    uint32_t vtor  = SLOT_VTOR_ADDR(slot);
    uint32_t sp    = *(uint32_t *)(vtor);
    uint32_t reset = *(uint32_t *)(vtor + 4U);

    __disable_irq();

        SysTick->CTRL = 0;
        SysTick->LOAD = 0;
        SysTick->VAL  = 0;

        SCB->VTOR = vtor;
        __DSB();
        __ISB();

        __set_MSP(sp);
        __DSB();
        __ISB();

        ((void (*)(void))reset)();
}

void BootManager_Run(void)
{
    BootConfig_t cfg;
    BootConfig_Read(&cfg);

    /* debug — blink N+1 times to show AppHeader_Validate result */


     /* pause so blinks are easy to count */

    uint8_t try_slot = (cfg.pending_slot != SLOT_NONE)
                       ? cfg.pending_slot
                       : cfg.active_slot;

    if (AppHeader_Validate(try_slot) != APP_HDR_OK)
    {
        uint8_t fallback = (try_slot == SLOT_A) ? SLOT_B : SLOT_A;

        if (AppHeader_Validate(fallback) != APP_HDR_OK)
        {
            while (1) { }
        }

        try_slot          = fallback;
        cfg.active_slot   = fallback;
        cfg.pending_slot  = SLOT_NONE;
        cfg.boot_attempts = 0;
        BootConfig_Write(&cfg);
        BootManager_JumpToApp(try_slot);
    }

    cfg.boot_attempts++;

    if (cfg.boot_attempts >= cfg.max_attempts)
    {
        uint8_t other = (try_slot == SLOT_A) ? SLOT_B : SLOT_A;

        if (AppHeader_Validate(other) == APP_HDR_OK)
        {
            cfg.active_slot   = other;
            cfg.pending_slot  = SLOT_NONE;
            cfg.boot_attempts = 0;
            BootConfig_Write(&cfg);
            BootManager_JumpToApp(other);
        }
        else
        {
            while (1) { }
        }
    }

    BootConfig_Write(&cfg);
    BootManager_JumpToApp(try_slot);
}
