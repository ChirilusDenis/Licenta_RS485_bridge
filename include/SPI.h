#ifndef SPI_H
#define SPI_H

#include "stdint.h"

void SPI_init();
uint8_t SPI_exchange(uint8_t data);
void SPI_select(uint8_t pin);
void SPI_deselect(uint8_t pin);

#endif