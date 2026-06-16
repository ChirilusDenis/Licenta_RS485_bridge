#include <avr/io.h>
#include "MODBUS.h"
#include "timers.h"
#include "USART.h"
#include "constants.h"
#include "holding_helpers.h"
#include "TWI.h"
#include "SPI.h"
#include "ADC.h"
#include "ASCII.h"

uint16_t mb_CRC(uint16_t len) {
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < len; i++) {

        crc ^= (uint16_t)frame[i];
        for (uint8_t j = 0; j < 8; j++) {

            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

uint8_t mb_check_frame(uint16_t len) {
    if (len < 4) return 0;

    uint16_t recv, mine;

    // check CRC
    if (MODE == MODE_RTU) {
        recv = frame[len] | ((uint16_t)frame[len + 1] << 8);
        mine = mb_CRC(len);
    } else if (MODE == MODE_ASCII) {
        recv = frame[len];
        mine = mb_LRC(len);
    } else return 0;

    // bad crc
    if (recv != mine) {
        error_count++; // count communication problems
        return 0;
    }
    if (frame[0] == MODBUS_ADDR) return 1; // for me
    if (frame[0] == 0) return 2; // broadcast
    return 0;
}

void mb_send_frame(uint16_t len) {
    if (len >= 255) return; // too many bytes to send

    PORTD |= (1 << PD2); // set rs485 tx mode 
    temp_delay_us(200); // delay so tx mode settles

    UCSR0A |= (1 << TXC0); // clear TXC flag

    // add frame crc
    if (MODE == MODE_RTU){
        if (!keep_crc) {
            uint16_t crc = mb_CRC(len);
            frame[len] = crc & 0xFF;
            frame[len + 1] = crc >> 8;
        }
        keep_crc = 0;

        for (uint16_t i = 0; i < len + 2; i++) {
            USART0_transmit(frame[i]);
        }

    // add frame lrc
    } else if (MODE == MODE_ASCII) {
        uint8_t hi, lo;

        if (!keep_crc) {
            uint8_t lrc = mb_LRC(len);
            frame[len] = lrc;
        }
        keep_crc = 0;
        USART0_transmit(':');
        for (uint16_t i = 0; i < len + 1; i++) {
            hi = (frame[i] >> 4);
            lo = (frame[i] & 0x0F);
            hi = (hi < 10) ? (hi + '0') : (hi - 10 + 'A');
            lo = (lo < 10) ? (lo + '0') : (lo - 10 + 'A');
            USART0_transmit(hi);
            USART0_transmit(lo);
        }
        USART0_transmit('\r');
        USART0_transmit('\n');
    } 

    // wait for last byte of frame to fully transmit
    while (!(UCSR0A & (1 << TXC0)));

    PORTD &= ~(1 << PD2); // set rs485 rx mode
    temp_delay_us(200); // delay so rx settles
}

uint16_t mb_make_exception_frame(uint16_t code) {
    ex_count++; // count modbus exceptions

    frame[1] = frame[1] | (1 << 7); // set function MSB to 1 for exception
    frame[2] = code;

    return 3;
}

uint16_t mb_read_coils(uint16_t len) {
    uint16_t start_addr = 0;
    uint16_t n_coils = 0;
    uint8_t n_data_bytes = 0;

    start_addr = ((uint16_t)(frame[2] << 8)) | frame[3];
    n_coils = ((uint16_t)(frame[4] << 8)) | frame[5];

    // check if addr in modbus limit
    if (n_coils > 0x07D0 || n_coils == 0)
        return mb_make_exception_frame(MODBUS_ILLEGAL_DATA_VALUE);
    
    // check if address and if adress + num coils are in scope
    if (start_addr > NUM_COILS || start_addr + n_coils > NUM_COILS)
        return mb_make_exception_frame(MODBUS_ILLEGAL_DATA_ADDR);

    uint16_t end = start_addr + n_coils < NUM_GPIO? start_addr + n_coils : NUM_GPIO;

    // check if it requests to read a pin set as input
    for (uint16_t i = start_addr; i < end; i++) {
        if (!(holding_regs[HOLDING_COILS_ADDR] & (1 << i)))
            return mb_make_exception_frame(MODBUS_ILLEGAL_DATA_ADDR);
    }

    n_data_bytes = (n_coils >> 3) + (n_coils % 8 != 0);

    frame[2] = n_data_bytes;

    // erase spaces for future data
    for (uint16_t i = 0; i < n_data_bytes; i++) frame[3 + i] = 0;

    uint8_t coil_val = 0;
    for (uint16_t i = 0; i < n_coils; i++) {
        coil_val = (coils_value[(start_addr + i) >> 3] >> ((start_addr + i) % 8)) & 1;
        frame[3 + (i >> 3)] |= coil_val << (i % 8);
    }

    return 3 + n_data_bytes;
}

uint16_t mb_read_discrete_inputs(uint16_t len) {
    uint16_t start_addr = 0;
    uint16_t n_inputs = 0;
    uint8_t n_data_bytes = 0;

    start_addr = ((uint16_t)(frame[2] << 8)) | frame[3];
    n_inputs = ((uint16_t)(frame[4] << 8)) | frame[5];

    if (n_inputs > 0x07D0 || n_inputs == 0)
        return mb_make_exception_frame(MODBUS_ILLEGAL_DATA_VALUE);
    
    // check if address and adress + num coils are in scope
    if (start_addr > NUM_GPIO ||  start_addr + n_inputs > NUM_GPIO)
        return mb_make_exception_frame(MODBUS_ILLEGAL_DATA_ADDR);

    // check if it requests to read a pin set as output
    for (uint16_t i = start_addr; i < start_addr + n_inputs; i++) {
        if (!(holding_regs[HOLDING_INPUT_ADDR] & (1 << i)))
            return mb_make_exception_frame(MODBUS_ILLEGAL_DATA_ADDR);
    }

    n_data_bytes = (n_inputs >> 3) + (n_inputs % 8 != 0);

    frame[2] = n_data_bytes;
 
    // erase spaces for futurea data
    for (uint16_t i = 0; i < n_data_bytes; i++) frame[3 + i] = 0;

    uint8_t input_val = 0;
    for (uint16_t i = 0; i < n_inputs; i++) {
        // PORTD |= (1 << coil_input_pins[start_addr + i]); // en pull up
        input_val = (PIND >> coil_input_pins[start_addr + i]) & 1;
        frame[3 + (i >> 3)] |= (input_val << (i % 8));
        // PORTD &= ~(1 << coil_input_pins[start_addr + i]); // disable pull up
    }

    return 3 + n_data_bytes;
}

uint16_t mb_write_single_coil(uint16_t len) {
    uint16_t addr = 0;

    addr = ((uint16_t)(frame[2] << 8)) | frame[3];

    // check if received "0" or "1" signal for coil
    if ((frame[4] != 0 && frame[4] != 0xFF) || frame[5] != 0) 
        return mb_make_exception_frame(MODBUS_ILLEGAL_DATA_VALUE);
    
    // check if addr is in bound and is output, else throw 2
    if (addr >= NUM_COILS)
            return mb_make_exception_frame(MODBUS_ILLEGAL_DATA_ADDR);

    uint8_t coil_write = frame[4];
    uint8_t coil_is_set = coils_value[addr >> 3] & (1 << (addr % 8));

    if (addr < NUM_GPIO) {
        if (!(holding_regs[HOLDING_COILS_ADDR] & (1 << addr))) return mb_make_exception_frame(MODBUS_ILLEGAL_DATA_ADDR);

        if (coil_write) {
            PORTD |= (1 << coil_input_pins[addr]);
            coils_value[addr >> 3] |= (1 << (addr % 8)); // update coil value
        } else {
            PORTD &= ~(1 << coil_input_pins[addr]);
            coils_value[addr >> 3] &= ~(1 << (addr % 8)); // update coil value
        }


    } else if (addr < NUM_GPIO + NUM_SPI_SLAVES) {
        // can't write 0 to trigger coil
        if (!coil_write) return mb_make_exception_frame(MODBUS_ILLEGAL_DATA_VALUE);
        if (coil_is_set) return mb_make_exception_frame(MODBUS_DEVICE_BUSY);

        uint8_t spi_index = addr - NUM_GPIO;
        if (!(holding_regs[HOLDING_SPI_SS_ADDR] & (1 << spi_index))) return mb_make_exception_frame(MODBUS_ILLEGAL_DATA_ADDR);

        uint8_t spi_offset = spi_index * SPI_SLAVE_REG_SIZE;
        uint8_t send_size =  holding_regs[HOLDING_SPI_0_DATA_COUNT + spi_offset];

        if (send_size) {
            coils_value[addr >> 3] |= (1 << (addr % 8)); // update coil value
            spi_timestamps[spi_index] = time;

            SPI_select(spi_pins[spi_index]);

            for (uint8_t i = 0; i < send_size; i++) {
                SPI_exchange(spi_buffer[spi_index][i]);
            }
            SPI_deselect(spi_pins[spi_index]);
            // clear send buffer
            holding_regs[HOLDING_SPI_0_DATA_COUNT + spi_offset] = 0;
        } else return mb_make_exception_frame(MODBUS_ILLEGAL_DATA_VALUE);


    } else if (addr < NUM_GPIO + NUM_SPI_SLAVES + NUM_TWI_SLAVES){
        // can't write 0 to trigger coil
        if (!coil_write) return mb_make_exception_frame(MODBUS_ILLEGAL_DATA_VALUE);
        if (coil_is_set) return mb_make_exception_frame(MODBUS_DEVICE_BUSY);

        uint8_t twi_index = addr - NUM_GPIO - NUM_SPI_SLAVES;
        if (!(holding_regs[HOLDING_VALID_TWI_ADDR] & (1 << twi_index))) return mb_make_exception_frame(MODBUS_ILLEGAL_DATA_ADDR);

        uint8_t twi_holding_offset = twi_index * TWI_SLAVE_REG_SIZE; // offset for twi slave compared to slave 0
        uint8_t send_size = holding_regs[HOLDING_TWI_0_DATA_COUNT + twi_holding_offset];

        if (send_size) {
            holding_regs[HOLDING_TWI_0_DATA_COUNT + twi_holding_offset] = 0;
            coils_value[addr >> 3] |= (1 << (addr % 8)); // update coil value
            twi_timestamps[twi_index] = time;

            TWI_start();
            TWI_write_sla_w(holding_regs[HOLDING_TWI_0_SLAVE + twi_holding_offset]);
            if (STATUS_REG & CLEAR_ON_READ_BITS) {
                TWI_stop();
                return mb_make_exception_frame(MODBUS_SERVER_FAILURE);
            }

            for(uint8_t i = 0; i < send_size; i++)
                TWI_write(twi_buffer[twi_index][i]);
            TWI_stop();
            // clear buffer that was sent
            // holding_regs[HOLDING_TWI_0_DATA_COUNT + twi_holding_offset] = 0;
        } else return mb_make_exception_frame(MODBUS_ILLEGAL_DATA_VALUE);
    
    
    } else {
        return mb_make_exception_frame(MODBUS_ILLEGAL_DATA_ADDR);
    }

    keep_crc = 1;
    return len;

}

uint16_t mb_read_holding_reg(uint16_t len) {
    uint16_t start_addr = 0;
    uint16_t n_regs = 0;
    uint8_t n_data_bytes = 0;

    start_addr = ((uint16_t)(frame[2] << 8)) | frame[3];
    n_regs = ((uint16_t)(frame[4] << 8)) | frame[5];

    // check if address in modbus limit
    if (n_regs > 0x007D)
        return mb_make_exception_frame(MODBUS_ILLEGAL_DATA_VALUE);

    // check if address in my limit
    if (start_addr > NUM_HOLDING_REGS || start_addr + n_regs > NUM_HOLDING_REGS)
        return mb_make_exception_frame(MODBUS_ILLEGAL_DATA_ADDR);

    n_data_bytes = n_regs << 1;
    frame[2] = n_data_bytes;

    uint16_t value = 0;
    for(uint8_t i = 0; i < n_regs; i++) {
        value = holding_regs[start_addr + i];
        frame[(i << 1) + 3] = value >> 8;
        frame[(i << 1) + 4] = value & 0xFF;
    }

    return 3 + n_data_bytes;
}

uint16_t mb_read_input_regs(uint16_t len) {
    uint16_t start_addr = 0;
    uint16_t n_regs = 0;
    uint8_t n_data_bytes = 0;

    start_addr = ((uint16_t)(frame[2] << 8)) | frame[3];
    n_regs = ((uint16_t)(frame[4] << 8)) | frame[5];

    // check if addr in modbus limit
    if (n_regs > 0x007D || n_regs == 0)
        return mb_make_exception_frame(MODBUS_ILLEGAL_DATA_VALUE);

    // check if addr and addr + num regs are ok
    if (start_addr > NUM_INPUT_REGS || start_addr + n_regs > NUM_INPUT_REGS)
        return mb_make_exception_frame(MODBUS_ILLEGAL_DATA_ADDR);

    n_data_bytes = n_regs << 1;
    frame[2] = n_data_bytes;

    uint16_t value = 0;
    uint8_t error = 0;
    for(uint16_t i = 0; i < n_regs; i++) {
        value = read_input_reg(start_addr + i, &error);
        
        if (error) // problem
            return mb_make_exception_frame(error);

        frame[(i << 1) + 3] = value >> 8;
        frame[(i << 1) + 4] = value & 0xFF;
    }

    if (adc_en) {
            adc_en = 0;
            ADC_stop();
        }

    return 3 + n_data_bytes;
}

uint16_t mb_write_single_reg(uint16_t len) {
    uint16_t reg_addr = 0;
    uint16_t reg_value = 0;

    reg_addr = ((uint16_t)(frame[2] << 8)) | frame[3];
    reg_value = ((uint16_t)(frame[4] << 8)) | frame[5];

    // check if value is in modbus limit
    if (reg_value > 0xFFFF)
        return mb_make_exception_frame(MODBUS_ILLEGAL_DATA_VALUE);

    // check if addr is in my limit
    if (reg_addr >= NUM_HOLDING_REGS)
        return mb_make_exception_frame(MODBUS_ILLEGAL_DATA_ADDR);

    uint8_t exit_code = write_holding_reg(reg_addr, reg_value);
    if (exit_code) return mb_make_exception_frame(exit_code);

    keep_crc = 1;
    return len;
}

uint16_t mb_read_exception_status(uint16_t len) {
    frame[2] = STATUS_REG;

    STATUS_REG &= ~(CLEAR_ON_READ_BITS);

    return 3;
}

uint16_t mb_diagnostics(uint16_t len) {
    uint16_t sub_function = ((uint16_t)(frame[2] << 8)) | frame[3];

    switch (sub_function) {
        case RETURN_QUERY_DATA: // repeat message, ping analogue
            keep_crc = 1;
            return len;

        case RESTART_COMM: // repeat and restart
            reset_flag = 1;
            keep_crc = 1;
            return len;

        case CLEAR_CNT_DIAG: // clear counters and status
            rx_count = 0;
            ex_count = 0;
            error_count = 0;
            STATUS_REG = 0;
            keep_crc = 1;
            return len;

        case RETURN_MSG_COUNT: // return message count
            frame[4] = rx_count >> 8;
            frame[5] = rx_count & 0xFF;
            return len;

        case RETURN_COMM_ERROR_COUNT: // return crc/lrc error count
            frame[4] = error_count >> 8;
            frame[5] = error_count & 0xFF;
            return len;

        case RETURN_EX_COUNT: // return exception count
            frame[4] = ex_count >> 8;
            frame[5] = ex_count & 0xFF;
            return len;
            
        default:
            return mb_make_exception_frame(MODBUS_ILLEGAL_FUNCTION);
    
    }
}

uint16_t mb_process_frame(uint16_t len) {
    if (len < 4) return 0;

    switch (frame[1]) {

        case 1: // read internal bits/outputs
            return mb_read_coils(len);

        case 2: // read exteral bits/GPIO
            return mb_read_discrete_inputs(len);

        case 3: // read internal 16bit regs
            return mb_read_holding_reg(len);

        case 4: // read external regs/sensors
            return mb_read_input_regs(len);  

        case 5: // write internal bit
            return mb_write_single_coil(len);

        case 6: // write single internal/holding reg
            return mb_write_single_reg(len);

        case 7: // read status register
            return mb_read_exception_status(len);

        case 8: // debugging
            return mb_diagnostics(len);

        default:
            return mb_make_exception_frame(MODBUS_ILLEGAL_FUNCTION);
            break;
    
    }
    return 0;
}