#include <ngdevkit/neogeo.h>
#include "rftester.h"

volatile u16 vblank = 0;
void rom_callback_VBlank() {
    vblank = 1;
    rftester_vblank_cb();
}

void ng_wait_vblank() {
    while (!vblank);
    vblank = 0;
}

int main(void) {
    rftester_init();

    while (1) {
        rftester_process_state();
    }
    return 0;
}

