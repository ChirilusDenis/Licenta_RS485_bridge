#ifndef ASCII_H
#define ASCII_H

#include <stdint.h>

uint8_t mb_LRC(uint8_t len);
uint16_t ascii_decode(uint8_t ascii_len);

#endif