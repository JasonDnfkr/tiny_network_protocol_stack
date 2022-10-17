#ifndef XNET_UDP_H
#define XNET_UDP_H

#include "xnet_define_cfg.h"
#include "xnet_packet.h"

typedef struct _xudp_t xudp_t;

typedef xnet_err_t (*xudp_handler_t)(xudp_t* udp, xipaddr_t* src_ip, uint16_t src_port, xnet_packet_t* packet);


// UDP 协议数据块
struct _xudp_t {
	enum {
		XUDP_STATE_FREE,
		XUDP_STATE_USED,
	} state;

	uint16_t local_port;
	xudp_handler_t handler;
};


// UDP 包头
#pragma pack(1)
typedef struct _xudp_hdr_t {
	uint16_t   src_port;
	uint16_t   dest_port;
	uint16_t   total_len;
	uint16_t   checksum;
} xudp_hdr_t;

#pragma pack()


xudp_t udp_socket[XUDP_CFG_MAX_UDP];

void xudp_init(void);

void xudp_in(xudp_t* udp, xipaddr_t* src, xnet_packet_t* packet);
xnet_err_t xudp_out(xudp_t* udp, xipaddr_t* dest_ip, uint16_t dest_port, xnet_packet_t* packet);

xudp_t* xudp_open(xudp_handler_t handler);
void xudp_close(xudp_t* udp);
xudp_t* xudp_find(uint16_t port);
xnet_err_t xudp_bind(xudp_t* udp, uint16_t local_port);


#endif // !XNET_UDP_H
