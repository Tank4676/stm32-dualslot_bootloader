/*
 * boot_config.h
 *
 *  Created on: Mar 7, 2026
 *      Author: tanmay
 */

#ifndef BOOT_CONFIG_H_
#define BOOT_CONFIG_H_


#include <stdint.h>
#include "flash_layout.h"

/* ── Magic ───────────────────────────────────────────────────────────────── */

#define BOOT_CONFIG_MAGIC       0xB007C0DEUL    /* "BOOT CODE" */
#define BOOT_CONFIG_MAX_TRIES   3U              /* rollback after 3 failed boots */

/* ── Struct ──────────────────────────────────────────────────────────────── */

/**
 * Stored at BOOT_CONFIG_ADDR (primary) and BOOT_CONFIG_BKP_ADDR (backup).
 * crc field covers all bytes BEFORE it in the struct.
 * Use __attribute__((packed)) so there is no padding between fields —
 * padding would shift the crc field offset and break the CRC calculation.
 */
typedef struct __attribute__((packed))
{
    uint32_t magic;             /* must equal BOOT_CONFIG_MAGIC             */
    uint8_t  active_slot;       /* SLOT_A or SLOT_B — currently running     */
    uint8_t  pending_slot;      /* SLOT_NONE or slot waiting to be tested   */
    uint8_t  boot_attempts;     /* how many times pending slot was tried    */
    uint8_t  max_attempts;      /* rollback if boot_attempts >= this        */
    uint32_t slot_a_version;    /* fw version currently in Slot A           */
    uint32_t slot_b_version;    /* fw version currently in Slot B           */
    uint32_t crc;               /* CRC32 of all fields above this one       */
} BootConfig_t;

/* ── API ─────────────────────────────────────────────────────────────────── */

/**
 * @brief  Write a default config to flash.
 *         Called on first boot (blank flash) or after full chip erase.
 *         Sets active = SLOT_A, pending = SLOT_NONE, attempts = 0.
 */
void BootConfig_Init(void);

/**
 * @brief  Read and validate config from flash into a RAM struct.
 *         Tries primary first, falls back to backup if primary is invalid.
 *         Calls BootConfig_Init() if both copies are invalid.
 * @param  cfg  Pointer to caller-allocated BootConfig_t to fill.
 */
void BootConfig_Read(BootConfig_t *cfg);

/**
 * @brief  Write a RAM struct to flash (both primary and backup).
 *         Computes and inserts CRC before writing.
 *         Erases sector before writing — you must pass the full struct.
 * @param  cfg  Pointer to the struct to write.
 */
void BootConfig_Write(BootConfig_t *cfg);

/**
 * @brief  Confirm the current pending slot booted successfully.
 *         Promotes pending → active, clears pending, resets attempts.
 *         Called by the APPLICATION after all init succeeds.
 */
void BootConfig_Confirm(void);

#endif /* BOOT_CONFIG_H */


