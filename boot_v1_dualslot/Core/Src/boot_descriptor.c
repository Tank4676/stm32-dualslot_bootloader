/*
 * boot_descriptor.c
 *
 *  Created on: May 4, 2026
 *      Author: tanmay
 */

#include "boot_descriptor.h"
#include "bootloader_config.h"
#include "stm32f4xx_hal.h"

uint8_t descriptor_read(boot_descriptor_t *desc)
{
    *desc = *((boot_descriptor_t*)BOOT_DESCRIPTOR_ADDR);

    if(desc->magic == BOOT_DESCRIPTOR_MAGIC)
        return 1;
    return 0;
}

void descriptor_init_default(boot_descriptor_t *desc)
{
    desc->magic          = BOOT_DESCRIPTOR_MAGIC;
    desc->active_slot    = 0;   // Slot A
    desc->slot_a_version = 1;
    desc->slot_b_version = 0;
    desc->slot_a_crc     = 0;
    desc->slot_b_crc     = 0;
    desc->slot_a_size    = 0;
    desc->slot_b_size    = 0;
    desc->boot_attempts  = 0;
    desc->reserved       = 0;
}

uint8_t descriptor_write(boot_descriptor_t *desc)
{
    FLASH_EraseInitTypeDef eraseInit;
    uint32_t sectorError;

    if(HAL_FLASH_Unlock() != HAL_OK)
        return 0;

    eraseInit.TypeErase    = FLASH_TYPEERASE_SECTORS;
    eraseInit.Sector       = FLASH_SECTOR_5;
    eraseInit.NbSectors    = 1;
    eraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    if(HAL_FLASHEx_Erase(&eraseInit, &sectorError) != HAL_OK)
    {
        HAL_FLASH_Lock();
        return 0;
    }

    uint32_t *src  = (uint32_t*)desc;
    uint32_t words = sizeof(boot_descriptor_t) / 4;

    for(uint32_t i = 0; i < words; i++)
    {
        if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                             BOOT_DESCRIPTOR_ADDR + (i * 4),
                             src[i]) != HAL_OK)
        {
            HAL_FLASH_Lock();
            return 0;
        }
    }

    HAL_FLASH_Lock();
    return 1;
}
