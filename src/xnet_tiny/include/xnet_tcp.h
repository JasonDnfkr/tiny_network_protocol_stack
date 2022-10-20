#ifndef XNET_TCP_H
#define XNET_TCP_H

#include "xnet_define_cfg.h"

typedef enum _xtcp_state_t {
	XTCP_STATE_FREE,
	XTCP_STATE_CLOSED,
	XTCP_STATE_LISTEN,
} xtcp_state_t;


typedef struct _xtcp_t xtcp_t;


typedef enum _xtcp_conn_state_t {
	XTCP_CONN_CONNECTED,
	XTCP_CONN_DATA_RECV,
	XTCP_CONN_CLOSED,
} xtcp_conn_state_t;


typedef xnet_err_t (*xtcp_handler_t)(xtcp_t* tcp, xtcp_conn_state_t event);


struct _xtcp_t {
	xtcp_state_t   state;
	uint16_t	   local_port;
	uint16_t	   remote_port;
	xipaddr_t	   remote_ip;

	xtcp_handler_t handler;
};


xtcp_t tcp_socket[XTCP_CFG_MAX_TCP];



void xtcp_init(void);

xtcp_t* xtcp_alloc(void);

void xtcp_free(xtcp_t* tcp);

xtcp_t* xtcp_open(xtcp_handler_t handler);

xnet_err_t xtcp_bind(xtcp_t* tcp, uint16_t local_port);

xnet_err_t xtcp_listen(xtcp_t* tcp);

xnet_err_t xtcp_close(xtcp_t* tcp);

#endif // !XNET_TCP_H
