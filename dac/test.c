/**
* @file  : main.c
* @author: julia jiang
* @brief : configure onboard DAC and read its output with the ADC  
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


    // for UART 
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_LPUART1_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWREx_EnableVddIO2();

    RCC_PeriphCLKInitTypeDef clock = {0};
    clock.PeriphClockSelection = RCC_PERIPHCLK_LPUART1;
    clock.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_PCLK1;
    HAL_RCCEx_PeriphCLKConfig(&clock);

    RCC->AHB2ENR |= (1 << 0);       // GPIOA clock
    RCC->APB1ENR1 |= (1 << 29);     // DAC clock
}

// initialize the GPIO pin for ADC (STM PA3 = A0)
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
    HAL_Delay(1);

    ADC1->CR &= ~ADC_CR_ADCALDIF; // single-ended calibration
    ADC1->CR |= ADC_CR_ADCAL;     // start calibration

    while (ADC1->CR & ADC_CR_ADCAL) {
    }
    HAL_Delay(1);

    ADC1->CFGR &= ~(3 << 3);      // 12-bit resolution
    ADC1->DIFSEL &= ~(1 << 8);    // channel 8: single-ended

    ADC1->SQR1 &= ~15;           // one conversion
    ADC1->SQR1 &= ~(31 << 6);
    ADC1->SQR1 |= (8 << 6);      // first channel: PA3 / channel 8

    ADC1->SMPR1 &= ~(7 << 24);
    ADC1->SMPR1 |= (4 << 24);    // channel 8: 47.5-cycle sampling time

    ADC1->ISR = ADC_ISR_ADRDY;   // clear old ready flag
    ADC1->CR |= ADC_CR_ADEN;     // enable ADC

    while (!(ADC1->ISR & ADC_ISR_ADRDY)) {
    }
}


// initialize DAC output on PA4
void init_dac(void)
{
    GPIOA->MODER |= (3 << 8);       // PA4: analog
    GPIOA->PUPDR &= ~(3 << 8);      // no pulls

    DAC1->CR &= ~DAC_CR_TEN1;       // no timer trigger
    DAC1->MCR &= ~DAC_MCR_MODE1;    // normal mode, output buffer enabled
    DAC1->CR |= DAC_CR_EN1;         // enable channel 1

    HAL_Delay(1);                  // allow output to start
}


int read_adc(void)
{
    ADC1->CR |= ADC_CR_ADSTART;  // start conversion

    while (!(ADC1->ISR & ADC_ISR_EOC)) {
    }

    return ADC1->DR; // reading DR also clears EOC
}

// increase DAC to maximum, then decrease to minimum
void test_dac(void)
{
    static int value = 0;
    static int step = 64;
    char text[80];

    DAC1->DHR12R1 = value;
    HAL_Delay(1); // allow output to settle

    int dac_mv = value * 3300 / 4096;
    int adc_mv = read_adc() * 3300 / 4096;

    int length = snprintf(text, sizeof(text),
                          "DAC: %d mV  ADC: %d mV  difference: %d mV\r\n",
                          dac_mv, adc_mv, adc_mv - dac_mv);

    HAL_UART_Transmit(&hlpuart1, (uint8_t *)text, length, 100);

    if (value == 4095) {
        step = -64;
    } else if (value == 0) {
        step = 64;
    }

    value += step;

    if (value > 4095) {
        value = 4095;
    } else if (value < 0) {
        value = 0;
    }
} 

int main(void)
{
    HAL_Init();
    init_clock();
    init_pin();
    init_uart();
    init_adc();
    init_dac();

    while (1) {
        test_dac();
        HAL_Delay(100);
    }
}