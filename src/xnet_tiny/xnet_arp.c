#include "include/xnet_arp.h"
#include "include/xnet_ether.h"
#include "include/xnet_ether.h"

#include <stdio.h>
#include <string.h>

extern const xipaddr_t netif_ipaddr;
extern const uint8_t ether_broadcast[];

/*  -- debug --  */
static void print_arp_packet(const xarp_packet_t* arp_packet) {
    printf("hardware type: %d\n", arp_packet->hw_type);
    printf("protocol type: %d\n", arp_packet->pro_type);
    printf("hardware length: %d\n", arp_packet->hw_len);
    printf("protocol length: %d\n", arp_packet->pro_len);
}

// ����1����ȡ��ǰϵͳʱ��� time
// ����2���жϾ���һ�ε��øú������Ƿ������� sec
// �����ڣ��򷵻� 1
// �����������ʱ������
// ��� timeout ���
// ����ڶ������� sec Ϊ 0�����ȡ��ǰʱ��� time
static int xnet_check_tmo(xnet_time_t* time, uint32_t sec) {
    xnet_time_t curr = xsys_get_time();

    // ����1����ȡ��ǰϵͳʱ��� time
    if (sec == 0) {
        *time = curr;
        return 0;
    }
    
    // ����2���жϾ���һ�ε��øú������Ƿ������� sec
    // �����ڣ��򷵻� 1
    // �����������ʱ������
    // �����ڵ�ʱ���ȥ�ϴ�ʱ�䣬��ֵ������ sec ָ����ֵ
    // �����ʱ�䣬������1
    if (curr - *time >= sec) {
        *time = curr;
        return 1;
    }

    return 0;
}


void xarp_init(void) {
    arp_entry.state = XARP_ENTRY_FREE;
    xnet_check_tmo(&arp_timer, 0);
}

// ���� ARP REQUEST ��
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

    //print_arp_packet(arp_packet);

    return ethernet_out_to(XNET_PROTOCOL_ARP, ether_broadcast, packet);
}


// ���� ARP RESPONSE ��
xnet_err_t xarp_make_response(xarp_packet_t* arp_packet) {
    printf("\nxarp_make_response : %d\n", xsys_get_time());

    xnet_packet_t* packet = xnet_alloc_for_send(sizeof(xarp_packet_t));
    xarp_packet_t* response_packet = (xarp_packet_t*)packet->data;

    response_packet->hw_type  = swap_order16(XARP_HW_ETHER);
    response_packet->pro_type = swap_order16(XNET_PROTOCOL_IP);
    response_packet->hw_len   = XNET_MAC_ADDR_SIZE;
    response_packet->pro_len  = XNET_IPV4_ADDR_SIZE;
    response_packet->opcode   = swap_order16(XARP_REPLY);

    memcpy(response_packet->sender_mac, netif_mac, XNET_MAC_ADDR_SIZE);
    memcpy(response_packet->sender_ip, netif_ipaddr.array, XNET_IPV4_ADDR_SIZE);
    memcpy(response_packet->target_mac, arp_packet->sender_mac, XNET_MAC_ADDR_SIZE);
    memcpy(response_packet->target_ip, arp_packet->sender_ip, XNET_IPV4_ADDR_SIZE);

    //print_arp_packet(response_packet);

    return ethernet_out_to(XNET_PROTOCOL_ARP, arp_packet->sender_mac, packet);
}


// �� ip ��ַ������ mac ��ַ
xnet_err_t xarp_resolve(const xipaddr_t* ipaddr, uint8_t** mac_addr) {
    if ((arp_entry.state == XARP_ENTRY_OK) && xipaddr_is_equal(ipaddr, &arp_entry.ipaddr)) {
        *mac_addr = arp_entry.macaddr;
        return XNET_ERR_OK;
    }

    printf("arp resolved failed. a arp request package will be sent\n");
    xarp_make_request(ipaddr);
    return XNET_ERR_NONE;
}


// ���� ARP ��
static void update_arp_entry(uint8_t* src_ip, uint8_t* mac_addr) {
    memcpy(arp_entry.ipaddr.array, src_ip, XNET_IPV4_ADDR_SIZE);
    memcpy(arp_entry.macaddr, mac_addr, XNET_MAC_ADDR_SIZE);
    arp_entry.state     = XARP_ENTRY_OK;
    arp_entry.tmo       = XARP_CFG_ENTRY_OK_TMO;
    arp_entry.retry_cnt = XARP_CFG_MAX_RETRIES;
}


// ���� ARP ��
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

        if (!xipaddr_is_equal_buf(&netif_ipaddr, arp_packet->target_ip)) {
            return;
        }

        switch (opcode) {
        case XARP_REQUEST:
            printf("received ARP request package\n");
            xarp_make_response(arp_packet);
            update_arp_entry(arp_packet->sender_ip, arp_packet->sender_mac);
            break;

        case XARP_REPLY:
            update_arp_entry(arp_packet->sender_ip, arp_packet->sender_mac);
            printf("received ARP reply package\n");
            break;
        }
    }
}

// arp �� ����ʱ���ѯ
void xarp_poll(void) {
    if (xnet_check_tmo(&arp_timer, XARP_TIMER_PERIOD)) {
        printf("[arp_entry timeout: %d]\n", arp_entry.tmo);
        switch (arp_entry.state) {
        case XARP_ENTRY_OK:
            arp_entry.tmo--;
            if (arp_entry.tmo == 0) { // timeout ʱ��Ϊ 0
                printf("\n[arp time out. change arp status to pending.]\n");
                xarp_make_request(&arp_entry.ipaddr); // ��ʱ�ش�
                arp_entry.state = XARP_ENTRY_PENDING; // ����Ϊ�ȴ�״̬
                arp_entry.tmo = XARP_CFG_ENTRY_PENDING_TMO; // �� timeout ����Ϊ��׼ֵ
                printf("[arp time out. request resend.]\n");
            }
            break;

        case XARP_ENTRY_PENDING:
            arp_entry.tmo--;
            if (arp_entry.tmo == 0) {
                printf("[retry_cnt: %d]\n", arp_entry.retry_cnt);
                if (arp_entry.retry_cnt-- == 0) {
                    printf("[retry_cnt is 0 now. ARP_FREE]\n");
                    arp_entry.state = XARP_ENTRY_FREE;
                }
                else {
                    xarp_make_request(&arp_entry.ipaddr); // ��ʱ�ش�
                    arp_entry.state = XARP_ENTRY_PENDING; // ����Ϊ�ȴ�״̬
                    arp_entry.tmo = XARP_CFG_ENTRY_PENDING_TMO; // �� timeout ����Ϊ��׼ֵ
                    printf("[arp time out. request resend.]\n");
                }
            }
            break;

        case XARP_ENTRY_FREE:
            break;
        }
    }
}