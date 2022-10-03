#include "include/xnet_packet.h"

#define min(a, b)           ((a) < (b) ? (b) : (a))

xnet_packet_t* xnet_alloc_for_send(uint16_t data_size) {
    // data ����ʼ��ַ���൱���� payload ���������˽ض�
    // �ضϺ�ߵ�ַ���������ݣ��͵�ַ�Ǳ�ͷ��Ԥ��λ��
    tx_packet.data = tx_packet.payload + XNET_CFG_PACKET_MAX_SIZE - data_size;

    tx_packet.size = data_size;

    return &tx_packet;
}

xnet_packet_t* xnet_alloc_for_read(uint16_t data_size) {
    rx_packet.data = rx_packet.payload;  // ָ��ͷ�����
    rx_packet.size = data_size;

    return &rx_packet;
}

// ���Ӱ�ͷ�����Զ����� size �� data ��λ��
void add_header(xnet_packet_t* packet, uint16_t header_size) {
    packet->data -= header_size;
    packet->size += header_size;
}


// �Ƴ���ͷ�����Զ����� size �� data ��λ��
void remove_header(xnet_packet_t* packet, uint16_t header_size) {
    packet->data += header_size;
    packet->size -= header_size;
}

void truncate_packet(xnet_packet_t* packet, uint16_t size) {
    packet->size = min(packet->size, size);
}