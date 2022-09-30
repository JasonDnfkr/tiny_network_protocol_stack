#ifndef XNET_SYS_H
#define XNET_SYS_H

#include <stdint.h>

//#define min(a, b)           ((a) > (b) ? (b) : (a))

#define swap_order16(v)    ( (((v) & 0xff) << 8) | (((v) >> 8) & 0xff) )
#define xipaddr_is_equal_buf(addr, buf) (memcmp((addr)->array, (buf), XNET_IPV4_ADDR_SIZE) == 0)
#define xipaddr_is_equal(addr1, addr2) ( (addr1)->addr == (addr2)->addr )
#define xipaddr_from_buf(dest, buf) ( (dest)->addr = *(uint32_t*)(buf) )

// ��ȡϵͳʱ��
typedef uint32_t xnet_time_t;
const xnet_time_t xsys_get_time(void);

// У���
uint16_t checksum16(uint16_t* buf, uint16_t len, uint16_t pre_sum, int complement);




#endif // !XNET_SYS_H