#ifndef XNET_TINY_H
#define XNET_TINY_H

#include <stdint.h>

#define XNET_CFG_NETIF_IP			{ 192, 168, 254, 2 }

#define XNET_CFG_PACKET_MAX_SIZE	1516

#define XNET_MAC_ADDR_SIZE			6
#define XNET_IPV4_ADDR_SIZE			4
#define XARP_CFG_ENTRY_OK_TMO		(20)
#define XARP_CFG_ENTRY_PENDING_TMO	(2)
#define XARP_CFG_MAX_RETRIES		40



#pragma pack(1)
// 以太网首部
typedef struct _xether_hdr_t {
	uint8_t     dest[XNET_MAC_ADDR_SIZE];	// 目的 ip
	uint8_t     src[XNET_MAC_ADDR_SIZE];	// 源 ip
	uint16_t    protocol;					// 协议类型
} xether_hdr_t;

#define XARP_HW_ETHER				0x1
#define XARP_REQUEST				0x1
#define XARP_REPLY					0x2

// ARP 数据包
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

#pragma pack()


typedef enum _xnet_err_t {
	XNET_ERR_OK    =  0,
	XNET_ERR_IO	   = -1,
	XNET_ERR_NONE  = -2,
} xnet_err_t;


typedef struct _xnet_packet_t {
	uint16_t    size;
	uint8_t*    data;
	uint8_t     payload[XNET_CFG_PACKET_MAX_SIZE];

} xnet_packet_t;


xnet_packet_t* xnet_alloc_for_send(uint16_t data_size);
xnet_packet_t* xnet_alloc_for_read(uint16_t data_size);


// 协议栈初始化
void xnet_init(void);
void xnet_poll(void);

typedef enum _xnet_protocol_t {
	XNET_PROTOCOL_ARP  = 0x0806,
	XNET_PROTOCOL_IP   = 0x0800,
	XNET_PROTOCOL_ICMP = 0x1,
} xnet_protocol_t;



typedef union _xipaddr_t {
	uint8_t     array[XNET_IPV4_ADDR_SIZE];
	uint32_t    addr;
} xipaddr_t;

#define XARP_ENTRY_FREE				0
#define XARP_ENTRY_OK				1
#define XARP_ENTRY_PENDING			2
#define XARP_TIMER_PERIOD			1  // 每隔 1 秒检查


// ARP 表
typedef struct _xarp_entry_t {
	xipaddr_t   ipaddr;						    // ip 地址
	uint8_t     macaddr[XNET_MAC_ADDR_SIZE];	// mac 地址
	uint8_t	    state;						    // 有效 / 无效 / 请求中
	uint16_t    tmo;							// 超过一定时间重新请求，避免无效 / 错误表项长期存在
	uint8_t     retry_cnt;					    // 重试次数
} xarp_entry_t;

// 获取系统时间
typedef uint32_t xnet_time_t;
const xnet_time_t xsys_get_time(void);


// ARP 协议处理

void xarp_init(void);
xnet_err_t xarp_make_request(const xipaddr_t* ipaddr);
void xarp_in(xnet_packet_t* packet);
void xarp_poll(void);
xnet_err_t xarp_resolve(const xipaddr_t* ipaddr, uint8_t** mac_addr);

// ip 协议处理

#define XNET_VERSION_IPV4	4

#pragma pack(1)
// ip 包头
typedef struct _xip_hdr_t {
	uint8_t		hdr_len : 4;
	uint8_t		version : 4;
	uint8_t		tos;
	uint16_t	total_len;
	uint16_t	id;
	uint16_t	flags_fragment;
	uint8_t		ttl;
	uint8_t		protocol;
	uint16_t	hdr_checksum;
	uint8_t		src_ip[XNET_IPV4_ADDR_SIZE];
	uint8_t		dest_ip[XNET_IPV4_ADDR_SIZE];
} xip_hdr_t;
#pragma pack()

#define XNET_IP_DEFAULT_TTL			64

void xip_init(void);
void xip_in(xnet_packet_t* packet);
xnet_err_t xip_out(xnet_protocol_t protocol, xipaddr_t* dest_ip, xnet_packet_t* packet);


#pragma pack(1)
// icmp 包头
typedef struct _xicmp_hdr_t {
	uint8_t		type;
	uint8_t		code;
	uint16_t	checksum;
	uint16_t	id;
	uint16_t	seq;
} xicmp_hdr_t;
#pragma pack()


// icmp 协议

#define XICMP_CODE_ECHO_REQUEST		8
#define XICMP_CODE_ECHO_REPLY		0

void xicmp_init(void);
void xicmp_in(xipaddr_t* src_ip, xnet_packet_t* packet);


// 驱动打开，发送，读取
xnet_err_t xnet_driver_open(uint8_t* mac_addr);
xnet_err_t xnet_driver_send(xnet_packet_t* packet);
xnet_err_t xnet_driver_read(xnet_packet_t** packet);

#endif // XNET_TINY_H

