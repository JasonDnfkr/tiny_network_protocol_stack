#include "include/xnet_tcp.h"
#include "include/xnet_sys.h"
#include "include/xnet_ether.h"
#include "include/xnet_ip.h"

#include <string.h>
#include <stdio.h>


static void print_buf(const char* buf) {
	for (int i = 0; i < strlen(buf); i++) {
		printf("%c", buf[i]);
	}
}


void xtcp_init() {
	memset(tcp_socket, 0, sizeof(tcp_socket));
}




static void xtcp_buf_init(xtcp_buf_t* tcp_buf) {
	tcp_buf->front = 0;
	tcp_buf->tail = 0;
	tcp_buf->data_count = 0;
	tcp_buf->unacked_count = 0;
	tcp_buf->next = 0;
}


static uint16_t xtcp_buf_free_count(xtcp_buf_t* tcp_buf) {
	return XTCP_CFG_RTX_BUF_SIZE - tcp_buf->data_count;
}


static uint16_t xtcp_buf_wait_send_count(xtcp_buf_t* tcp_buf) {
	return tcp_buf->data_count - tcp_buf->unacked_count;
}


static void tcp_buf_add_acked_count(xtcp_buf_t* tcp_buf, uint16_t size) {
	tcp_buf->tail += size;
	if (tcp_buf->tail >= XTCP_CFG_RTX_BUF_SIZE) {
		tcp_buf->tail = 0;
	}

	tcp_buf->data_count -= size;
	tcp_buf->unacked_count -= size;
}


static void tcp_buf_add_unacked_count(xtcp_buf_t* tcp_buf, uint16_t size) {
	tcp_buf->unacked_count += size;
}

// 将 from 字符串中的数据写入 tcp 缓存中
// size 的大小不会超过 buf 中空闲的大小
static uint16_t xtcp_buf_write(xtcp_buf_t* tcp_buf, uint8_t* from, uint16_t size) {
	size = min(size, xtcp_buf_free_count(tcp_buf));

	for (int i = 0; i < size; i++) {
		tcp_buf->data[tcp_buf->front++] = *from++;
		if (tcp_buf->front >= XTCP_CFG_RTX_BUF_SIZE) {
			tcp_buf->front = 0;
		}
	}

	tcp_buf->data_count += size;
	return size;
}


// 将 tcp_buf 的内容读入至 to 中
// size 的大小不会超过 tcp_buf 中可读取的数据量 (tcp_buf->data_count)
static uint16_t xtcp_buf_read(xtcp_buf_t* tcp_buf, uint8_t* to, uint16_t size) {
	size = min(size, tcp_buf->data_count);
	for (int i = 0; i < size; i++) {
		*to++ = tcp_buf->data[tcp_buf->tail++];
		if (tcp_buf->tail >= XTCP_CFG_RTX_BUF_SIZE) {
			tcp_buf->tail = 0;
		}
	}
	tcp_buf->data_count -= size;
	return size;
}


static uint16_t xtcp_buf_read_for_send(xtcp_buf_t* tcp_buf, uint8_t* to, uint16_t size) {
	size = min(size, xtcp_buf_wait_send_count(tcp_buf));
	for (int i = 0; i < size; i++) {
		*to++ = tcp_buf->data[tcp_buf->next++];
		if (tcp_buf->next >= XTCP_CFG_RTX_BUF_SIZE) {
			tcp_buf->next = 0;
		}
	}

	return size;
}

// 将 from 的数据写入 tcp 的读缓存中(rx_buf)
// 并将 tcp 的 ack 值增大
static uint16_t tcp_recv(xtcp_t* tcp, uint8_t flags, uint8_t* from, uint16_t size) {
	uint16_t read_size = xtcp_buf_write(&tcp->rx_buf, from, size);
	printf("tcp_recv:\n");
	print_buf(tcp->rx_buf.data);
	printf("\norigin from:\n");
	print_buf(from);
	tcp->ack += read_size;
	if (flags & (XTCP_FLAG_FIN | XTCP_FLAG_SYN)) {
		tcp->ack++;
	}
	return read_size;
}


static xnet_err_t tcp_send(xtcp_t* tcp, uint8_t flags) {
	xnet_packet_t* packet;
	xtcp_hdr_t* tcp_hdr;
	xnet_err_t err;

	uint16_t data_size = xtcp_buf_wait_send_count(&tcp->tx_buf);
	uint16_t opt_size = (flags & XTCP_FLAG_SYN) ? 4 : 0;

	// 这里判断对方的 window 大小
	// 如果自己待发送的数据 大于 对方 window 的大小，
	// 会被直接截断
	if (tcp->remote_win > 0) {
		data_size = min(data_size, tcp->remote_win);
		data_size = min(data_size, tcp->remote_mss); // 和 ip 分片有关，这里不涉及
		if (data_size + opt_size > XTCP_DATA_MAX_SIZE) {
			data_size = XTCP_DATA_MAX_SIZE - opt_size;
		}
	}
	else {
		data_size = 0;
	}


	packet = xnet_alloc_for_send(data_size + sizeof(xtcp_hdr_t) + opt_size);
	tcp_hdr = (xtcp_hdr_t*)packet->data;

	tcp_hdr->src_port = swap_order16(tcp->local_port);
	tcp_hdr->dest_port = swap_order16(tcp->remote_port);
	tcp_hdr->seq = swap_order32(tcp->next_seq);
	tcp_hdr->ack = swap_order32(tcp->ack);
	tcp_hdr->window = swap_order16(xtcp_buf_free_count(&tcp->rx_buf));
	tcp_hdr->checksum = 0;
	tcp_hdr->urgent_ptr = 0;
	if (flags & XTCP_FLAG_SYN) {
		uint8_t* opt_data = packet->data + sizeof(xtcp_hdr_t);
		opt_data[0] = XTCP_KIND_MSS;
		opt_data[1] = 4;
		*(uint16_t*)(opt_data + 2) = swap_order16(XTCP_MSS_DEFAULT);
	}

	tcp_hdr->hdr_flags.all = 0;
	tcp_hdr->hdr_flags.hdr_len = (sizeof(xtcp_hdr_t) + opt_size) / 4;
	tcp_hdr->hdr_flags.flags = flags;
	tcp_hdr->hdr_flags.all = swap_order16(tcp_hdr->hdr_flags.all);

	xtcp_buf_read_for_send(&tcp->tx_buf, packet->data + opt_size + sizeof(xtcp_hdr_t), data_size);

	tcp_hdr->checksum = checksum_peso(&netif_ipaddr, &tcp->remote_ip, XNET_PROTOCOL_TCP, (uint16_t*)packet->data, packet->size);
	tcp_hdr->checksum = tcp_hdr->checksum ? tcp_hdr->checksum : 0xffff;

	printf("TCP send\n");

	err = xip_out(XNET_PROTOCOL_TCP, &tcp->remote_ip, packet);
	if (err < 0) {
		return err;
	}

	tcp->remote_win -= data_size;
	tcp->next_seq += data_size;
	tcp_buf_add_unacked_count(&tcp->tx_buf, data_size);

	if (flags & (XTCP_FLAG_SYN | XTCP_FLAG_FIN)) {
		tcp->next_seq++;
	}


	return XNET_ERR_OK;
}


xnet_err_t tcp_send_reset(uint32_t remote_ack, uint16_t local_port, xipaddr_t* remote_ip, uint16_t remote_port) {
	xnet_packet_t* packet = xnet_alloc_for_send(sizeof(xtcp_hdr_t));
	xtcp_hdr_t* tcp_hdr = (xtcp_hdr_t*)packet->data;

	tcp_hdr->src_port	= swap_order16(local_port);
	tcp_hdr->dest_port	= swap_order16(remote_port);
	tcp_hdr->seq		= 0;
	tcp_hdr->ack		= swap_order32(remote_ack);
	tcp_hdr->window		= 0;
	tcp_hdr->checksum	= 0;
	tcp_hdr->urgent_ptr = 0;

	tcp_hdr->hdr_flags.all     = 0;
	tcp_hdr->hdr_flags.hdr_len = sizeof(xtcp_hdr_t) / 4;
	tcp_hdr->hdr_flags.flags   = XTCP_FLAG_RST | XTCP_FLAG_ACK;
	tcp_hdr->hdr_flags.all	   = swap_order16(tcp_hdr->hdr_flags.all);

	tcp_hdr->checksum = checksum_peso(&netif_ipaddr, remote_ip, XNET_PROTOCOL_TCP, (uint16_t*)packet->data, packet->size);
	tcp_hdr->checksum = tcp_hdr->checksum ? tcp_hdr->checksum : 0xffff;

	printf("TCP send reset\n");

	return xip_out(XNET_PROTOCOL_TCP, remote_ip, packet);
}


static void tcp_read_mss(xtcp_t* tcp, xtcp_hdr_t* tcp_hdr) {
	uint16_t opt_len = tcp_hdr->hdr_flags.hdr_len * 4 - sizeof(xtcp_hdr_t);

	if (opt_len == 0) {
		tcp->remote_mss = XTCP_MSS_DEFAULT;
	}
	else {
		uint8_t* opt_data = (uint8_t*)tcp_hdr + sizeof(xtcp_hdr_t);
		uint8_t* opt_end = opt_data + opt_len;

		while ((*opt_data != XTCP_KIND_END) && (opt_data < opt_end)) {
			if ((*opt_data++ == XTCP_KIND_MSS) && (*opt_data++ == 4)) {
				tcp->remote_mss = swap_order16(*(uint16_t*)opt_data);
				return;
			}
			//if (*opt_data == XTCP_KIND_MSS && *(opt_data + 1) == 4) {
			//	tcp->remote_mss = swap_order16(*(uint16_t*)opt_data);
			//	return;
			//}
		}
	}
}


// LISTEN -> SYN_RCVD 监听状态下只负责接受 SYN 连接请求
static void tcp_process_accept(xtcp_t* listen_tcp, xipaddr_t* remote_ip, xtcp_hdr_t* tcp_hdr) {
	uint16_t hdr_flags = tcp_hdr->hdr_flags.all;

	// 第一次握手
	if (hdr_flags & XTCP_FLAG_SYN) {
		// 创建一个新的控制块为对方处理后续
		xtcp_t* new_tcp = xtcp_alloc();
		if (!new_tcp) {
			return;
		}

		uint32_t ack = tcp_hdr->seq + 1;
		xnet_err_t err;

		uint32_t init_seq = tcp_get_init_seq();

		new_tcp->state			= XTCP_STATE_SYN_RECVD;
		new_tcp->local_port		= listen_tcp->local_port;
		new_tcp->handler		= listen_tcp->handler;
		new_tcp->remote_port	= tcp_hdr->src_port;
		new_tcp->remote_ip.addr = remote_ip->addr;
		new_tcp->ack			= ack;
		new_tcp->next_seq		= init_seq;
		new_tcp->unacked_seq	= init_seq;
		new_tcp->remote_win		= tcp_hdr->window;

		tcp_read_mss(new_tcp, tcp_hdr);

		err = tcp_send(new_tcp, XTCP_FLAG_SYN | XTCP_FLAG_ACK);
		if (err < 0) {
			xtcp_free(new_tcp);
			return;
		}
		return;
	}
	else { // 在监听状态下只接受连接请求，其他状态不管
	tcp_send_reset(tcp_hdr->seq, listen_tcp->local_port, remote_ip, tcp_hdr->src_port);
	}
};


// 接收 tcp 数据包
void xtcp_in(xipaddr_t* remote_ip, xnet_packet_t* packet) {
	xtcp_hdr_t* tcp_hdr = (xtcp_hdr_t*)packet->data;

	if (packet->size < sizeof(xtcp_hdr_t)) {
		return;
	}

	uint16_t pre_checksum = 0;
	xtcp_t* tcp = NULL;

	pre_checksum = tcp_hdr->checksum;
	tcp_hdr->checksum = 0;

	// 如果接收的包 checksum 为 0，则不要检查。
	// 非 0 才是需要检查
	if (pre_checksum != 0) {
		uint16_t checksum = checksum_peso(remote_ip, &netif_ipaddr, XNET_PROTOCOL_TCP, (uint16_t*)tcp_hdr, packet->size);
		checksum = (checksum == 0) ? 0xffff : checksum;
		if (pre_checksum != checksum) {
			printf("error: TCP checksum invalid\n");
			return;
		}
	}

	printf("received TCP packet\n");

	// 用于后续计算，直接转换成小端
	tcp_hdr->src_port = swap_order16(tcp_hdr->src_port);
	tcp_hdr->dest_port = swap_order16(tcp_hdr->dest_port);
	tcp_hdr->hdr_flags.all = swap_order16(tcp_hdr->hdr_flags.all);
	tcp_hdr->seq = swap_order32(tcp_hdr->seq);
	tcp_hdr->ack = swap_order32(tcp_hdr->ack);
	// checksum 用不上
	tcp_hdr->window = swap_order16(tcp_hdr->window);

	tcp = xtcp_find(remote_ip, tcp_hdr->src_port, tcp_hdr->dest_port);

	// 如果没有找到符合要求的
	// 符合要求：(本地端口、远程端口、远程IP一致)
	// 符合要求：或 (处于 LISTEN 状态时，本地端口一致、但其他不一致) 
	if (tcp == (xtcp_t*)0) {
		tcp_send_reset(tcp_hdr->seq + 1, tcp_hdr->dest_port, remote_ip, tcp_hdr->src_port);
		return;
	}

	tcp->remote_win = tcp_hdr->window;

	// 判断是否 LISTEN 状态
	if (tcp->state == XTCP_STATE_LISTEN) {
		tcp_process_accept(tcp, remote_ip, tcp_hdr);
		return;
	}

	// 判断是否合法：hdr->seq == tcp->ack
	if (tcp_hdr->seq != tcp->ack) {
		printf("error: tcp_hdr->seq != tcp->ack");
		tcp_send_reset(tcp_hdr->seq + 1, tcp_hdr->dest_port, remote_ip, tcp_hdr->src_port);
		return;
	}

	remove_header(packet, tcp_hdr->hdr_flags.hdr_len * 4);
	switch (tcp->state) {
	case XTCP_STATE_SYN_RECVD:
		if (tcp_hdr->hdr_flags.flags & XTCP_FLAG_ACK) {
			tcp->unacked_seq++;	// SYN 报文
			tcp->state = XTCP_STATE_ESTABLISHED;
			tcp->handler(tcp, XTCP_CONN_CONNECTED);
		}
		break;

	case XTCP_STATE_FIN_WAIT_1:
		if ((tcp_hdr->hdr_flags.flags & (XTCP_FLAG_FIN | XTCP_FLAG_ACK)) == (XTCP_FLAG_FIN | XTCP_FLAG_ACK)) {
			xtcp_free(tcp);
		}
		else if (tcp_hdr->hdr_flags.flags & XTCP_FLAG_ACK) {
			tcp->state = XTCP_STATE_FIN_WAIT_2;
		}
		break;

	case XTCP_STATE_FIN_WAIT_2:
		if (tcp_hdr->hdr_flags.flags & XTCP_FLAG_FIN) {
			//tcp->ack++;
			tcp->ack = tcp_hdr->seq + 1;
			tcp_send(tcp, XTCP_FLAG_ACK);
			xtcp_free(tcp);
		}
		break;

	case XTCP_STATE_ESTABLISHED:
		// 先确认是否是 ACK
		// 如果是 ACK，则判断是否是响应报文 (hdr ack 在 未确认的缓冲区之间) 
		if (tcp_hdr->hdr_flags.flags & (XTCP_FLAG_ACK)) {
			if (tcp->unacked_seq <= tcp_hdr->ack && tcp_hdr->ack <= tcp->next_seq) {
				uint16_t curr_ack_size = tcp_hdr->ack - tcp->unacked_seq;
				tcp_buf_add_acked_count(&tcp->tx_buf, curr_ack_size);
				tcp->unacked_seq += curr_ack_size;
			}
		}

		// (A) 将 packet 中的数据写入 rx 缓存中
		uint16_t read_size = tcp_recv(tcp, (uint8_t)tcp_hdr->hdr_flags.flags, packet->data, packet->size);


		if (tcp_hdr->hdr_flags.flags & (XTCP_FLAG_FIN)) {
			tcp->state = XTCP_STATE_LAST_ACK;
			//tcp->ack++;
			tcp->ack = tcp_hdr->seq + 1;
			tcp_send(tcp, XTCP_FLAG_FIN | XTCP_FLAG_ACK);
		}
		else if (read_size) { // 接收到了数据，发送确认报文
			tcp_send(tcp, XTCP_FLAG_ACK);
			tcp->handler(tcp, XTCP_CONN_DATA_RECV);
		}
		// 或者看看有没有数据要发，有的话，同时发数据即ack
		// 没有收到数据，可能是对方发来的ACK。此时，有数据有就发数据，没数据就不理会
		// 什么情况下会有这个？
		else if (xtcp_buf_wait_send_count(&tcp->tx_buf)) {
			tcp_send(tcp, XTCP_FLAG_ACK);
		}

		break;

	case XTCP_STATE_LAST_ACK:
		if (tcp_hdr->hdr_flags.flags & XTCP_FLAG_ACK) {
			tcp->handler(tcp, XTCP_CONN_CLOSED);
			xtcp_free(tcp);
		}
		break;
	}
}


xtcp_t* xtcp_alloc() {
	xtcp_t* tcp;
	xtcp_t* end;

	for (tcp = tcp_socket, end = tcp_socket + XTCP_CFG_MAX_TCP; tcp < end; tcp++) {
		if (tcp->state == XTCP_STATE_FREE) {
			uint32_t init_seq = tcp_get_init_seq();

			tcp->local_port		= 0;
			tcp->remote_port	= 0;
			tcp->remote_ip.addr = 0;
			tcp->handler		= (xtcp_handler_t)0;
			tcp->remote_win		= XTCP_MSS_DEFAULT;
			tcp->remote_mss		= XTCP_MSS_DEFAULT;
			tcp->next_seq		= init_seq;
			tcp->unacked_seq	= init_seq;
			tcp->ack			= 0;
			
			xtcp_buf_init(&tcp->tx_buf);
			xtcp_buf_init(&tcp->rx_buf);
			return tcp;
		}
	}

	return (xtcp_t*)0;
}


void xtcp_free(xtcp_t* tcp) {
	tcp->state = XTCP_STATE_FREE;
}

// 根据 tcp 报文的 remote_ip，端口号寻找本地的 tcp 控制块
// 返回情况：
// 1. remote_ip，remote_port, 本地 port 都符合，则返回
// 2. 只有 remote_port 符合，但该 tcp 控制块 state 处于 LISTEN 
xtcp_t* xtcp_find(xipaddr_t* remote_ip, uint16_t remote_port, uint16_t local_port) {
	xtcp_t* tcp;
	xtcp_t* end;
	xtcp_t* founded_tcp = (xtcp_t*)0;

	for (tcp = tcp_socket, end = &tcp_socket[XTCP_CFG_MAX_TCP]; tcp < end; tcp++) {
		if (tcp->state == XTCP_STATE_FREE) {
			continue;
		}

		if (tcp->local_port != local_port) {
			continue;
		}

		if (xipaddr_is_equal(remote_ip, &tcp->remote_ip) && (remote_port == tcp->remote_port)) {
			return tcp;
		}

		if (tcp->state == XTCP_STATE_LISTEN) {
			founded_tcp = tcp;
		}
	}

	return founded_tcp;
}


xtcp_t* xtcp_open(xtcp_handler_t handler) {
	xtcp_t* tcp = xtcp_alloc();
	if (!tcp) return (xtcp_t*)0;

	tcp->state   = XTCP_STATE_CLOSED;
	tcp->handler = handler;
	return tcp;
}


xnet_err_t xtcp_bind(xtcp_t* tcp, uint16_t local_port) {
	xtcp_t* curr;
	xtcp_t* end;

	for (curr = tcp_socket, end = &tcp_socket[XTCP_CFG_MAX_TCP]; curr < end; curr++) {
		if ((curr != tcp) && (curr->local_port == local_port)) {
			return XNET_ERR_BINDED;
		}
	}

	tcp->local_port = local_port;
	return XNET_ERR_OK;
}


xnet_err_t xtcp_listen(xtcp_t* tcp) {
	tcp->state = XTCP_STATE_LISTEN;
	return XNET_ERR_OK;
}


xnet_err_t xtcp_close(xtcp_t* tcp) {
	xnet_err_t err;
	if (tcp->state == XTCP_STATE_ESTABLISHED) {
		err = tcp_send(tcp, XTCP_FLAG_FIN | XTCP_FLAG_ACK);
		if (err < 0) {
			return err;
		}

		tcp->state = XTCP_STATE_FIN_WAIT_1;
	}
	else {
		xtcp_free(tcp);
	}
	return XNET_ERR_OK;
}


// 将 size 字节的 data 发送出去
int xtcp_write(xtcp_t* tcp, uint8_t* data, uint16_t size) {
	int sended_count = 0;

	if ((tcp->state != XTCP_STATE_ESTABLISHED)) {
		printf("return -1\n");
		return -1;
	}

	sended_count = xtcp_buf_write(&tcp->tx_buf, data, size);

	if (sended_count > 0) {
		printf("xtcp write\n");
		tcp_send(tcp, XTCP_FLAG_ACK);
	}

	return sended_count;
}


//将 tcp 控制块中，写缓存(rx_buf)的内容读入至 data
int xtcp_read(xtcp_t* tcp, uint8_t* data, uint16_t size) {
	return xtcp_buf_read(&tcp->rx_buf, data, size);
}