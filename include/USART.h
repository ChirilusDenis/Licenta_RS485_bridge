#ifndef USART_H
#define USART_H

#include "stdint.h"

void USART0_init();
void USART0_transmit(uint8_t data);
char USART0_receive();
// void USART0_flush();

#endif