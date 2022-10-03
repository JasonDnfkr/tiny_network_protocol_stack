#include "include/xnet_define_cfg.h"

// 本机虚拟网卡 (netif) 的 IP 地址
const xipaddr_t netif_ipaddr = XNET_CFG_NETIF_IP;

// mac 广播地址 ff:ff:ff:ff:ff:ff
const uint8_t ether_broadcast[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };