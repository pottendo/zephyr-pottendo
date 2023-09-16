#include <iostream>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <zephyr/device.h>

#include "c64-lib.h"
#include "oc-coproc.h"

volatile uint32_t confirm;
using namespace std;

c64 c64i(OC_SHM);
oc_coproc co_proc(c64i);
static int no_irqs = 0;
pthread_t cr_th;
sem_t cr_sem;
pthread_attr_t attr;

void oc_isr(void *arg)
{
    switch (no_irqs++ % 4) 
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
    OC_IRQCONFIRM();
    sem_post(&cr_sem);
}

void *cr_thread(void *arg)
{
    cout << "coroutine thread started...\n";
    while (true)
    {
        sem_wait(&cr_sem);
        if (co_proc.isr_req())
            TRIGGER_C64_ISR();
    }
    return nullptr;
}

#define STACK_SIZE 1024
#ifdef __ZEPHYR__
#if (NO_THREADS > 16)
#error "too many threads for Orangencart's STACK_SIZE"
#endif
static char *stacks = (char *)0x10000000;   // fast SRAM on Orangecart, only 16k! so NO_THREADS <= 16

#else

#undef STACK_SIZE 
#define STACK_SIZE PTHREAD_STACK_MIN
static char *stacks;

#endif

void
mailbox(void)
{
    cout << __FUNCTION__ << ": testing...\n";

    cout << std::hex
         << "shm:" << DT_REG_ADDR_BY_NAME(DT_NODELABEL(mailbox), shm) << '\n'
         << "trigger:" << DT_REG_ADDR_BY_NAME(DT_NODELABEL(mailbox), trigger) << '\n'
         << "confirm:" << DT_REG_ADDR_BY_NAME(DT_NODELABEL(mailbox), confirm) << '\n'
         << "irq:" << DT_IRQ_BY_IDX(DT_NODELABEL(mailbox), 0, irq) << '\n'
         << "irqprio:" << DT_IRQ_BY_IDX(DT_NODELABEL(mailbox), 0, priority) << '\n'
         << std::dec << '\n';

    sem_init(&cr_sem, 0, 0);
    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
    pthread_attr_setstack(&attr, stacks, STACK_SIZE);
    int ret;
    ret = pthread_create(&cr_th, &attr, cr_thread, nullptr);
    if (ret != 0)
        printf("pthread_create failed: %d\n", ret);
    pthread_detach(cr_th);

    IRQ_CONNECT(OC_IRQ, OC_IRQPRIO, oc_isr, nullptr, 0);
    irq_enable(OC_IRQ);
    
    int l = 1;
    while (true)
    {
        cout << "waiting for ISR..." << l++ << ", irq-no# " << no_irqs << '\n';
        sleep(1);
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
    c64i.screencols(VIC::DARK_GREY, VIC::BLUE);
#endif    
    mailbox();
    std::cout << "done\n";
    return 0;
}
