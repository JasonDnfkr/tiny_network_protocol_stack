#include "xserver_http.h"
#include <include/xnet_tcp.h>

#include <stdio.h>

#define XTCP_FIFO_SIZE		XTCP_CFG_MAX_TCP

typedef struct _xhttp_fifo_t {
	xtcp_t* buffer[XTCP_FIFO_SIZE];
	uint8_t front;
	uint8_t tail;
	uint8_t count;
} xhttp_fifo_t;


static xhttp_fifo_t http_fifo;


static void xhttp_fifo_init(xhttp_fifo_t* fifo) {
	fifo->count = 0;
	fifo->front = 0;
	fifo->tail	= 0;
}


static xnet_err_t xhttp_fifo_in(xhttp_fifo_t* fifo, xtcp_t* tcp) {
	if (fifo->count >= XTCP_FIFO_SIZE) {
		return XNET_ERR_MEM;
	}

	fifo->buffer[fifo->front++] = tcp;
	if (fifo->front >= XTCP_FIFO_SIZE) {
		fifo->front = 0;
	}

	fifo->count++;
	return XNET_ERR_OK;
}


static xtcp_t* xhttp_fifo_out(xhttp_fifo_t* fifo) {
	if (fifo->count == 0) {
		return (xtcp_t*)0;
	}

	xtcp_t* tcp = fifo->buffer[fifo->tail++];
	if (fifo->tail >= XTCP_FIFO_SIZE) {
		fifo->tail = 0;
	}

	fifo->count--;
	return tcp;
}


static uint8_t tx_buffer[1280];

static xnet_err_t http_handler(xtcp_t* tcp, xtcp_conn_state_t event) {
	static char* num = "0123456789ABCDEF";
	
	if (event == XTCP_CONN_CONNECTED) {
		xhttp_fifo_in(&http_fifo, tcp);
		printf("http connected\n");
		//for (int i = 0; i < 1024; i++) {
		//	tx_buffer[i] = num[i % 16];
		//}

		//xtcp_write(tcp, tx_buffer, sizeof(tx_buffer));

		//xtcp_write(tcp, tx_buffer, sizeof(tx_buffer));
		//xtcp_write(tcp, tx_buffer, sizeof(tx_buffer));
		//xtcp_write(tcp, tx_buffer, sizeof(tx_buffer));
		//xtcp_write(tcp, tx_buffer, sizeof(tx_buffer));
		//xtcp_write(tcp, tx_buffer, sizeof(tx_buffer));


		//xtcp_close(tcp);
;	}
	else if (event == XTCP_CONN_DATA_RECV) {
		uint8_t* data = tx_buffer;
		// tcp 控制块已经将接收到的数据存入了 rx buffer 
		// 这里将数据从 rx 缓存里读出
		uint16_t read_size = xtcp_read(tcp, tx_buffer, sizeof(tx_buffer));
		
		// 将读出的数据回发
		while (read_size) {
			printf("[]");
			uint16_t curr_size = xtcp_write(tcp, data, read_size);
			data += curr_size;
			read_size -= curr_size;
		}
	}
	else if (event == XTCP_CONN_CLOSED) {
		printf("http closed\n");
	}

	return XNET_ERR_OK;
}


void xserver_http_run() {

}


xnet_err_t xserver_http_create(uint16_t port) {
	xtcp_t* tcp = xtcp_open(http_handler);

	xtcp_bind(tcp, port);
	xtcp_listen(tcp);

	xhttp_fifo_init(&http_fifo);

	printf("http server created. port: %d\n", tcp->local_port);

	return XNET_ERR_OK;
}