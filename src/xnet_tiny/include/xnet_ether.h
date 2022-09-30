#ifndef XNET_ETHER_H
#define XNET_ETHER_H

#include "xnet_define_cfg.h"
#include "xnet_packet.h"


#pragma pack(1)
// ��̫���ײ�
typedef struct _xether_hdr_t {
	uint8_t     dest[XNET_MAC_ADDR_SIZE];	// Ŀ�� ip
	uint8_t     src[XNET_MAC_ADDR_SIZE];	// Դ ip
	uint16_t    protocol;					// Э������
} xether_hdr_t;

#pragma pack()

xnet_err_t ethernet_init(void);

// ����Э�顢mac ��ַ������̫����
xnet_err_t ethernet_out_to(xnet_protocol_t protocol, const uint8_t* dest_mac_addr, xnet_packet_t* packet);

// ���� ip ������̫����
xnet_err_t ethernet_out(xipaddr_t* dest_ip, xnet_packet_t* packet);


void ethernet_in(xnet_packet_t* packet);

void ethernet_poll(void);


#endif
