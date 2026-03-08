/**
 * @file    boot_config.c
 * @brief   Boot configuration read/write/init/confirm.
 */

#include <string.h>
#include "boot_config.h"
#include "flash_layout.h"
#include "crc32.h"
#include "stm32f4xx_hal.h"

/* ── Private helper — compute CRC over data fields only ─────────────────── */

static uint32_t config_crc(const BootConfig_t *cfg)
{
    /*
     * Compute CRC32 over every byte in the struct EXCEPT the crc field.
     * crc is the last field (4 bytes) so we compute over (total - 4) bytes.
     */
    return crc32_compute(
        (const uint8_t *)cfg,
        sizeof(BootConfig_t) - sizeof(uint32_t)
    );
}

/* ── Private helper — validate magic + CRC ───────────────────────────────── */

static uint8_t config_is_valid(const BootConfig_t *cfg)
{
    if (cfg->magic != BOOT_CONFIG_MAGIC)
    {
        return 0;
    }

    if (config_crc(cfg) != cfg->crc)
    {
        return 0;
    }

    return 1;
}

/* ── Private helper — erase one sector and write struct to it ────────────── */
/*
 * Pulled into its own function because we do the exact same
 * erase+write sequence twice in BootConfig_Write (primary + backup).
 * Instead of copy-pasting the same 15 lines twice, we call this once
 * for each address. This is a common C pattern — if you write the same
 * block twice, make it a function.
 */
static void write_to_sector(const BootConfig_t *cfg,
                             uint32_t           base_addr,
                             uint8_t            sector)
{
    FLASH_EraseInitTypeDef erase_cfg = {0};
    uint32_t sector_error = 0;

    /* Erase the target sector — flash must be blank before writing */
    erase_cfg.TypeErase    = FLASH_TYPEERASE_SECTORS;
    erase_cfg.Sector       = sector;
    erase_cfg.NbSectors    = 1;
    erase_cfg.VoltageRange = FLASH_VOLTAGE_RANGE_3;  /* 2.7V-3.6V supply */

    HAL_FLASHEx_Erase(&erase_cfg, &sector_error);

    /*
     * Write word by word (4 bytes at a time).
     * STM32F4 HAL only supports word-width flash programming.
     * Cast the struct pointer to uint32_t* so we can index it in 4-byte steps.
     */
    uint32_t *src   = (uint32_t *)cfg;
    uint32_t  addr  = base_addr;
    uint32_t  words = sizeof(BootConfig_t) / sizeof(uint32_t);

    for (uint32_t i = 0; i < words; i++)
    {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, src[i]);
        addr += 4;
    }
}

/* ── BootConfig_Init ─────────────────────────────────────────────────────── */

void BootConfig_Init(void)
{
    /*
     * Called on first ever boot (blank flash) or after full chip erase.
     * Creates a safe default state — Slot A active, nothing pending.
     */
    BootConfig_t cfg;

    cfg.magic          = BOOT_CONFIG_MAGIC;
    cfg.active_slot    = SLOT_A;
    cfg.pending_slot   = SLOT_NONE;        /* 0xFF — no update pending     */
    cfg.boot_attempts  = 0;
    cfg.max_attempts   = BOOT_CONFIG_MAX_TRIES;   /* 3                     */
    cfg.slot_a_version = 0;                /* unknown until app confirms   */
    cfg.slot_b_version = 0;
    cfg.crc            = 0;                /* BootConfig_Write fills this  */

    BootConfig_Write(&cfg);
}

/* ── BootConfig_Read ─────────────────────────────────────────────────────── */

void BootConfig_Read(BootConfig_t *cfg)
{
    /*
     * Try primary first. Fall back to backup. Init if both are invalid.
     * After this function returns, cfg is always in a valid usable state.
     */

    /* STEP 1 — try primary */
    memcpy(cfg, (void *)BOOT_CONFIG_ADDR, sizeof(BootConfig_t));
    if (config_is_valid(cfg))
    {
        return;   /* primary is good, done */
    }

    /* STEP 2 — primary bad, try backup */
    memcpy(cfg, (void *)BOOT_CONFIG_BKP_ADDR, sizeof(BootConfig_t));
    if (config_is_valid(cfg))
    {
        return;   /* backup is good, done */
    }

    /*
     * STEP 3 — both invalid.
     * This happens on a fresh chip or after a full chip erase.
     * Init writes a safe default to flash, then read it back.
     */
    BootConfig_Init();
    memcpy(cfg, (void *)BOOT_CONFIG_ADDR, sizeof(BootConfig_t));
}

/* ── BootConfig_Write ────────────────────────────────────────────────────── */

void BootConfig_Write(BootConfig_t *cfg)
{
    /*
     * Always writes to BOTH primary and backup.
     * CRC is computed and stored in the struct before any flash operation.
     */

    /* STEP 1 — compute CRC and store it in the struct */
    cfg->crc = config_crc(cfg);

    /* STEP 2 — unlock flash for writing */
    HAL_FLASH_Unlock();

    /* STEP 3+4 — erase sector 2, write primary copy */
    write_to_sector(cfg, BOOT_CONFIG_ADDR, 2);

    /* STEP 5 — erase sector 3, write backup copy */
    write_to_sector(cfg, BOOT_CONFIG_BKP_ADDR, 3);

    /* STEP 6 — lock flash — always lock after you're done */
    HAL_FLASH_Lock();
}

/* ── BootConfig_Confirm ──────────────────────────────────────────────────── */

void BootConfig_Confirm(void)
{
    /*
     * Called by the APPLICATION after all init succeeds.
     * Promotes pending_slot to active_slot and clears the pending state.
     * If this never gets called, the watchdog will reset the chip and
     * the bootloader will eventually roll back to the previous slot.
     */
    BootConfig_t cfg;

    /* STEP 1 — load current state */
    BootConfig_Read(&cfg);

    /* STEP 2 — promote pending to active */
    cfg.active_slot   = cfg.pending_slot;  /* pending is now confirmed     */
    cfg.pending_slot  = SLOT_NONE;         /* no longer waiting to be tested */
    cfg.boot_attempts = 0;                 /* reset counter                */

    /* STEP 3 — save back to flash */
    BootConfig_Write(&cfg);
}
