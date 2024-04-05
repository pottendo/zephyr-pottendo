#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <cstring>
#include <math.h>
#include <vector>

#define NO_THREADS 16       // max 16 for Orangecart!
#define PIXELW 2 // 2
#define MAX_ITER 128
#define IMG_W 320 // 320
#define IMG_H 200 // 200
#define MTYPE double

#define CSIZE (IMG_W * IMG_H) / 8
#define PAL_SIZE (2 * PIXELW)

// set this to enable direct output on C64 gfx mem.
//#define C64

//#define NO_LOG
#ifdef NO_LOG
#define log_msg(...) 
#else
#define log_msg printf
#endif

#ifdef C64
#include "c64-lib.h"
#else
static char cv[CSIZE] = {};
//char *cv;
#endif

#define STACK_SIZE 1024
#ifdef CONFIG_BOARD_ORANGECART
#if (NO_THREADS > 16)
#error "too many threads for Orangencart's STACK_SIZE"
#endif
static char *stacks = (char *)0x10000000;   // fast SRAM on Orangecart, only 16k! so NO_THREADS <= 16
#else
static char *stacks;
#endif

#include "mandelbrot.h"
MTYPE xrat = 1.0;

typedef struct { point_t lu; point_t rd; } rec_t;
std::vector<rec_t> recs = { 
        {{00, 00},{80,100}}, 
        {{80, 100},{159,199}}, 
        {{00, 50},{40,100}},        
        {{80, 110}, {120, 160}},
        {{60,75}, {100, 125}},
        {{60,110}, {100, 160}},
        {{60,75}, {100, 125}},
        {{60,75}, {100, 125}},
        {{40,50}, {80, 100}},
        {{120,75}, {159, 125}},
    };

int main(void)
{
    log_msg("Welcome mandelbrot...\n");
#ifndef CONFIG_BOARD_ORANGECART
    stacks = (char *) alloca(STACK_SIZE * NO_THREADS); //new char[STACK_SIZE * NO_THREADS]();
    log_msg("%s: stack_size per thread = %d, no threads=%d\n", __FUNCTION__, STACK_SIZE, NO_THREADS);
#endif

#ifdef C64
    c64 c64;
    std::cout << "C64 memory @0x" << std::hex << int(c64.get_mem()) << std::dec << '\n';
    char *cv = (char *)&c64.get_mem()[0x4000];
    c64.screencols(VIC::BLACK, VIC::BLACK);
    c64.gfx(VICBank1, VICModeGfxMC, 15);
    //xrat = 16.0 / 9.0;
#endif
    int col1, col2, col3;
    col1 = 0xb;
    col2 = 0xc;
    col3 = 14; // VIC::LIGHT_BLUE;
    for (int i = 0; i < 16; i++) 
    {
#ifdef C64
            memset(&cv[0x3c00], (col1<<4)|col2, 1000);
            memset(&c64.get_mem()[0xd800], col3, 1000);
#endif 
        mandel<MTYPE> *m = new mandel<MTYPE>{cv, stacks, -1.5, -1.0, 0.5, 1.0, IMG_W / PIXELW, IMG_H, xrat};
        for (size_t i = 0; i < recs.size(); i++)
        {
            auto it = &recs[i];
            //log_msg("Zooming into [%d,%d]x[%d,%d]...stacks=%p\n", it->lu.x, it->lu.y, it->rd.x, it->rd.y, m->get_stacks());
            m->select_start(it->lu);
            m->select_end(it->rd);
            //sleep(2);
        }
        col1++; col2++; col3++;
        col1 %= 0xf; if (col1 == 0) col1++;
        col2 %= 0xf; if (col2 == 0) col2++;
        col3 %= 0xf; if (col3 == 0) col3++;
        delete m;
    }
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
