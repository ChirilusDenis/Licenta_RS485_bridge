#include <avr/common.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "USART.h"
#include "MODBUS.h"
#include "timers.h"
#include "ADC.h"
#include "SPI.h"
#include "holding_helpers.h"
#include "TWI.h"
#include "avr/wdt.h"
#include "ASCII.h"

// here lie our used constants and shared variables
#include "constants.h"

volatile uint8_t frame_ready = 0;
volatile uint8_t reset_flag = 0;
volatile uint8_t ms_trigger = 0;

volatile uint16_t frame_len = 0;
volatile uint8_t frame[ASCII_MAX];
volatile uint8_t ignore_frame = 0;
volatile uint8_t ascii_started = 0;

uint8_t MODBUS_ADDR;
uint8_t MODE;

uint16_t holding_regs[NUM_HOLDING_REGS];
uint16_t rx_count, error_count, ex_count;
volatile uint8_t STATUS_REG = 0b00000000;
uint8_t keep_crc;
uint8_t twi_buffer[NUM_TWI_SLAVES][TWI_BUFFER_SIZE];
uint8_t spi_buffer[NUM_SPI_SLAVES][SPI_BUFFER_SIZE];
uint8_t coils_value[(NUM_COILS + 7) >> 3];
uint8_t spi_timestamps[NUM_SPI_SLAVES];
uint8_t twi_timestamps[NUM_TWI_SLAVES];
uint8_t adc_en = 0;

uint8_t time;

uint8_t coil_input_pins[NUM_GPIO] = {
    PD3,
    PD4,
    PD5,
    PD6,
    PD7
};
uint8_t spi_pins[NUM_DEFAULT_SS] = {
    PB2,
    PB1,
    PB0
};

// precalculated UBRR values for the indicated baudrates
const uint16_t UBRR_TABLE[NUM_BAUD_RATES] = {
    832, // 1200
    416, // 2400
    207, // 4800
    103, // 9600
    68, // 14400
    51, // 19200
    34, // 28800
    25, // 38400
    16, // 57600
    12, // 76800
    8 // 115200
};

// precalculated timeout values for the indicated baudrates
// equal to 3.5 chars
const uint16_t OCR1A_TABLE[NUM_BAUD_RATES] = {
    2000, // 1200
    1000, // 2400
    500, // 4800
    250, // 9600
    169, // 14400
    125, // 19200
    110, // 28800
    110, // 38400
    110, // 57600
    110, // 76800
    110 // 115200
};

ISR(TIMER1_COMPA_vect) {
    TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10)); // timer1_stop;

    frame_ready = 1;
}

ISR(USART_RX_vect) {
    uint8_t byte = UDR0;

    if (frame_ready) return;

    if (MODE == MODE_RTU) {
        if (frame_len == FRAME_MAX) {
            STATUS_REG |= EX_FRAME_OVERFLOW;
            ignore_frame = 1;
        } else frame[frame_len++] = byte;

        TCNT1 = 0; // timer1_reset;
        TCCR1B |= (1 << CS12); // timer1_start with ps 256

    } else if (MODE == MODE_ASCII) {
        if (byte == ':') ascii_started = 1;

        else if (ascii_started) {
            if (byte == '\r') frame_ready = 1;

            if (frame_len == ASCII_MAX) {
                STATUS_REG |= EX_FRAME_OVERFLOW;
                ignore_frame = 1;
            } else frame[frame_len++] = byte;
        }
    } 
}

ISR(TIMER0_COMPA_vect) {
    time++;
    ms_trigger = 1;
}

void (*sw_reset)(void) = 0;

int main() {
    TWI_init(DEFAULT_TWI_SPEED);
    // init_eeprom();
    init_holding();

    USART0_init();
    if (MODE == MODE_RTU) timer1_init();
    timer0_init();
    SPI_init();
    ADC_init();

    set_GPIO_pins();

    DDRD |= (1 << PD2); // set RS485 pin as output
    PORTD &= ~(1 << PD2); // set RS485 receive mode
    sei(); // interrupt enable
    // USART0_flush();

    uint16_t len = 0;
    while (1) {

        if (frame_ready) {

            if (ignore_frame) {
                ignore_frame = 0;
                frame_len = 0;
                continue;
            }

            rx_count++;
            if (MODE == MODE_RTU) len = frame_len - 2; // discard crc
            else if (MODE == MODE_ASCII) 
                len = ascii_decode(frame_len) - 1; // discard lrc

            // check crc and if it is for me
            switch (mb_check_frame(len)) {

                case 1: // frame for me
                    len = mb_process_frame(len);
                    mb_send_frame(len);
                    break;
                
                case 2: // frame was broadcast
                    mb_process_frame(len);
                    break;

                default:
                    break;
            }

            frame_len = 0;
            frame_ready = 0;
            ascii_started = 0;
        }

        if (ms_trigger) {
            ms_trigger = 0;
            update_sensor_coils();
        }

        if (reset_flag) {
            // sw_reset();
            cli();
            wdt_enable(WDTO_15MS);
            while (1);
        }
    }

    cli();

    return 0;
}