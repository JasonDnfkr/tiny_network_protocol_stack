#include "xnet_tiny.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

static const xipaddr_t netif_ipaddr      = XNET_CFG_NETIF_IP;
static const uint8_t   ether_broadcast[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

#define min(a, b)           ((a) > (b) ? (b) : (a))

#define swap_order16(v)    ( (((v) & 0xff) << 8) | (((v) >> 8) & 0xff) )
#define xipaddr_is_equal_buf(addr, buf) (memcmp((addr)->array, (buf), XNET_IPV4_ADDR_SIZE) == 0)
#define xipaddr_is_equal(addr1, addr2) ( (addr1)->addr == (addr2)->addr )
#define xipaddr_from_buf(dest, buf) ( (dest)->addr = *(uint32_t*)(buf) )


static uint8_t         netif_mac[XNET_MAC_ADDR_SIZE];

static xnet_packet_t   tx_packet;
static xnet_packet_t   rx_packet;
static xarp_entry_t    arp_entry;
static xnet_time_t     arp_timer;

// 检查 timeout 情况
// 如果第二个参数 sec 为 0，则获取当前时间给 time
int xnet_check_tmo(xnet_time_t* time, uint32_t sec) {
    xnet_time_t curr = xsys_get_time();

    if (sec == 0) {
        *time = curr;
        return 0;
    }
    else if (curr - *time >= sec) {
        *time = curr;
        return 1;
    }

    return 0;
}

/*  -- debug --  */
static void print_arp_packet(const xarp_packet_t* arp_packet) {
    printf("hardware type: %d\n", arp_packet->hw_type);
    printf("protocol type: %d\n", arp_packet->pro_type);
    printf("hardware length: %d\n", arp_packet->hw_len);
    printf("protocol length: %d\n", arp_packet->pro_len);
}




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
static void add_header(xnet_packet_t* packet, uint16_t header_size) {
    packet->data -= header_size;
    packet->size += header_size;
}


// 移除包头，会自动更新 size 和 data 的位置
static void remove_header(xnet_packet_t* packet, uint16_t header_size) {
    packet->data += header_size;
    packet->size -= header_size;
}

static void truncate_packet(xnet_packet_t* packet, uint16_t size) {
    packet->size = min(packet->size, size);
}

static xnet_err_t ethernet_init(void) {
    printf("ethernet_init\n");

    // 驱动将 mac 地址填充至此
    xnet_err_t err = xnet_driver_open(netif_mac);

    if (err < 0) return err;

    //return XNET_ERR_OK;
    return xarp_make_request(&netif_ipaddr);
}


/*
    @para dest_mac_addr 对方的 mac 地址
*/
// 根据协议、mac 地址发出以太网包
static xnet_err_t ethernet_out_to(xnet_protocol_t protocol, const uint8_t* dest_mac_addr, xnet_packet_t* packet) {
    xether_hdr_t* ether_hdr;

    add_header(packet, sizeof(xether_hdr_t));
    ether_hdr = (xether_hdr_t*)packet->data;

    memcpy(ether_hdr->dest, dest_mac_addr, XNET_MAC_ADDR_SIZE);
    memcpy(ether_hdr->src, netif_mac, XNET_MAC_ADDR_SIZE);
    ether_hdr->protocol = swap_order16(protocol);

    //xnet_err_t res = xnet_driver_send(packet);
    //return res;
    return xnet_driver_send(packet);
}

// 根据 ip 发出以太网包
static xnet_err_t ethernet_out(xipaddr_t* dest_ip, xnet_packet_t* packet) {
    xnet_err_t err;
    uint8_t*   mac_addr = NULL;

    if ((err = xarp_resolve(dest_ip, &mac_addr)) == XNET_ERR_OK) {
        return ethernet_out_to(XNET_PROTOCOL_IP, mac_addr, packet);
    }

    return err;
}


static void ethernet_in(xnet_packet_t* packet) {
    // 报文不可能小于 报头 大小
    if (packet->size <= sizeof(xether_hdr_t)) {
        return;
    }

    uint16_t protocol;
    xether_hdr_t* ether_hdr = (xether_hdr_t*)packet->data;
    protocol = swap_order16(ether_hdr->protocol);

    switch (protocol) {
    case XNET_PROTOCOL_ARP:
        printf("received ARP protocol\n");
        remove_header(packet, sizeof(xether_hdr_t));
        xarp_in(packet);
        break;
    case XNET_PROTOCOL_IP:
        remove_header(packet, sizeof(xether_hdr_t));
        xip_in(packet);
        break;
    }
}


static void ethernet_poll(void) {
    xnet_packet_t* packet;

    if (xnet_driver_read(&packet) == XNET_ERR_OK) {
        ethernet_in(packet);
    }
}

void xarp_init(void) {
    arp_entry.state = XARP_ENTRY_FREE;
    xnet_check_tmo(&arp_timer, 0);
}

// 发送 ARP REQUEST 包
xnet_err_t xarp_make_request(const xipaddr_t* ipaddr) {
    printf("\nxarp_make_request : %d\n", xsys_get_time());

    xnet_packet_t* packet     = xnet_alloc_for_send(sizeof(xarp_packet_t));
    xarp_packet_t* arp_packet = (xarp_packet_t*)packet->data;

    arp_packet->hw_type  = swap_order16(XARP_HW_ETHER);
    arp_packet->pro_type = swap_order16(XNET_PROTOCOL_IP);
    arp_packet->hw_len   = XNET_MAC_ADDR_SIZE;
    arp_packet->pro_len  = XNET_IPV4_ADDR_SIZE;
    arp_packet->opcode   = swap_order16(XARP_REQUEST);

    memcpy(arp_packet->sender_mac, netif_mac, XNET_MAC_ADDR_SIZE);
    memcpy(arp_packet->sender_ip, netif_ipaddr.array, XNET_IPV4_ADDR_SIZE);
    memset(arp_packet->target_mac, 0, XNET_MAC_ADDR_SIZE);
    memcpy(arp_packet->target_ip, ipaddr->array, XNET_IPV4_ADDR_SIZE);
    
    print_arp_packet(arp_packet);
    
    return ethernet_out_to(XNET_PROTOCOL_ARP, ether_broadcast, packet);
}


// 发送 ARP RESPONSE 包
xnet_err_t xarp_make_response(xarp_packet_t* arp_packet) {
    printf("\nxarp_make_response : %d\n", xsys_get_time());

    xnet_packet_t* packet = xnet_alloc_for_send(sizeof(xarp_packet_t));
    xarp_packet_t* response_packet = (xarp_packet_t*)packet->data;

    response_packet->hw_type = swap_order16(XARP_HW_ETHER);
    response_packet->pro_type = swap_order16(XNET_PROTOCOL_IP);
    response_packet->hw_len = XNET_MAC_ADDR_SIZE;
    response_packet->pro_len = XNET_IPV4_ADDR_SIZE;
    response_packet->opcode = swap_order16(XARP_REPLY);

    memcpy(response_packet->sender_mac, netif_mac, XNET_MAC_ADDR_SIZE);
    memcpy(response_packet->sender_ip, netif_ipaddr.array, XNET_IPV4_ADDR_SIZE);
    memcpy(response_packet->target_mac, arp_packet->sender_mac, XNET_MAC_ADDR_SIZE);
    memcpy(response_packet->target_ip, arp_packet->sender_ip, XNET_IPV4_ADDR_SIZE);

    print_arp_packet(response_packet);

    return ethernet_out_to(XNET_PROTOCOL_ARP, arp_packet->sender_mac, packet);
}


// 从 ip 地址解析出 mac 地址
xnet_err_t xarp_resolve(const xipaddr_t* ipaddr, uint8_t** mac_addr) {
    if ((arp_entry.state == XARP_ENTRY_OK) && xipaddr_is_equal(ipaddr, &arp_entry.ipaddr)) {
        *mac_addr = arp_entry.macaddr;
        return XNET_ERR_OK;
    }

    xarp_make_request(ipaddr);
    return XNET_ERR_NONE;
}


// 更新 ARP 表
static void update_arp_entry(uint8_t* src_ip, uint8_t* mac_addr) {
    memcpy(arp_entry.ipaddr.array, src_ip, XNET_IPV4_ADDR_SIZE);
    memcpy(arp_entry.macaddr, mac_addr, XNET_MAC_ADDR_SIZE);
    arp_entry.state     = XARP_ENTRY_OK;
    arp_entry.tmo       = XARP_CFG_ENTRY_OK_TMO;
    arp_entry.retry_cnt = XARP_CFG_MAX_RETRIES;
}


// 接收 ARP 包
void xarp_in(xnet_packet_t* packet) {
    if (packet->size >= sizeof(xarp_packet_t)) {
        xarp_packet_t* arp_packet = (xarp_packet_t*)packet->data;
        uint16_t opcode = swap_order16(arp_packet->opcode);

        if ((swap_order16(arp_packet->hw_type) != XARP_HW_ETHER) ||
            (arp_packet->hw_len != XNET_MAC_ADDR_SIZE) ||
            (swap_order16(arp_packet->pro_type) != XNET_PROTOCOL_IP) ||
            (arp_packet->pro_len != XNET_IPV4_ADDR_SIZE)
            || ((opcode != XARP_REQUEST) && (opcode != XARP_REPLY))) {
            printf("invalid arp packet\n");
            return;
        }

        //if (swap_order16(arp_packet->hw_type) != XARP_HW_ETHER) {
        //    printf("swap_order16(arp_packet->hw_type) != XARP_HW_ETHER\n");
        //    printf("hw type(swaped): %d    XARP_HW_ETHER: %d\n", swap_order16(arp_packet->hw_type), XARP_HW_ETHER);
        //    return;
        //}
        //if (arp_packet->hw_len != XNET_MAC_ADDR_SIZE) {
        //    printf("arp_packet->hw_len != XNET_MAC_ADDR_SIZE\n");
        //    printf("hw len: %d    XNET_MAC_ADDR_SIZE: %d\n", arp_packet->hw_len, XNET_MAC_ADDR_SIZE);
        //    return;
        //}
        //if (swap_order16(arp_packet->pro_type) != XNET_PROTOCOL_IP) {
        //    printf("swap_order16(arp_packet->pro_type) != XNET_PROTOCOL_IP\n");
        //    printf("pro type(swaped): %d    XNET_PROTOCOL_IP: %d\n", swap_order16(arp_packet->pro_type), XNET_PROTOCOL_IP);
        //    return;
        //}
        //if (arp_packet->pro_len != XNET_IPV4_ADDR_SIZE) {
        //    printf("arp_packet->pro_len != XNET_IPV4_ADDR_SIZE\n");
        //    printf("pro len: %d    XNET_IPV4_ADDR_SIZE %d\n", arp_packet->pro_len, XNET_IPV4_ADDR_SIZE);
        //    return;
        //}
        //if ((opcode != XARP_REQUEST) && (opcode != XARP_REPLY)) {
        //    printf("(opcode != XARP_REQUEST) && (opcode != XARP_REPLY)\n");
        //    printf("opcode: %d\n   XARP_REQUEST: %d,  XARP_REPLY: %d\n", opcode, XARP_REQUEST, XARP_REPLY);
        //    return;
        //}

        if (!xipaddr_is_equal_buf(&netif_ipaddr, arp_packet->target_ip)) {
            return;
        }

        switch (opcode) {
        case XARP_REQUEST:
            xarp_make_response(arp_packet);
            update_arp_entry(arp_packet->sender_ip, arp_packet->sender_mac);
            break;
        case XARP_REPLY:
            break;
        }
    }
}

// arp 表 存在时间查询
void xarp_poll(void) {
    if (xnet_check_tmo(&arp_timer, XARP_TIMER_PERIOD)) {
        switch (arp_entry.state) {
        case XARP_ENTRY_OK:
            if (--arp_entry.tmo == 0) { // timeout 时间为 0
                printf("\n[arp time out. change arp status to pending.]\n");
                xarp_make_request(&arp_entry.ipaddr); // 超时重传
                arp_entry.state = XARP_ENTRY_PENDING; // 设置为等待状态
                arp_entry.tmo   = XARP_CFG_ENTRY_PENDING_TMO; // 将 timeout 设置为标准值
                printf("[arp time out. request resend.]\n");
            }
            break;

        case XARP_ENTRY_PENDING:
            if (--arp_entry.tmo == 0) {
                printf("[retry_cnt: %d]\n", arp_entry.retry_cnt);
                if (arp_entry.retry_cnt-- == 0) {
                    printf("[retry_cnt is 0 now. ARP_FREE]\n");
                    arp_entry.state = XARP_ENTRY_FREE;
                }
                else {
                    xarp_make_request(&arp_entry.ipaddr); // 超时重传
                    arp_entry.state = XARP_ENTRY_PENDING; // 设置为等待状态
                    arp_entry.tmo   = XARP_CFG_ENTRY_PENDING_TMO; // 将 timeout 设置为标准值
                    printf("[arp time out. request resend.]\n");
                }
            }
        case XARP_ENTRY_FREE:
            printf(".");
            break;
        }
    }
}


// 校验和
static uint16_t checksum16(uint16_t* buf, uint16_t len, uint16_t pre_sum, int complement) {
    uint32_t checksum = 0; //pre_sum
    uint16_t high;

    while (len > 1) {
        checksum += *buf++;
        len -= 2;
    }

    if (len > 0) {
        checksum += *(uint8_t*)buf;
    }

    while ((high = checksum >> 16) != 0) {
        checksum = high + (checksum & 0xffff);
    }

    return (uint16_t)~checksum;
}

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

    if (!xipaddr_is_equal_buf(&netif_ipaddr, iphdr->dest_ip)) {
        return;
    }

    xipaddr_from_buf(&src_ip, iphdr->src_ip);

    switch (iphdr->protocol) {
    case XNET_PROTOCOL_ICMP:
        remove_header(packet, header_size);
        xicmp_in(&src_ip, packet);
        break;
    default:
        xicmp_dest_unreach(XICMP_CODE_PRO_UNREACH, iphdr);
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

    iphdr->hdr_checksum   = 0;
    iphdr->hdr_checksum   = checksum16((uint16_t*)iphdr, sizeof(xip_hdr_t), 0, 1);
    
    ip_packet_id++;

    return ethernet_out(dest_ip, packet);
}


void xicmp_init(void) { 

}


static xnet_err_t reply_icmp_request(xicmp_hdr_t* icmp_hdr, xipaddr_t* src_ip, xnet_packet_t* packet) {
    xnet_packet_t* tx = xnet_alloc_for_send(packet->size);
    // 传进来的 packet 是一个完整的带 icmp 报头的数据包
    // 所以指针可以直接指向 data
    xicmp_hdr_t* reply_hdr = (xicmp_hdr_t*)tx->data;

    reply_hdr->type     = XICMP_CODE_ECHO_REPLY;
    reply_hdr->code     = 0;
    reply_hdr->id       = icmp_hdr->id;
    reply_hdr->seq      = icmp_hdr->seq;
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

    uint16_t ip_hdr_size  = ip_hdr->hdr_len * 4;
    uint16_t ip_data_size = swap_order16(ip_hdr->total_len) - ip_hdr_size;
    ip_data_size = ip_hdr_size + min(ip_data_size, 8);

    packet = xnet_alloc_for_send(sizeof(xicmp_hdr_t) + ip_data_size);

    icmp_hdr       = (xicmp_hdr_t*)packet->data;
    icmp_hdr->type = XICMP_TYPE_UNREACH;
    icmp_hdr->id   = 0;
    icmp_hdr->seq  = 0;
    memcpy((uint8_t*)icmp_hdr + sizeof(xicmp_hdr_t), ip_hdr, ip_data_size);
    icmp_hdr->checksum = 0;
    icmp_hdr->checksum = checksum16((uint16_t*)icmp_hdr, packet->size, 0, 0);

    xipaddr_from_buf(&dest_ip, ip_hdr->src_ip);

    return xip_out(XNET_PROTOCOL_ICMP, &dest_ip, packet);
}




void xnet_init(void) {
    ethernet_init();
    xarp_init();
    xip_init();
    xicmp_init();
}


void xnet_poll(void) {
    // 调用以太网查询函数看看有没有包
    // 有包则去 ethernet_poll() 处理
    ethernet_poll();
    xarp_poll();
}


const xnet_time_t xsys_get_time(void) {
    return clock() / CLOCKS_PER_SEC;
}

