#ifndef XNET_TCP_H
#define XNET_TCP_H

#include "xnet_define_cfg.h"
#include "xnet_packet.h"

#include <stdlib.h>

#define XTCP_KIND_END			0
#define XTCP_KIND_MSS			2
#define XTCP_MSS_DEFAULT		1460

#define tcp_get_init_seq() ((rand() << 16) + rand())

typedef enum _xtcp_state_t {
	XTCP_STATE_FREE,
	XTCP_STATE_CLOSED,
	XTCP_STATE_LISTEN,
	XTCP_STATE_SYN_RECVD,
	XTCP_STATE_ESTABLISHED,
} xtcp_state_t;


typedef struct _xtcp_t xtcp_t;


typedef enum _xtcp_conn_state_t {
	XTCP_CONN_CONNECTED,
	XTCP_CONN_DATA_RECV,
	XTCP_CONN_CLOSED,
} xtcp_conn_state_t;


typedef xnet_err_t (*xtcp_handler_t)(xtcp_t* tcp, xtcp_conn_state_t event);


#pragma pack(1)
typedef struct _xtcp_hdr_t {
	uint16_t src_port;
	uint16_t dest_port;
	uint32_t seq;
	uint32_t ack;

#define XTCP_FLAG_FIN		(1 << 0)
#define XTCP_FLAG_SYN		(1 << 1)
#define XTCP_FLAG_RST		(1 << 2)
#define XTCP_FLAG_ACK		(1 << 4)

	union {
		struct {
			uint16_t flags : 6;
			uint16_t reserved : 6;
			uint16_t hdr_len : 4;
		};
		uint16_t all;
	} hdr_flags;

	uint16_t window;
	uint16_t checksum;
	uint16_t urgent_ptr;
} xtcp_hdr_t;

#pragma pack()


struct _xtcp_t {
	xtcp_state_t   state;
	uint16_t	   local_port;
	uint16_t	   remote_port;
	xipaddr_t	   remote_ip;

	uint32_t	   next_seq;
	uint32_t	   ack;

	uint16_t	   remote_mss;
	uint16_t	   remote_win;

	xtcp_handler_t handler;
};


xtcp_t tcp_socket[XTCP_CFG_MAX_TCP];



void xtcp_init(void);

void xtcp_in(xipaddr_t* remote_ip, xnet_packet_t* packet);

xtcp_t* xtcp_alloc(void);

void xtcp_free(xtcp_t* tcp);

xtcp_t* xtcp_find(xipaddr_t* remote_ip, uint16_t remote_port, uint16_t local_port);

xtcp_t* xtcp_open(xtcp_handler_t handler);

xnet_err_t xtcp_bind(xtcp_t* tcp, uint16_t local_port);

xnet_err_t xtcp_listen(xtcp_t* tcp);

xnet_err_t xtcp_close(xtcp_t* tcp);

#endif // !XNET_TCP_H
