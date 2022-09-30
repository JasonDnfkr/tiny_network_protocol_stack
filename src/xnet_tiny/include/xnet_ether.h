#ifndef XNET_ETHER_H
#define XNET_ETHER_H

#include "xnet_define_cfg.h"
#include "xnet_packet.h"


#pragma pack(1)
// 以太网首部
typedef struct _xether_hdr_t {
	uint8_t     dest[XNET_MAC_ADDR_SIZE];	// 目的 ip
	uint8_t     src[XNET_MAC_ADDR_SIZE];	// 源 ip
	uint16_t    protocol;					// 协议类型
} xether_hdr_t;

#pragma pack()

xnet_err_t ethernet_init(void);

// 根据协议、mac 地址发出以太网包
xnet_err_t ethernet_out_to(xnet_protocol_t protocol, const uint8_t* dest_mac_addr, xnet_packet_t* packet);

// 根据 ip 发出以太网包
xnet_err_t ethernet_out(xipaddr_t* dest_ip, xnet_packet_t* packet);


void ethernet_in(xnet_packet_t* packet);

void ethernet_poll(void);


#endif
