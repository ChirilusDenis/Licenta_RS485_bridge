
#ifndef HOLDING_HELPERS_H
#define HOLDING_HELPERS_H

#include "stdint.h"

void init_holding();
void set_GPIO_pins();
void set_output();
void set_input();
uint8_t write_holding_reg(uint16_t addr, uint16_t value);
uint16_t read_input_reg(uint16_t addr, uint8_t *error);
void init_eeprom();
void test_holding();
void update_sensor_coils();

#endif