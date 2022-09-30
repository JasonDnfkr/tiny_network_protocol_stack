#ifndef XNET_ARP_H
#define XNET_ARP_H

#include "xnet_define_cfg.h"
#include "xnet_packet.h"
#include "xnet_sys.h"

#pragma pack(1)

// ARP ���ݰ�
typedef struct _xarp_packet_t {
	uint16_t	hw_type;	// hardware type ( ethernet (1) )
	uint16_t	pro_type;	// protocol type ( ipv4 0x0800, arp 0x0806 )
	uint8_t		hw_len;		// hardware size 6 (mac)
	uint8_t		pro_len;	// protocol size 4 (ipv4)
	uint16_t	opcode;		// ARP REQ: 1, ARP RSP: 2, RARP REQ: 3, RARP RSP: 4

	uint8_t		sender_mac[XNET_MAC_ADDR_SIZE];
	uint8_t		sender_ip[XNET_IPV4_ADDR_SIZE];
	uint8_t		target_mac[XNET_MAC_ADDR_SIZE];
	uint8_t		target_ip[XNET_IPV4_ADDR_SIZE];

} xarp_packet_t;


// ARP ��
typedef struct _xarp_entry_t {
	xipaddr_t   ipaddr;						    // ip ��ַ
	uint8_t     macaddr[XNET_MAC_ADDR_SIZE];	// mac ��ַ
	uint8_t	    state;						    // ��Ч / ��Ч / ������
	uint16_t    tmo;							// ����һ��ʱ���������󣬱�����Ч / �������ڴ���
	uint8_t     retry_cnt;					    // ���Դ���
} xarp_entry_t;

#pragma pack()

xarp_entry_t arp_entry;
xnet_time_t  arp_timer;


// ARP Э�鴦��
void xarp_init(void);
xnet_err_t xarp_make_request(const xipaddr_t* ipaddr);
void xarp_in(xnet_packet_t* packet);
void xarp_poll(void);
xnet_err_t xarp_resolve(const xipaddr_t* ipaddr, uint8_t** mac_addr);



#endif // !XNET_ARP_H
