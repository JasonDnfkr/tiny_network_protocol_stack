/**
 * 用1500行代码从0开始实现TCP/IP协议栈+WEB服务器
 *
 * 本源码旨在用最简单、最易懂的方式帮助你快速地了解TCP/IP以及HTTP工作原理的主要核心知识点。
 * 所有代码经过精心简化设计，避免使用任何复杂的数据结构和算法，避免实现其它无关紧要的细节。
 *
 * 本源码配套高清的视频教程，免费提供下载！具体的下载网址请见下面。
 * 视频中的PPT暂时提供下载，但配套了学习指南，请访问下面的网址。
 *
 * 作者：李述铜
 * 网址: http://01ketang.cc/tcpip
 * QQ群1：78034806   QQ群2：1063238091 （加群时请注明：tcpip），免费提供关于该源码的支持和问题解答。
 * 微信公众号：请搜索 01课堂
 *
 * 版权声明：源码仅供学习参考，请勿用于商业产品，不保证可靠性。二次开发或其它商用前请联系作者。
 * 注：
 * 1.源码不断升级中，该版本可能非最新版。如需获取最新版，请访问上述网址获取最新版本的代码
 * 2.1500行代码指未包含注释的代码。
 *
 * 如果你在学习本课程之后，对深入研究TCP/IP感兴趣，欢迎关注我的后续课程。我将开发出一套更加深入
 * 详解TCP/IP的课程。采用多线程的方式，实现更完善的功能，包含但不限于
 * 1. IP层的分片与重组
 * 2. Ping功能的实现
 * 3. TCP的流量控制等
 * 4. 基于UDP的TFTP服务器实现
 * 5. DNS域名接触
 * 6. DHCP动态地址获取
 * 7. HTTP服务器
 * ..... 更多功能开发中...........
 * 如果你有兴趣的话，欢迎关注。
 */
#ifndef XNET_TINY_H
#define XNET_TINY_H

#include <stdint.h>

#define XNET_CFG_PACKET_MAX_SIZE	1516

#define XNET_MAC_ADDR_SIZE			6

#define XNET_IPV4_ADDR_SIZE			4

// 报头
#pragma pack(0)
typedef struct _xether_hdr_t {
	uint8_t     dest[XNET_MAC_ADDR_SIZE];
	uint8_t     src[XNET_MAC_ADDR_SIZE];
	uint16_t    protocol;
} xether_hdr_t;
#pragma pack()


typedef enum _xnet_err_t {
	XNET_ERR_OK = 0,
	XNET_ERR_IO = -1,
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
	XNET_PROTOCOL_ARP = 0x0806,
	XNET_PROTOCOL_IP  = 0x0800,
} xnet_protocol_t;




typedef union _xipaddr_t {
	uint8_t    array[XNET_IPV4_ADDR_SIZE];
	uint32_t   addr;
} xipaddr_t;

#define XARP_ENTRY_FREE				0

typedef struct _xarp_entry_t {
	xipaddr_t  ipaddr;						// ip 地址
	uint8_t    macaddr[XNET_MAC_ADDR_SIZE];	// mac 地址
	uint8_t	   state;						// 有效 / 无效 / 请求中
	uint16_t   tmo;							// 超过一定时间重新请求，避免无效 / 错误表项长期存在
	uint8_t    retry_cnt;					// 重试次数
} xarp_entry_t;


void xarp_init(void);


// 驱动打开，发送，读取
xnet_err_t xnet_driver_open(uint8_t* mac_addr);
xnet_err_t xnet_driver_send(xnet_packet_t* packet);
xnet_err_t xnet_driver_read(xnet_packet_t** packet);


#endif // XNET_TINY_H

