#include "include/xnet_define_cfg.h"

// ������������ (netif) �� IP ��ַ
const xipaddr_t netif_ipaddr = XNET_CFG_NETIF_IP;

// mac �㲥��ַ ff:ff:ff:ff:ff:ff
const uint8_t ether_broadcast[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };