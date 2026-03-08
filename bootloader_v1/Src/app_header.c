/*
 * app_header.c
 *
 *  Created on: Mar 8, 2026
 *      Author: tanmay
 */

/*
 * app_header.c
 *
 *  Created on: Mar 8, 2026
 *      Author: tanmay
 */

/**
 * @file    app_header.c
 * @brief   Firmware header validation — bootloader project only.
 */

#include "app_header.h"   /* AppHeader_t, AppHeader_Status, AppHeader_Get()  */
#include "crc32.h"        /* crc32_compute()                                  */
#include "flash_layout.h" /* SLOT_VTOR_ADDR(), APP_FW_MAX_SIZE                */

/* ── AppHeader_Validate ──────────────────────────────────────────────────── */

AppHeader_Status AppHeader_Validate(uint8_t slot)
{
    /*
     *
     *
     * Flash on STM32 is memory-mapped — the CPU can read it like RAM using
     * normal addresses. We don't need any HAL function.
     *
     * AppHeader_Get(slot) converts the slot index (0 or 1) into the actual
     * flash address (0x08010000 for Slot A, 0x08040000 for Slot B) and
     * returns it as a const AppHeader_t pointer.
     *
     * After this line hdr->magic, hdr->fw_size etc. all read directly
     * from flash — no copying needed.
     *
     * WHY const? We are only reading the header, never writing through
     * this pointer. const tells the compiler (and other developers) that
     * this function will not modify flash.
     */
    const AppHeader_t *hdr = AppHeader_Get(slot);

    /*
     *
     *
     * APP_MAGIC_WORD is 0xDEADBEEF (defined in flash_layout.h).
     * The build tool writes this into hdr->magic when it creates the firmware.
     *
     * If magic doesn't match it means:
     *   - Slot is blank (erased flash reads as 0xFFFFFFFF)
     *   - Slot has garbage / was never programmed
     *   - Completely wrong data at this address
     *
     * No point doing any further checks — return immediately.
     * This is called "early exit" or "guard clause" — a very common C pattern.
     *
     * WHY check this first? It's the cheapest check (one comparison).
     * The CRC checks are expensive (read entire firmware). Never do expensive
     * work when a cheap check would have caught the problem first.
     */
    if (hdr->magic != APP_MAGIC_WORD)
    {
        return APP_HDR_BAD_MAGIC;
    }

    /*
     *
     *
     * APP_HEADER_VERSION is defined in app_header.h (currently = 1).
     * If we ever add/remove fields from AppHeader_t we bump this number.
     *
     * If versions don't match it means this firmware was built for a
     * different bootloader that had a different header layout.
     * Reading it would give us garbage values because the fields are
     * in different positions.
     *
     * Example: old bootloader expects fields at offsets 0,4,8,12...
     *          new firmware puts them at 0,4,8,12,16... (added a field)
     *          → version mismatch catches this before we misread fields
     */
    if (hdr->hdr_version != APP_HEADER_VERSION)
    {
        return APP_HDR_BAD_VERSION;
    }

    /*
     *
     *
     * hdr_crc protects the header struct itself from corruption.
     * (A separate fw_crc protects the firmware bytes — Step 6.)
     *
     * HOW IT WORKS:
     *   At build time: Python script computes CRC over the header struct
     *                  (excluding hdr_crc itself) and stores the result
     *                  in hdr->hdr_crc inside the .bin file.
     *
     *   At boot time:  We recompute the same CRC right now and compare.
     *                  If flash got corrupted the bytes changed → CRC won't match.
     *
     * WHY exclude hdr_crc from its own CRC?
     *   If you included hdr_crc in the computation, the value would depend
     *   on itself — impossible to compute. Classic chicken-and-egg problem.
     *   Solution: always compute CRC over everything EXCEPT the crc field.
     *   Since hdr_crc is the LAST field, we just compute over
     *   (total size - 4 bytes).
     *
     * (const uint8_t *)hdr  — cast the struct pointer to a byte pointer
     *                         so crc32_compute can walk through it byte by byte.
     *                         crc32_compute doesn't know about structs,
     *                         it just sees a sequence of bytes.
     *
     * sizeof(AppHeader_t) - sizeof(uint32_t)  — all bytes except last 4
     *                                            (hdr_crc field = 4 bytes)
     */
    uint32_t computed_hdr_crc = crc32_compute(
                                    (const uint8_t *)hdr,
                                    sizeof(AppHeader_t) - sizeof(uint32_t)
                                );

    if (computed_hdr_crc != hdr->hdr_crc)
    {
        return APP_HDR_BAD_HDR_CRC;
    }

    /*
     *
     * hdr->fw_size is the number of firmware bytes in this slot
     * (NOT including the header itself — just the code after VTOR).
     *
     * APP_FW_MAX_SIZE = 176KB (defined in flash_layout.h).
     * Both slots are capped at 176KB so the same .bin fits either slot.
     *
     * WHY check this before fw_crc?
     *   If fw_size is corrupted and contains a huge garbage value,
     *   the fw_crc check in Step 6 would try to compute CRC over
     *   gigabytes of memory — reading way past the end of flash.
     *   Checking size first protects against that.
     */
    if (hdr->fw_size > APP_FW_MAX_SIZE)
    {
        return APP_HDR_BAD_SIZE;
    }

    /*
     *
     *
     * This is the most expensive check — reads the ENTIRE firmware image.
     * That's why we do all the cheap checks first and only get here if
     * everything else passed.
     *
     * SLOT_VTOR_ADDR(slot) — start address of the actual firmware bytes.
     *   For Slot A: 0x08014000 (base + 16KB header offset)
     *   For Slot B: 0x08044000
     *   The firmware (vector table + code) starts here, after the header.
     *
     * hdr->fw_size — how many bytes to check. Stored in the header by
     *   the build tool. We already validated this is <= APP_FW_MAX_SIZE.
     *
     * If even one byte of the firmware changed since it was flashed
     * (power cut, flash cell decay, bad write) — CRC won't match.
     */
    uint32_t computed_fw_crc = crc32_compute(
                                   (const uint8_t *)SLOT_VTOR_ADDR(slot),
                                   hdr->fw_size
                               );

    if (computed_fw_crc != hdr->fw_crc)
    {
        return APP_HDR_BAD_FW_CRC;
    }

    /*
     *
     *
     * magic OK + version OK + header intact + size OK + firmware intact.
     * This slot contains a valid, uncorrupted, bootable firmware image.
     * Safe to jump to it.
     */
    return APP_HDR_OK;
}
