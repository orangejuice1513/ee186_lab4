/**
* @file  : main.c
* @author: julia jiang
* @brief : configure onboard DAC and read its output with the ADC
*/

#include <stdio.h>
#include "stm32l4xx_hal.h"
#include <math.h>

UART_HandleTypeDef hlpuart1 = {0};
uint16_t sine[64];


// initialize clock sources: GPIO pin and ADC
void init_clock(void)
{
    RCC->AHB2ENR |= (1 << 0) | (1 << 13); // enable clock for GPIOA and ADC bus

    // ADC clock mode: 01 = HCLK/1 pg.724
    ADC1_COMMON->CCR &= ~(3 << 16);
    ADC1_COMMON->CCR |= (1 << 16);

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


// generate one sine-wave cycle
void init_sine(void)
{
    for (int i = 0; i < 64; i++) {
        sine[i] = (uint16_t)(2048 + 1800 * sinf(6.2831853f * i / 64));
    }
}


// update DAC about 4192 times per second
void init_timer(void)
{
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;

    // update DAC about 16736 times per second
    TIM2->PSC = 0;
    TIM2->ARR = 238; // approximately 261.51 Hz with 64 samples    TIM2->EGR = TIM_EGR_UG;      // load timer settings
    TIM2->SR = 0;               // clear update flag
    TIM2->DIER |= TIM_DIER_UIE;  // enable update interrupt

    NVIC_EnableIRQ(TIM2_IRQn);
    TIM2->CR1 |= TIM_CR1_CEN;    // start timer
}

// output the next sine-table value
void TIM2_IRQHandler(void)
{
    static int index = 0;

    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR = 0;

        DAC1->DHR12R1 = sine[index];
        index++;

        if (index == 64) {
            index = 0;
        }
    }
}


int main(void)
{
    HAL_Init();
    init_clock();
    init_pin();
    init_dac();
    init_sine();
    init_timer();

    while (1) {
    }
}
