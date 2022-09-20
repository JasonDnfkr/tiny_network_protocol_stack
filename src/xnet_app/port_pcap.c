#include "pcap_device.h"
#include "xnet_tiny.h"

#include <stdlib.h>
#include <string.h>

static pcap_t* pcap;

// VirtualBox 在本地设置的虚拟网卡的地址
const char* ip_str = "192.168.254.1";

// 给虚拟机指定一个 mac 网卡地址
const char my_mac_addr[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };


// 由驱动将 mac 地址写入参数，
// 供上层调用
xnet_err_t xnet_driver_open(uint8_t* mac_addr) {
	memcpy(mac_addr, my_mac_addr, sizeof(my_mac_addr));
	// 打开网卡指定的 ip，设置网卡的 mac 地址
	pcap = pcap_device_open(ip_str, my_mac_addr, 1);
	if (pcap == (pcap_t*) 0) {
		exit(-1);
	}

	return XNET_ERR_OK;
}


xnet_err_t xnet_driver_send(xnet_packet_t* packet) {
	return pcap_device_send(pcap, packet->data, packet->size);
}


xnet_err_t xnet_driver_read(xnet_packet_t** packet) {
	uint16_t size;
	xnet_packet_t* r_packet = xnet_alloc_for_read(XNET_CFG_PACKET_MAX_SIZE);
	size = pcap_device_read(pcap, r_packet->data, XNET_CFG_PACKET_MAX_SIZE);
	if (size) {
		r_packet->size = size;
		*packet = r_packet;
		return XNET_ERR_OK;
	}

	return XNET_ERR_IO;
}