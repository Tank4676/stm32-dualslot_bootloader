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
#define APP_SIZE 0x2EEC



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
	uint32_t computed = sw_crc32(0xFFFFFFFF, (uint8_t*)APP_START_ADDRESS, APP_SIZE - 4);
	computed ^= 0xFFFFFFFF;
	uint32_t stored = *((volatile uint32_t*)(APP_START_ADDRESS + APP_SIZE - 4));


    if(computed == stored)
        return 1;
    return 0;
}
