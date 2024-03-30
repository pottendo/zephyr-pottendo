#
# Copyright (c) 2018 - 2019 Antmicro <www.antmicro.com>
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources(
    ../../common/riscv-privileged/soc_irq.S
    ../../common/riscv-privileged/vector.S
)

zephyr_sources_ifdef(CONFIG_BOARD_ORANGECART_SMP irq.c)
zephyr_sources_ifdef(CONFIG_BOARD_ORANGECART_FPU irq.c)
zephyr_sources_ifdef(CONFIG_BOARD_ORANGECART2 irq.c)
