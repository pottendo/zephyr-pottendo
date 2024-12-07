/* -*-c++-*-
 * This file is part of pottendos-playground.
 *
 * FE playground is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FE playground is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FE playground.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef __MANDELBROT_H__
#define __MANDELBROT_H__

#include <stdlib.h>
#include <unistd.h>
#include <complex>
// #include <iostream>
#include <signal.h>

#include <time.h>
#include <cstdint>
typedef struct
{
    int x;
    int y;
} point_t;

#ifdef PTHREADS
#include <pthread.h>
#include <semaphore.h>
extern pthread_mutex_t canvas_sem;
#define P P_
#define V V_
#else
// make those calls dummies
#define pthread_mutex_init(...)
#define pthread_mutex_destroy(...)
#define pthread_mutex_lock(...)
#define pthread_mutex_unlock(...)
#define sem_wait(...)
#define sem_post(...)
#define sem_init(...)
#define sem_destroy(...)
#define pthread_getschedparam(...)
#define pthread_attr_setschedpolicy(...)
#define pthread_attr_init(...)
#define pthread_attr_destroy(...) 0
#define pthread_attr_setstack(...) 0
#define pthread_create(...) 0
#define pthread_detach(...) 0
#define pthread_exit(...)
#define sem_t int
#define pthread_t int
#define pthread_mutex_t int
#define sched_yield(...)
#define P(...)
#define V(...)
#endif /* PTHREADS */


template <typename myDOUBLE>
class mandel
{
    typedef char *canvas_t;
    typedef uint16_t coord_t;
    typedef uint16_t color_t;
    struct tparam_t
    {
        int tno, width, height;
        myDOUBLE xl, yl, xh, yh, incx, incy;
        int xoffset, yoffset;
        pthread_mutex_t go;
        sem_t &sem;
        mandel<myDOUBLE> *mo;

        tparam_t(int t, int w, int h, myDOUBLE x1, myDOUBLE y1, myDOUBLE x2, myDOUBLE y2, myDOUBLE ix, myDOUBLE iy,
                 int xo, int yo, sem_t &se, mandel<myDOUBLE> *th)
            : tno(t), width(w), height(h), xl(x1), yl(y1), xh(x2), yh(y2), incx(ix), incy(iy), xoffset(xo), yoffset(yo), sem(se), mo(th)
        {
            pthread_mutex_init(&go, nullptr);
            P(go); // lock it
            // std::cout << *this << '\n';
        }
        ~tparam_t()
        {
            // log_msg("cleaning up params\n");
            pthread_mutex_destroy(&go);
        }
        friend std::ostream &operator<<(std::ostream &ostr, tparam_t &t)
        {
            ostr << "t=" << t.tno << ", w=" << t.width << ",h=" << t.height
                 << ",xl=" << t.xl << ",yl=" << t.yl << ",xh=" << t.xh << ",yh=" << t.yh
                 << ",incx=" << t.incx << ",incy=" << t.incy
                 << ",xo=" << t.xoffset << ",yo=" << t.yoffset << ", go: " << t.go;
            return ostr;
        }
    };

    tparam_t *tp[NO_THREADS];
    // char *stacks[NO_THREADS];
    int &max_iter = MAX_ITER;
    uint8_t _mask;

    /* class local variables */
#ifdef PTHREADS
    pthread_attr_t attr[NO_THREADS];
#endif
    pthread_t worker_tasks[NO_THREADS];
    sem_t master_sem;

    canvas_t canvas;
    char *stacks;
    int xres, yres;
    color_t col_pal[PAL_SIZE];// = {0, 1, 2, 3};
    coord_t mark_x1, mark_y1, mark_x2, mark_y2;
    myDOUBLE last_xr, last_yr, ssw, ssh, transx, transy, xratio;
    int stop;
    struct timespec tstart, tend;

    static void P_(pthread_mutex_t &m)
    {
        pthread_mutex_lock(&m);
    }
    static void V_(pthread_mutex_t &m)
    {
        pthread_mutex_unlock(&m);
    }
    static void PSem(sem_t &s)
    {
        sem_wait(&s);
    }
    static void VSem(sem_t &s)
    {
        sem_post(&s);
    }

    void canvas_setpx_(canvas_t &canvas, coord_t x, coord_t y, color_t c)
    {
        uint32_t h = x % (8 / PIXELW);
        uint32_t shift = (8 / PIXELW - 1) - h;
        uint32_t val = (c << (shift * PIXELW));
        uint8_t mask = ~(_mask << (shift * PIXELW));
        const uint32_t lineb = IMG_W / 8 * 8; // line 8 bytes per 8x8 pixel
        const uint32_t colb = 8;              // 8 bytes per 8x8 pixel

        // log_msg("x/y %d/%d offs %d/%d\n", x, y, (x / (8 / PIXELW)) * colb, (y / 8) * lineb  + (y % 8));
        uint32_t cidx = (y / 8) * lineb + (y % 8) + (x / (8 / PIXELW)) * colb;
        if (cidx >= (uint32_t)CSIZE)
        {
            // log_msg("Exceeding canvas!! %d, %d/%d\n", cidx, x, y);
            // delay (100 * 1000);
            return;
        }
        char t = canvas[cidx];
        t &= mask;
        t |= val;
        canvas[cidx] = t;
#ifdef __ZEPHYR__
        // volatile char *led= (char *)0xf0002000;
        //*led = ((*led) + 1);
#endif
        // sched_yield();
    }

    void dump_bits(uint8_t c)
    {
        for (int i = 7; i >= 0; i--)
        {
            log_msg("%c", (c & (1 << i)) ? '*' : '.');
        }
    }

    void canvas_dump(canvas_t c)
    {
        for (int y = 0; y < IMG_H; y++)
        {
            log_msg("%03d: ", y);
            int offs = (y / 8) * IMG_W + (y % 8);
            for (auto i = 0; i < IMG_W / 1; i += 8)
            {
                // log_msg("idx: %032d ", offs + i);
                dump_bits(c[offs + i]);
            }
            log_msg("\n");
        }
    }

    /* class private functinos */
    inline void timespec_diff(struct timespec *a, struct timespec *b, struct timespec *result)
    {
        result->tv_sec = a->tv_sec - b->tv_sec;
        result->tv_nsec = a->tv_nsec - b->tv_nsec;
        if (result->tv_nsec < 0)
        {
            --result->tv_sec;
            result->tv_nsec += 1000000000L;
        }
    }

    // abs() would calc sqr() as well, we don't need that for this fractal
    inline myDOUBLE abs2(std::complex<myDOUBLE> f)
    {
        myDOUBLE r = f.real(), i = f.imag();
        return r * r + i * i;
    }

    int mandel_calc_point(myDOUBLE x, myDOUBLE y)
    {
        const std::complex<myDOUBLE> point{x, y};
        // std::cout << "calc: " << point << '\n';
        std::complex<myDOUBLE> z = point;
        int nb_iter = 1;
        while (abs2(z) < INTIFY2(4) && nb_iter <= max_iter)
        {
            z = (DEINTIFY(z * z) + point);
            nb_iter++;
        }
        if (nb_iter < max_iter)
            return (col_pal[(nb_iter % (PAL_SIZE - 1)) + 1]);
        else
            return 0;
    }

    void mandel_helper(myDOUBLE xl, myDOUBLE yl, myDOUBLE xh, myDOUBLE yh, myDOUBLE incx, myDOUBLE incy, int xo, int yo, int width, int height)
    {
        myDOUBLE x, y;
        int xk = xo;
        int yk = yo;
        if ((xl == xh) || (yl == yh))
        {
            //log_msg("assertion failed: xl=%d, xh=%d, yl=%d, yh=%d\n", xl, xh, yl, yh);
            log_msg("assertion failed - reached fixp/fpu limit!\n");
            return;
        }
        x = xl;
        for (xk = 0; xk < width; xk++)
        {
            y = yl;
            for (yk = 0; yk < height; yk++)
            {
                int d = mandel_calc_point(x, y);
                P(canvas_sem);
#ifdef __amiga__
                int amiga_setpixel(void *, int x, int y, int col);
                if (stop || (stop = amiga_setpixel(NULL, xk + xo, yk + yo, d)))
                {
                    V(canvas_sem);
                    goto out;
                }
#else
                canvas_setpx(canvas, xk + xo, yk + yo, d);
#endif
                V(canvas_sem);
                y += incy;
            }
            x += incx;
        }
#ifdef __amiga__	
        out:
        ;
#endif	
    }

    static void *mandel_wrapper(void *param)
    {
        tparam_t *p = static_cast<tparam_t *>(param);
        p->mo->mandel_wrapper_2(param);
        pthread_exit(nullptr);
        return nullptr;
    }

    int mandel_wrapper_2(void *param)
    {
        tparam_t *p = (tparam_t *)param;

#if defined(__ZEPHYR__) && defined(CONFIG_FPU)
        // make sure FPU regs are saved during context switch
        int r;
        if ((r = k_float_enable(k_current_get(), 0)) != 0)
        {
            log_msg("%s: k_float_enable() failed: %d.\n", __FUNCTION__, r);
        }
#endif
        P(p->go);
#ifdef PTHREADS
#ifdef __linux__
        sched_param sp;
        int pol = -1;
        int ret;
        ret = pthread_getschedparam(worker_tasks[p->tno], &pol, &sp);
        if (ret != 0)
            log_msg("ptherad_getschedparam()... failed: %d\n", ret);
        sp.sched_priority = sp.sched_priority + (p->tno % 3) + 1;
        if ((ret = pthread_setschedparam(worker_tasks[p->tno], SCHED_RR, &sp)) != 0)
            log_msg("pthread setschedparam (pol=%d) failed for thread %d, %d - need sudo!\n", SCHED_RR, p->tno, ret);
        pthread_getschedparam(worker_tasks[p->tno], &pol, &sp);
        log_msg("starting thread %d with priority %d\n", p->tno, sp.sched_priority);
#endif
#endif
        sched_yield();
        mandel_helper(p->xl, p->yl, p->xh, p->yh, p->incx, p->incy, p->xoffset, p->yoffset, p->width, p->height);
        //log_msg("finished thread %d\n", p->tno);
        VSem(p->sem); // report we've done our job
        return 0;
    }


    void mandel_setup(const int thread_no, myDOUBLE sx, myDOUBLE sy, myDOUBLE tx, myDOUBLE ty)
    {
        int t = 0;
        mandel_presetup(sx, sy, tx, ty);
        myDOUBLE stepx = (xres / thread_no) * ssw * xratio;
        myDOUBLE stepy = (yres / thread_no) * ssh;
        pthread_t th = (pthread_t)0;

        if (thread_no > 16)
        {
            if (thread_no != 100) /* 100 is a dummy to just initialize, don't confuse us with this log */
                log_msg("%s: too many threads(%d)... giving up.\n", __FUNCTION__, thread_no);
            return;
        }
        int w = (xres / thread_no);
        int h = (yres / thread_no);

        for (int tx = 0; tx < thread_no; tx++)
        {
            int xoffset = w * tx;
            for (int ty = 0; ty < thread_no; ty++)
            {
                int ret;
                int yoffset = h * ty;
                tp[t] = new tparam_t(t,
                                     w, h,
                                     tx * stepx + transx,
                                     ty * stepy + transy,
                                     tx * stepx + stepx + transx,
                                     ty * stepy + stepy + transy,
                                     ssw, ssh, xoffset, yoffset,
                                     master_sem, this);
                pthread_attr_init(&attr[t]);
                ret = pthread_attr_setstack(&attr[t], stacks + t * STACK_SIZE, STACK_SIZE);
                if (ret != 0)
                    log_msg("setstack: %d - ssize = %d\n", ret, STACK_SIZE);
                if ((ret = pthread_create(&th, &attr[t], mandel_wrapper, tp[t])) != 0)
                    log_msg("pthread create failed for thread %d, %d\n", t, ret);
                worker_tasks[t] = th;
#ifndef PTHREADS
                if (clock_gettime(CLOCK_REALTIME, &tstart) < 0)
                    perror("clock_gettime()");
                //log_msg("start at %ld.%06ld\n", tstart.tv_sec % 60, tstart.tv_nsec / 1000);
                mandel_wrapper(tp[t]);
#endif
#ifdef __linux__
                ret = pthread_detach(th);
                if (ret != 0)
                    log_msg("pthread detach failed for thread %d, %d\n", t, ret);
#endif
#if (CONFIG_LOG_DEFAULT_LEVEL > 3)
                usleep(20 * 1000); // needed to make zephyr happy when loggin is enabled. Seems some race with the the KickOff mutex. Maybe some bug?
#endif
                t++;
            }
        }
    }

    void go(void)
    {
#ifdef PTHREADS
        memset(canvas, 0, CSIZE);
        if (clock_gettime(CLOCK_REALTIME, &tstart) < 0)
            perror("clock_gettime()");
            // log_msg("start at %ld.%06ld\n", tstart.tv_sec % 60, tstart.tv_nsec / 1000);
#endif
        for (int i = 0; i < NO_THREADS; i++)
        {
            // usleep(250 * 1000);
            V(tp[i]->go);
        }
        // log_msg("main thread waiting for %i threads...\n", NO_THREADS);
        for (int i = NO_THREADS; i; i--)
        {
            // log_msg("main thread waiting for %i threads...\n", i);
            PSem(master_sem); // wait until all workers have finished
        }

        if (clock_gettime(CLOCK_REALTIME, &tend) < 0)
            perror("clock_gettime()");
        //log_msg("end at %ld.%06ld\n", tend.tv_sec % 60, tend.tv_nsec / 1000);

        //log_msg("all threads finished.\n");
        free_ressources();
        stop = 0;
    }

    void free_ressources(void)
    {
        for (int i = 0; i < NO_THREADS; i++)
        {
            int ret;
#ifdef __ZEPHYR__
            // in recent Zephyr not needed enymore - even crashes soon!
            // needed to cleanup all resources, namely a mutex within a pthread
            // void *retval;
            // if ((ret = pthread_join(worker_tasks[i], &retval)) != 0)
            //    log_msg("pthread_join failed: %d\n", ret);
#endif
            if ((ret = pthread_attr_destroy(&attr[i])) != 0)
                log_msg("pthread_attr_destroy failed: %d\n", ret);

            delete tp[i];
        }
    }

public:
    mandel(canvas_t c, char *st, myDOUBLE xl, myDOUBLE yl, myDOUBLE xh, myDOUBLE yh, int xr, int yr, myDOUBLE xrat = 1.0)
        : canvas(c), stacks(st), xres(xr), yres(yr), xratio(xrat), stop(0)
    {
#ifdef LUCKFOX
	luckfox_palette(col_pal);
	int c1 = 0;
	for (int i = 0; i < 320; i += 2, c1++)
	    luckfox_rect(i, 0, i+1, 240, col_pal[c1]);
#else	
        for (int i = 0; i < PAL_SIZE; i++)
            col_pal[i] = i;
#endif	
        _mask = 1;
        for (int i = 1; i < PIXELW; i++)
        {
            _mask = (_mask << 1) | 1;
        }

        pthread_mutex_init(&canvas_sem, nullptr);
        sem_init(&master_sem, 0, 0);
#if 0	
        log_msg("hit enter to start...\n");
        char c1;
        read(0, &c1, 1);
#endif
        mandel_setup(sqrt(NO_THREADS), xl, yl, xh, yh); // initialize some stuff, but don't calculate
        go();
        dump_result();
    }
    ~mandel()
    {
        //log_msg("%s destructor\n", __FUNCTION__);
        pthread_mutex_destroy(&canvas_sem);
        sem_destroy(&master_sem);
    };

    char *get_stacks(void) { return stacks; }

    void dump_result(void)
    {
#if !defined(C64) && !defined(LUCKFOX) && !defined(__amiga__)
        canvas_dump(canvas);
#endif
        struct timespec dt;
        timespec_diff(&tend, &tstart, &dt);
        log_msg("mandelbrot set done in: %lld.%06lds\n", (long long int)dt.tv_sec, dt.tv_nsec / 1000L);
    }

    void select_start(point_t &p)
    {
        mark_x1 = p.x; // - ((LV_HOR_RES_MAX - IMG_W) / 2);
        mark_y1 = p.y; // - ((LV_VER_RES_MAX - IMG_H) / 2);
        if (mark_x1 < 0)
            mark_x1 = 0;
        if (mark_y1 < 0)
            mark_y1 = 0;
    }

    void select_end(point_t &p)
    {
        mark_x2 = p.x; // - ((LV_HOR_RES_MAX - IMG_W) / 2);
        mark_y2 = p.y; // - ((LV_VER_RES_MAX - IMG_H) / 2);
        if (mark_x2 < 0)
            mark_x2 = 0;
        if (mark_y2 < 0)
            mark_y2 = 0;
        //log_msg("rect coord: [%d,%d]x[%d,%d] - ssw=%d,ssh=%d,trx=%d,try=%d\n", mark_x1, mark_y1, mark_x2, mark_y2, ssw, ssh, transx, transy);
        log_msg("rect coord: [%d,%d]x[%d,%d]\n", mark_x1, mark_y1, mark_x2, mark_y2);
        mandel_setup(sqrt(NO_THREADS),
                     static_cast<myDOUBLE>(mark_x1 * ssw + transx),
                     static_cast<myDOUBLE>(mark_y1 * ssh + transy),
                     static_cast<myDOUBLE>(mark_x2 * ssw + transx),
                     static_cast<myDOUBLE>(mark_y2 * ssh + transy));
        go();
        dump_result();
        mark_x1 = -1;
        mark_x2 = mark_x1;
        mark_y2 = mark_y1;
    }

    void mandel_presetup(myDOUBLE sx, myDOUBLE sy, myDOUBLE tx, myDOUBLE ty)
    {
        last_xr = (tx - sx);
        last_yr = (ty - sy);
        ssw = last_xr / xres;
        ssh = last_yr / yres;
        transx = sx;
        transy = sy;
    }
#if 0
    void select_update(point_t &p)
    {
        if (mark_x1 < 0)
            return;
        coord_t tx = mark_x2;
        coord_t ty = mark_y2;
        mark_x2 = p.x; // - ((LV_HOR_RES_MAX - IMG_W) / 2);
        mark_y2 = p.y; // - ((LV_VER_RES_MAX - IMG_H) / 2);
        if (mark_x2 < 0)
            mark_x2 = 0;
        if (mark_y2 < 0)
            mark_y2 = 0;
        if ((tx == mark_x2) && (ty == mark_y2))
            return;
        //log_msg("rect coord: %dx%d - %dx%d\n", mark_x1, mark_y1, mark_x2, mark_y2);
    }
#endif
};

#endif
