// -*-c++-*-
// $Id$
//
// File:		c64-lib.c
// Date:		Fri Dec 16 09:38:08 2022
// Author:		pottendo (pottendo)
//
#include <stdio.h>
#include <math.h>
#ifndef ZEPHYR
#include <fcntl.h>
#include <sys/mman.h>
#endif

#include <unistd.h>
#include <iostream>
#include <cstring>
#include "c64-lib.h"

volatile const char *__led = (char *)0xf0002000;

c64::c64(uint16_t cr, uint16_t c, uint32_t phys_addr)
    : c64_phys(phys_addr)
{
    mem = vic = cia1 = cia2 = sid = oc_ctrl = nullptr;
    mem = map_memory(c64_phys, 0x10000);
    if (mem == (unsigned char *)-1)
        perror("map_memory failed.");
    else
    {
        vic = &mem[0xd000];
        cia1 = &mem[0xdc00];
        cia2 = &mem[0xdd00];
        sid = &mem[0xd400];
        oc_ctrl = &mem[0xdf00];
        coproc_reg = &mem[cr];
        canvas = &mem[c];
        vp.x1 = 0; vp.y2 = 0;
        vp.x2 = IMG_W / PIXELW; vp.y2 = IMG_H;
    }
    _mask = 1;
    for (int i = 1; i < PIXELW; i++)
    {
        _mask = (_mask << 1) | 1;
    }
}

c64::~c64()
{
    gfx(VICBank0, VICModeText, 1);
#ifndef ZEPHYR
    munmap(mem, 0x10000);
#endif
}

void c64::gfx(c64_consts bank, c64_consts mode, uint8_t vram)
{
    if (!vic)
    {
        log_msg("vic not mapped.\n");
        return;
    }
    _mask = 0;
    cia2[0] &= 0b11111100;
    cia2[0] |= bank;
    switch (mode)
    {
    case VICModeGfxMC:
        vic[VIC::CR2] |= 0b00010000;
        _mask = (1 << 1);
    case VICModeGfxHR:
        _mask |= 1;
        vic[VIC::CR1] |= 0b00100000;
        if (vram > 15)
            log_msg("%s: illegal VRAM: 0x%02x\n", __FUNCTION__, vram);
        vic[VIC::MEM] &= 0b00001111;
        vic[VIC::MEM] |= (vram << 4);
        break;
    case VICModeText:
        vic[VIC::CR1] &= 0b11011111;
        vic[VIC::CR2] &= 0b11101111;
        if (vram > 15)
            log_msg("%s: illegal VRAM: 0x%02x\n", __FUNCTION__, vram);
        vic[VIC::MEM] &= 0b00001111;
        vic[VIC::MEM] |= (vram << 4);
        break;
    default:
        log_msg("%s: unkown VIC mode %d.\n", __FUNCTION__, mode);
        break;
    }
}

unsigned char *
c64::map_memory(off_t offset, size_t len)
{
#ifndef ZEPHYR
    // Truncate offset to a multiple of the page size, or mmap will fail.
    size_t pagesize = sysconf(_SC_PAGE_SIZE);
    off_t page_base = (offset / pagesize) * pagesize;
    off_t page_offset = offset - page_base;

    int fd = open("/dev/mem", (O_RDWR | O_DSYNC));
    unsigned char *mem = (unsigned char *)mmap(NULL, page_offset + len, PROT_READ | PROT_WRITE,
                                               MAP_SHARED, fd, page_base);
    if (mem == MAP_FAILED)
    {
        perror("Can't map memory");
        return (unsigned char *)-1;
    }
#else
    mem = (unsigned char *)offset;
#endif
    return mem;
}

void c64::setpx(int x, int y, uint8_t c)
{
    if ((x < 0) || (y < 0) ||
        (x >= (IMG_W / PIXELW)) ||
        (y >= IMG_H))
        return;
    if ((x < vp.x1) || (x >= vp.x2))
        return;
    if ((y < vp.y1) || (y >= vp.y2))
        return;
    uint32_t h = x % (8 / PIXELW);
    uint32_t shift = (8 / PIXELW - 1) - h;
    uint32_t val = (c << (shift * PIXELW));
    uint8_t mask = ~(_mask << (shift * PIXELW));
    //log_msg("%s(%d,%d,%d), shift = %d, _mask=%02x, mask = %02x\n", __FUNCTION__, x, y, c, shift, _mask, mask);

    const uint32_t lineb = IMG_W / 8 * 8; // line 8 bytes per 8x8 pixel
    const uint32_t colb = 8;              // 8 bytes per 8x8 pixel

    // log_msg("x/y %d/%d offs %d/%d\n", x, y, (x / (8 / PIXELW)) * colb, (y / 8) * lineb  + (y % 8));
    uint32_t cidx = (y / 8) * lineb + (y % 8) + (x / (8 / PIXELW)) * colb;
    if (cidx >= CSIZE)
    {
        std::cerr << "Exceeding canvas: cv[0x" << std::hex << cidx << "], " << std::dec << x << '/' << y << '\n';
        // delay (100 * 1000);
        return;
    }
    char t = canvas[cidx];
    if (!invert)
        t &= mask;
    t ^= val;
    canvas[cidx] = t;
#ifdef ZEPHYR
    volatile char *led = (char *)0xf0002000;
    *led = ((*led) + 1);
#endif
    // sched_yield();
}

 
#if 1
// code reference https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
int c64::line(int X1, int Y1, int X2, int Y2, uint8_t c)
{
    if (X1 == X2) return fvline(X1, Y1, X2, Y2, c);
    if (Y1 == Y2) return fhline(X1, Y1, X2, Y2, c);

    //printf("%s: 1 (%d,%d) -> (%d,%d), col = %d\n", __FUNCTION__, X1, Y1, X2, Y2, c);
    if (X2 < X1) { std::swap(X1, X2); std::swap(Y1, Y2); }
    if (X2 < 0) return -1;
    if (Y2 < 0) Y2 = 0;
    if (X1 >= IMG_W / PIXELW) return -1;
    if (Y1 >= IMG_H) Y1 = IMG_H - 1;
    if (X1 < 0) X1 = 0;
    if (Y1 < 0) Y1 = 0;
    if (X2 >= IMG_W / PIXELW) X2 = IMG_W / PIXELW - 1;
    if (Y2 >= IMG_H) Y2 = IMG_H - 1;
    //printf("%s: 2 (%d,%d) -> (%d,%d), col = %d\n", __FUNCTION__, X1, Y1, X2, Y2, c);
 
    invert = (c & c64Invert);
    c &= 0x0f; // clear drawing flags

    int dx = abs(X2 - X1);
    int sx = 1;
    int dy = -abs(Y2 - Y1);
    int sy = (Y1 < Y2) ? 1 : -1;
    int error = dx + dy;
    int e2;

    while (true)
    {
        setpx(X1, Y1, c);
        if ((X1 == X2) && (Y1 == Y2)) break;
        e2 = 2 *error;
        if (e2 >= dy)
        {
            if (X1 == X2) break;
            error += dy;
            X1 += sx;
        }
        if (e2 <= dx) 
        {
            if (Y1 == Y2) break;
            error += dx;
            Y1 += sy;
        }
    }
    return 0;
}

#else
// code reference: https://www.geeksforgeeks.org/mid-point-line-generation-algorithm/
static void _line(c64 &c64i, int X1, int Y1, int X2, int Y2, uint8_t c)
{
    printf("%s: 1 (%d,%d) -> (%d,%d), col = %d\n", __FUNCTION__, X1, Y1, X2, Y2, c);
    if (X2 < X1) { std::swap(X1, X2); std::swap(Y1, Y2); }
    if (X2 < 0) return;
    if (Y2 < 0) Y2 = 0;
    if (X1 >= IMG_W / PIXELW) return;
    if (Y1 >= IMG_H) Y1 = IMG_H - 1;
    if (X1 < 0) X1 = 0;
    if (Y1 < 0) Y1 = 0;
    if (X2 >= IMG_W / PIXELW) X2 = IMG_W / PIXELW - 1;
    if (Y2 >= IMG_H) Y2 = IMG_H - 1;
    printf("%s: 2 (%d,%d) -> (%d,%d), col = %d\n", __FUNCTION__, X1, Y1, X2, Y2, c);
    // calculate dx & dy
    int dx = X2 - X1;
    int dy = Y2 - Y1;
    
    if (dy <= dx)
    {
        // initial value of decision parameter d
        int d = dy - (dx/2);
        int x = X1, y = Y1;
 
        // Plot initial given point
        setpx(x, y, c);
        // iterate through value of X
        while (x < X2)
        {
            x++;
            // E or East is chosen
            if (d < 0)
                d = d + dy;
            else
            {
                // NE or North East is chosen
                d += (dy - dx);
                y++;
            }
            setpx(x, y, c);
        }
    }
    else if (dx < dy)
    {
        // initial value of decision parameter d
        int d = dx - (dy / 2);
        int x = X1, y = Y1;

        // Plot initial given point
        setpx(x, y, c);


        // iterate through value of X
        while (y < Y2)
        {
            y++;
            // E or East is chosen
            if (d < 0)
                d = d + dx;
            else
            {
                // NE or North East is chosen
                d += (dx - dy);
                x++;
            }
            setpx(x, y, c);
        }
    }
}
#endif

const uint8_t bitml[] = {
    0b11111111,
    0b01111111,
    0b00111111,
    0b00011111,
    0b00001111,
    0b00000111,
    0b00000011,
    0b00000001
};

const uint8_t bitmr[] = {
    0b10000000,
    0b11000000,
    0b11100000,
    0b11110000,
    0b11111000,
    0b11111100,
    0b11111110,
    0b11111111
};

int c64::fhline(int X1, int Y1, int X2, int Y2, uint8_t c)
{
    //printf("%s: (%d,%d)->(%d,%d), col = %d\n", __FUNCTION__, X1, Y1, X2, Y2, c);
    if (Y1 < 0) return -1;
    if (Y1 >= IMG_H) return -1;
    if (X2 < X1) std::swap(X1, X2);
    if (X1 >= IMG_W / PIXELW) return -1;
    if (X2 < 0) return -1;
    if (X2 >= IMG_W / PIXELW) X2 = IMG_W / PIXELW - 1;
    if (X1 < 0) X1 = 0;
    //printf("%s: (%d,%d)->(%d,%d), col = %d\n", __FUNCTION__, X1, Y1, X2, Y2, c);

    const uint32_t lineb = IMG_W / 8 * 8; // line 8 bytes per 8x8 pixel
    const int bpb = 8 / PIXELW;
    int x, it, cidx;
    int boffs = (X1 % bpb);
    int boffsx2 = (X2 % bpb);
    unsigned char *cv = get_canvas();
    unsigned char bcol = (c & _mask);
    for (int i = 1; i < (8 / PIXELW); i++)
    {
        bcol = (bcol << PIXELW) | (c & _mask);
    }
    cidx = (Y1 / 8) * lineb + (Y1 % 8) + (X1 / (8 / PIXELW)) * 8;

    //printf("%s: c=%d, X1=%d, X2=%d(%d offs), Y1=%d, boffs=%d, bpb=%d, cidx=0x%04x\n", __FUNCTION__, bcol, X1, X2, (X2%8), Y1, boffs, bpb, cidx);
    x = X1 + (bpb - boffs); // align with next byte
    if (x <= X2)
    {
        if (!invert)
            cv[cidx] &= ~bitml[boffs];
        cv[cidx] ^= (bitml[boffs] & bcol);
        cidx += 8;
    }
    else
    { 
        // just within a byte
        //printf("boffs = %d -> boffsx2 = %d\n", boffs, boffsx2);
        uint8_t m;
        for (int i = boffs; i <= boffsx2; i++)
        {
            m = (_mask << ((bpb - i - 1) * PIXELW));
            //printf("_mask = %d, bcol = %d, mask = 0x%02x\n", _mask, bcol, m);
            if (!invert)
                cv[cidx] &= ~m;
            cv[cidx] ^= (bcol & m);
        }
        return 0;
    }

    for (it = x; (it <= (X2 - bpb)) && (it < IMG_W / PIXELW); it += 8)
    {   
        if (invert)
            cv[cidx] ^= bcol;
        else 
            cv[cidx] = bcol;
        cidx += 8;
    }
    if (it < IMG_W / PIXELW)
    {
        if (!invert)
            cv[cidx] &= ~bitmr[boffsx2];
        cv[cidx] ^= (bitmr[boffsx2] & bcol);
    }
    return 0;
}

int c64::fvline(int X1, int Y1, int X2, int Y2, uint8_t c)
{
    if (X1 < 0) return -1;
    if (X1 >= IMG_W / PIXELW) return -1;
    if (Y2 < Y1) std::swap(Y1, Y2);
    if (Y2 < 0) return -1;
    if (Y1 >= IMG_H) return -1;
    if (Y1 < 0) Y1 = 0;
    if (Y2 >= IMG_H) Y2 = IMG_H - 1;
    //printf("%s: (%d,%d)->(%d,%d), col = %d\n", __FUNCTION__, X1, Y1, X2, Y2, c);

    const uint32_t lineb = IMG_W / 8 * 8; // line 8 bytes per 8x8 pixel
    const int bpb = 8 / PIXELW;
    unsigned char *cv = get_canvas();
    int y, boffsx1, cidx, m, col;

    boffsx1 = X1 % (8 / PIXELW);
    m = (_mask << (bpb - boffsx1 - 1));
    col = ((c & _mask) << (bpb - boffsx1 - 1));
    cidx = (Y1 / 8) * lineb + (Y1 % 8) + (X1 / (8 / PIXELW)) * 8;
    if (!invert)
        cv[cidx] &= ~m;     
    cv[cidx] ^= col;    // first pixel
    cidx &= 0xfff8;     // align to line
    for (y = Y1 + 1; y <= Y2; y++)
    {
        if ((y % 8) == 0)
            cidx += lineb;
        if (!invert)
            cv[cidx + (y % 8)] &= ~m;
        cv[cidx + (y % 8)] ^= col;
    }
    return 0;
}

static int filled[IMG_H];
// code reference: https://www.geeksforgeeks.org/mid-point-circle-drawing-algorithm
int c64::circle(int X1, int Y1, int r, uint8_t c)
{
    if (r > IMG_W) return -1; 
    int x_centre = X1;
    int y_centre = Y1;
    int x = r, y = 0;
    bool fill = (c & c64Fill);
    invert = (c & c64Invert);
    //printf("(%d,%d) rad = %d, col = 0x%02x, invert = %d, fill=%d\n", X1, Y1, r, c, invert, fill);
    c &= 0xf;    // clear draw flags
    if (fill) memset(filled, 0, IMG_H * sizeof(int));
    // Printing the initial point on the axes
    setpx(r + x_centre, y_centre, c);

    // when radius is zero only a single point will be printed
    if (r > 0)
    {
        setpx(-r + x_centre, y_centre, c);
        if (fill)
        {
            fhline(-r + x_centre + 1, y_centre, r + x_centre - 1, y_centre, c);
            if ((y_centre < IMG_H) && (y_centre >= 0)) 
                filled[y_centre]++;
        }
        else
        {
            setpx(x_centre, r + y_centre, c);
            setpx(x_centre, -r + y_centre, c);
        }
    }
    else
        return 0;
     
    // Initialising the value of P
    int P = 1 - r;
    while (x > y)
    {
        y++;
         
        // Mid-point is inside or on the perimeter
        if (P <= 0)
            P = P + 2*y + 1;
        else
        {
            // Mid-point is outside the perimeter
            x--;
            P = P + 2*y - 2*x + 1;
        }
         
        // All the perimeter points have already been printed
        if (x < y)
            break;
         
        // Printing the generated point and its reflection
        // in the other octants after translation
        if (fill)
        {
            int yh1 = y + y_centre;
            int yh2 = -y + y_centre;
            if ((yh1 < IMG_H) && (yh1 >= 0) && !filled[yh1])
            {
                fhline(-x + x_centre, yh1, x + x_centre, yh1, c);
                filled[yh1]++;
            }
            if ((yh2 < IMG_H) && (yh2 >= 0) && !filled[yh2])
            {
                fhline(-x + x_centre, yh2, x + x_centre, yh2, c);
                filled[yh2]++;
            }
       }    
        else
        {
            setpx(x + x_centre, y + y_centre, c);
            setpx(-x + x_centre, y + y_centre, c);
            setpx(x + x_centre, -y + y_centre, c);
            setpx(-x + x_centre, -y + y_centre, c);
        }
        // If the generated point is on the line x = y then
        // the perimeter points have already been printed
        if (x != y)
        {
            if (fill)
            {
                int yh1 = x + y_centre;
                int yh2 = -x + y_centre;
                if ((yh1 < IMG_H) && (yh1 >= 0) && !filled[yh1])
                {
                    fhline(-y + x_centre, yh1, y + x_centre, yh1, c);
                    filled[yh1]++;
                }
                if ((yh2 < IMG_H) && (yh2 >= 0) && !filled[yh2]) 
                {
                    fhline(-y + x_centre, yh2, y + x_centre, yh2, c);
                    filled[yh2]++;
                }

            }    
            else
            {
                setpx(y + x_centre, x + y_centre, c);
                setpx(-y + x_centre, x + y_centre, c);
                setpx(y + x_centre, -x + y_centre, c);
                setpx(-y + x_centre, -x + y_centre, c);
            }
        }
    }
    return 0;
}
