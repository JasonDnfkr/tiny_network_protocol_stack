#include "xtimer.h"
#include <stdio.h>


void timer_send(xtcp_t* tcp) {
	static char* num = "0123456789ABCDEF";
	char tx_buffer[16];
	for (int i = 0; i < 16; i++) {
		tx_buffer[i] = num[i];
	}

	xtcp_write(tcp, tx_buffer, sizeof(tx_buffer));
}

xnet_err_t timer_handler(xtcp_t* tcp, xtcp_conn_state_t event) {
	if (event == XTCP_CONN_CONNECTED) {
		// TODO: poll open
	}
	else if (event == XTCP_CONN_CLOSED) {
		printf("timer closed\n");
	}

	return XNET_ERR_OK;
}


xnet_err_t timer_create(uint16_t port) {
	xtcp_t* tcp = xtcp_open(timer_handler);
	xtcp_bind(tcp, port);
	xtcp_listen(tcp);
	printf("timer created. port: %d\n", tcp->local_port);
	return XNET_ERR_OK;
}
