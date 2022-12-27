#
# Copyright (c) 2018 - 2019 Antmicro <www.antmicro.com>
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources(
    ../riscv-privilege/common/soc_irq.S
    ../riscv-privilege/common/vector.S
)

zephyr_sources_ifdef(CONFIG_BOARD_ORANGECART_SMP irq.c)
zephyr_sources_ifdef(CONFIG_BOARD_ORANGECART_FPU irq.c)
