/*
 * app_header.h
 *
 *  Created on: Mar 8, 2026
 *      Author: tanmay
 */

#ifndef APP_HEADER_H_
#define APP_HEADER_H_

/*
 * app_header.h
 *
 *  Created on: Mar 8, 2026
 *      Author: tanmay
 */

/**
 * @file    app_header.h
 * @brief   Firmware image header — placed at the start of every slot.
 *
 * ── WHY A HEADER? ────────────────────────────────────────────────────────
 *   The bootloader needs to know, BEFORE jumping to an app, whether the
 *   image sitting in a slot is:
 *     (a) actually a valid firmware image (magic word check)
 *     (b) the right version / not corrupted (CRC check)
 *     (c) the right size (doesn't overflow the slot)
 *
 *   All of that info lives in this struct, placed at the very beginning
 *   of each slot (SLOT_A_HDR_ADDR / SLOT_B_HDR_ADDR from flash_layout.h).
 *
 * ── MEMORY LAYOUT INSIDE A SLOT ─────────────────────────────────────────
 *
 *   SLOT_X_BASE_ADDR  ┌──────────────────────┐
 *                     │   AppHeader_t        │  <- this struct (32 bytes)
 *                     │   (padding to 16KB)  │  <- rest of header sector
 *   SLOT_X_VTOR_ADDR  ├──────────────────────┤  <- APP_HEADER_SIZE offset
 *                     │   Vector Table       │  <- actual firmware starts here
 *                     │   Application Code   │
 *   SLOT_X_END_ADDR   └──────────────────────┘
 *
 * ── HOW THE LINKER SCRIPT USES THIS ─────────────────────────────────────
 *   The application's linker script places a .header section at
 *   SLOT_X_BASE_ADDR. The startup code fills in this struct at compile
 *   time (magic, version, size, fw_crc). The bootloader reads it at
 *   runtime to validate before jumping.
 *
 * ── COPY THIS FILE INTO ALL THREE PROJECTS ───────────────────────────────
 *   bootloader_v1/Core/Inc/app_header.h
 *   application_slotA/Core/Inc/app_header.h
 *   application_slotB/Core/Inc/app_header.h
 */


#include <stdint.h>
#include "flash_layout.h"

/* ── Header version — bump this if you change the struct layout ─────────── */
/*
 * If you ever add/remove/reorder fields, increment this.
 * The bootloader can then reject headers with an unexpected version
 * rather than misinterpreting garbage bytes as valid fields.
 */
#define APP_HEADER_VERSION      APP_HEADER_STRUCT_VER   /* currently 1 */


/* ── AppHeader_t ─────────────────────────────────────────────────────────── */
/*
 * This struct is written ONCE by the build system / OTA host into flash.
 * The bootloader reads it as a plain memory-mapped struct — no HAL needed.
 *
 * Size must stay a multiple of 4 bytes (word-aligned) because:
 *   - flash_if writes in 4-byte words
 *   - memcpy from flash behaves correctly only with aligned structs
 *
 * Fields:
 *   magic        — sanity check. Must equal APP_MAGIC_WORD (0xDEADBEEF).
 *                  If not, this slot is blank or corrupted — skip it.
 *
 *   hdr_version  — version of THIS struct layout.
 *                  Bootloader rejects headers where hdr_version != APP_HEADER_VERSION.
 *
 *   fw_version   — monotonically increasing firmware version number.
 *                  Set by the build system. Bootloader uses this to log
 *                  which slot it booted and to fill boot_config slot versions.
 *
 *   fw_size      — size of the firmware image in bytes, NOT including this
 *                  header. i.e. the number of bytes starting at SLOT_X_VTOR_ADDR.
 *                  Bootloader checks: fw_size <= APP_FW_MAX_SIZE.
 *
 *   fw_crc       — CRC32 of the firmware bytes only (from VTOR onwards).
 *                  Does NOT cover this header struct itself.
 *                  Bootloader recomputes CRC over fw_size bytes starting at
 *                  SLOT_X_VTOR_ADDR and compares to this value.
 *
 *   hdr_crc      — CRC32 of THIS header struct, excluding hdr_crc itself.
 *                  Protects the header from corruption independent of the firmware.
 *                  Computed over the first (sizeof(AppHeader_t) - 4) bytes.
 *
 *   _reserved    — padding to keep the struct word-aligned and leave room
 *                  for future fields without breaking the layout.
 *                  Must be zeroed by the build tool.
 */
typedef struct __attribute__((packed))
{
    uint32_t magic;          /* 0xDEADBEEF — slot is populated             */
    uint32_t hdr_version;    /* struct layout version — must match bootloader */
    uint32_t fw_version;     /* monotonic firmware version (e.g. 0x00010002) */
    uint32_t fw_size;        /* firmware size in bytes (after header)       */
    uint32_t fw_crc;         /* CRC32 of firmware bytes only                */
    uint32_t hdr_crc;        /* CRC32 of this struct (all fields above)     */
    uint32_t _reserved[2];   /* zero-padded, reserved for future use        */
} AppHeader_t;

/* Confirm struct is exactly 32 bytes */
_Static_assert(sizeof(AppHeader_t) == 32U, "AppHeader_t must be 32 bytes");


/* ── AppHeader_Status ────────────────────────────────────────────────────── */
/*
 * Return codes for AppHeader_Validate().
 * Ordered by check sequence — higher value = deeper into validation.
 *
 *   APP_HDR_OK          — all checks passed, safe to boot
 *   APP_HDR_BAD_MAGIC   — slot is blank or not a firmware image
 *   APP_HDR_BAD_VERSION — built with a different header layout version
 *   APP_HDR_BAD_HDR_CRC — header itself is corrupted
 *   APP_HDR_BAD_SIZE    — fw_size exceeds APP_FW_MAX_SIZE
 *   APP_HDR_BAD_FW_CRC  — firmware bytes are corrupted
 */
typedef enum
{
    APP_HDR_OK           = 0,
    APP_HDR_BAD_MAGIC    = 1,
    APP_HDR_BAD_VERSION  = 2,
    APP_HDR_BAD_HDR_CRC  = 3,
    APP_HDR_BAD_SIZE     = 4,
    APP_HDR_BAD_FW_CRC   = 5,
} AppHeader_Status;


/* ── API ─────────────────────────────────────────────────────────────────── */

/**
 * @brief  Validate the firmware header for a given slot.
 *
 * Runs all checks in order (magic → version → hdr_crc → size → fw_crc).
 * Stops and returns the first failure. fw_crc is last because it requires
 * reading the entire firmware image — most expensive check.
 *
 * @param  slot   SLOT_A or SLOT_B (defined in flash_layout.h)
 * @return APP_HDR_OK if the slot contains a valid bootable image.
 *         One of the APP_HDR_BAD_* codes otherwise.
 */
AppHeader_Status AppHeader_Validate(uint8_t slot);

/**
 * @brief  Get a pointer to the header struct for a slot directly from flash.
 *
 * No copy — returns a pointer into the flash address space.
 * Valid to read at any time (flash is memory-mapped on STM32).
 * Do NOT write through this pointer — use FlashIF_Write() instead.
 *
 * @param  slot   SLOT_A or SLOT_B
 * @return Const pointer to the AppHeader_t at the slot's base address.
 */
static inline const AppHeader_t *AppHeader_Get(uint8_t slot)
{
    return (const AppHeader_t *)SLOT_HDR_ADDR(slot);
}

#endif /* APP_HEADER_H_ */
