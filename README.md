# Zephyr OrangeCart FPU (& SMP) support

Look [here][2] for general information about *Zephyr*.

This project relies on bitstreams (FPGA firmware) for the [OrangeCart][4] from [this][3] project.

This adds 3 more 'boards' to Zephyr RTOS:
- Orangecart: use `-b orangecard/VexRV32` for builds with `west` (RVCop64 original bitstream)
- Orangecart FPU support: use `-b orangecart/VexRV32SMP/fpu` for builds with `west` (requires 'RVCop64-scfp' bitstream from [here][3])
- Orangecart SMP (DualCore): use `-b orangecart/VexRV32SMP/smp` for builds with `west` (requires 'RVCop64-dc' bitstream from [here][3])  
- Orangecart IMA instructionset on vexriscv_smp CPU: use `-b orangecart/VexRV32SMP/ima` for builds with `west` (requires 'RVCop64-dc' bitstream from [here][3])  
  
## Development Environment

Checkout this repo to your favorite local destination and follow the instructions on [Zephyr][1] to setup the local development environment. 

**However, instead of step 5. `west update`, move `zephyr -> zephyr.master' and make a symlink `zephyr -> <path-to>/zephyr-pottendo`.**

[1]: https://docs.zephyrproject.org/latest/develop/getting_started/index.html
[2]: https://docs.zephyrproject.org/latest/introduction/index.html#
[3]: https://github.com/pottendo/RVCop64-pottendo
[4]: https://github.com/zeldin/OrangeCart
