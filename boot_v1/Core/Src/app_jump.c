/*
 * app_jump.c
 *
 *  Created on: Apr 30, 2026
 *      Author: tanmay
 */
#include "main.h"
#include "stm32f4xx_hal.h"

extern UART_HandleTypeDef huart3;

#define APP_START_ADDRESS 0X08004000



void bootloader__jumpt_to_app(){
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
