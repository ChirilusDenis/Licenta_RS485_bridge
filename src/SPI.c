#include <avr/io.h>
#include "SPI.h"

void SPI_init() {
    // set SCK, MOSI and SS as output
    DDRB |= (1 << PB5) | (1 << PD3) | (1 << PB2) | (1 << PB1) | (1 << PB0);
    PORTB |= (1 << PB2) | (1 << PB1) | (1 << PB0); // unset all slave select

    SPCR |= (1 << MSTR) | (1 << SPR0) | (1 << SPE);
}

void SPI_select(uint8_t pin) {
    PORTB &= ~(1 << pin); // select slave
}
void SPI_deselect(uint8_t pin) {
    PORTB |= (1 << pin); // select slave
}

uint8_t SPI_exchange(uint8_t data) {
    SPDR = data; // set data to init transfer
    while (!(SPSR & (1 << SPIF))) // wait untill transfer is complete
        ;

    return (SPDR);
}