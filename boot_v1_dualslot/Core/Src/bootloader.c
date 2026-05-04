/*
 * bootloader.c
 *
 *  Created on: May 4, 2026
 *      Author: tanmay
 */

#include "bootloader.h"
#include "bootloader_config.h"
#include "boot_descriptor.h"
#include <stdio.h>
#include <string.h>
#include "stm32f4xx_hal.h"
#include "bootloader_config.h"

#define CHUNK_SIZE 256

extern UART_HandleTypeDef huart3;

uint32_t sw_crc32(uint32_t crc, const uint8_t *data, uint32_t len)
{
    for(uint32_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for(int j = 0; j < 8; j++)
        {
            if(crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }
    return crc;
}

uint8_t is_slot_valid(uint32_t slot_address)
{
    uint32_t msp = *((volatile uint32_t*)slot_address);

    if(msp >= RAM_START && msp <= RAM_END)
        return 1;
    return 0;
}

uint8_t verify_slot_crc(uint32_t slot_address, uint32_t meta_addr, uint32_t crc_addr)
{
    uint32_t app_size = *((volatile uint32_t*)meta_addr);
    uint32_t stored = *((volatile uint32_t*)crc_addr);

    uint32_t computed = sw_crc32(0xFFFFFFFF, (uint8_t*)slot_address, app_size) ^ 0xFFFFFFFF;

    if(computed == stored)
        return 1;
    return 0;
}

void bootloader_jump(uint32_t app_address)
{
    uint32_t msp = *((volatile uint32_t*)app_address);
    uint32_t reset_handler = *((volatile uint32_t*)(app_address + 4));




    __disable_irq();

    __set_MSP(msp);
    SCB->VTOR = app_address;

    void (*app_reset_handler)(void) = (void (*)(void))reset_handler;
    app_reset_handler();
}


uint8_t wait_for_trigger(void)
{
    char buffer[7] = {0};
    uint32_t startTime = HAL_GetTick();

    while((HAL_GetTick() - startTime) < 3000)
    {
        HAL_StatusTypeDef status = HAL_UART_Receive(&huart3, (uint8_t*)buffer, 6, 100);
        if(status == HAL_OK)
        {
            if(strncmp(buffer, "UPDATE", 6) == 0)
                return 1;
        }
    }
    return 0;
}


void flash_erase_slot(uint8_t slot)
{
    FLASH_EraseInitTypeDef eraseInit;
    uint32_t sectorError;

    if(HAL_FLASH_Unlock() != HAL_OK)
        return;

    eraseInit.TypeErase    = FLASH_TYPEERASE_SECTORS;
    eraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    if(slot == 0)  // Slot A — sectors 1-4
    {
        eraseInit.Sector    = FLASH_SECTOR_1;
        eraseInit.NbSectors = 4;
    }
    else           // Slot B — sector 6
    {
        eraseInit.Sector    = FLASH_SECTOR_6;
        eraseInit.NbSectors = 1;
    }

    HAL_FLASHEx_Erase(&eraseInit, &sectorError);
    HAL_FLASH_Lock();
}



HAL_StatusTypeDef flash_write_firmware(uint32_t address, uint8_t *data, uint32_t length)
{
    if(HAL_FLASH_Unlock() != HAL_OK)
        return HAL_ERROR;

    for(uint32_t i = 0; i < length; i += 4)
    {
        uint32_t word = *(uint32_t *)(data + i);
        if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address + i, word) != HAL_OK)
        {
            HAL_FLASH_Lock();
            return HAL_ERROR;
        }
    }

    HAL_FLASH_Lock();
    return HAL_OK;
}


uint8_t ota_receive_firmware(uint32_t target_slot_address)
{
    uint8_t size_buf[4] = {0};
    uint32_t firmware_size = 0;
    uint32_t bytes_received = 0;
    uint8_t chunk[CHUNK_SIZE];
    uint8_t crc_buf[4];
    uint32_t received_crc;
    uint32_t computed_crc;
    uint32_t write_address = target_slot_address;

    /* Tell host we are ready */
    HAL_UART_Transmit(&huart3, (uint8_t*)"READY\r\n", 7, 100);

    /* Step 1: receive firmware size (4 bytes) */
    if(HAL_UART_Receive(&huart3, size_buf, 4, 10000) != HAL_OK)
    {
        HAL_UART_Transmit(&huart3, (uint8_t*)"SIZE TIMEOUT\r\n", 14, 100);
        return 0;
    }
    firmware_size = *(uint32_t*)size_buf;

    char buf[64];
    sprintf(buf, "Receiving %lu bytes\r\n", firmware_size);
    HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), 100);

    /* Step 2: erase the app slot before writing */
    uint8_t target_slot = (target_slot_address == SLOT_A_ADDRESS) ? 0 : 1;
        flash_erase_slot(target_slot);
        HAL_UART_Transmit(&huart3, (uint8_t*)"ERASED\r\n", 8, 100);

    /* Step 3: receive chunks until full firmware received */
    while(bytes_received < firmware_size)
    {
        uint32_t this_chunk = (firmware_size - bytes_received) > CHUNK_SIZE
                                ? CHUNK_SIZE
                                : (firmware_size - bytes_received);

        /* Receive chunk data */
        if(HAL_UART_Receive(&huart3, chunk, this_chunk, 10000) != HAL_OK)
        {
            HAL_UART_Transmit(&huart3, (uint8_t*)"CHUNK TIMEOUT\r\n", 15, 100);
            return 0;
        }

        /* Receive 4 byte CRC for this chunk */
        if(HAL_UART_Receive(&huart3, crc_buf, 4, 10000) != HAL_OK)
        {
            HAL_UART_Transmit(&huart3, (uint8_t*)"CRC TIMEOUT\r\n", 13, 100);
            return 0;
        }
        received_crc = *(uint32_t*)crc_buf;

        /* Compute CRC over received chunk */
        computed_crc = sw_crc32(0xFFFFFFFF, chunk, this_chunk) ^ 0xFFFFFFFF;

        /* Compare */
        if(computed_crc != received_crc)
        {
            HAL_UART_Transmit(&huart3, (uint8_t*)"NACK\r\n", 6, 100);
            return 0;
        }

        /* CRC ok — write chunk to flash */
        if(flash_write_firmware(write_address, chunk, this_chunk) != HAL_OK)
        {
            HAL_UART_Transmit(&huart3, (uint8_t*)"FLASH FAIL\r\n", 12, 100);
            return 0;
        }

        write_address  += this_chunk;
        bytes_received += this_chunk;

        HAL_UART_Transmit(&huart3, (uint8_t*)"ACK\r\n", 5, 100);
    }

    HAL_UART_Transmit(&huart3, (uint8_t*)"DONE\r\n", 6, 100);
    return 1;
}

void bootloader_run(void)
{
    boot_descriptor_t desc;

    /* 1. Read descriptor — if invalid, initialize defaults */
    if(!descriptor_read(&desc))
    {
        HAL_UART_Transmit(&huart3, (uint8_t*)"First boot, init descriptor\r\n", 29, 100);
        descriptor_init_default(&desc);
        descriptor_write(&desc);
    }

    /* 2. Wait for OTA trigger */
    if(wait_for_trigger())
    {
        /* Write to inactive slot */
        uint32_t target = (desc.active_slot == 0) ? SLOT_B_ADDRESS : SLOT_A_ADDRESS;

        HAL_UART_Transmit(&huart3, (uint8_t*)"Entering update mode\r\n", 22, 100);

        if(ota_receive_firmware(target))
        {
            /* Update descriptor — flip active slot */
            desc.active_slot = (desc.active_slot == 0) ? 1 : 0;
            descriptor_write(&desc);
            HAL_UART_Transmit(&huart3, (uint8_t*)"OTA OK, resetting\r\n", 19, 100);
            NVIC_SystemReset();
        }
        else
        {
            HAL_UART_Transmit(&huart3, (uint8_t*)"OTA failed\r\n", 12, 100);
        }
    }

    /* 3. Try to boot active slot */
    uint32_t active_addr  = (desc.active_slot == 0) ? SLOT_A_ADDRESS  : SLOT_B_ADDRESS;
    uint32_t active_meta  = (desc.active_slot == 0) ? SLOT_A_META_ADDR : SLOT_B_META_ADDR;
    uint32_t active_crc   = (desc.active_slot == 0) ? SLOT_A_CRC_ADDR  : SLOT_B_CRC_ADDR;

    if(is_slot_valid(active_addr) && verify_slot_crc(active_addr, active_meta, active_crc))
    {
        HAL_UART_Transmit(&huart3, (uint8_t*)"Booting active slot\r\n", 21, 100);
        char dbg[64];
        sprintf(dbg, "MSP at slot: 0x%08lX\r\n", *((volatile uint32_t*)active_addr));
        HAL_UART_Transmit(&huart3, (uint8_t*)dbg, strlen(dbg), 100);
        sprintf(dbg, "Reset at slot: 0x%08lX\r\n", *((volatile uint32_t*)(active_addr + 4)));
        HAL_UART_Transmit(&huart3, (uint8_t*)dbg, strlen(dbg), 100);
        bootloader_jump(active_addr);
    }

    /* 4. Active failed — try other slot */
    uint32_t other_addr  = (desc.active_slot == 0) ? SLOT_B_ADDRESS  : SLOT_A_ADDRESS;
    uint32_t other_meta  = (desc.active_slot == 0) ? SLOT_B_META_ADDR : SLOT_A_META_ADDR;
    uint32_t other_crc   = (desc.active_slot == 0) ? SLOT_B_CRC_ADDR  : SLOT_A_CRC_ADDR;

    if(is_slot_valid(other_addr) && verify_slot_crc(other_addr, other_meta, other_crc))
    {
        HAL_UART_Transmit(&huart3, (uint8_t*)"Active failed, booting other slot\r\n", 35, 100);
        desc.active_slot = (desc.active_slot == 0) ? 1 : 0;
        descriptor_write(&desc);
        bootloader_jump(other_addr);
    }

    /* 5. Both invalid */
    HAL_UART_Transmit(&huart3, (uint8_t*)"Both slots invalid, halt\r\n", 26, 100);
}
