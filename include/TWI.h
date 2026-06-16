#ifndef TWI_H
#define TWI_H

#include "avr/io.h"

#define TWI_STATUS (TWSR & 0xF8)

void TWI_init(uint8_t speed);
void TWI_start();
void TWI_stop();
uint8_t TWI_read_ack();
uint8_t TWI_read_nack();
void TWI_write(uint8_t data);
void TWI_write_sla_r(uint8_t addr);
void TWI_write_sla_w(uint8_t addr);
void eeprom_write_reg(uint16_t addr, uint16_t data);

#endif