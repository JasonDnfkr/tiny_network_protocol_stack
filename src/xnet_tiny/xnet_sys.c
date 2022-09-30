#include "include/xnet_sys.h"
#include <time.h>

const xnet_time_t xsys_get_time(void) {
    return clock() / CLOCKS_PER_SEC;
}

// Ð£ÑéºÍ
uint16_t checksum16(uint16_t* buf, uint16_t len, uint16_t pre_sum, int complement) {
    uint32_t checksum = 0; //pre_sum
    uint16_t high;

    while (len > 1) {
        checksum += *buf++;
        len -= 2;
    }

    if (len > 0) {
        checksum += *(uint8_t*)buf;
    }

    while ((high = checksum >> 16) != 0) {
        checksum = high + (checksum & 0xffff);
    }

    return (uint16_t)~checksum;
}