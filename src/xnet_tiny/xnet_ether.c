#include "include/xnet_ether.h"
#include "include/xnet_driver.h"
#include "include/xnet_packet.h"
#include "include/xnet_arp.h"
#include "include/xnet_ip.h"
#include "include/xnet_icmp.h"

#include <stdio.h>
#include <string.h>

extern const xipaddr_t netif_ipaddr;
extern const uint8_t ether_broadcast[];

xnet_err_t ethernet_init(void) {
    printf("ethernet_init\n");

    // 驱动将 mac 地址填充至此
    xnet_err_t err = xnet_driver_open(netif_mac);

    if (err < 0) return err;

    //return XNET_ERR_OK;
    return xarp_make_request(&netif_ipaddr);
}


/*
    @para dest_mac_addr 对方的 mac 地址
*/
// 根据协议、mac 地址发出以太网包
xnet_err_t ethernet_out_to(xnet_protocol_t protocol, const uint8_t* dest_mac_addr, xnet_packet_t* packet) {
    xether_hdr_t* ether_hdr;

    add_header(packet, sizeof(xether_hdr_t));
    ether_hdr = (xether_hdr_t*)packet->data;

    memcpy(ether_hdr->dest, dest_mac_addr, XNET_MAC_ADDR_SIZE);
    memcpy(ether_hdr->src, netif_mac, XNET_MAC_ADDR_SIZE);
    ether_hdr->protocol = swap_order16(protocol);

    //xnet_err_t res = xnet_driver_send(packet);
    //return res;
    return xnet_driver_send(packet);
}

// 根据 ip 发出以太网包
xnet_err_t ethernet_out(xipaddr_t* dest_ip, xnet_packet_t* packet) {
    xnet_err_t err;
    uint8_t* mac_addr = NULL;

    if ((err = xarp_resolve(dest_ip, &mac_addr)) == XNET_ERR_OK) {
        return ethernet_out_to(XNET_PROTOCOL_IP, mac_addr, packet);
    }

    return err;
}


void ethernet_in(xnet_packet_t* packet) {
    // 报文不可能小于 报头 大小
    if (packet->size <= sizeof(xether_hdr_t)) {
        return;
    }

    uint16_t protocol;
    xether_hdr_t* ether_hdr = (xether_hdr_t*)packet->data;
    protocol = swap_order16(ether_hdr->protocol);

    switch (protocol) {
    case XNET_PROTOCOL_ARP:
        //printf("received ARP protocol\n");
        remove_header(packet, sizeof(xether_hdr_t));
        xarp_in(packet);
        break;

    case XNET_PROTOCOL_IP:
        remove_header(packet, sizeof(xether_hdr_t));
        xip_in(packet);
        break;
    }
}


void ethernet_poll(void) {
    xnet_packet_t* packet = NULL;

    if (xnet_driver_read(&packet) == XNET_ERR_OK) {
        ethernet_in(packet);
    }
}