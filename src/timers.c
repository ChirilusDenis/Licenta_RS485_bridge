#include <avr/io.h>
#include "timers.h"
#include "constants.h"

void timer0_init() {
    TIMSK0 |= (1 << OCIE0A); // enable counter interrupt
    TCCR0A = (1 << WGM01); // set CTC mode
    TCCR0B |= (1 << CS01) | (1 << CS00); // set prescaler to 64

    OCR0A = 250; // for 1ms interrupt
}


void timer1_init() {
    TIMSK1 |= (1 << OCIE1A); // enable compare interrupt
    TCCR1B |= (1 << WGM12); // set CTC mode
    OCR1A = OCR1A_TABLE[holding_regs[HOLDING_RS485_BAUD]];
}

void timer1_reset() {
    TCNT1 = 0;
}

void timer1_stop() {
    // set prescaler to 0
    TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));
}

void timer1_start() {
    TCCR1B |= (1 << CS12); // set prescaler to 256
}

void temp_delay_us(uint16_t us) {
    uint16_t i = us * 4;
    while (i--) {
        __asm__ __volatile__ ("nop");
    }
}