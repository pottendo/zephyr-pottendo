#include <stdarg.h>
#include <cstring>
#include <math.h>
#include <vector>

#include "mandel-arch.h"

#ifdef PTHREADS
pthread_mutex_t canvas_sem;
pthread_mutex_t logmutex;
void log_msg(const char *s, ...)
{
    char t[256];
    va_list args;
    pthread_mutex_lock(&logmutex);
    va_start(args, s);
    vsnprintf(t, 256, s, args);
    printf(t);
    pthread_mutex_unlock(&logmutex);
}
#endif  /* PTHREADS */

// globals
int img_w, img_h;   // used by luckfox
int iter = MAX_ITER_INIT;  // used by Amiga
MTYPE xrat = 1.0;

static char *cv;
static char *stacks;

#include "mandelbrot.h"

typedef struct
{
    point_t lu;
    point_t rd;
} rec_t;

int main(void)
{
    pthread_mutex_init(&logmutex, NULL);
    log_msg("Welcome mandelbrot...\n");
    stacks = alloc_stack;
    setup_screen();
    cv = alloc_canvas;
    log_msg("%s: stack_size per thread = %d, no threads=%d, iter = %d, palette = %ld, stacks = %p, cv = %p, CSIZE = %d\n", 
        __FUNCTION__, STACK_SIZE, NO_THREADS, iter, PAL_SIZE, stacks, cv, CSIZE);
#if 0
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
#else
std::vector<rec_t> recs = {
    {{00, IMG_H / 4}, {IMG_W / 2, IMG_H / 4 * 3}},
    {{00, IMG_H / 4}, {IMG_W / 2, IMG_H / 4 * 3}},
    {{IMG_W / 4, IMG_H / 4}, {IMG_W / 4 + 200, IMG_H / 4 * 3}},
};
#endif

    for (int i = 0; i < 1; i++)
    {
        hook1();
        mandel<MTYPE> *m = new mandel<MTYPE>{cv, stacks, 
                                            static_cast<MTYPE>(INTIFY(-1.5)), static_cast<MTYPE>(INTIFY(-1.0)), 
                                            static_cast<MTYPE>(INTIFY(0.5)), static_cast<MTYPE>(INTIFY(1.0)), 
                                            IMG_W / PIXELW, IMG_H, xrat};
        zoom_ui(m);
        //return 0;
        for (size_t i = 0; i < recs.size(); i++)
        {
            auto it = &recs[i];
            log_msg("%d/%d, zooming into [%d,%d]x[%d,%d]...stacks=%p\n", (int)i, (int)recs.size(), it->lu.x, it->lu.y, it->rd.x, it->rd.y, m->get_stacks());
            m->select_start(it->lu);
            m->select_end(it->rd);
        }
        delete m;
        hook2();
    }
#ifndef C64
    delete cv;
    delete stacks;
#endif
    return 0;
}
