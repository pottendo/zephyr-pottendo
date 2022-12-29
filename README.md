# Zephyr OrangeCart FPU (& SMP) support

Look [here][2] for general inforamtion about *Zephyr*.

This project relies on bitstreams (FPGA firmware) for the [OrangeCart][4] from [this][3] project.

This adds 3 more 'boards' to Zephyr RTOS:
- Orangecart: use `-b orangecard` for builds with `west` (RVCop64 original bitstream)
- Orangecart FPU support: use `-b orangecard_fpu` for builds with `west` (requires 'RVCop64-scfp' bitstream from [here][3])

Prepared support for 
- Orangecart SMP (DualCore): use `-b orangecard_smp` for builds with `west` (requires 'RVCop64-dc' bitstream from [here][3])  
  This will not enable the second CPU, as this is not yet supported by Zephyr (i.e. enabling CONFIG_SMP will fail to build).

## Devevelopment Environment

Checkout this repo to your favorite local destination and follow the instructions on [Zephyr][1] to setup the local development environment. 

**However, instead of step 5. `west update`, just make a symlink `zephyr -> <path-to>/zephyr-pottendo`.**

[1]: https://docs.zephyrproject.org/latest/develop/getting_started/index.html
[2]: https://docs.zephyrproject.org/latest/introduction/index.html#
[3]: https://github.com/pottendo/RVCop64-pottendo
[4]: https://github.com/zeldin/OrangeCart
