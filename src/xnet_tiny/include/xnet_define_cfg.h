#ifndef XNET_DEFINE_CFG_H
#define XNET_DEFINE_CFG_H

#include <stdint.h>


#define XNET_CFG_NETIF_IP			{ 192, 168, 254, 2 } // ��������������ַ
#define XNET_CFG_PACKET_MAX_SIZE	1516                 // ��̫�����ݰ���󳤶� (�������ײ�)


#define XNET_MAC_ADDR_SIZE			6   // MAC ��ַ���ȣ�6 bytes
#define XNET_IPV4_ADDR_SIZE			4   // IPV4 ��ַ���ȣ�4 bytes
#define XNET_VERSION_IPV4			4   // IPV4 �汾��


#define XARP_CFG_ENTRY_OK_TMO		(20)// ARP �����ʱ�䣺20 seconds
#define XARP_CFG_ENTRY_PENDING_TMO	(2) // ARP ��ʱ�󣬵ȴ���ʱ�䣺2 seconds
#define XARP_CFG_MAX_RETRIES		40  // ARP ��ʱ�ش�����������40 ��
#define XARP_ENTRY_FREE				0   // ARP ������
#define XARP_ENTRY_OK				1   // ARP �������״̬����
#define XARP_ENTRY_PENDING			2   // ARP ��ȴ��޸�
#define XARP_TIMER_PERIOD			1   // ARP ���Լ��ʱ�䣺1 second


#define XARP_HW_ETHER				0x1 // ARP ?
#define XARP_REQUEST				0x1 // ARP ����
#define XARP_REPLY					0x2 // ARP ��Ӧ


#define XICMP_CODE_ECHO_REQUEST		8   // ICMP Code �ֶΣ�REQUEST
#define XICMP_CODE_ECHO_REPLY		0   // ICMP Code �ֶΣ�REPLY
#define XICMP_CODE_PORT_UNREACH		3   // ICMP Code �ֶΣ�Port Unreachable
#define XICMP_CODE_PRO_UNREACH		2   // ICMP Code �ֶΣ�Protocol Unreachable
#define XICMP_TYPE_UNREACH			3   // ICMP Type �ֶΣ�Unreachable


#define XNET_IP_DEFAULT_TTL			64  // IP ���ݰ�����ʱ��


// Э�����
typedef enum _xnet_protocol_t {
	XNET_PROTOCOL_ARP = 0x0806, // ARP  0x8086
	XNET_PROTOCOL_IP = 0x0800, // IP   0x0800
	XNET_PROTOCOL_ICMP = 0x1,	 // ICMP 0x1
} xnet_protocol_t;


// ip ���ݽṹ
typedef union _xipaddr_t {
	uint8_t     array[XNET_IPV4_ADDR_SIZE];
	uint32_t    addr;
} xipaddr_t;


// xnet_err_t
typedef enum _xnet_err_t {
	XNET_ERR_OK = 0,
	XNET_ERR_IO = -1,
	XNET_ERR_NONE = -2,
} xnet_err_t;


// ������������ (netif) �� IP ��ַ
static const xipaddr_t netif_ipaddr = XNET_CFG_NETIF_IP;

// mac �㲥��ַ ff:ff:ff:ff:ff:ff
static const uint8_t ether_broadcast[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

// ������������ (netif) �� mac ��ַ
uint8_t netif_mac[XNET_MAC_ADDR_SIZE];

#endif