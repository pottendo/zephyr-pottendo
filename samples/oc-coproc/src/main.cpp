#include <iostream>
#include <string.h>
#include <pthread.h>

#include "c64-lib.h"
#include "oc-coproc.h"

c64 c64i(0xc000, 0x4000);
oc_coproc cp(c64i);

char *cv = (char *)&c64i.get_mem()[0x4000];

#if 0
void demo(void)
{
    c64i.gfx(VICBank1, VICModeGfxHR, 15);
    for (int i = 0; i < IMG_W / PIXELW - 1; i += 10)
    {
        c64i.circle(cv, i, 100, 99, 1);
        k_sleep(K_MSEC(20));
    }
    k_sleep(K_MSEC(000));

    int c = 1;
    for (int x = 0; x < IMG_W / PIXELW; x += 10)
    {
        c64i.line(cv, 0, 0, x, 199, 1 /*c % 3 + 1*/);
        k_sleep(K_MSEC(150));
//        c64i.line(cv, 0, 0, x, 199, 0);
        c++;
    }
    for (int x = IMG_W / PIXELW - 1; x >= 0; x -= 10)
    {
        c64i.line(cv, x, 199, IMG_W / PIXELW - 1, 0, 1 /*c % 3 + 1*/);
        k_sleep(K_MSEC(150));
//        c64i.line(cv, 0, 0, x, 199, 0);
        c++;
    }
    for (int x = 0; x <= IMG_H / 2; x += 10)
    {
        c64i.line(cv, 0, x, IMG_W / PIXELW - 1, x, 1 /*c % 3 + 1*/);
        k_sleep(K_MSEC(150));
//        c64i.line(cv, 0, 0, x, 199, 0);
        c++;
    }
    for (int x = IMG_H - 1; x >= IMG_H / 2; x -= 10)
    {
        c64i.line(cv, IMG_W / PIXELW - 1, x, 0, x, 1 /*c % 3 + 1*/);
        k_sleep(K_MSEC(150));
//        c64i.line(cv, 0, 0, x, 199, 0);
        c++;
    }
    for (int x = 0; x <= (IMG_W / PIXELW) / 2; x += 10)
    {
        c64i.line(cv, x, 0, x, IMG_H - 1, 1 /*c % 3 + 1*/);
        k_sleep(K_MSEC(150));
//        c64i.line(cv, 0, 0, x, 199, 0);
        c++;
    }
    for (int x = (IMG_W / PIXELW) - 1; x > (IMG_W / PIXELW) / 2; x -= 10)
    {
        c64i.line(cv, x, IMG_H - 1, x, 0, 1 /*c % 3 + 1*/);
        k_sleep(K_MSEC(150));
//        c64i.line(cv, 0, 0, x, 199, 0);
        c++;
    }
    c64i.gfx(VICBank0, VICModeText, 1);
}
#endif

int
main(void)
{
    std::cout << "GFX CoProc...\n";
#ifdef C64    
    std::cout << "C64 memory @0x" << std::hex << int(c64i.get_mem()) << std::dec << '\n';
    c64i.screencols(VIC::GREY, VIC::BLUE);
    memset(cv, 0, 8000);
    memset(&cv[0x3c00], 0x10, 0x3f8);
    memset(&c64i.get_mem()[0xd800], 0, 1000);  
#endif    
    //demo();
    cp.loop();
    std::cout << "done\n";
    return 0;
}
