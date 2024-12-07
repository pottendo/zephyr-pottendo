# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
add_compile_options(-DPTHREADS)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cpp)
target_sources(app PRIVATE /work/src/pottendo-mandel/mandel.cpp /work/src/pottendo-mandel/c64-lib.cpp)
