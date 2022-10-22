#ifndef XNET_SYS_H
#define XNET_SYS_H

#include <stdint.h>
#include "xnet_define_cfg.h"

//#define min(a, b)           ((a) > (b) ? (b) : (a))

#define swap_order16(v)    ( (((v) & 0xff) << 8) | (((v) >> 8) & 0xff) )

// 12 34 56 78 -> 78 56 34 12
#define swap_order32(v)    ( ( ((v >> 0) & 0xff) << 24 ) | ( ((v >> 8) & 0xff) << 16 ) | ( ((v >> 16) & 0xff) << 8 ) | ((v >> 24) & 0xff) << 0 )


#define xipaddr_is_equal_buf(addr, buf) (memcmp((addr)->array, (buf), XNET_IPV4_ADDR_SIZE) == 0)
#define xipaddr_is_equal(addr1, addr2) ( (addr1)->addr == (addr2)->addr )
#define xipaddr_from_buf(dest, buf) ( (dest)->addr = *(uint32_t*)(buf) )

// 获取系统时间
typedef uint32_t xnet_time_t;
const xnet_time_t xsys_get_time(void);

// 校验和
uint16_t checksum16(uint16_t* buf, uint16_t len, uint16_t pre_sum, int complement);

// UDP 伪校验和
uint16_t checksum_peso(const xipaddr_t* src_ip, const xipaddr_t* dest_ip, uint8_t protocol, uint16_t* buf, uint16_t len);


#endif // !XNET_SYS_H
