/*
 * boot_descriptor.h
 *
 *  Created on: May 4, 2026
 *      Author: tanmay
 */

#ifndef INC_BOOT_DESCRIPTOR_H_
#define INC_BOOT_DESCRIPTOR_H_

#include <stdint.h>

typedef struct {
    uint32_t magic;
    uint32_t active_slot;     // 0 = Slot A, 1 = Slot B
    uint32_t slot_a_version;
    uint32_t slot_b_version;
    uint32_t slot_a_crc;
    uint32_t slot_b_crc;
    uint32_t slot_a_size;
    uint32_t slot_b_size;
    uint32_t boot_attempts;
    uint32_t reserved;        // padding to align
} boot_descriptor_t;

uint8_t descriptor_read(boot_descriptor_t *desc);
uint8_t descriptor_write(boot_descriptor_t *desc);
void    descriptor_init_default(boot_descriptor_t *desc);


#endif /* INC_BOOT_DESCRIPTOR_H_ */
