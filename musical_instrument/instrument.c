/**
* @file  : main.c
* @author: julia jiang
* @brief : theremin! light controlled pentatonic scale notes using DAC and ADC 
*/

#include <math.h>
#include "stm32l4xx_hal.h"

uint16_t sine[64];

// C4, D4, E4, G4, A4, C5, D5, E5 pentatonic scale 
const int notes[8] = {262, 294, 330, 392, 440, 523, 587, 659};

// initialize clock sources: GPIO pin, ADC, DAC
void init_clock(void)
{
    RCC->AHB2ENR |= (1 << 0) | (1 << 13); // GPIOA and ADC clock 

    // ADC clock mode: 01 = HCLK/1 pg.724
    ADC1_COMMON->CCR &= ~(3 << 16);
    ADC1_COMMON->CCR |= (1 << 16);

    RCC->APB1ENR1 |= (1 << 29); // DAC clock
}

// initialize the GPIO pin for ADC (STM PA3 = A0)
void init_pin(void)
{
    GPIOA->MODER |= (3 << 6);  // PA3 bits 7:6 = 11: analog
    GPIOA->PUPDR &= ~(3 << 6); // PA3 bits 7:6 = 00: no pulls
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

    ADC1->CFGR &= ~(3 << 3);   // 12-bit resolution
    ADC1->DIFSEL &= ~(1 << 8); // channel 8: single-ended

    ADC1->SQR1 &= ~15;        // one conversion
    ADC1->SQR1 &= ~(31 << 6);
    ADC1->SQR1 |= (8 << 6);   // first channel: PA3 / channel 8

    ADC1->SMPR1 &= ~(7 << 24);
    ADC1->SMPR1 |= (4 << 24); // channel 8: 47.5-cycle sampling time

    ADC1->ISR = ADC_ISR_ADRDY; // clear old ready flag
    ADC1->CR |= ADC_CR_ADEN;   // enable ADC

    while (!(ADC1->ISR & ADC_ISR_ADRDY)) {
    }
}

int read_adc(void)
{
    ADC1->CR |= ADC_CR_ADSTART; // start conversion

    while (!(ADC1->ISR & ADC_ISR_EOC)) {
    }

    return ADC1->DR; // reading DR also clears EOC
}

// initialize DAC output on PA4
void init_dac(void)
{
    GPIOA->MODER |= (3 << 8);  // PA4: analog
    GPIOA->PUPDR &= ~(3 << 8); // no pulls

    DAC1->CR &= ~DAC_CR_TEN1;    // CPU writes each sample in timer ISR
    DAC1->MCR &= ~DAC_MCR_MODE1; // normal mode, output buffer enabled
    DAC1->DHR12R1 = 2048;        // start at midpoint
    DAC1->CR |= DAC_CR_EN1;      // enable channel 1

    HAL_Delay(1);
}

// generate one sine-wave cycle
void init_sine(void)
{
    for (int i = 0; i < 64; i++) {
        sine[i] = (uint16_t)(2048 + 1800 * sinf(6.2831853f * i / 64)); // 2 pi radians = 1 cycle 
    }
}

// change pitch. assumes 4 MHz timer clock and 64 samples per cycle
void changeNote(int frequency){
    TIM2->ARR = 4000000 / (64 * frequency) - 1;
}

// initialize timer
void init_timer(void)
{
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;

    TIM2->CR1 = TIM_CR1_ARPE; // buffer period changes until next update
    TIM2->PSC = 0;
    TIM2->ARR = 238;         // approximately 261.51 Hz with 64 samples
    TIM2->EGR = TIM_EGR_UG;  // load timer settings
    TIM2->SR = 0;
    TIM2->DIER = TIM_DIER_UIE;

    NVIC_EnableIRQ(TIM2_IRQn);
    TIM2->CR1 |= TIM_CR1_CEN;
}

// output the next sine value
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

// initialize everything 
void init_everything(void){
    HAL_Init();
    init_clock();
    init_pin();
    init_adc();
    init_dac();
    init_sine();
    init_timer();
}


int main(void)
{
    init_everything();

    int note = 0;
    int range = 4095;
    int margin = range / 64; // for hysteresis 

    changeNote(notes[note]);

    while (1) {
        int light = read_adc();

        // hysteresis 
        while (note < 7 &&
            light > (note + 1) * range / 8 + margin) { // light reading is enough to move to the next note 
         note++;
        }
     
        while (note > 0 &&
                light < note * range / 8 - margin) { // light reading is too low -> go to the lower note 
            note--;
        }

        changeNote(notes[note]);
        HAL_Delay(20);
    }
}