#include "xnet_tiny.h"

#include <stdio.h>
#include <string.h>

static const xipaddr_t netif_ipaddr      = XNET_CFG_NETIF_IP;
static const uint8_t   ether_broadcast[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

#define min(a, b)           ((a) > (b) ? (b) : (a))

#define swap_order16(v)     (((v) & 0xff) << 8) | (((v) >> 8) & 0xff)

static uint8_t         netif_mac[XNET_MAC_ADDR_SIZE];

static xnet_packet_t   tx_packet;
static xnet_packet_t   rx_packet;

static xarp_entry_t    arp_entry;


/*  -- debug --  */
static void print_arp_packet(const xarp_packet_t* arp_packet) {
    printf("hardware type: %d\n", arp_packet->hw_type);
    printf("protocol type: %d\n", arp_packet->pro_type);
    printf("hardware length: %d\n", arp_packet->hw_len);
    printf("protocol length: %d\n", arp_packet->pro_len);
}




xnet_packet_t* xnet_alloc_for_send(uint16_t data_size) {
    // data 的起始地址。相当于在 payload 数组中做了截断
    // 截断后高地址是数据内容，低地址是报头的预留位置
    tx_packet.data = tx_packet.payload + XNET_CFG_PACKET_MAX_SIZE - data_size;

    tx_packet.size = data_size;

    return &tx_packet;
}

xnet_packet_t* xnet_alloc_for_read(uint16_t data_size) {
    rx_packet.data = rx_packet.payload;  // 指向报头最左端
    rx_packet.size = data_size;

    return &rx_packet;
}

static void add_header(xnet_packet_t* packet, uint16_t header_size) {
    packet->data -= header_size;
    packet->size += header_size;
}

static void remove_header(xnet_packet_t* packet, uint16_t header_size) {
    packet->data += header_size;
    packet->size -= header_size;
}

static void truncate_packet(xnet_packet_t* packet, uint16_t size) {
    packet->size = min(packet->size, size);
}

static xnet_err_t ethernet_init(void) {
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
static xnet_err_t ethernet_out_to(xnet_protocol_t protocol, const uint8_t* dest_mac_addr, xnet_packet_t* packet) {
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


static void ethernet_in(xnet_packet_t* packet) {
    // 报文不可能小于 报头 大小
    if (packet->size <= sizeof(xether_hdr_t)) {
        return;
    }

    uint16_t protocol;
    xether_hdr_t* ether_hdr = (xether_hdr_t*)packet->data;
    protocol = swap_order16(ether_hdr->protocol);

    switch (protocol) {
    case XNET_PROTOCOL_ARP:
        printf("received ARP protocol\n");
        break;
    case XNET_PROTOCOL_IP:
        break;
    }
}


static void ethernet_poll(void) {
    xnet_packet_t* packet;

    if (xnet_driver_read(&packet) == XNET_ERR_OK) {
        ethernet_in(packet);
    }
}

void xarp_init(void) {
    arp_entry.state = XARP_ENTRY_FREE;
}


xnet_err_t xarp_make_request(const xipaddr_t* ipaddr) {
    printf("xarp_make_request\n");

    xnet_packet_t* packet = xnet_alloc_for_send(sizeof(xarp_packet_t));
    xarp_packet_t* arp_packet = (xarp_packet_t*)packet->data;

    arp_packet->hw_type  = swap_order16(XARP_HW_ETHER);
    arp_packet->pro_type = swap_order16(XNET_PROTOCOL_IP);
    arp_packet->hw_len   = XNET_MAC_ADDR_SIZE;
    arp_packet->pro_len  = XNET_IPV4_ADDR_SIZE;
    arp_packet->opcode   = swap_order16(XARP_REQUEST);

    memcpy(arp_packet->sender_mac, netif_mac, XNET_MAC_ADDR_SIZE);
    memcpy(arp_packet->sender_ip, netif_ipaddr.array, XNET_IPV4_ADDR_SIZE);
    memset(arp_packet->target_mac, 0, XNET_MAC_ADDR_SIZE);
    memcpy(arp_packet->target_ip, ipaddr->array, XNET_IPV4_ADDR_SIZE);
    
    print_arp_packet(arp_packet);
    
    return ethernet_out_to(XNET_PROTOCOL_ARP, ether_broadcast, packet);

}


void xnet_init(void) {
    ethernet_init();
    xarp_init();
}


void xnet_poll(void) {
    // 调用以太网查询函数看看有没有包
    // 有包则去 ethernet_poll() 处理
    ethernet_poll(); 
}




