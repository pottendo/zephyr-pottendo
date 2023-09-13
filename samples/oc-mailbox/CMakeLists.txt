# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cpp)
zephyr_compile_definitions(ZEPHYR=1 C64=1)
target_sources(app PRIVATE src/main.cpp src/c64-lib.cpp src/oc-coproc.cpp)
