#include <iostream>
#include <string.h>
#include <pthread.h>

#include "c64-lib.h"

const int OC_IRQ = 4;
const int OC_IRQPRIO = 2;

#define OC_IRQCONFIRM ((uint32_t *) 0xe000003c)
volatile uint32_t confirm;
#define TRIGGER_C64_ISR() (*((uint8_t *)0xe000003b))++   // trigger C64 ISR

using namespace std;

c64 c64i;

void oc_isr(void *arg)
{
    static int i = 10;
    LED_R(i);
    i+=10;
    confirm = *OC_IRQCONFIRM;
}

void
mailbox(void)
{
    cout << __FUNCTION__ << ": testing...\n";

    IRQ_CONNECT(OC_IRQ, OC_IRQPRIO, oc_isr, nullptr, 0);
    irq_enable(OC_IRQ);
    
    int l = 1;
    while (true)
    {
        cout << "waiting for ISR..." << l++ << '\n';
        sleep(1);
        TRIGGER_C64_ISR();
    }
}

int
main(void)
{
#ifdef ZEPHYR
    LED_B(32); LED_R(32);
#endif
    std::cout << "Orangecart mailbox test...\n";
#ifdef C64    
    std::cout << "C64 memory @0x" << std::hex << int(c64i.get_mem()) << std::dec << '\n';
    c64i.screencols(VIC::GREY, VIC::BLUE);
#endif    
    mailbox();
    std::cout << "done\n";
    return 0;
}
