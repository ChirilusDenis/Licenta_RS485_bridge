#ifndef MODBUS_H
#define MODBUS_H

#include "stdint.h"

uint16_t mb_CRC(uint16_t len);
uint8_t mb_check_frame(uint16_t len);
void mb_send_frame(uint16_t len);
uint16_t mb_process_frame(uint16_t len);

#endif