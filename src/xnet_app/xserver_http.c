#include "xserver_http.h"
#include <include/xnet_tcp.h>

#include <stdio.h>

static xnet_err_t http_handler(xtcp_t* tcp, xtcp_conn_state_t event) {
	if (event == XTCP_CONN_CONNECTED) {
		printf("http connected\n");
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