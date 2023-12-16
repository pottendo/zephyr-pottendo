/*
 * Copyright (c) 2018 - 2019 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT litex_uart0

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/types.h>

#define UART_EV_TX		(1 << 0)
#define UART_EV_RX		(1 << 1)

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
typedef void (*irq_cfg_func_t)(void);
#endif

struct uart_liteuart_device_config {
	uint32_t port;
	uint32_t sys_clk_freq;
	uint32_t baud_rate;
	unsigned long rxtx_addr;
	unsigned long txfull_addr;
	unsigned long rxempty_addr;
	unsigned long ev_status_addr;
	unsigned long ev_pending_addr;
	unsigned long ev_enable_addr;
	unsigned long txempty_addr;
	unsigned long rxfull_addr;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif
};

struct uart_liteuart_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

/**
 * @brief Output a character in polled mode.
 *
 * Writes data to tx register. Waits for space if transmitter is full.
 *
 * @param dev UART device struct
 * @param c Character to send
 */
static void uart_liteuart_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_liteuart_device_config *config = dev->config;
	/* wait for space */
	while (litex_read8(config->txfull_addr)) {
	}

	litex_write8(c, config->rxtx_addr);
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device struct
 * @param c Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer if empty.
 */
static int uart_liteuart_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_liteuart_device_config *config = dev->config;
	if (!litex_read8(config->rxempty_addr)) {
		*c = litex_read8(config->rxtx_addr);

		/* refresh UART_RXEMPTY by writing UART_EV_RX
		 * to UART_EV_PENDING
		 */
		litex_write8(UART_EV_RX, config->ev_pending_addr);
		return 0;
	} else {
		return -1;
	}
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
/**
 * @brief Enable TX interrupt in event register
 *
 * @param dev UART device struct
 */
static void uart_liteuart_irq_tx_enable(const struct device *dev)
{
	const struct uart_liteuart_device_config *config = dev->config;
	uint8_t enable = litex_read8(config->ev_enable_addr);

	litex_write8(enable | UART_EV_TX, config->ev_enable_addr);
}

/**
 * @brief Disable TX interrupt in event register
 *
 * @param dev UART device struct
 */
static void uart_liteuart_irq_tx_disable(const struct device *dev)
{
	const struct uart_liteuart_device_config *config = dev->config;
	uint8_t enable = litex_read8(config->ev_enable_addr);

	litex_write8(enable & ~(UART_EV_TX), config->ev_enable_addr);
}

/**
 * @brief Enable RX interrupt in event register
 *
 * @param dev UART device struct
 */
static void uart_liteuart_irq_rx_enable(const struct device *dev)
{
	const struct uart_liteuart_device_config *config = dev->config;
	uint8_t enable = litex_read8(config->ev_enable_addr);
	litex_write8(enable | UART_EV_RX, config->ev_enable_addr);
}

/**
 * @brief Disable RX interrupt in event register
 *
 * @param dev UART device struct
 */
static void uart_liteuart_irq_rx_disable(const struct device *dev)
{
	const struct uart_liteuart_device_config *config = dev->config;
	uint8_t enable = litex_read8(config->ev_enable_addr);

	litex_write8(enable & ~(UART_EV_RX), config->ev_enable_addr);
}

/**
 * @brief Check if Tx IRQ has been raised and UART is ready to accept new data
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ has been raised, 0 otherwise
 */
static int uart_liteuart_irq_tx_ready(const struct device *dev)
{
	const struct uart_liteuart_device_config *config = dev->config;
	uint8_t val = litex_read8(config->txfull_addr);

	return !val;
}

/**
 * @brief Check if Rx IRQ has been raised and there's data to be read from UART
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ has been raised, 0 otherwise
 */
static int uart_liteuart_irq_rx_ready(const struct device *dev)
{
	uint8_t pending;
	const struct uart_liteuart_device_config *config = dev->config;

	pending = litex_read8(config->ev_pending_addr);

	if (pending & UART_EV_RX) {
		return 1;
	} else {
		return 0;
	}
}

/**
 * @brief Fill FIFO with data
 *
 * @param dev UART device struct
 * @param tx_data Data to transmit
 * @param size Number of bytes to send
 *
 * @return Number of bytes sent
 */
static int uart_liteuart_fifo_fill(const struct device *dev,
				   const uint8_t *tx_data, int size)
{
	int i;
	const struct uart_liteuart_device_config *config = dev->config;

	for (i = 0; i < size && !litex_read8(config->txfull_addr); i++) {
		litex_write8(tx_data[i], config->rxtx_addr);
	}

	return i;
}

/**
 * @brief Read data from FIFO
 *
 * @param dev UART device struct
 * @param rxData Data container
 * @param size Container size
 *
 * @return Number of bytes read
 */
static int uart_liteuart_fifo_read(const struct device *dev,
				   uint8_t *rx_data, const int size)
{
	int i;
	const struct uart_liteuart_device_config *config = dev->config;

	for (i = 0; i < size && !litex_read8(config->rxempty_addr); i++) {
		rx_data[i] = litex_read8(config->rxtx_addr);

		/* refresh UART_RXEMPTY by writing UART_EV_RX
		 * to UART_EV_PENDING
		 */
		litex_write8(UART_EV_RX, config->ev_pending_addr);
	}

	return i;
}

static void uart_liteuart_irq_err(const struct device *dev)
{
	ARG_UNUSED(dev);
}

/**
 * @brief Check if any IRQ is pending
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is pending, 0 otherwise
 */
static int uart_liteuart_irq_is_pending(const struct device *dev)
{
	uint8_t pending;
	const struct uart_liteuart_device_config *config = dev->config;

	pending = litex_read8(config->ev_pending_addr);

	if (pending & (UART_EV_TX | UART_EV_RX)) {
		return 1;
	} else {
		return 0;
	}
}

static int uart_liteuart_irq_update(const struct device *dev)
{
	return 1;
}

/**
 * @brief Set the callback function pointer for IRQ.
 *
 * @param dev UART device struct
 * @param cb Callback function pointer.
 */
static void uart_liteuart_irq_callback_set(const struct device *dev,
					   uart_irq_callback_user_data_t cb,
					   void *cb_data)
{
	struct uart_liteuart_data *data;
	data = dev->data;
	data->callback = cb;
	data->cb_data = cb_data;
}

static void liteuart_uart_irq_handler(const struct device *dev)
{
	struct uart_liteuart_data *data = dev->data;
	unsigned int key = irq_lock();
	const struct uart_liteuart_device_config *config = dev->config;

	if (data->callback) {
		data->callback(dev, data->cb_data);
	}

	/* Clear RX events, TX events still needed to enqueue the next transfer */
	litex_write8(UART_EV_RX, UART_EV_PENDING_ADDR);
	/* clear events */
	//litex_write8(UART_EV_TX | UART_EV_RX, config->ev_pending_addr);

	irq_unlock(key);
}
#endif	/* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_liteuart_driver_api = {
	.poll_in		= uart_liteuart_poll_in,
	.poll_out		= uart_liteuart_poll_out,
	.err_check		= NULL,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill		= uart_liteuart_fifo_fill,
	.fifo_read		= uart_liteuart_fifo_read,
	.irq_tx_enable		= uart_liteuart_irq_tx_enable,
	.irq_tx_disable		= uart_liteuart_irq_tx_disable,
	.irq_tx_ready		= uart_liteuart_irq_tx_ready,
	.irq_rx_enable		= uart_liteuart_irq_rx_enable,
	.irq_rx_disable		= uart_liteuart_irq_rx_disable,
	.irq_rx_ready		= uart_liteuart_irq_rx_ready,
	.irq_err_enable		= uart_liteuart_irq_err,
	.irq_err_disable	= uart_liteuart_irq_err,
	.irq_is_pending		= uart_liteuart_irq_is_pending,
	.irq_update		= uart_liteuart_irq_update,
	.irq_callback_set	= uart_liteuart_irq_callback_set
#endif
};

static int uart_liteuart_init(const struct device *dev)
{
	const struct uart_liteuart_device_config *config = dev->config;
	litex_write8(UART_EV_TX | UART_EV_RX, config->ev_pending_addr);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif

	return 0;
}
extern void plic_irq_handler2(void *a);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define LITEX_UART_CONFIG_FUNC(node_id, n) \
	static void uart_config_func_##n(const struct device *dev) { \
	IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), \
			liteuart_uart_irq_handler, DEVICE_DT_INST_GET(n), \
			0); \
 	irq_enable(DT_INST_IRQN(n)); \
	}

#define LITEX_UART_CONFIG_INIT(node_id, n) \
	.irq_config_func = uart_config_func_##n

#else
#define LITEX_UART_CONFIG_FUNC(node_id, n)
#define LITEX_UART_CONFIG_INIT(node_id, n)
#endif	/* CONFIG_UART_INTERRUPT_DRIVEN */


#define LITEX_UART_INIT(node_id, n) \
	LITEX_UART_CONFIG_FUNC(node_id, n) \
	static struct uart_liteuart_data litex_uart_##n##_data; \
	static const struct uart_liteuart_device_config litex_uart_##n##_config = { \
		.port		= DT_INST_REG_ADDR_BY_NAME(n, rxtx), \
		.baud_rate	= DT_INST_PROP(n, current_speed), \
		.rxtx_addr  = DT_INST_REG_ADDR_BY_NAME(n, rxtx),  \
		.txfull_addr = DT_INST_REG_ADDR_BY_NAME(n, txfull), \
		.rxempty_addr = DT_INST_REG_ADDR_BY_NAME(n, rxempty), \
		.ev_status_addr = DT_INST_REG_ADDR_BY_NAME(n, ev_status), \
		.ev_pending_addr = DT_INST_REG_ADDR_BY_NAME(n, ev_pending), \
		.ev_enable_addr = DT_INST_REG_ADDR_BY_NAME(n, ev_enable), \
		.txempty_addr = DT_INST_REG_ADDR_BY_NAME(n, txempty), \
		.rxfull_addr = DT_INST_REG_ADDR_BY_NAME(n, rxfull), \
		LITEX_UART_CONFIG_INIT(node_id, n)			\
	}; \
	DEVICE_DT_DEFINE(node_id, &uart_liteuart_init,			\
		 PM_DEVICE_DT_GET(node_id),			\
		 &litex_uart_##n##_data,			\
		 &litex_uart_##n##_config,			\
		 PRE_KERNEL_1,					\
		 CONFIG_SERIAL_INIT_PRIORITY,			\
		 (void *) &uart_liteuart_driver_api)

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(uart0), DT_DRV_COMPAT, okay)
LITEX_UART_INIT(DT_NODELABEL(uart0), 0);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(uart1), DT_DRV_COMPAT, okay)
LITEX_UART_INIT(DT_NODELABEL(uart1), 1);
#endif
/* just in case, one uses 'liteuart...' as this is the output for the Linux DTS */
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(liteuart0), DT_DRV_COMPAT, okay)
LITEX_UART_INIT(DT_NODELABEL(liteuart0), 0);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(liteuart1), DT_DRV_COMPAT, okay)
LITEX_UART_INIT(DT_NODELABEL(liteuart1), 1);
#endif

