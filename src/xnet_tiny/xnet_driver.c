#include "include/xnet_driver.h"
#include "include/xnet_ether.h"
#include "include/xnet_arp.h"
#include "include/xnet_ip.h"
#include "include/xnet_icmp.h"
#include "include/xnet_udp.h"
#include "include/xnet_tcp.h"

#include <stdlib.h>

void xnet_init(void) {
    ethernet_init();
    xarp_init();
    xip_init();
    xicmp_init();
    xudp_init();
    xtcp_init();
    srand(xsys_get_time());
}


void xnet_poll(void) {
    // 调用以太网查询函数看看有没有包
    // 有包则去 ethernet_poll() 处理
    ethernet_poll();
    xarp_poll();
}
