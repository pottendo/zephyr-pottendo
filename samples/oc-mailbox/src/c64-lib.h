/*  -*-c-*-
 * File:		c64-linux.h
 * Date:		Fri Dec 16 09:39:01 2022
 * Author:		pottendo (pottendo)
 *  
 * Abstract:
 *     some aux functions for c64 @linux orange cart
 * 
 * Modifications:
 * $Log$
 */

#ifndef __c64_linux_h__
#define __c64_linux_h__
#include <stdint.h>

extern volatile const char *__led;
#define LED_R(val) { uint32_t v = *__led; v &= ~(((uint32_t)(1 << 8)-1) << 16); v |= ((val) << 16); (* ((uint32_t *)__led)) = v; }
#define LED_G(val) { uint32_t v = *__led; v &= ~(((uint32_t)(1 << 8)-1) << 8); v |= ((val) << 8); (* ((uint32_t *)__led)) = v; }
#define LED_B(val) { uint32_t v = *__led; v &= ~(((uint32_t)(1 << 8)-1) << 0); v |= ((val) << 0); (* ((uint32_t *)__led)) = v; }
#define LED(val) { (* ((uint32_t *)__led)) = (val);}

#ifdef ZEPHYR
#define CACHE_FLUSH() __asm__ __volatile__(".word 0x500F")
  
#define OC_IRQ DT_IRQ_BY_IDX(DT_NODELABEL(mailbox), 0, irq)
#define OC_IRQPRIO DT_IRQ_BY_IDX(DT_NODELABEL(mailbox), 0, priority)
#define OC_SHM DT_REG_ADDR_BY_NAME(DT_NODELABEL(mailbox), shm)
#define OC_IRQCONFIRM() { volatile uint32_t d __attribute((unused)) = \
                          *((uint32_t *) DT_REG_ADDR_BY_NAME(DT_NODELABEL(mailbox), confirm)); }
#define TRIGGER_C64_ISR() (*((uint8_t *) DT_REG_ADDR_BY_NAME(DT_NODELABEL(mailbox), trigger)))++   // trigger C64 ISR
#else
#define CACHE_FLUSH()
#endif

#define PIXELW 1 // 2
#define IMG_W 320 // 320
#define IMG_H 200 // 200
#define CSIZE (IMG_W * IMG_H) / 8
#define PAL_SIZE (2 * PIXELW)

#define log_msg printf

typedef enum {
    VICBank0 = 3, VICBank1 = 2, VICBank2 = 1, VICBank3 = 0,
    VICModeGfxHR = 4, VICModeGfxMC = 5, VICModeText = 6
} c64_consts;

namespace VIC
{
    const uint8_t BoC = 0x20;
    const uint8_t BgC = 0x21;
    const uint8_t CR1 = 0x11;
    const uint8_t CR2 = 0x16;
    const uint8_t MEM = 0x18;
    enum { BLACK = 0, WHITE, RED, CYAN, PURPLE, GREEN, BLUE, YELLOW,
	   ORANGE, BROWN, PINK, DARK_GREY, GREY, LIGHT_GREEN, LIGHT_BLUE, LIGHT_GREY };
};

const uint32_t c64_physaddress = CONFIG_C64_MEMBASE;  // as defined in basesoc.py, RVCop64

class c64
{
    uint32_t c64_phys;
    unsigned char *map_memory(off_t addr, size_t len);
    unsigned char *mem, *vic, *cia1, *cia2, *sid, *oc_ctrl, *coproc_reg, *canvas;
    uint8_t _mask = 0;
    enum { c64Fill = 0b10000000, c64Invert = 0b01000000}; // drawing opions as coded in color high-nibble
    bool invert = false;
    struct { int x1; int y1; int x2; int y2; } vp;

    int fvline(int X1, int Y1, int X2, int Y2, uint8_t c);
    int fhline(int X1, int Y1, int X2, int Y2, uint8_t c);

  public:
    c64(uint32_t cr = 0xc000, uint16_t c = 0x4000, uint32_t phys_addr = c64_physaddress);
    ~c64();

    inline unsigned char *get_mem(void) { return mem; }
    inline unsigned char *get_canvas(void) { return canvas; }
    inline void set_canvas(uint16_t c) { canvas = &mem[c]; }
    inline unsigned char *get_coprocreq(void) { return coproc_reg; };
    inline void set_viewport(int x1, int y1, int x2, int y2) { vp.x1 = x1; vp.y1 = y1; vp.x2 = x2; vp.y2 = y2; }
    void gfx(c64_consts bank, c64_consts mode, uint8_t vram);
    void screencols(uint8_t bo, uint8_t bg) { vic[VIC::BoC] = bo; vic[VIC::BgC] = bg; }
    void setpx(int x, int y, uint8_t c);
    inline void setinvert(bool i) { invert = i; }
    int circle(int X1, int Y1, int r, uint8_t c);
    int line(int X1, int Y1, int X2, int Y2, uint8_t c);
};

#endif /* __c64_linux_h__ */
