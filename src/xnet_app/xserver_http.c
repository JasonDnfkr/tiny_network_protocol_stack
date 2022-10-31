#include "xserver_http.h"
#include <include/xnet_tcp.h>
#include <include/xnet_driver.h>

#include <stdio.h>
#include <string.h>

#define XTCP_FIFO_SIZE		XTCP_CFG_MAX_TCP

static char rx_buffer[2048];
static char tx_buffer[2048];

static char url_path[255];
static char file_path[255];

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
	printf("xhttp_fifo_in\n");
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


static xnet_err_t http_handler(xtcp_t* tcp, xtcp_conn_state_t event) {
	static char* num = "0123456789ABCDEF";
	
	if (event == XTCP_CONN_CONNECTED) {
		xhttp_fifo_in(&http_fifo, tcp);
		printf("http connected\n");
		//for (int i = 0; i < 1024; i++) {
		//	tx_buffer[i] = num[i % 16];
		//}

		//xtcp_write(tcp, tx_buffer, sizeof(tx_buffer));


		//xtcp_close(tcp);
;	}
	//else if (event == XTCP_CONN_DATA_RECV) {
	//	uint8_t* data = tx_buffer;
	//	// tcp 控制块已经将接收到的数据存入了 rx buffer 
	//	// 这里将数据从 rx 缓存里读出
	//	uint16_t read_size = xtcp_read(tcp, tx_buffer, sizeof(tx_buffer));
	//	
	//	// 将读出的数据回发
	//	while (read_size) {
	//		printf("[]");
	//		uint16_t curr_size = xtcp_write(tcp, data, read_size);
	//		data += curr_size;
	//		read_size -= curr_size;
	//	}
	//}
	else if (event == XTCP_CONN_CLOSED) {
		printf("http closed\n");
	}

	return XNET_ERR_OK;
}


static int get_line(xtcp_t* tcp, char* buf, int size) {
	int i = 0;
	//printf("getline<<\n");
	while (i < size) {
		char c;

		if (xtcp_read(tcp, (uint8_t*)&c, 1) > 0) {
			//printf("%c", c);
			if ((c != '\n') && (c != '\r')) {
				buf[i++] = c;
			}
			else if (c == '\n') {
				break;
			}
		}

		xnet_poll();
	}
	//printf("\n>>getline\n");

	buf[i] = '\0';
	return i; //返回写到的总量
}


static int http_send(xtcp_t* tcp, char* buf, int size) {
	int sent_size = 0;

	while (size > 0) {
		int curr_size = xtcp_write(tcp, (uint8_t*)buf, (uint16_t)size);
		if (curr_size < 0) break;

		size -= curr_size;
		buf += curr_size;

		sent_size += curr_size;

		xnet_poll();
	}

	return sent_size;
}


static void http_close(xtcp_t* tcp) {
	xtcp_close(tcp);
	printf("http close.\n");
}


static void send_404_not_found(xtcp_t* tcp) {
	sprintf(tx_buffer,
		"HTTP/1.0 404 NOT FOUND\r\n\r\n");
	http_send(tcp, tx_buffer, sizeof(tx_buffer));
}


// 组建 http 报头，并根据路径发送文件
static void send_file(xtcp_t* tcp, const char* url) {
	FILE* file;
	uint32_t size;

	while (*url == '/') url++;
	sprintf(file_path, "%s/%s", XHTTP_DOC_PATH, url);

	file = fopen(file_path, "rb");
	if (file == NULL) {
		send_404_not_found(tcp);
		return;
	}

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);

	sprintf(tx_buffer, 
		"HTTP/1.0 200 OK\r\n"
		"Content-Length:%d\r\n"
		"\r\n",
		(int)size);

	http_send(tcp, tx_buffer, (int)strlen(tx_buffer));

	while (!feof(file)) {
		size = fread(tx_buffer, 1, sizeof(tx_buffer), file);
		if (http_send(tcp, tx_buffer, size) < 0) {
			fclose(file);
			return;
		}
	}

	fclose(file);
}


static void print_buf(const char* buf) {
	for (int i = 0; i < strlen(buf); i++) {
		printf("%c", buf[i]);
	}
}


void xserver_http_run() {
	xtcp_t* tcp;
	
	while ((tcp = xhttp_fifo_out(&http_fifo)) != (xtcp_t*)0) {
		printf("xhttp_fifo_out\n");
		char* c = rx_buffer;
		printf("rx buffer: \n");
		print_buf((char*)tcp->rx_buf.data);
		// 先把 HTTP 报文中的 GET 请求行读出来
		int get_size = get_line(tcp, rx_buffer, sizeof(rx_buffer));

		if (get_size <= 0) {
			http_close(tcp);
			continue;
		}

		while (*c == ' ') c++;	// GET 3个字符，非空格
		// 判断第一行的前三个字符是不是 GET
		if (strncmp(rx_buffer, "GET", 3) != 0) {
			http_close(tcp);
			continue;
		}

		while (*c != ' ') c++;	// GET 3个字符，非空格
		while (*c == ' ') c++;	// GET 后有一个空格
		int i;
		for (i = 0; i < sizeof(url_path); i++) {
			if (*c == ' ') {
				break;
			}
			url_path[i] = *c++;
		}
		url_path[i] = '\0';

		send_file(tcp, url_path);

		http_close(tcp);
	}
}


xnet_err_t xserver_http_create(uint16_t port) {
	xtcp_t* tcp = xtcp_open(http_handler);

	xtcp_bind(tcp, port);
	xtcp_listen(tcp);

	xhttp_fifo_init(&http_fifo);

	printf("http server created. port: %d\n", tcp->local_port);

	return XNET_ERR_OK;
}