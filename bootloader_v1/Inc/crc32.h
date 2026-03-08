/*
 * crc32.h
 *
 *  Created on: Mar 7, 2026
 *      Author: tanmay
 */



/**
 * @brief   Software CRC32 — table-driven, ISO-HDLC / zlib compatible.
 *
 * Matches exactly:
 *   Python : zlib.crc32(data) & 0xFFFFFFFF
 *   Python : binascii.crc32(data) & 0xFFFFFFFF
 *
 * Algorithm parameters (CRC-32/ISO-HDLC):
 *   Poly   : 0x04C11DB7  (stored as 0xEDB88320 reflected)
 *   Init   : 0xFFFFFFFF
 *   RefIn  : Yes
 *   RefOut : Yes
 *   XorOut : 0xFFFFFFFF
 */

#ifndef CRC32_H
#define CRC32_H

#include <stdint.h>
#include <stddef.h>

/* ── One-shot ────────────────────────────────────────────────────────────── */

/**
 * @brief  Compute CRC32 over a contiguous buffer.
 * @param  data    Pointer to input bytes.
 * @param  length  Number of bytes.
 * @return Finalised CRC32 value.
 */
uint32_t crc32_compute(const uint8_t *data, size_t length);

/* ── Streaming (chunked) API ─────────────────────────────────────────────── */

/**
 * @brief  Feed a chunk into a running CRC32 calculation.
 *
 * Example — computing CRC in two chunks:
 * @code
 *   uint32_t crc = CRC32_INIT;
 *   crc = crc32_update(crc, chunk1, len1);
 *   crc = crc32_update(crc, chunk2, len2);
 *   uint32_t result = CRC32_FINAL(crc);
 * @endcode
 *
 * @param  crc     Running state. Pass CRC32_INIT to start fresh.
 * @param  data    Pointer to data chunk.
 * @param  length  Number of bytes in this chunk.
 * @return Updated running CRC state (NOT yet finalised).
 */
uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t length);

/** Initial value — pass to crc32_update() to start a new calculation */
#define CRC32_INIT          0xFFFFFFFFUL

/** Finalise a running CRC — apply after the last crc32_update() call */
#define CRC32_FINAL(crc)    ((crc) ^ 0xFFFFFFFFUL)



#endif /* CRC32_H_ */
