#include <iostream>
#include <string.h>
#include <pthread.h>

#include "c64-lib.h"

const int OC_IRQ = 5;
const int OC_IRQPRIO = 7;

volatile uint32_t confirm;
#define OC_SHM 0xe0000000
using namespace std;

c64 c64i(OC_SHM);
oc_coproc co_proc(c64i);

void oc_isr(void *arg)
{
    static int i = 0;
    switch (i % 4) 
    {
    case 1:
        LED_R(64);
        break;
    case 2:
        LED_G(64);
        break;
    case 3:
        LED_B(64);
        break;
    default:
        LED(0);
    }
    i++;

    co_proc.isr_req();
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
    LED_R(32);
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
