#include <avr/io.h>
#include "constants.h"

void USART0_init() {
    uint16_t baud_idx = holding_regs[HOLDING_RS485_BAUD];

    // double speed for high baudrates
    if (baud_idx > 8)
        UCSR0A |= (1 << U2X0);

    // set baudrate
    uint16_t baud_rate = UBRR_TABLE[baud_idx];
    UBRR0H = (uint8_t)(baud_rate >> 8);
    UBRR0L = (uint8_t)baud_rate;

    // set character format 8 bit
    UCSR0C = (3 << UCSZ00) | (holding_regs[HOLDING_RS485_PARITY] << UPM00);
    if (holding_regs[HOLDING_RS485_PARITY] == PARITY_NO) UCSR0C |= (1 << USBS0);

    // enable
    UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);
}

void USART0_transmit(uint8_t data) {
    while(!(UCSR0A & (1 << UDRE0))) // wait until register is empty
        ;
 
    UDR0 = data; // put data in sending register
}

uint8_t USART0_receive() {
    while (!(UCSR0A & (1 << RXC0))) // wait until finishes recv
        ;
 
    return UDR0; // get data
}

// void USART0_flush() {
//     while (UCSR0A & (1 << RXC0))
//         (void)UDR0;
// }