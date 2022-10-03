#include "include/xnet_udp.h"

#include <stdio.h>
#include <string.h>

void xudp_init() {
	memset(udp_socket, 0, sizeof(udp_socket));
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