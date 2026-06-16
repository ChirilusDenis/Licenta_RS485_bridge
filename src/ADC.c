#include <avr/io.h>
#include "ADC.h"

void ADC_init() {
    ADCSRA = (7 << ADPS0); // prescaler = 128
    ADMUX = (1 << REFS0);
}

void ADC_enable() {
    ADCSRA |= (1 <<ADEN);
    while (ADCSRA & (1 << ADSC)) //throw out first conversion
        ;
}

void ADC_read(uint8_t channel) {
    ADMUX &= ~7;
    ADMUX |= (channel & 7);
    ADCSRA |= (1 << ADSC); // start second conversion
    
    while (ADCSRA & (1 << ADSC))
        ;

}

void ADC_stop() {
    ADCSRA &= ~(1 << ADEN); // stop ADC from eating power
}