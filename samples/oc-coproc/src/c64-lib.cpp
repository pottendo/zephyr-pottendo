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
#include "c64-lib.h"

c64::c64(uint16_t cr, uint16_t c)
{
    mem = vic = cia1 = cia2 = sid = oc_ctrl = nullptr;
    mem = map_memory(c64_physaddress, 0x10000);
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

void c64::setpx(uint16_t x, uint8_t y, uint8_t c)
{
    if (x >= (IMG_W / PIXELW)) return;
    if (y >= IMG_H) return;
    
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
        std::cerr << "Exceeding canvas: " << cidx << ", " << x << '/' << y << '\n';
        // delay (100 * 1000);
        return;
    }
    char t = canvas[cidx];
    t &= mask;
    t |= val;
    canvas[cidx] = t;
#ifdef ZEPHYR
    volatile char *led = (char *)0xf0002000;
    *led = ((*led) + 1);
#endif
    // sched_yield();
}
