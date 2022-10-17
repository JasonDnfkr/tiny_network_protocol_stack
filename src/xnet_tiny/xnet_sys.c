#include "include/xnet_sys.h"
#include <time.h>

const xnet_time_t xsys_get_time(void) {
    return clock() / CLOCKS_PER_SEC;
}

// 校验和
uint16_t checksum16(uint16_t* buf, uint16_t len, uint16_t pre_sum, int complement) {
    uint32_t checksum = pre_sum; //pre_sum
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

    return complement ? (uint16_t)~checksum : checksum;
}

// 伪首部校验和
uint16_t checksum_peso(const xipaddr_t* src_ip, const xipaddr_t* dest_ip, uint8_t protocol, uint16_t* buf, uint16_t len) {
    uint8_t zero_protocol[] = { 0, protocol };
    uint16_t c_len = swap_order16(len);
    
    uint32_t sum;
    sum = checksum16((uint16_t*)src_ip->array, XNET_IPV4_ADDR_SIZE, 0, 0);
    sum = checksum16((uint16_t*)dest_ip->array, XNET_IPV4_ADDR_SIZE, sum, 0);
    sum = checksum16((uint16_t*)zero_protocol, 2, sum, 0);
    sum = checksum16((uint16_t*)&c_len, 2, sum, 0);

    return checksum16(buf, len, sum, 1);
}