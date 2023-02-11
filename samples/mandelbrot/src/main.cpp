#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <cstring>
#include <math.h>

#define NO_THREADS 16       // max 16 for Orangecart!
#define STACK_SIZE 1024
#define PIXELW 2 // 2
#define MAX_ITER 24
#define IMG_W 320 // 320
#define IMG_H 200 // 200
#define MTYPE double

#define CSIZE (IMG_W * IMG_H) / 8
#define PAL_SIZE (2 * PIXELW)

//#define C64
#ifdef C64
#include "c64-lib.h"
#else
static char cv[CSIZE] = {};
#endif

#ifdef __ZEPHYR__
#define M_PI            3.14159265358979323846
#if (NO_THREADS > 16)
#error "too many threads for Orangencart's STACK_SIZE"
#endif
static char *stacks = (char *)0x10000000;   // fast SRAM on Orangecart, only 16k! so NO_THREADS <= 16

#else

#undef STACK_SIZE 
#define STACK_SIZE PTHREAD_STACK_MIN
static char *stacks;

#endif

#include "mandelbrot.h"

int main(int argc, char *argv[])
{
#ifndef __ZEPHYR__
    stacks = new char[STACK_SIZE * NO_THREADS]();
#endif
#ifdef C64
    c64 c64;
    std::cout << "C64 memory @0x" << std::hex << int(c64.get_mem()) << std::dec << '\n';
    char *cv = (char *)&c64.get_mem()[0x4000];
    c64.screencols(VIC::BLACK, VIC::BLUE);
    c64.gfx(VICBank1, VICModeGfxMC, 15);
    memset(cv, 0, 8000);
    memset(&cv[0x3c00], 0xbc, 0x3f8);
    memset(&c64.get_mem()[0xd800], 0x98, 1000);
#endif    
    mandel<MTYPE> *m = new mandel<MTYPE>{cv, stacks, -1.5, -1.0, 0.5, 1.0, IMG_W / PIXELW, IMG_H};
    delete m;
#ifdef __ZEPHYR__
    while(1)
    {
        std::cout << "system halted.\n";
        sleep(10);
    }
#endif
    sleep(2);
    return 0;
}
