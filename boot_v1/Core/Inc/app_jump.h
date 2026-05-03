/*
 * app_jump.h
 *
 *  Created on: Apr 30, 2026
 *      Author: tanmay
 */

#ifndef INC_APP_JUMP_H_
#define INC_APP_JUMP_H_

void bootloader__jump_to_app(void);

uint8_t is_app_valid(void);

uint32_t sw_crc32(uint32_t crc, const uint8_t *data, uint32_t len);

uint8_t verify_app_crc(void);

#endif /* INC_APP_JUMP_H_ */
