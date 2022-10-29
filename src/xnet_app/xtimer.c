#include "xtimer.h"
#include <stdio.h>

int timer_enabled;

xtcp_t* timer_tcp;

time_t timer_time;


static void timer_send(xtcp_t* tcp) {
	static char* num = "0123456789ABCDEF";
	char tx_buffer[16];
	for (int i = 0; i < 16; i++) {
		tx_buffer[i] = num[i];
	}

	xtcp_write(tcp, tx_buffer, sizeof(tx_buffer));
}

xnet_err_t timer_handler(xtcp_t* tcp, xtcp_conn_state_t event) {
	if (event == XTCP_CONN_CONNECTED) {
		timer_enabled = 1;
		timer_tcp = tcp;
	}
	else if (event == XTCP_CONN_CLOSED) {
		printf("timer closed\n");
		timer_enabled = 0;
	}

	return XNET_ERR_OK;
}

void timer_poll(xnet_time_t* time, uint32_t sec) {
	xnet_time_t curr = xsys_get_time();

	if (sec == 0) {
		*time = curr;
		return 1;
	}

	if (curr - *time >= sec) {
		*time = curr;
		if (timer_enabled) {
			printf("[%d sec]\n", sec);
			timer_send(timer_tcp);
		}
	}
}


xnet_err_t timer_create(uint16_t port) {
	xtcp_t* tcp = xtcp_open(timer_handler);
	xtcp_bind(tcp, port);
	xtcp_listen(tcp);
	printf("timer created. port: %d\n", tcp->local_port);
	return XNET_ERR_OK;
}
