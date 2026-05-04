/*
 * bootloader_config.h
 *
 *  Created on: May 4, 2026
 *      Author: tanmay
 */

#ifndef INC_BOOTLOADER_CONFIG_H_
#define INC_BOOTLOADER_CONFIG_H_

#define SLOT_A_ADDRESS       0x08004000U
#define SLOT_A_SIZE          0x1C000U
#define SLOT_A_META_ADDR     0x0801FFF8U
#define SLOT_A_CRC_ADDR      0x0801FFFCU

#define SLOT_B_ADDRESS       0x08040000U
#define SLOT_B_SIZE          0x20000U
#define SLOT_B_META_ADDR     0x0805FFF8U
#define SLOT_B_CRC_ADDR      0x0805FFFCU

#define BOOT_DESCRIPTOR_ADDR 0x08020000U
#define BOOT_DESCRIPTOR_MAGIC 0xDEADBEEFU

#define RAM_START            0x20000000U
#define RAM_END              0x20020000U

#endif /* INC_BOOTLOADER_CONFIG_H_ */
