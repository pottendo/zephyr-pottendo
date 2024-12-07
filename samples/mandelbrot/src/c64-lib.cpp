// -*-c++-*-
// $Id$
//
// File:		c64-linux.c
// Date:		Fri Dec 16 09:38:08 2022
// Author:		pottendo (pottendo)
// 
// Abstract:
//      
//
// Modifications:
// 	$Log$
//
#include <stdio.h>
#ifndef __ZEPHYR__
#include <fcntl.h>
#include <sys/mman.h>
#endif

#include "mandel-arch.h"

#include <unistd.h>
#include <iostream>
#include <cstring>
#include "c64-lib.h"

c64_t::c64_t()
{
    mem = vic = cia1 = cia2 = sid = oc_ctrl = nullptr;
    mem = map_memory(c64_physaddress, 0x10000);
    if (mem == (unsigned char*) -1)
	    perror("map_memory failed.");
    else
    {
	    vic = &mem[0xd000];
	    cia1 = &mem[0xdc00];
	    cia2 = &mem[0xdd00];
	    sid = &mem[0xd400];
	    oc_ctrl = &mem[0xdf00];
    }
}

c64_t::~c64_t()
{
    gfx(VICBank0, VICModeText, 1);
#ifndef __ZEPHYR__
    munmap(mem, 0x10000);
#endif    
}

void c64_t::gfx(c64_consts bank, c64_consts mode, uint8_t vram)
{
    if (!vic)
    {
	log_msg("vic not mapped.\n");
	return;
    }
    cia2[0] &= 0b11111100;
    cia2[0] |= bank;
    switch(mode)
    {
    case VICModeGfxMC:
	vic[VIC::CR2] |= 0b00010000;
    case VICModeGfxHR:
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
c64_t::map_memory(off_t offset, size_t len)
{
#ifndef __ZEPHYR__
    // Truncate offset to a multiple of the page size, or mmap will fail.
    size_t pagesize = sysconf(_SC_PAGE_SIZE);
    off_t page_base = (offset / pagesize) * pagesize;
    off_t page_offset = offset - page_base;

    int fd = open("/dev/mem", (O_RDWR | O_DSYNC));
    unsigned char *mem = (unsigned char *) mmap(NULL, page_offset + len, PROT_READ | PROT_WRITE,
						MAP_SHARED, fd, page_base);
    if (mem == MAP_FAILED) {
        perror("Can't map memory");
        return (unsigned char *)-1;
    }
#else
    mem = (unsigned char *)offset;
#endif    
    return mem;
}

/* globals */
c64_t c64;
int col1, col2, col3;
char *c64_stack = (char *)0x10000000; // fast SRAM on Orangecart, only 16k! so NO_THREADS <= 16;

void c64_screen_init(void)
{
    std::cout << "C64 memory @0x" << std::hex << int(c64.get_mem()) << std::dec << '\n';
    c64.screencols(VIC::BLACK, VIC::BLACK);
    c64.gfx(VICBank1, VICModeGfxMC, 15);
    // xrat = 16.0 / 9.0;
    col1 = 0xb;
    col2 = 0xc;
    col3 = 14; // VIC::LIGHT_BLUE;
}

void c64_hook1(void)
{
        memset(&c64.get_mem()[0x3c00], (col1 << 4) | col2, 1000);
        memset(&c64.get_mem()[0xd800], col3, 1000);
}

void c64_hook2(void)
{
        col1++; col2++; col3++;
        col1 %= 0xf;
        if (col1 == 0)
            col1++;
        col2 %= 0xf;
        if (col2 == 0)
            col2++;
        col3 %= 0xf;
        if (col3 == 0)
            col3++;
}
