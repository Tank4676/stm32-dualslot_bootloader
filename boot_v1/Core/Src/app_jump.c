/*
 * app_jump.c
 *
 *  Created on: Apr 30, 2026
 *      Author: tanmay
 */
#include "main.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>


extern CRC_HandleTypeDef hcrc;
extern UART_HandleTypeDef huart3;


#define APP_START_ADDRESS 0X08004000
#define CHUNK_SIZE 256




void bootloader__jump_to_app(){
uint32_t msp  = *((volatile uint32_t*)APP_START_ADDRESS);
uint32_t reset_handler = *((volatile uint32_t*)(APP_START_ADDRESS +4));

HAL_GPIO_DeInit(GPIOB, GPIO_PIN_0 | GPIO_PIN_14);
HAL_UART_DeInit(&huart3);
HAL_RCC_DeInit();
SysTick->CTRL = 0;
SysTick->LOAD = 0;
SysTick->VAL  = 0;
__disable_irq();

__set_MSP (msp);
SCB->VTOR = APP_START_ADDRESS;

void (*app_reset_handler)(void) = (void (*)(void))reset_handler;
app_reset_handler();

}


uint8_t is_app_valid(void){

	uint32_t msp = *((volatile uint32_t*)APP_START_ADDRESS);

	if(msp >= 0x20000000 && msp <= 0x20020000)
	    {
	        return 1;
	    }
	    return 0;

}

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

uint8_t verify_app_crc(void)
{
    uint32_t app_size = *((volatile uint32_t*)0x0801FFF8);
    uint32_t stored_crc = *((volatile uint32_t*)0x0801FFFC);

    uint32_t computed = sw_crc32(0xFFFFFFFF, (uint8_t*)APP_START_ADDRESS, app_size);
    computed ^= 0xFFFFFFFF;

    char buf[64];
    sprintf(buf, "Computed: 0x%08lX\r\n", computed);
    HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), 100);
    sprintf(buf, "Stored:   0x%08lX\r\n", stored_crc);
    HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), 100);

    if(computed == stored_crc)
        return 1;
    return 0;
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


void flash_erase_app_slot(void)
{
    FLASH_EraseInitTypeDef eraseInit;
    uint32_t sectorError;

    if(HAL_FLASH_Unlock() != HAL_OK)
        return;

    eraseInit.TypeErase    = FLASH_TYPEERASE_SECTORS;
    eraseInit.Sector       = 1;
    eraseInit.NbSectors    = 4;
    eraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    HAL_FLASHEx_Erase(&eraseInit, &sectorError);

    if(sectorError != 0xFFFFFFFF)
        HAL_UART_Transmit(&huart3, (uint8_t*)"Erase failed\r\n", 14, 100);

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

uint8_t ota_receive_firmware(void)
{
    uint8_t size_buf[4] = {0};
    uint32_t firmware_size = 0;
    uint32_t bytes_received = 0;
    uint8_t chunk[CHUNK_SIZE];
    uint8_t crc_buf[4];
    uint32_t received_crc;
    uint32_t computed_crc;
    uint32_t write_address = APP_START_ADDRESS;

    /* Tell host we are ready */
    HAL_UART_Transmit(&huart3, (uint8_t*)"READY\r\n", 7, 100);

    /* Step 1: receive firmware size (4 bytes) */
    if(HAL_UART_Receive(&huart3, size_buf, 4, 5000) != HAL_OK)
    {
        HAL_UART_Transmit(&huart3, (uint8_t*)"SIZE TIMEOUT\r\n", 14, 100);
        return 0;
    }
    firmware_size = *(uint32_t*)size_buf;

    char buf[64];
    sprintf(buf, "Receiving %lu bytes\r\n", firmware_size);
    HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), 100);

    /* Step 2: erase the app slot before writing */
    flash_erase_app_slot();

    /* Step 3: receive chunks until full firmware received */
    while(bytes_received < firmware_size)
    {
        uint32_t this_chunk = (firmware_size - bytes_received) > CHUNK_SIZE
                                ? CHUNK_SIZE
                                : (firmware_size - bytes_received);

        /* Receive chunk data */
        if(HAL_UART_Receive(&huart3, chunk, this_chunk, 5000) != HAL_OK)
        {
            HAL_UART_Transmit(&huart3, (uint8_t*)"CHUNK TIMEOUT\r\n", 15, 100);
            return 0;
        }

        /* Receive 4 byte CRC for this chunk */
        if(HAL_UART_Receive(&huart3, crc_buf, 4, 5000) != HAL_OK)
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

