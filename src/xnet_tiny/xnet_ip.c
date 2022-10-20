#include "include/xnet_packet.h"
#include "include/xnet_ip.h"
#include "include/xnet_icmp.h"
#include "include/xnet_ether.h"
#include "include/xnet_sys.h"
#include "include/xnet_udp.h"
#include "include/xnet_tcp.h"

#include <stdio.h>
#include <string.h>


void xip_init(void) {

}


void xip_in(xnet_packet_t* packet) {
    xip_hdr_t*  iphdr = (xip_hdr_t*)packet->data;
    uint32_t    header_size;
    uint32_t    total_size;
    uint16_t    pre_checksum;
    xipaddr_t   src_ip;

    if (iphdr->version != XNET_VERSION_IPV4) {
        return;
    }

    header_size = iphdr->hdr_len * 4;
    total_size  = swap_order16(iphdr->total_len);

    if (header_size < sizeof(xip_hdr_t)) {
        printf("\nip header size < xip_hdr_t\n");
        return;
    }
    if (total_size < header_size) {
        printf("\ntotal size < header_size\n");
        return;
    }

    pre_checksum = iphdr->hdr_checksum;
    iphdr->hdr_checksum = 0;
    if (pre_checksum != checksum16((uint16_t*)iphdr, header_size, 0, 1)) {
        printf("\nchecksum failed\n");
        return;
    }
    iphdr->hdr_checksum = pre_checksum;

    if (!xipaddr_is_equal_buf(&netif_ipaddr, iphdr->dest_ip)) {
        return;
    }

    xipaddr_from_buf(&src_ip, iphdr->src_ip);

    switch (iphdr->protocol) {
    case XNET_PROTOCOL_UDP:
        if (packet->size >= sizeof(xudp_hdr_t)) {
            // 先让 udp 指针指向 IP 报头后的位置
            xudp_hdr_t* udp_hdr = (xudp_hdr_t*)(packet->data + header_size);
            xudp_t* udp = xudp_find(swap_order16(udp_hdr->dest_port));
            if (udp) {
                remove_header(packet, header_size);
                xudp_in(udp, &src_ip, packet);
            }
            else {
                xicmp_dest_unreach(XICMP_CODE_PORT_UNREACH, iphdr);
            }
        }
        break;

    case XNET_PROTOCOL_ICMP:
        remove_header(packet, header_size);
        xicmp_in(&src_ip, packet);
        break;

    case XNET_PROTOCOL_TCP:
        remove_header(packet, header_size);
        xtcp_in(&src_ip, packet);
        break;

    default:
        xicmp_dest_unreach(XICMP_CODE_PORT_UNREACH, iphdr);
        break;
    }
}


xnet_err_t xip_out(xnet_protocol_t protocol, xipaddr_t* dest_ip, xnet_packet_t* packet) {
    static uint32_t ip_packet_id = 0;
    xip_hdr_t* iphdr;

    add_header(packet, sizeof(xip_hdr_t));
    iphdr = (xip_hdr_t*)packet->data;

    iphdr->version        = XNET_VERSION_IPV4;
    iphdr->hdr_len        = sizeof(xip_hdr_t) / 4;
    iphdr->tos            = 0;
    iphdr->total_len      = swap_order16(packet->size);
    iphdr->id             = swap_order16(ip_packet_id);
    iphdr->flags_fragment = 0;
    iphdr->ttl            = XNET_IP_DEFAULT_TTL;
    iphdr->protocol       = protocol;

    memcpy(iphdr->src_ip, &netif_ipaddr.array, XNET_IPV4_ADDR_SIZE);
    memcpy(iphdr->dest_ip, dest_ip->array, XNET_IPV4_ADDR_SIZE);

    iphdr->hdr_checksum = 0;
    iphdr->hdr_checksum = checksum16((uint16_t*)iphdr, sizeof(xip_hdr_t), 0, 1);

    ip_packet_id++;

    return ethernet_out(dest_ip, packet);
}