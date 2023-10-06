#include <iostream>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <zephyr/device.h>

#include "c64-lib.h"
#include "oc-coproc.h"

volatile uint32_t confirm;
using namespace std;

//c64 c64i(OC_SHM);
c64 c64i((uint8_t *)(CONFIG_C64_MEMBASE + 0xe0));    // for elite harmless
oc_coproc co_proc(c64i);
static int no_irqs = 0;
pthread_t cr_th;
sem_t cr_sem;
pthread_attr_t attr;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
struct timespec tstart, tend, dt;

void oc_isr(void *arg)
{
#if 0    
    switch (no_irqs % 4) 
    {
    case 1:
        LED_R(64);
        break;
    case 2:
        LED_G(64);
        break;
    case 3:
        //LED_B(64);
        break;
    default:
        LED(0);
    }
#endif
    no_irqs++;
    OC_IRQCONFIRM();
    clock_gettime(CLOCK_REALTIME, &tstart);
    sem_post(&cr_sem);
    //LED_B(64);
}

void timespec_diff(struct timespec *a, struct timespec *b, struct timespec *result)
{
  	result->tv_sec  = a->tv_sec  - b->tv_sec;
   	result->tv_nsec = a->tv_nsec - b->tv_nsec;
   	if (result->tv_nsec < 0)
   	{
        --result->tv_sec;
        result->tv_nsec += 1000000000L;
    }
}

void *cr_thread(void *arg)
{
    sched_param sp;
    int pol, ret;
    pthread_getschedparam(cr_th, &pol, &sp);
    sp.sched_priority++;
    if ((ret = pthread_setschedparam(cr_th, pol, &sp)) != 0)
        printf("pthread setschedparam failed for thread: %d\n", ret);   
    pthread_getschedparam(cr_th, &pol, &sp);
    printf("starting cr thread with priority %d\n", sp.sched_priority);
    while (true)
    {
        sem_wait(&cr_sem);
        clock_gettime(CLOCK_REALTIME, &tend);
        timespec_diff(&tend, &tstart, &dt);
        //printf("cr blocked for: %lld.%03lds\n", dt.tv_sec, dt.tv_nsec / 1000000L);
        //pthread_mutex_lock(&mutex);
        if (co_proc.isr_req())
        {
            //OC_SHM[0x3f] = '\0';
            TRIGGER_C64_ISR();
        }
        //pthread_mutex_unlock(&mutex);
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
circle_test(void)
{
    struct timespec tstart, tend, dt;

    clock_gettime(CLOCK_REALTIME, &tstart);
    //pthread_mutex_lock(&mutex);
    memset(c64i.get_canvas(), 0, 8000);
    for (int i = 50; i < 180; i+=2)
    {
        c64i.circle(i, 100, 10 + i, 0xC1);
        //sched_yield();
        usleep(1000);
    }
    //pthread_mutex_unlock(&mutex);
    clock_gettime(CLOCK_REALTIME, &tend);
    timespec_diff(&tend, &tstart, &dt);
    printf("circles done in: %lld.%03lds\n", dt.tv_sec, dt.tv_nsec / 1000000L);
    LED(0);
}

void
line_test(void)
{
    struct timespec tstart, tend, dt;

    clock_gettime(CLOCK_REALTIME, &tstart);
    //pthread_mutex_lock(&mutex);
    memset(c64i.get_canvas(), 0, 8000);
    for (int i = 0; i < 320; i+=2)
    {
        c64i.line(i, 0, 319-i, 199, 0xC1);
        //sched_yield();
        //usleep(10);
    }
    //pthread_mutex_unlock(&mutex);
    clock_gettime(CLOCK_REALTIME, &tend);
    timespec_diff(&tend, &tstart, &dt);
    printf("lines done in: %lld.%03lds\n", dt.tv_sec, dt.tv_nsec / 1000000L);
    LED(0);
}

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
    //pthread_mutex_init(&mutex, nullptr);
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
        cout << "waiting for ISR..." << l << ", irq-no# " << no_irqs << '\n';
        l++;
        usleep(1000 * 1000);
        //TRIGGER_C64_ISR();
        //circle_test();
        //line_test();
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
