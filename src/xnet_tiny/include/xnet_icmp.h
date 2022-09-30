#ifndef XNET_ICMP_H
#define XNET_ICMP_H

#include "xnet_define_cfg.h"
#include "xnet_packet.h"
#include "xnet_ip.h"

#pragma pack(1)
// icmp Ê×²¿
typedef struct _xicmp_hdr_t {
	uint8_t		type;
	uint8_t		code;
	uint16_t	checksum;
	uint16_t	id;
	uint16_t	seq;
} xicmp_hdr_t;
#pragma pack()


void xicmp_init(void);
void xicmp_in(xipaddr_t* src_ip, xnet_packet_t* packet);
xnet_err_t xicmp_dest_unreach(uint8_t code, xip_hdr_t* ip_hdr);


#endif // !XNET_ICMP_H
