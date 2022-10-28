#include <stdio.h>
#include "xnet_tiny.h"

#include "xnet_tiny/include/xnet_driver.h"
#include "xnet_app/xserver_datetime.h"
#include "xnet_app/xserver_http.h"
#include "xnet_app/xtimer.h"

int main(void) {
    xnet_init();

    xserver_datetime_create(13);
    xserver_http_create(80);
    timer_create(9527);

    printf("xnet running\n");
    while (1) {
        xnet_poll();
    }

    return 0;
}
