#include <stdint.h>
#include "constants.h"

uint8_t mb_LRC(uint8_t len) {
    uint8_t lrc = 0;
    for (uint8_t i = 0; i < len; i++)
        lrc += frame[i];
    return (~lrc) + 1;
}

uint16_t ascii_decode(uint16_t ascii_len) {
    uint16_t bin_len = 0;
    uint8_t hi, lo;
    for (uint16_t i = 0; i < ascii_len; i += 2) {
        hi = frame[i];
        lo = frame[i + 1];
        hi = (hi >= 'A') ? (hi - 'A' + 10) : (hi - '0');
        lo = (lo >= 'A') ? (lo - 'A' + 10) : (lo - '0');
        frame[bin_len++] = (hi << 4) | lo;
    }
    return bin_len;
}