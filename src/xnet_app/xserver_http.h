#ifndef XSERVER_HTTP_H
#define XSERVER_HTTP_H

#include <include/xnet_define_cfg.h>

#define XHTTP_DOC_PATH		"E:\\Data\\Work\\Job\\Cpp\\webserver\\Learning-DIY-TCPIP_WEB\\xnet_tiny_c0000\\htdocs"

xnet_err_t xserver_http_create(uint16_t port);

void xserver_http_run(void);


#endif // !XSERVER_HTTP_H
