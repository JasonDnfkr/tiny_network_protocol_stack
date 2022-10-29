#ifndef XTIMER_H
#define XTIMER_H

#include "../xnet_tiny/include/xnet_sys.h"
#include "../xnet_tiny/include/xnet_tcp.h"

xnet_err_t timer_handler(xtcp_t* tcp, xtcp_conn_state_t event);

xnet_err_t timer_create(uint16_t port);

void timer_poll(xnet_time_t* time, uint32_t sec);

extern int timer_enabled;

extern xtcp_t* timer_tcp;

extern time_t timer_time;

#endif // !XTIMER_H
