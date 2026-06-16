#include "TWI.h"
#include "util/twi.h"
#include "constants.h"


void TWI_init(uint8_t speed) {
    TWSR = (0 << TWPS0); // set prescaler to 1
    TWBR = speed;
}

void TWI_start() {
    // clear twint, send start and enable comm
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);

	while (!(TWCR & (1 << TWINT))) // wait for START condition to be sent
        ;

    if (TWI_STATUS != TW_START && TWI_STATUS != TW_REP_START)
        STATUS_REG |= EX_TWI_START_FAIL;
}

void TWI_stop() {
    // clear twint, enable comm, and sent stop
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
}

uint8_t TWI_read_ack() {
    // clear twint, enable comm and enable ack
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);

    while (!(TWCR & (1 << TWINT))) // wait for exchange to end
        ;

    if (TWI_STATUS != TW_MR_DATA_ACK)
        STATUS_REG |= EX_TWI_R_FAIL;

    return TWDR;
}

uint8_t TWI_read_nack() {
    
    TWCR &= ~(1 << TWEA); // disale ack
    TWCR = (1 << TWINT) | (1 << TWEN); // clear twint and enale comm
    
    while (!(TWCR & (1 << TWINT))) // wait for exchange to end
        ;

    if (TWI_STATUS != TW_MR_DATA_NACK)
        STATUS_REG |= EX_TWI_R_FAIL;

    return TWDR;
}

void TWI_write(uint8_t data) {
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN); // clear twint, enable comm
    
    while (!(TWCR & (1 << TWINT))) // wait for exchange to end
        ;

    if (TWI_STATUS != TW_MT_DATA_ACK)
        STATUS_REG |= EX_TWI_WR_FAIL;
}

void TWI_write_sla_r(uint8_t addr) {
    TWDR = (addr << 1) | 1;
    TWCR = (1 << TWINT) | (1 << TWEN); // clear twint and enable comm

    while (!(TWCR & (1 << TWINT))) // wait for exchange to end
        ;

    if (TWI_STATUS != TW_MR_SLA_ACK)
        STATUS_REG |= EX_TWI_START_FAIL;
}

void TWI_write_sla_w(uint8_t addr) {
    TWDR = (addr << 1);
    TWCR = (1 << TWINT) | (1 << TWEN); // clear twint and enable comm

    while (!(TWCR & (1 << TWINT))) // wait for exchange to end
        ;

    if (TWI_STATUS != TW_MT_SLA_ACK)
        STATUS_REG |= EX_TWI_START_FAIL;
}

void eeprom_write_reg(uint16_t addr, uint16_t data) {
    TWI_start();
    TWI_write_sla_w(EEPROM_ADDR);
    
    if (STATUS_REG & EX_TWI_START_FAIL) {
        TWI_stop();
        return;
    }

    TWI_write(addr >> 8);
    TWI_write(addr & 0xFF);

    TWI_write(data >> 8);
    TWI_write(data & 0xFF);

    TWI_stop();
}
