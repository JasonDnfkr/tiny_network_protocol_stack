#ifndef XNET_PACKET_H
#define XNET_PACKET_H

#include "xnet_define_cfg.h"

typedef struct _xnet_packet_t {
	uint16_t    size;
	uint8_t*	data;
	uint8_t     payload[XNET_CFG_PACKET_MAX_SIZE];
} xnet_packet_t;


static xnet_packet_t tx_packet;
static xnet_packet_t rx_packet;


xnet_packet_t* xnet_alloc_for_send(uint16_t data_size);
xnet_packet_t* xnet_alloc_for_read(uint16_t data_size);

// 增加包头，会自动更新 size 和 data 的位置
void add_header(xnet_packet_t* packet, uint16_t header_size);


// 移除包头，会自动更新 size 和 data 的位置
void remove_header(xnet_packet_t* packet, uint16_t header_size);

void truncate_packet(xnet_packet_t* packet, uint16_t size);

#endif // !XNET_PACKET_H
