#ifndef XNET_DEFINE_CFG_H
#define XNET_DEFINE_CFG_H

#include <stdint.h>


#define XNET_CFG_NETIF_IP			{ 192, 168, 254, 2 } // 本地虚拟网卡地址
#define XNET_CFG_PACKET_MAX_SIZE	1516                 // 以太网数据包最大长度 (包含了首部)


#define XNET_MAC_ADDR_SIZE			6   // MAC 地址长度：6 bytes
#define XNET_IPV4_ADDR_SIZE			4   // IPV4 地址长度：4 bytes
#define XNET_VERSION_IPV4			4   // IPV4 版本号


#define XARP_CFG_ENTRY_OK_TMO		(20)// ARP 表持续时间：20 seconds
#define XARP_CFG_ENTRY_PENDING_TMO	(2) // ARP 表超时后，等待的时间：2 seconds
#define XARP_CFG_MAX_RETRIES		40  // ARP 表超时重传的最大次数：40 次
#define XARP_ENTRY_FREE				0   // ARP 表不存在
#define XARP_ENTRY_OK				1   // ARP 表就绪，状态可用
#define XARP_ENTRY_PENDING			2   // ARP 表等待修改
#define XARP_TIMER_PERIOD			1   // ARP 表自检的时间：1 second


#define XARP_HW_ETHER				0x1 // ARP ?
#define XARP_REQUEST				0x1 // ARP 请求
#define XARP_REPLY					0x2 // ARP 响应


#define XICMP_CODE_ECHO_REQUEST		8   // ICMP Code 字段：REQUEST
#define XICMP_CODE_ECHO_REPLY		0   // ICMP Code 字段：REPLY
#define XICMP_CODE_PORT_UNREACH		3   // ICMP Code 字段：Port Unreachable
#define XICMP_CODE_PRO_UNREACH		2   // ICMP Code 字段：Protocol Unreachable
#define XICMP_TYPE_UNREACH			3   // ICMP Type 字段：Unreachable


#define XNET_IP_DEFAULT_TTL			64  // IP 数据包生存时间


// 协议代码
typedef enum _xnet_protocol_t {
	XNET_PROTOCOL_ARP = 0x0806, // ARP  0x8086
	XNET_PROTOCOL_IP = 0x0800, // IP   0x0800
	XNET_PROTOCOL_ICMP = 0x1,	 // ICMP 0x1
} xnet_protocol_t;


// ip 数据结构
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


// 本机虚拟网卡 (netif) 的 IP 地址
static const xipaddr_t netif_ipaddr = XNET_CFG_NETIF_IP;

// mac 广播地址 ff:ff:ff:ff:ff:ff
static const uint8_t ether_broadcast[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

// 本机虚拟网卡 (netif) 的 mac 地址
uint8_t netif_mac[XNET_MAC_ADDR_SIZE];

#endif