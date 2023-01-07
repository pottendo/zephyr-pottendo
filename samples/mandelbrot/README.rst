.. _mandelbrot:

Mandelbrot
###########

Overview
********

A sample that can be used with any :ref:`supported board <boards>` and
prints calculates the mandelbrot set. Output either to the C64
graphics memory or on the console (use C64=1 in pr.conf to select)

This program makes heavy use of
- posix threading
- FPU calculations
- posix time function
- orangecart's direct C64 memory interface for both controlling the VIC and RAM

Building and Running
********************

This application can be built and executed on the orangecart

.. zephyr-app-commands::
   :zephyr-app: samples/mandelbrot
   :host-os: unix
   :board: orangecart, orangecart_fpu, orangecart_smp
   :goals: run
   :compact:


Sample Output
=============
n/a
