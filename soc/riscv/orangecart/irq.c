/*
 * SPDX-License-Identifier: Apache-2.0
 */

/* fixme: introduce proper SOC KConfig item */
#if defined(CONFIG_BOARD_ORANGECART_SMP) || defined(CONFIG_BOARD_ORANGECART_FPU)
#include <stdint.h>
//#include <zephyr/arch/riscv/arch.h>

/*
 * needed for VexRiscV clint to make clock working 
 */
void arch_irq_enable(unsigned int irq)
{
	uint32_t mie;
	/*
	 * CSR mie register is updated using atomic instruction csrrs
	 * (atomic read and set bits in CSR register)
	 */
	__asm__ volatile ("csrrs %0, mie, %1\n"
			  : "=r" (mie)
			  : "r" (1 << irq));    
}
#endif /* CONFIG_BOARD_ORANGECARD_SMP */

void led_ping(void)
{
	volatile char *led = 0xf0002000;
	*led = (*led) + 50;	
}
