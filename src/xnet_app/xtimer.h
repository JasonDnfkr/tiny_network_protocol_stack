#ifndef XTIMER_H
#define XTIMER_H

#include "../xnet_tiny/include/xnet_sys.h"
#include "../xnet_tiny/include/xnet_tcp.h"

xnet_err_t timer_handler(xtcp_t* tcp, xtcp_conn_state_t event);

xnet_err_t timer_create(uint16_t port);

#endif // !XTIMER_H
