#include "include/xnet_packet.h"

#define min(a, b)           ((a) < (b) ? (b) : (a))

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

// 增加包头，会自动更新 size 和 data 的位置
void add_header(xnet_packet_t* packet, uint16_t header_size) {
    packet->data -= header_size;
    packet->size += header_size;
}


// 移除包头，会自动更新 size 和 data 的位置
void remove_header(xnet_packet_t* packet, uint16_t header_size) {
    packet->data += header_size;
    packet->size -= header_size;
}

void truncate_packet(xnet_packet_t* packet, uint16_t size) {
    packet->size = min(packet->size, size);
}