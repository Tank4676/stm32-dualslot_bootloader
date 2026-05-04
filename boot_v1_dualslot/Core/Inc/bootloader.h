/*
 * bootloader.h
 *
 *  Created on: May 4, 2026
 *      Author: tanmay
 */

#ifndef INC_BOOTLOADER_H_
#define INC_BOOTLOADER_H_

#include <stdint.h>
#include "stm32f4xx_hal.h"

void    bootloader_jump(uint32_t app_address);

uint8_t is_slot_valid(uint32_t slot_address);

uint8_t verify_slot_crc(uint32_t slot_address, uint32_t meta_addr, uint32_t crc_addr);

uint32_t sw_crc32(uint32_t crc, const uint8_t *data, uint32_t len);

uint8_t wait_for_trigger(void);

void    flash_erase_slot(uint8_t slot);

HAL_StatusTypeDef flash_write_firmware(uint32_t address, uint8_t *data, uint32_t length);

uint8_t ota_receive_firmware(uint32_t target_slot_address);

void    bootloader_run(void);


#endif /* INC_BOOTLOADER_H_ */
