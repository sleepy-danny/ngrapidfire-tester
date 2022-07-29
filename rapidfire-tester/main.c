#include <ngdevkit/neogeo.h>
#include <ngdevkit/ng-fix.h>
#include <stdio.h>

int main(void) {
    ng_cls();

    // Set up a minimal palette
    const u16 palette[]={0x8000, 0x0fff, 0x0555};
    for (u16 i=0; i<3; i++) {
        MMAP_PALBANK1[i]=palette[i];
    }

    const char hello[] = "Hello world!";
    ng_text(2, 13, 0, hello);

    while (1);
    return 0;
}

