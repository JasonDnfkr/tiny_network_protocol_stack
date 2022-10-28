#include "xserver_http.h"
#include <include/xnet_tcp.h>

#include <stdio.h>

static uint8_t tx_buffer[32];

static xnet_err_t http_handler(xtcp_t* tcp, xtcp_conn_state_t event) {
	static char* num = "0123456789ABCDEF";
	
	if (event == XTCP_CONN_CONNECTED) {
		printf("http connected\n");
		for (int i = 0; i < 1024; i++) {
			tx_buffer[i] = num[i % 16];
		}

		xtcp_write(tcp, tx_buffer, sizeof(tx_buffer));

		//xtcp_write(tcp, tx_buffer, sizeof(tx_buffer));
		//xtcp_write(tcp, tx_buffer, sizeof(tx_buffer));
		//xtcp_write(tcp, tx_buffer, sizeof(tx_buffer));
		//xtcp_write(tcp, tx_buffer, sizeof(tx_buffer));
		//xtcp_write(tcp, tx_buffer, sizeof(tx_buffer));


		//xtcp_close(tcp);
;	}
	else if (event == XTCP_CONN_CLOSED) {
		printf("http closed\n");
	}

	return XNET_ERR_OK;
}

xnet_err_t xserver_http_create(uint16_t port) {
	xtcp_t* tcp = xtcp_open(http_handler);

	xtcp_bind(tcp, port);
	xtcp_listen(tcp);

	printf("http server created. port: %d\n", tcp->local_port);

	return XNET_ERR_OK;
}