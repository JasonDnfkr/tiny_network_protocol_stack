#include "include/xnet_udp.h"
#include "include/xnet_sys.h"
#include "include/xnet_ip.h"

#include <stdio.h>
#include <string.h>

void xudp_init() {
	memset(udp_socket, 0, sizeof(udp_socket));
}


void xudp_in(xudp_t* udp, xipaddr_t* src_ip, xnet_packet_t* packet) {
	xudp_hdr_t* udp_hdr = (xudp_hdr_t*)packet->data;
	uint16_t pre_checksum;

	if ((packet->size < sizeof(xudp_hdr_t)) || (packet->size < swap_order16(udp_hdr->total_len))) {
		return;
	}

	pre_checksum = udp_hdr->checksum;
	udp_hdr->checksum = 0;

	// 如果接收的包 checksum 为 0，则不要检查。
	// 非 0 才是需要检查
	if (pre_checksum != 0) {
		uint16_t checksum = checksum_peso(src_ip, &netif_ipaddr, XNET_PROTOCOL_UDP, (uint16_t*)udp_hdr, swap_order16(udp_hdr->total_len));
		checksum = (checksum == 0) ? 0xffff : checksum;
		if (pre_checksum != checksum) {
			printf("error: UDP checksum invalid\n");
			return;
		}
	}

	printf("received UDP packet\n");

	remove_header(packet, sizeof(xudp_hdr_t));
	uint16_t src_port = swap_order16(udp_hdr->src_port);
	if (udp->handler) {
		udp->handler(udp, src_ip, src_port, packet);
	}
}

xnet_err_t xudp_out(xudp_t* udp, xipaddr_t* dest_ip, uint16_t dest_port, xnet_packet_t* packet) {
	xudp_hdr_t* udp_hdr;
	uint16_t checksum;

	add_header(packet, sizeof(xudp_hdr_t));
	udp_hdr = (xudp_hdr_t*)packet->data;

	udp_hdr->src_port  = swap_order16(udp->local_port);
	udp_hdr->dest_port = swap_order16(dest_port);
	udp_hdr->total_len = swap_order16(packet->size);
	udp_hdr->checksum  = 0;

	checksum = checksum_peso(&netif_ipaddr, dest_ip, XNET_PROTOCOL_UDP, (uint16_t*)udp_hdr, packet->size);

	udp_hdr->checksum = checksum;

	return xip_out(XNET_PROTOCOL_UDP, dest_ip, packet);
}


// 查找一个 UDP
xudp_t* xudp_open(xudp_handler_t handler) {
	xudp_t* udp;
	xudp_t* end;
	for (udp = udp_socket, end = &udp_socket[XUDP_CFG_MAX_UDP]; udp < end; udp++) {
		if (udp->state == XUDP_STATE_FREE) {
			udp->state	    = XUDP_STATE_USED;
			udp->local_port = 0;
			udp->handler    = handler;
			return udp;
		}
	}

	return (xudp_t*)0;
}


// 关闭
void xudp_close(xudp_t* udp) {
	udp->state = XUDP_STATE_FREE;
}



xudp_t* xudp_find(uint16_t port) {
	xudp_t* curr;
	xudp_t* end;
	for (curr = udp_socket, end = &udp_socket[XUDP_CFG_MAX_UDP]; curr < end; curr++) {
		if ((curr->state == XUDP_STATE_USED) && (curr->local_port == port)) {
			return curr;
		}
	}

	return (xudp_t*)0;
}


// 绑定端口
xnet_err_t xudp_bind(xudp_t* udp, uint16_t local_port) {
	xudp_t* curr;
	xudp_t* end;
	for (curr = udp_socket, end = &udp_socket[XUDP_CFG_MAX_UDP]; curr < end; curr++) {
		if ((curr != udp) && (curr->local_port == local_port)) {
			printf("port occupied.\n");
			return XNET_ERR_BINDED;
		}
	}

	udp->local_port = local_port;
	return XNET_ERR_OK;
}