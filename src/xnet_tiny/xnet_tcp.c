#include "include/xnet_tcp.h"
#include "include/xnet_sys.h"
#include "include/xnet_ip.h"

#include <string.h>
#include <stdio.h>


void xtcp_init() {
	memset(tcp_socket, 0, sizeof(tcp_socket));
}


static xnet_err_t tcp_send(xtcp_t* tcp, uint8_t flags) {
	xnet_packet_t* packet;
	xtcp_hdr_t* tcp_hdr;
	xnet_err_t err;
	uint16_t opt_size = (flags & XTCP_FLAG_SYN) ? 4 : 0;


	packet = xnet_alloc_for_send(sizeof(xtcp_hdr_t) + opt_size);
	tcp_hdr = (xtcp_hdr_t*)packet->data;

	tcp_hdr->src_port = swap_order16(tcp->local_port);
	tcp_hdr->dest_port = swap_order16(tcp->remote_port);
	tcp_hdr->seq = swap_order32(tcp->next_seq);
	tcp_hdr->ack = swap_order32(tcp->ack);
	tcp_hdr->window = 1024;
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

	tcp_hdr->checksum = checksum_peso(&netif_ipaddr, &tcp->remote_ip, XNET_PROTOCOL_TCP, (uint16_t*)packet->data, packet->size);
	tcp_hdr->checksum = tcp_hdr->checksum ? tcp_hdr->checksum : 0xffff;

	printf("TCP send\n");

	err = xip_out(XNET_PROTOCOL_TCP, &tcp->remote_ip, packet);
	if (err < 0) {
		return err;
	}

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

		new_tcp->state = XTCP_STATE_SYN_RECVD;
		new_tcp->local_port = listen_tcp->local_port;
		new_tcp->handler = listen_tcp->handler;
		new_tcp->remote_port = tcp_hdr->src_port;
		new_tcp->remote_ip.addr = remote_ip->addr;
		new_tcp->ack = ack;
		new_tcp->next_seq = tcp_get_init_seq();
		new_tcp->remote_win = tcp_hdr->window;

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
			tcp->ack++;
			tcp_send(tcp, XTCP_FLAG_ACK);
			xtcp_free(tcp);
		}
		break;

	case XTCP_STATE_ESTABLISHED:
		//printf("tcp control block status: ESTABLISHED\n");
		if (tcp_hdr->hdr_flags.flags & (XTCP_FLAG_FIN)) {
			tcp->state = XTCP_STATE_LAST_ACK;
			tcp->ack++;
			tcp_send(tcp, XTCP_FLAG_FIN | XTCP_FLAG_ACK);
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
			tcp->local_port		= 0;
			tcp->remote_port	= 0;
			tcp->remote_ip.addr = 0;
			tcp->handler		= (xtcp_handler_t)0;
			tcp->remote_win		= XTCP_MSS_DEFAULT;
			tcp->remote_mss		= XTCP_MSS_DEFAULT;
			tcp->next_seq		= tcp_get_init_seq();
			tcp->ack			= 0;
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