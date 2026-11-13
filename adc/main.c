/**
* @file  : main.c
* @author: julia jiang
* @brief : configure onboard ADC and read from the ADC pin 
*/

#include <stdio.h>
#include "stm32l4xx_hal.h"

UART_HandleTypeDef hlpuart1 = {0};

// initialize clock sources: GPIO pin and ADC
void init_clock(void)
{
RCC->AHB2ENR |= (1 << 0) | (1 << 13); // enable clock for GPIOA and ADC bus

// ADC clock mode: 01 = HCLK/1 pg.724
ADC1_COMMON->CCR &= ~(3 << 16);
ADC1_COMMON->CCR |= (1 << 16);

// enable clock for GPIO G for UART to print shi to the terminal 
__HAL_RCC_GPIOG_CLK_ENABLE();
__HAL_RCC_LPUART1_CLK_ENABLE();
__HAL_RCC_PWR_CLK_ENABLE();
HAL_PWREx_EnableVddIO2();

RCC_PeriphCLKInitTypeDef clock = {0};
clock.PeriphClockSelection = RCC_PERIPHCLK_LPUART1;
clock.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_PCLK1;
HAL_RCCEx_PeriphCLKConfig(&clock);
}

// initialize the GPIO pin for ADC (STM PAbbb3 = A0)
void init_pin(void)
{
GPIOA->MODER |= (3 << 6);      // PA3 bits 7:6 = 11: analog
GPIOA->PUPDR &= ~(3 << 6);     // PA3 bits 7:6 = 00: no pulls

// PG7 = UART TX
GPIO_InitTypeDef pins = {0};
pins.Pin = GPIO_PIN_7;
pins.Mode = GPIO_MODE_AF_PP;
pins.Alternate = GPIO_AF8_LPUART1;
HAL_GPIO_Init(GPIOG, &pins);
}

void init_uart(void)
{
    hlpuart1.Instance = LPUART1;
    hlpuart1.Init.BaudRate = 115200;
    hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
    hlpuart1.Init.StopBits = UART_STOPBITS_1;
    hlpuart1.Init.Parity = UART_PARITY_NONE;
    hlpuart1.Init.Mode = UART_MODE_TX;
    hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&hlpuart1);
}

// initialize adc
void init_adc(void)
{
    ADC1->CR &= ~ADC_CR_DEEPPWD;  // exit deep-power-down
    ADC1->CR |= ADC_CR_ADVREGEN;  // enable internal regulator
    HAL_Delay(1);                // wait for regulator

    ADC1->CR &= ~ADC_CR_ADCALDIF; // single-ended calibration
    ADC1->CR |= ADC_CR_ADCAL;     // start calibration

    while (ADC1->CR & ADC_CR_ADCAL) {
    }
    HAL_Delay(1);                // delay before enabling ADC

    ADC1->CFGR &= ~(3 << 3);      // 12-bit resolution
    ADC1->DIFSEL &= ~(1 << 8);    // channel 8: single-ended

    ADC1->SQR1 &= ~15;           // one conversion
    ADC1->SQR1 &= ~(31 << 6);
    ADC1->SQR1 |= (8 << 6);      // PA3 / channel 8

    ADC1->SMPR1 &= ~(7 << 24);
    ADC1->SMPR1 |= (4 << 24);    // 47.5-cycle sampling time

    ADC1->ISR = ADC_ISR_ADRDY;
    ADC1->CR |= ADC_CR_ADEN;

    while (!(ADC1->ISR & ADC_ISR_ADRDY)) {
    }
}

int read_adc(void)
{
    ADC1->CR |= ADC_CR_ADSTART;  // start conversion

    while (!(ADC1->ISR & ADC_ISR_EOC)) {
    }

    return ADC1->DR; // reading DR also clears EOC
}

int main(void)
{
    HAL_Init();
    init_clock();
    init_pin();
    init_uart();
    init_adc();

    char text[16];

    while (1) {
        int length = snprintf(text, sizeof(text), "%d\r\n", read_adc());
        HAL_UART_Transmit(&hlpuart1, (uint8_t *)text, length, 100);
        HAL_Delay(200);
    }
}