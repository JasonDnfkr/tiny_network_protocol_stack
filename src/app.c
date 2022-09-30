#include <stdio.h>
#include "xnet_tiny.h"

#include "xnet_tiny/include/xnet_driver.h"

int main(void) {
    xnet_init();

    printf("xnet running\n");
    while (1) {
        xnet_poll();
    }

    return 0;
}
