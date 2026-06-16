#ifndef ADC_H
#define ADC_H

#include "stdint.h"

extern uint8_t adc_en;

void ADC_init();
void ADC_read(uint8_t channel);
void ADC_enable();
void ADC_stop();

#endif