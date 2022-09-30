#ifndef XNET_DRIVER_H
#define XNET_DRIVER_H

#include "xnet_define_cfg.h"
#include "xnet_packet.h"

// Э��ջ��ʼ��
void xnet_init(void);
void xnet_poll(void);


// �����򿪣����ͣ���ȡ
xnet_err_t xnet_driver_open(uint8_t* mac_addr);
xnet_err_t xnet_driver_send(xnet_packet_t* packet);
xnet_err_t xnet_driver_read(xnet_packet_t** packet);

#endif // !XNET_DRIVER_H
