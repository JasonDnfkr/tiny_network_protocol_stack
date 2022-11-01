## Tiny project for learning network

#### 简介

一个用于学习计算机网络层协议栈的项目，以 npcap 作为驱动程序，从 0 实现了一个简易的 http 服务器。参考的是李述铜的教程。



#### 涉及到的内容

实现了：

- Ethernet II 协议
- ARP 协议
- IPv4 协议 
- UDP 协议
- TCP 协议
- HTTP 报文处理 GET 请求

其中很多功能并不是齐全的，比如 IP 处理不支持 ip 数据包分片，TCP 处理不支持报文重传等。



#### 接下来的目标

用 C++ 重构
