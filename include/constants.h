#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "stdint.h"

#define DEFAULT_MODBUS_ADDR     247
#define MODBUS_MAX              247
#define DEFAULT_BAUD_IDX          5
#define FRAME_MAX               256
#define ASCII_MAX               510

// #define BAUD_PARITY_LOCATION 14
#define NUM_BAUD_RATES       11
#define PARITY_NO             0
#define PARITY_EVEN           2
#define PARITY_ODD            3
#define MODE_RTU              0
#define MODE_ASCII            1

// MODBUS excpetion codes
#define MODBUS_ILLEGAL_FUNCTION    1
#define MODBUS_ILLEGAL_DATA_ADDR   2
#define MODBUS_ILLEGAL_DATA_VALUE  3
#define MODBUS_SERVER_FAILURE      4
#define MODBUS_DEVICE_BUSY         6

// MODBUS subfunctions
#define RETURN_QUERY_DATA        0
#define RESTART_COMM             1
#define CLEAR_CNT_DIAG          10
#define RETURN_MSG_COUNT        11
#define RETURN_COMM_ERROR_COUNT 12
#define RETURN_EX_COUNT         13

// status bits
#define EX_FRAME_OVERFLOW          (1 << 0)
// #define EX_ADC_FAIL             (1 << 0)
// #define EX_SPI_FAIL             (1 << 1)
// #define EX_TWI_FAIL             (1 << 2)

#define EX_TWI_START_FAIL       (1 << 3)
#define EX_TWI_R_FAIL           (1 << 4)
#define EX_TWI_WR_FAIL          (1 << 5)

#define CLEAR_ON_READ_BITS  (EX_TWI_START_FAIL | EX_TWI_R_FAIL | EX_TWI_WR_FAIL)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//                  HOLDING REGS
//              Example for 1 SPI and 1 I2C sensor
//ADDR|    MEANING         |     DETAILS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 0  | MODBUS_ADDR        |     self explanatory
// 1  | MODBUS_MODE        |     RTU or ASCII transmission mode
// 2  | RS485_BAUD         |     baud rate index
// 3  | RS485_PARITY       |     rs comm parity
// 4  | COILS(outputs)     |     each bit represents one coil pin: PD3...PD7
// 5  | INPUT              |     each bit reprsents one input pin: PD3...PD7
//    |                    |     extra condition -> no overlap
//                               added coils for spi and twi send
// 6  | ADC_VALID_INPUTS   |     each bit represents if that input is valid: PC0...PC3
//
// 7  | SPI_VALID_PINS     |     each bit represents if that slave is valid: PB2...PB0
// 8  | SPI_WRITE_DATA     |     2 bytes to be sent in the next transaction (default FF FF)
//
// 9  | VALID_TWI_ADDR     |     each bit represents if the twi address is valid, stating at bit 0
// 10 | TWI_ADDR_0         |     twi address 0
// 11 | TWI_DATA_SLA0      |     holds data that is sent each time

// 12 | TWI_ADDR_1         |     twi address 1
// 13 | TWI_ADDR_2         |     twi address 2
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define NUM_ADC_REG          4
#define NUM_SPI_SLAVES       3
#define NUM_TWI_SLAVES       3
#define TWI_SLAVE_REG_SIZE   4
#define SPI_SLAVE_REG_SIZE   3

#define NUM_DEFAULT_SS       3


#define NUM_BASE_REGS           8
#define NUM_HOLDING_REGS    (NUM_BASE_REGS + 1 + NUM_SPI_SLAVES * SPI_SLAVE_REG_SIZE + NUM_TWI_SLAVES * TWI_SLAVE_REG_SIZE)

#define HOLDING_MB_ADDRESS       0
#define HOLDING_MB_MODE          1

#define HOLDING_RS485_BAUD       2
#define HOLDING_RS485_PARITY     3
#define HOLDING_COILS_ADDR       4
#define HOLDING_INPUT_ADDR       5
#define HOLDING_ADC_IN_ADDR      6

#define HOLDING_SPI_SS_ADDR      7

#define HOLDING_SPI_0_DELAY      8
#define HOLDING_SPI_0_DATA_COUNT 9
#define HOLDING_SPI_0_DATA      10


#define HOLDING_VALID_TWI_ADDR   (NUM_BASE_REGS + NUM_SPI_SLAVES * SPI_SLAVE_REG_SIZE)

// precalculated values for first slave for convenience
#define HOLDING_TWI_0_SLAVE      (NUM_BASE_REGS + 1 + NUM_SPI_SLAVES * SPI_SLAVE_REG_SIZE)
#define HOLDING_TWI_0_DELAY      (NUM_BASE_REGS + 2 + NUM_SPI_SLAVES * SPI_SLAVE_REG_SIZE)
#define HOLDING_TWI_0_DATA_COUNT (NUM_BASE_REGS + 3 + NUM_SPI_SLAVES * SPI_SLAVE_REG_SIZE)
#define HOLDING_TWI_0_DATA       (NUM_BASE_REGS + 4 + NUM_SPI_SLAVES * SPI_SLAVE_REG_SIZE)

// SPI struct offsets
#define SPI_DELAY_FIELD     0
#define SPI_COUNT_FIELD     1
#define SPI_DATA_FIELD      2

// TWI structs offsets
#define TWI_ADDR_FIELD      0
#define TWI_DELAY_FIELD     1
#define TWI_COUNT_FIELD     2
#define TWI_DATA_FIELD      3

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//        INPUT REGS
//Example for 3 SPI and 3 I2C sensors
//ADDR | MEANING  | PIN
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 0   | ADC0     | PC0
// 1   | ADC1     | PC1
// 2   | ADC2     | PC2
// 3   | ADC3     | PC3
//     |          |
// 4   | SPI0     | PB2
// 5   | SPI1     | PB1
// 6   | SPI2     | PB0
//     |          |
// 7   | TWI0     | n/a
// 8   | TWI1     | n/a
// 9   | TWI2     | n/a
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define NUM_INPUT_REGS      (NUM_ADC_REG + NUM_SPI_SLAVES + NUM_TWI_SLAVES)

#define DEFAULT_TWI_SPEED   52
#define TWI_BUFFER_SIZE      4
#define SPI_BUFFER_SIZE      4

#define NUM_GPIO    5
#define NUM_COILS   (NUM_GPIO + NUM_SPI_SLAVES + NUM_TWI_SLAVES)    

#define EEPROM_ADDR     0b1010000

#define EEPROM_MB_ADDR       0
#define EEPROM_MB_MODE       2
#define EEPROM_BAUD          4
#define EEPROM_PARITY        6
#define EEPROM_COILS         8
#define EEPROM_INPUTS       10
#define EEPROM_ADC          12

#define EEPROM_SPI          14

#define EEPROM_SPI_DELAY_0   16

#define EEPROM_TWI          (16 + 16 * 2)

// made for 16 max SPI slaves
#define EEPROM_TWI_SLAVE0   (18 + 16 * 2)
#define EEPROM_TWI_DELAY_0  (20 + 16 * 2)

// end is at 20 + 32 + 64

extern uint16_t holding_regs[NUM_HOLDING_REGS];
extern uint16_t rx_count, error_count, ex_count;
extern volatile uint8_t STATUS_REG;
extern uint8_t coil_input_pins[NUM_GPIO];
extern uint8_t spi_pins[NUM_SPI_SLAVES];
extern volatile uint8_t reset_flag;
extern volatile uint8_t ms_trigger;
extern  volatile uint8_t frame[ASCII_MAX];
extern volatile uint16_t frame_len;
extern const uint16_t UBRR_TABLE[];
extern const uint16_t OCR1A_TABLE[];
extern uint8_t keep_crc;
extern uint8_t MODBUS_ADDR;
extern uint8_t twi_buffer[NUM_TWI_SLAVES][TWI_BUFFER_SIZE];
extern uint8_t spi_buffer[NUM_SPI_SLAVES][SPI_BUFFER_SIZE];
extern uint8_t coils_value[(NUM_COILS + 7) >> 3];
extern uint8_t spi_timestamps[NUM_SPI_SLAVES];
extern uint8_t twi_timestamps[NUM_TWI_SLAVES];
extern uint8_t time;
extern uint8_t MODE;

#endif