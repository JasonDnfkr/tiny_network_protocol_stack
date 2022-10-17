#include <stdio.h>
#include "xnet_tiny.h"

#include "xnet_tiny/include/xnet_driver.h"
#include "xnet_app/xserver_datetime.h"

int main(void) {
    xnet_init();

    xserver_datetime_create(13);

    printf("xnet running\n");
    while (1) {
        xnet_poll();
    }

    return 0;
}
