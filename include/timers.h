#ifndef TIMERS_H
#define TIMERS_H

#include "stdint.h"

void timer0_init();
void timer1_init();
void timer1_reset();
void timer1_stop();
void timer1_start();
void temp_delay_us(uint16_t us);


#endif