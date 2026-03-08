/*
 * flash_if.h
 *
 *  Created on: Mar 8, 2026
 *      Author: tanmay
 */



/* flash_if.h */

#ifndef FLASH_IF_H
#define FLASH_IF_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

typedef enum {
    FLASH_IF_OK    = 0,
    FLASH_IF_ERROR = 1
} FlashIF_Status;

FlashIF_Status FlashIF_Erase  (uint32_t addr, uint32_t size);
FlashIF_Status FlashIF_Write  (uint32_t addr, const uint8_t *data, uint32_t size);
FlashIF_Status FlashIF_Verify (uint32_t addr, const uint8_t *data, uint32_t size);
uint8_t        FlashIF_IsEmpty(uint32_t addr, uint32_t size);


#endif /* FLASH_IF_H_ */
