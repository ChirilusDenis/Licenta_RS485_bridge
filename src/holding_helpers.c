#include "holding_helpers.h"
#include "avr/io.h"
#include "ADC.h"
#include "SPI.h"
#include "constants.h"
#include "TWI.h"
#include "timers.h"

void test_holding() {
    MODBUS_ADDR = DEFAULT_MODBUS_ADDR;
    holding_regs[HOLDING_MB_ADDRESS] = MODBUS_ADDR;
    MODE = MODE_RTU;
    holding_regs[HOLDING_MB_MODE] = MODE;
    holding_regs[HOLDING_RS485_BAUD] = DEFAULT_BAUD_IDX;
    holding_regs[HOLDING_RS485_PARITY] = PARITY_EVEN;

    uint16_t coils = 0b11000;
    holding_regs[HOLDING_COILS_ADDR] = coils;
    holding_regs[HOLDING_INPUT_ADDR] = 0b111;
}

void init_eeprom() {
    uint16_t coils = 0b11000 ;

    TWI_start();
    TWI_write_sla_w(EEPROM_ADDR);

    if (STATUS_REG & EX_TWI_START_FAIL) {
        TWI_stop();
        return;
    }

    TWI_write(EEPROM_MB_ADDR >> 8);
    TWI_write(EEPROM_MB_ADDR & 0xFF);

    // use sequential write of eeprom

    TWI_write(DEFAULT_MODBUS_ADDR >> 8); // modbus addr
    TWI_write(DEFAULT_MODBUS_ADDR & 0xFF);

    TWI_write(MODE_RTU >> 8); // modbus addr
    TWI_write(MODE_RTU & 0xFF);

    TWI_write(DEFAULT_BAUD_IDX >> 8); // baud rate idx
    TWI_write(DEFAULT_BAUD_IDX & 0xFF);

    TWI_write(PARITY_EVEN >> 8); // rs485 parity
    TWI_write(PARITY_EVEN & 0xFF);

    TWI_write(coils >> 8); // coils
    TWI_write(coils & 0xFF);

    TWI_write(0b00000111 >> 8); // inputs
    TWI_write(0b00000111 & 0xFF);

    TWI_write(1 >> 8); // adc inputs
    TWI_write(1 & 0xFF);

    TWI_write(1 >> 8); // spi valid
    TWI_write(1 & 0xFF);

    TWI_write(5 >> 8); // spi delay 0
    TWI_write(5 & 0xFF);

    // if passing address 64, write address again to pass paging problems

    TWI_stop();
    temp_delay_us(5000);
    // jump to twi address space
    TWI_start();
    TWI_write_sla_w(EEPROM_ADDR);

    TWI_write(EEPROM_TWI >> 8);
    TWI_write(EEPROM_TWI & 0xFF);

    TWI_write(1 >> 8); // twi valid slaves
    TWI_write(1 & 0xFF);

    TWI_write(0b1110111 >> 8); // bmp180 address
    TWI_write(0b1110111 & 0xFF);

    TWI_write(5 >> 8); // i2c delay 0
    TWI_write(5 & 0xFF);

    TWI_stop();
    temp_delay_us(5000);
}

void init_holding() {

    TWI_start();
    TWI_write_sla_w(EEPROM_ADDR);

    if (STATUS_REG & EX_TWI_START_FAIL) {
        TWI_stop();
        test_holding();
        return;
    }

    TWI_write(EEPROM_MB_ADDR >> 8);
    TWI_write(EEPROM_MB_ADDR & 0xFF);

    // use sequential read of eeprom
    TWI_start();
    TWI_write_sla_r(EEPROM_ADDR);
    
    MODBUS_ADDR = (TWI_read_ack() << 8) | TWI_read_ack();

    // memory is empty, got 0xFF both reads
    if (MODBUS_ADDR > 247) {
        test_holding();
        return;
    }

    holding_regs[HOLDING_MB_ADDRESS] = MODBUS_ADDR;
    MODE = (TWI_read_ack() << 8) | TWI_read_ack();
    holding_regs[HOLDING_MB_MODE]     = MODE;
    holding_regs[HOLDING_RS485_BAUD] = (TWI_read_ack() << 8) | TWI_read_ack();
    holding_regs[HOLDING_RS485_PARITY] = (TWI_read_ack() << 8) | TWI_read_ack();

    holding_regs[HOLDING_COILS_ADDR] = (TWI_read_ack() << 8) | TWI_read_ack();
    holding_regs[HOLDING_INPUT_ADDR] = (TWI_read_ack() << 8) | TWI_read_ack();
    
    holding_regs[HOLDING_ADC_IN_ADDR] = (TWI_read_ack() << 8) | TWI_read_ack();
    
    holding_regs[HOLDING_SPI_SS_ADDR] = (TWI_read_ack() << 8) | TWI_read_ack();

    for (uint8_t i = 0; i < NUM_SPI_SLAVES; i++)
        if (i == (NUM_SPI_SLAVES - 1))
            holding_regs[HOLDING_SPI_0_DELAY + i * SPI_SLAVE_REG_SIZE] = (TWI_read_ack() << 8) | TWI_read_nack();
        else 
            holding_regs[HOLDING_SPI_0_DELAY + i * SPI_SLAVE_REG_SIZE] = (TWI_read_ack() << 8) | TWI_read_ack();
    
    TWI_start();
    TWI_write_sla_w(EEPROM_ADDR);

    TWI_write(EEPROM_TWI >> 8);
    TWI_write(EEPROM_TWI & 0xFF);

    TWI_start();
    TWI_write_sla_r(EEPROM_ADDR);

    holding_regs[HOLDING_VALID_TWI_ADDR] = (TWI_read_ack() << 8) | TWI_read_ack();

    for (uint8_t i = 0; i < NUM_TWI_SLAVES; i++) {
        holding_regs[HOLDING_TWI_0_SLAVE + i * TWI_SLAVE_REG_SIZE] = (TWI_read_ack() << 8) | TWI_read_ack();
        if (i == (NUM_TWI_SLAVES - 1))
            holding_regs[HOLDING_TWI_0_DELAY + i * TWI_SLAVE_REG_SIZE] = (TWI_read_ack() << 8) | TWI_read_nack();
        else 
            holding_regs[HOLDING_TWI_0_DELAY + i * TWI_SLAVE_REG_SIZE] = (TWI_read_ack() << 8) | TWI_read_ack();
    }

    TWI_stop();
}

void set_GPIO_pins() {
    uint8_t coil_pins = holding_regs[HOLDING_COILS_ADDR];
    uint8_t input_pins = holding_regs[HOLDING_INPUT_ADDR];

    for (uint8_t i = 0; i < NUM_GPIO; i++) {
        uint8_t pin = coil_input_pins[i];

        if ((coil_pins  >> i) & 1) {
            DDRD  |=  (1 << pin); // set as output
            PORTD &= ~(1 << pin); // initialise low
        } else if ((input_pins >> i) & 1) {
            DDRD  &= ~(1 << pin); // set as input
            PORTD |= (1 << pin); // en pull-up
        }
    }
}

void set_output() {
    uint8_t coil_pins = holding_regs[HOLDING_COILS_ADDR];

    for (uint8_t i = 0; i < NUM_GPIO; i++) {
        uint8_t pin = coil_input_pins[i];

        if ((coil_pins  >> i) & 1) {
            holding_regs[HOLDING_INPUT_ADDR] &= ~(1 << i); // remove from input pins
            DDRD  |=  (1 << pin); // set as output
            PORTD &= ~(1 << pin); // initialise low
        }
    }
}

void set_input() {
    uint8_t input_pins = holding_regs[HOLDING_INPUT_ADDR];

    for (uint8_t i = 0; i < NUM_GPIO; i++) {
        uint8_t pin = coil_input_pins[i];
        
        if ((input_pins >> i) & 1) {
            holding_regs[HOLDING_COILS_ADDR] &= ~(1 << i); // remove from output pins
            coils_value[i >> 3] &= ~(1 << i);       // clear coil value
            DDRD  &= ~(1 << pin); // set as input
            PORTD |=  (1 << pin); // en pull-up
        }
    }
}

void update_sensor_coils() {
    uint8_t coil_addr = 0;
    uint8_t timeout = 0;

    // update SPI sensors
    for(uint8_t i = 0; i < NUM_SPI_SLAVES; i++) {

       coil_addr = NUM_GPIO + i;
        if(coils_value[coil_addr >> 3] & (1 << (coil_addr % 8))) {

            timeout = holding_regs[HOLDING_SPI_0_DELAY + i * SPI_SLAVE_REG_SIZE];
            if (time - spi_timestamps[i] >= timeout) {
                coils_value[coil_addr >> 3] &= ~(1 << (coil_addr % 8));
            }
        }
    }

    // update I2C sensors
    for(uint8_t i = 0; i < NUM_TWI_SLAVES; i++) {

        coil_addr = NUM_GPIO + NUM_SPI_SLAVES + i;
        if(coils_value[coil_addr >> 3] & (1 << (coil_addr % 8))) {

            timeout = holding_regs[HOLDING_TWI_0_DELAY + i * TWI_SLAVE_REG_SIZE];
            if (time - twi_timestamps[i] >= timeout) {
                coils_value[coil_addr >> 3] &= ~(1 << (coil_addr % 8));
            }
        }
    }

}

uint8_t write_holding_reg(uint16_t addr, uint16_t value) {

    if (addr == HOLDING_MB_ADDRESS) {
        if (value > MODBUS_MAX || value == 0) return MODBUS_ILLEGAL_DATA_VALUE;
        eeprom_write_reg(EEPROM_MB_ADDR, value);

    } else if (addr == HOLDING_MB_MODE) {
        if (value != MODE_RTU || value != MODE_ASCII) return MODBUS_ILLEGAL_DATA_VALUE;

        holding_regs[HOLDING_MB_MODE] = value;
        eeprom_write_reg(EEPROM_MB_MODE, value);

    } else if (addr == HOLDING_RS485_BAUD) {
        if (value >= NUM_BAUD_RATES) return MODBUS_ILLEGAL_DATA_VALUE;

        holding_regs[HOLDING_RS485_BAUD] = value;
        eeprom_write_reg(EEPROM_BAUD, value);
    
    } else if (addr == HOLDING_RS485_PARITY){
        if (value != PARITY_ODD && 
            value != PARITY_EVEN &&
            value != PARITY_NO)  return MODBUS_ILLEGAL_DATA_VALUE;

        holding_regs[HOLDING_RS485_PARITY] = value;
        eeprom_write_reg(EEPROM_PARITY, value);
        
    } else if (addr == HOLDING_COILS_ADDR) {
        // max bits for coil/output pins
        if (value >= (1 << NUM_COILS)) return MODBUS_ILLEGAL_DATA_VALUE;

        holding_regs[HOLDING_COILS_ADDR] = value;
        eeprom_write_reg(EEPROM_COILS, value);
        set_output();

    } else if(addr == HOLDING_INPUT_ADDR) {
        // max bits for input pins
        if (value >= (1 << NUM_GPIO)) return MODBUS_ILLEGAL_DATA_VALUE;

        holding_regs[HOLDING_INPUT_ADDR] = value;
        eeprom_write_reg(EEPROM_INPUTS, value);
        set_input();

    } else if (addr == HOLDING_ADC_IN_ADDR) {
        // max bits for ADC valid inputs
        if (value >= (1 << NUM_ADC_REG)) return MODBUS_ILLEGAL_DATA_VALUE;

        holding_regs[HOLDING_ADC_IN_ADDR] = value;
        eeprom_write_reg(EEPROM_ADC, value);

    } else if (addr == HOLDING_SPI_SS_ADDR) {
        // max bits for SPI slaves
        if (value >= (1 << NUM_SPI_SLAVES)) return MODBUS_ILLEGAL_DATA_VALUE;

        uint16_t diff = ~holding_regs[HOLDING_SPI_SS_ADDR] & value;
        uint8_t spi_offset = 0;
        for (uint8_t i = 0; diff != 0; diff >>= 1, i++)
            if (diff & 1) {
                spi_offset = i * SPI_SLAVE_REG_SIZE;
                // invalidate old buffer
                holding_regs[HOLDING_SPI_0_DATA_COUNT + spi_offset] = 0;
            }
        
        holding_regs[HOLDING_SPI_SS_ADDR] = value;
        eeprom_write_reg(EEPROM_SPI, value);
    
    } else if (addr < HOLDING_VALID_TWI_ADDR) {
        uint8_t field = (addr - HOLDING_SPI_0_DELAY) % SPI_SLAVE_REG_SIZE;
        uint8_t offset = addr - HOLDING_SPI_0_DELAY - field; // address offset between first slave and current slave
        uint8_t spi_index = offset / SPI_SLAVE_REG_SIZE;

        switch (field) {

            case SPI_DELAY_FIELD: {
                if (value > 127) return MODBUS_ILLEGAL_DATA_VALUE;
                if (!(holding_regs[HOLDING_SPI_SS_ADDR] & (1 << spi_index))) // check if slave is enabled
                    return MODBUS_ILLEGAL_DATA_ADDR;

                holding_regs[addr] = value;
                uint8_t eeprom_offset = spi_index << 1;
                eeprom_write_reg(EEPROM_SPI_DELAY_0 + eeprom_offset, value);
                break;
            } 

            case SPI_COUNT_FIELD:
                return MODBUS_ILLEGAL_DATA_ADDR;
                break;

            case SPI_DATA_FIELD: 
                if (value > 0xFF) {
                    holding_regs[HOLDING_SPI_0_DATA_COUNT + offset] = 0;
                    return MODBUS_ILLEGAL_DATA_VALUE;
                }
                if (holding_regs[HOLDING_SPI_0_DATA_COUNT + offset] == SPI_BUFFER_SIZE || // check if buffer full
                    !(holding_regs[HOLDING_SPI_SS_ADDR] & (1 << spi_index))) // check if slave is enabled
                    return MODBUS_ILLEGAL_DATA_ADDR;

                holding_regs[HOLDING_SPI_0_DATA + offset] = value;
                spi_buffer[spi_index][holding_regs[HOLDING_SPI_0_DATA_COUNT + offset]++] = value & 0xFF;
                break;
    }

    } else if (addr == HOLDING_VALID_TWI_ADDR) {
        // max bits for TWI slaves
        if (value >= (1 << NUM_TWI_SLAVES)) return MODBUS_ILLEGAL_DATA_VALUE;

        holding_regs[HOLDING_VALID_TWI_ADDR] = value;
        eeprom_write_reg(EEPROM_TWI, value);

    } else if (addr < NUM_HOLDING_REGS) {
        uint8_t field = (addr - HOLDING_TWI_0_SLAVE) % TWI_SLAVE_REG_SIZE;
        uint8_t twi_offset = addr - HOLDING_TWI_0_SLAVE - field; // address offset between first slave and this slave
        uint8_t twi_index = twi_offset / TWI_SLAVE_REG_SIZE;

        switch (field) {
            case TWI_ADDR_FIELD: {
                // max value for TWI address or taken address
                if (value > 127 || value == EEPROM_ADDR) return MODBUS_ILLEGAL_DATA_VALUE;

                holding_regs[addr] = value;
                uint8_t eeprom_offset = twi_index * 4;
                eeprom_write_reg(EEPROM_TWI_SLAVE0 + eeprom_offset, value);

                // validate the newly written address
                holding_regs[HOLDING_VALID_TWI_ADDR] |= (1 << twi_index);
                eeprom_write_reg(EEPROM_TWI, holding_regs[HOLDING_VALID_TWI_ADDR]);

                // invalidate old buffer
                holding_regs[HOLDING_TWI_0_DATA_COUNT + twi_offset] = 0; // size is 2 fields under twi address
                holding_regs[HOLDING_TWI_0_DELAY + twi_offset] = 0; // invalidate old delay
                break;
            }

            case TWI_DELAY_FIELD: {
                if (value > 127) return MODBUS_ILLEGAL_DATA_VALUE;
                if (!(holding_regs[HOLDING_VALID_TWI_ADDR] & (1 << twi_index))) // check if slave is enabled
                    return MODBUS_ILLEGAL_DATA_ADDR;

                holding_regs[addr] = value;
                uint8_t eeprom_offset = twi_index * 4;
                eeprom_write_reg(EEPROM_TWI_DELAY_0 + eeprom_offset, value);
                break;
            }

            case TWI_COUNT_FIELD:
                return MODBUS_ILLEGAL_DATA_ADDR;
                break;

            case TWI_DATA_FIELD:
                if (value > 0xFF) {
                    holding_regs[HOLDING_TWI_0_DATA_COUNT + twi_offset] = 0; // set size 0
                    return 0;
                }
                if (holding_regs[HOLDING_TWI_0_DATA_COUNT + twi_offset] == TWI_BUFFER_SIZE || // check size
                            !(holding_regs[HOLDING_VALID_TWI_ADDR] & (1 << twi_index))) // check if slave is enabled
                    return MODBUS_ILLEGAL_DATA_ADDR;

                holding_regs[addr] = value;
                twi_buffer[twi_index][holding_regs[HOLDING_TWI_0_DATA_COUNT + twi_offset]++] = value & 0xFF;
                // increment size after adding byte
                break;
        }

    } else return MODBUS_ILLEGAL_DATA_ADDR;

    return 0;
}

uint16_t read_input_reg(uint16_t addr, uint8_t *error) {
    if (addr < NUM_ADC_REG) {
        if (!adc_en) {
            adc_en = 1;
            ADC_enable();
        }

        // check if current ADC input is enabled/valid
        if (!(holding_regs[HOLDING_ADC_IN_ADDR] & (1 << addr))) {
            *error = MODBUS_ILLEGAL_DATA_ADDR;
            return -1;
        }

        ADC_read(addr);

        return (ADC);

    } else if (addr < NUM_ADC_REG + NUM_SPI_SLAVES) {
        uint8_t spi_index = addr - NUM_ADC_REG;
        uint8_t spi_holding_offset = spi_index * SPI_SLAVE_REG_SIZE;

        // check if this SPI slave is enabled/valid
        if (!(holding_regs[HOLDING_SPI_SS_ADDR] & (1 << spi_index))) {
            *error = MODBUS_ILLEGAL_DATA_ADDR;
            return -1;
        }

        // check if the sensor finished converting data
        uint8_t coil_addr = NUM_GPIO + spi_index;
        if (coils_value[coil_addr >> 3] & (1 << (coil_addr % 8))) {
            *error = MODBUS_DEVICE_BUSY;
            return -1;
        }

        // read and send SPI value in 2 steps
        uint16_t val = 0;
        uint8_t send_size = holding_regs[HOLDING_SPI_0_DATA_COUNT + spi_holding_offset];

        if (spi_index < NUM_DEFAULT_SS) SPI_select(spi_pins[spi_index]);

        for (uint8_t i = 0; i < send_size; i++) {
            SPI_exchange(spi_buffer[spi_index][i]);
        }

        val |= (uint16_t)(SPI_exchange(0xFF) << 8);
        val |= SPI_exchange( 0xFF) & 0xFF;
        if (spi_index < NUM_DEFAULT_SS) SPI_deselect(spi_pins[spi_index]);

        return val;

    } else { // TWI
        uint8_t twi_index = addr - NUM_ADC_REG - NUM_SPI_SLAVES;
        uint8_t twi_holding_offset = twi_index * TWI_SLAVE_REG_SIZE; // offset compared to twi slave 0

        // check if this TWI slave is enabled/valid
        if (!(holding_regs[HOLDING_VALID_TWI_ADDR] & (1 << twi_index))) {
            *error = MODBUS_ILLEGAL_DATA_ADDR;
            return -1;
        }

        // did the sensor finish converting data?
        uint8_t coil_addr = NUM_GPIO + NUM_SPI_SLAVES + twi_index;
        if (coils_value[coil_addr >> 3] & (1 << (coil_addr % 8))) {
            *error = MODBUS_DEVICE_BUSY;
            return -1;
        }

        uint8_t twi_addr = holding_regs[HOLDING_TWI_0_SLAVE + twi_holding_offset];
        
        uint16_t data = 0;

        // if i was given an i2c address within a certain slave
        uint8_t send_size = holding_regs[HOLDING_TWI_0_DATA_COUNT + twi_holding_offset];
        if(send_size) {
            TWI_start();
            TWI_write_sla_w(twi_addr);
            if (STATUS_REG & CLEAR_ON_READ_BITS) {
                TWI_stop();
                *error = MODBUS_SERVER_FAILURE;
                return -1;
            }   

            for (uint8_t i = 0; i < send_size; i++) {
                TWI_write(twi_buffer[twi_index][i]);
            }
        }

        TWI_start();

        TWI_write_sla_r(twi_addr);
        if (STATUS_REG & CLEAR_ON_READ_BITS) {
            TWI_stop();
            *error = MODBUS_SERVER_FAILURE;
            return -1;
        }

        data |= (uint16_t)TWI_read_ack() << 8;
        data |= TWI_read_nack();

        TWI_stop();

        return data;
    }
}