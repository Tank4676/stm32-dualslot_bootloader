/*
 * flash_layout.h
 *
 *  Created on: Mar 6, 2026
 *      Author: tanmay
 */

#ifndef FLASH_LAYOUT_H_
#define FLASH_LAYOUT_H_

/**
 * @file    flash_layout.h
 * @brief   Flash memory map for STM32F446ZET6 dual-slot bootloader system.
 *
 * ── COPY THIS FILE INTO ALL THREE PROJECTS ───────────────────────────────
 *   bootloader_v1/Core/Inc/flash_layout.h
 *   application_slotA/Core/Inc/flash_layout.h
 *   application_slotB/Core/Inc/flash_layout.h
 *
 * ── WHY ONE FILE FOR ALL PROJECTS? ───────────────────────────────────────
 *   The bootloader needs to know where the apps live (to jump to them).
 *   The apps need to know where the boot config is (to confirm themselves).
 *   If addresses are hardcoded in each project separately, one change
 *   requires updating three files and it's easy to forget one.
 *   One file = one place to change = no mismatches.
 *
 * ── STM32F446ZET6 PHYSICAL SECTOR LAYOUT (512KB total) ───────────────────
 *
 *   Sector   Size     Start        End
 *   ──────────────────────────────────────
 *     0      16KB     0x08000000   0x08003FFF
 *     1      16KB     0x08004000   0x08007FFF
 *     2      16KB     0x08008000   0x0800BFFF
 *     3      16KB     0x0800C000   0x0800FFFF
 *     4      64KB     0x08010000   0x0801FFFF
 *     5     128KB     0x08020000   0x0803FFFF
 *     6     128KB     0x08040000   0x0805FFFF
 *     7     128KB     0x08060000   0x0807FFFF
 *
 * ── OUR ALLOCATION ───────────────────────────────────────────────────────
 *
 *   0x08000000  ┌─────────────────────┐
 *               │   Bootloader        │  Sectors 0+1  (32KB)
 *   0x08008000  ├─────────────────────┤
 *               │   Boot Config       │  Sector 2     (16KB) primary
 *   0x0800C000  ├─────────────────────┤
 *               │   Boot Config BKP   │  Sector 3     (16KB) backup
 *   0x08010000  ├─────────────────────┤
 *               │   SLOT A Header     │  │
 *   0x08014000  │   ─────────────     │  Sectors 4+5  (192KB total)
 *               │   SLOT A App Code   │  │
 *   0x08040000  ├─────────────────────┤
 *               │   SLOT B Header     │  │
 *   0x08044000  │   ─────────────     │  Sectors 6+7  (256KB total)
 *               │   SLOT B App Code   │  │
 *   0x08080000  └─────────────────────┘  (end of flash)
 */


/* ── Bootloader ──────────────────────────────────────────────────────────── */

#define BL_START_ADDR           0x08000000UL
#define BL_SIZE                 0x00008000UL    /* 32KB — sectors 0+1       */
#define BL_END_ADDR             (BL_START_ADDR + BL_SIZE)


/* ── Boot Config ─────────────────────────────────────────────────────────── */
/*
 * Stores which slot is active, which is pending, boot attempt counter etc.
 * Gets a backup copy in case of power cut mid-write.
 * Both are 16KB sectors but the struct itself is tiny (~32 bytes).
 * The extra space is wasted but that is fine — these sectors cannot be
 * shared with code anyway since erasing them would erase partial code.
 */
#define BOOT_CONFIG_ADDR        0x08008000UL    /* Sector 2 — primary       */
#define BOOT_CONFIG_BKP_ADDR    0x0800C000UL    /* Sector 3 — backup        */
#define BOOT_CONFIG_SECTOR_SIZE 0x00004000UL    /* 16KB                     */


/* ── Slot Definitions ────────────────────────────────────────────────────── */
/*
 * APP_HEADER_SIZE: the gap between slot base and VTOR.
 * Your .header section in the linker script pads to this size.
 * Must be >= next power-of-2 above (num_vectors * 4 bytes).
 * STM32F446: 114 vectors * 4 = 456 bytes. Next power-of-2 = 512.
 * We use 16KB (0x4000) which is >> 512, and conveniently equals one sector.
 */
#define APP_HEADER_SIZE         0x00004000UL    /* 16KB                     */
#define APP_MAGIC_WORD          0xDEADBEEFUL
#define APP_HEADER_STRUCT_VER   1U


/* ── Slot A (Sectors 4+5 = 64KB + 128KB = 192KB) ────────────────────────── */

#define SLOT_A_BASE_ADDR        0x08010000UL    /* Sector 4 start           */
#define SLOT_A_HDR_ADDR         SLOT_A_BASE_ADDR
#define SLOT_A_VTOR_ADDR        (SLOT_A_BASE_ADDR + APP_HEADER_SIZE) /* 0x08014000 */
#define SLOT_A_TOTAL_SIZE       0x00030000UL    /* 192KB                    */
#define SLOT_A_FW_MAX_SIZE      (SLOT_A_TOTAL_SIZE - APP_HEADER_SIZE) /* 176KB */
#define SLOT_A_END_ADDR         (SLOT_A_BASE_ADDR + SLOT_A_TOTAL_SIZE)

/* Sectors that belong to Slot A — needed when erasing before an update */
#define SLOT_A_SECTOR_FIRST     4
#define SLOT_A_SECTOR_LAST      5
#define SLOT_A_NUM_SECTORS      2


/* ── Slot B (Sectors 6+7 = 128KB + 128KB = 256KB) ───────────────────────── */

#define SLOT_B_BASE_ADDR        0x08040000UL    /* Sector 6 start           */
#define SLOT_B_HDR_ADDR         SLOT_B_BASE_ADDR
#define SLOT_B_VTOR_ADDR        (SLOT_B_BASE_ADDR + APP_HEADER_SIZE) /* 0x08044000 */
#define SLOT_B_TOTAL_SIZE       0x00040000UL    /* 256KB                    */
#define SLOT_B_FW_MAX_SIZE      (SLOT_B_TOTAL_SIZE - APP_HEADER_SIZE) /* 240KB */
#define SLOT_B_END_ADDR         (SLOT_B_BASE_ADDR + SLOT_B_TOTAL_SIZE)

/* Sectors that belong to Slot B */
#define SLOT_B_SECTOR_FIRST     6
#define SLOT_B_SECTOR_LAST      7
#define SLOT_B_NUM_SECTORS      2


/* ── Firmware Size Cap ───────────────────────────────────────────────────── */
/*
 * Both slots must accept the same firmware binary.
 * So the max firmware size is limited by the SMALLER slot (Slot A = 176KB).
 * Even though Slot B could hold 240KB, we cap both at 176KB.
 * This means the same .bin file can be written to either slot.
 */
#define APP_FW_MAX_SIZE         SLOT_A_FW_MAX_SIZE   /* 176KB — applies to both */


/* ── Slot Index — used in Boot Config ───────────────────────────────────── */

#define SLOT_A                  0U
#define SLOT_B                  1U
#define SLOT_NONE               0xFFU   /* no pending slot                  */
#define NUM_SLOTS               2U


/* ── Slot Lookup Helpers ─────────────────────────────────────────────────── */
/*
 * Use these in bootloader code instead of if/else chains:
 *   uint32_t base = SLOT_BASE_ADDR(active_slot);
 *   uint32_t vtor = SLOT_VTOR_ADDR(active_slot);
 */
#define SLOT_BASE_ADDR(slot)    ((slot) == SLOT_A ? SLOT_A_BASE_ADDR : SLOT_B_BASE_ADDR)
#define SLOT_VTOR_ADDR(slot)    ((slot) == SLOT_A ? SLOT_A_VTOR_ADDR : SLOT_B_VTOR_ADDR)
#define SLOT_HDR_ADDR(slot)     ((slot) == SLOT_A ? SLOT_A_HDR_ADDR  : SLOT_B_HDR_ADDR)
#define SLOT_TOTAL_SIZE(slot)   ((slot) == SLOT_A ? SLOT_A_TOTAL_SIZE : SLOT_B_TOTAL_SIZE)
#define SLOT_SECTOR_FIRST(slot) ((slot) == SLOT_A ? SLOT_A_SECTOR_FIRST : SLOT_B_SECTOR_FIRST)
#define SLOT_SECTOR_LAST(slot)  ((slot) == SLOT_A ? SLOT_A_SECTOR_LAST  : SLOT_B_SECTOR_LAST)


/* ── Compile-time Sanity Checks ─────────────────────────────────────────── */
/*
 * These will cause a BUILD ERROR if the layout is wrong.
 * Much better than discovering a mismatch at runtime.
 */
_Static_assert(BL_END_ADDR        == BOOT_CONFIG_ADDR,     "BL and Boot Config not contiguous");
_Static_assert(SLOT_A_BASE_ADDR   == 0x08010000UL,         "Slot A must start at sector 4");
_Static_assert(SLOT_B_BASE_ADDR   == 0x08040000UL,         "Slot B must start at sector 6");
_Static_assert(SLOT_A_VTOR_ADDR % 0x200 == 0,              "Slot A VTOR alignment fail");
_Static_assert(SLOT_B_VTOR_ADDR % 0x200 == 0,              "Slot B VTOR alignment fail");
_Static_assert(SLOT_B_END_ADDR   == 0x08080000UL,          "Slot B must end at flash end");
_Static_assert(APP_FW_MAX_SIZE   == SLOT_A_FW_MAX_SIZE,    "FW cap must equal smaller slot");

#endif /* FLASH_LAYOUT_H */


