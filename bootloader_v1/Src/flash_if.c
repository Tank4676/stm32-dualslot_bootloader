/*
 * flash_if.c
 *
 *  Created on: Mar 8, 2026
 *      Author: tanmay
 */
/**
 * @file    flash_if.c
 * @brief   Flash interface — erase, write, verify, empty check.
 */

/*
 * flash_if.c
 *
 *  Created on: Mar 8, 2026
 *      Author: tanmay
 */
/**
 * @file    flash_if.c
 * @brief   Flash interface — erase, write, verify, empty check.
 */

#include <string.h>
#include "flash_if.h"
#include "flash_layout.h"

/* ── Private helper — get sector number from address ────────────────────── */

static uint32_t addr_to_sector(uint32_t addr)   /* FIX: was 'Address' (undefined), now uses parameter 'addr' throughout */
{
    uint32_t sector = 0;

    if ((addr >= 0x08000000) && (addr <= 0x08003FFF)) {
        sector = FLASH_SECTOR_0;
    } else if ((addr >= 0x08004000) && (addr <= 0x08007FFF)) {
        sector = FLASH_SECTOR_1;
    } else if ((addr >= 0x08008000) && (addr <= 0x0800BFFF)) {
        sector = FLASH_SECTOR_2;
    } else if ((addr >= 0x0800C000) && (addr <= 0x0800FFFF)) {
        sector = FLASH_SECTOR_3;
    } else if ((addr >= 0x08010000) && (addr <= 0x0801FFFF)) {
        sector = FLASH_SECTOR_4;
    } else if ((addr >= 0x08020000) && (addr <= 0x0803FFFF)) {
        sector = FLASH_SECTOR_5;
    } else if ((addr >= 0x08040000) && (addr <= 0x0805FFFF)) {
        sector = FLASH_SECTOR_6;
    } else if ((addr >= 0x08060000) && (addr <= 0x0807FFFF)) {
        sector = FLASH_SECTOR_7;
    }

    return sector;
}

/* ── FlashIF_Erase ───────────────────────────────────────────────────────── */

FlashIF_Status FlashIF_Erase(uint32_t addr, uint32_t size)
{
    FLASH_EraseInitTypeDef erase_init;
    uint32_t sector_error = 0;
    uint32_t first_sector = addr_to_sector(addr);
    uint32_t last_sector  = addr_to_sector(addr + size - 1);

    if (HAL_FLASH_Unlock() != HAL_OK) {
        return FLASH_IF_ERROR;
    }

    erase_init.TypeErase    = FLASH_TYPEERASE_SECTORS;
    erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;   /* 2.7 V – 3.6 V supply */
    erase_init.Sector       = first_sector;
    erase_init.NbSectors    = last_sector - first_sector + 1;

    if (HAL_FLASHEx_Erase(&erase_init, &sector_error) != HAL_OK) {
        HAL_FLASH_Lock();
        return FLASH_IF_ERROR;
    }

    HAL_FLASH_Lock();
    return FLASH_IF_OK;
}

/* ── FlashIF_Write ───────────────────────────────────────────────────────── */

FlashIF_Status FlashIF_Write(uint32_t addr, const uint8_t *data, uint32_t size)
{
    if (data == NULL || size == 0) {
        return FLASH_IF_ERROR;
    }

    if (HAL_FLASH_Unlock() != HAL_OK) {
        return FLASH_IF_ERROR;
    }

    /* Write 4 bytes at a time (word-aligned) for efficiency */
    uint32_t i = 0;
    for (; i + 4 <= size; i += 4) {
        uint32_t word;
        memcpy(&word, &data[i], 4);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + i, word) != HAL_OK) {
            HAL_FLASH_Lock();
            return FLASH_IF_ERROR;
        }
    }

    /* Handle any remaining bytes one at a time */
    for (; i < size; i++) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, addr + i, data[i]) != HAL_OK) {
            HAL_FLASH_Lock();
            return FLASH_IF_ERROR;
        }
    }

    HAL_FLASH_Lock();
    return FLASH_IF_OK;
}

/* ── FlashIF_Verify ──────────────────────────────────────────────────────── */

FlashIF_Status FlashIF_Verify(uint32_t addr, const uint8_t *data, uint32_t size)
{
    /* FIX: added loop with declared index 'i'; added success return;
             return proper enum values instead of raw 0/1 */
    const uint8_t *flash = (const uint8_t *)addr;

    for (uint32_t i = 0; i < size; i++) {
        if (flash[i] != data[i]) {
            return FLASH_IF_ERROR;
        }
    }

    return FLASH_IF_OK;
}

/* ── FlashIF_IsEmpty ─────────────────────────────────────────────────────── */

uint8_t FlashIF_IsEmpty(uint32_t addr, uint32_t size)
{
    /* FIX: added loop with declared index 'i'; early-exit on first
             non-0xFF byte; return 1 (empty) only after all bytes checked */
    const uint8_t *flash = (const uint8_t *)addr;

    for (uint32_t i = 0; i < size; i++) {
        if (flash[i] != 0xFF) {
            return 0;   /* not empty */
        }
    }

    return 1;           /* all bytes erased */
}
