/*
 * boot_manager.h
 *
 *  Created on: Mar 8, 2026
 *      Author: tanmay
 */

#ifndef BOOT_MANAGER_H_
#define BOOT_MANAGER_H_



#include <stdint.h>

/*
 * BootManager_Run — main bootloader decision logic.
 * Call once from main(). Never returns.
 */
void BootManager_Run(void);

/*
 * BootManager_JumpToApp — hands control to firmware in given slot.
 * Never returns if successful.
 */
void BootManager_JumpToApp(uint8_t slot);

#endif /* BOOT_MANAGER_H_ */
