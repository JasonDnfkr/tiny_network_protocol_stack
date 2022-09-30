#ifndef XNET_IP_H
#define XNET_IP_H

#include "xnet_define_cfg.h"
#include "xnet_packet.h"

#pragma pack(1)
// ip °üÍ·
typedef struct _xip_hdr_t {
	uint8_t		hdr_len : 4;
	uint8_t		version : 4;
	uint8_t		tos;
	uint16_t	total_len;
	uint16_t	id;
	uint16_t	flags_fragment;
	uint8_t		ttl;
	uint8_t		protocol;
	uint16_t	hdr_checksum;
	uint8_t		src_ip[XNET_IPV4_ADDR_SIZE];
	uint8_t		dest_ip[XNET_IPV4_ADDR_SIZE];
} xip_hdr_t;
#pragma pack()

#define XNET_IP_DEFAULT_TTL			64

void xip_init(void);
void xip_in(xnet_packet_t* packet);
xnet_err_t xip_out(xnet_protocol_t protocol, xipaddr_t* dest_ip, xnet_packet_t* packet);


#endif // !XNET_IP_H
