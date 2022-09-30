#include "include/xnet_icmp.h"
#include "include/xnet_ip.h"
#include "include/xnet_sys.h"

#include <stdio.h>
#include <string.h>

void xicmp_init(void) {

}


static xnet_err_t reply_icmp_request(xicmp_hdr_t* icmp_hdr, xipaddr_t* src_ip, xnet_packet_t* packet) {
    xnet_packet_t* tx = xnet_alloc_for_send(packet->size);
    // 传进来的 packet 是一个完整的带 icmp 报头的数据包
    // 所以指针可以直接指向 data
    xicmp_hdr_t* reply_hdr = (xicmp_hdr_t*)tx->data;

    reply_hdr->type = XICMP_CODE_ECHO_REPLY;
    reply_hdr->code = 0;
    reply_hdr->id = icmp_hdr->id;
    reply_hdr->seq = icmp_hdr->seq;
    reply_hdr->checksum = 0;

    memcpy((uint8_t*)reply_hdr + sizeof(xicmp_hdr_t), (uint8_t*)icmp_hdr + sizeof(xicmp_hdr_t), packet->size - sizeof(xicmp_hdr_t));

    printf("data (32 bytes): ");

    uint8_t* ptr = (uint8_t*)icmp_hdr + sizeof(xicmp_hdr_t);

    for (int i = 0; i < 32; i++) {
        printf("%c", *(ptr + i));
    }
    printf("\n");

    reply_hdr->checksum = checksum16((uint16_t*)reply_hdr, tx->size, 0, 1);

    return xip_out(XNET_PROTOCOL_ICMP, src_ip, tx);
}


void xicmp_in(xipaddr_t* src_ip, xnet_packet_t* packet) {
    xicmp_hdr_t* icmphdr = (xicmp_hdr_t*)packet->data;

    if ((packet->size >= sizeof(xicmp_hdr_t)) && (icmphdr->type == XICMP_CODE_ECHO_REQUEST)) {
        reply_icmp_request(icmphdr, src_ip, packet);
    }
}


// ICMP 不可达
xnet_err_t xicmp_dest_unreach(uint8_t code, xip_hdr_t* ip_hdr) {
    xicmp_hdr_t* icmp_hdr;
    xnet_packet_t* packet;
    xipaddr_t dest_ip;

    uint16_t ip_hdr_size = ip_hdr->hdr_len * 4;
    uint16_t ip_data_size = swap_order16(ip_hdr->total_len) - ip_hdr_size;
    ip_data_size = ip_hdr_size + min(ip_data_size, 8);

    packet = xnet_alloc_for_send(sizeof(xicmp_hdr_t) + ip_data_size);

    icmp_hdr = (xicmp_hdr_t*)packet->data;
    icmp_hdr->type = XICMP_TYPE_UNREACH;
    icmp_hdr->id = 0;
    icmp_hdr->seq = 0;
    memcpy((uint8_t*)icmp_hdr + sizeof(xicmp_hdr_t), ip_hdr, ip_data_size);
    icmp_hdr->checksum = 0;
    icmp_hdr->checksum = checksum16((uint16_t*)icmp_hdr, packet->size, 0, 0);

    xipaddr_from_buf(&dest_ip, ip_hdr->src_ip);

    return xip_out(XNET_PROTOCOL_ICMP, &dest_ip, packet);
}