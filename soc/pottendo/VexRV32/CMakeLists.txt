#
# Copyright (c) 2018 - 2019 Antmicro <www.antmicro.com>
#
# SPDX-License-Identifier: Apache-2.0
#
set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")

zephyr_sources(
    ../../common/riscv-privileged/soc_irq.S
    ../../common/riscv-privileged/vector.S
)

