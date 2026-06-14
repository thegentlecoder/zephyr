# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(BOARD_FLASH_RUNNER stm32cubeprogrammer)
set(BOARD_DEBUG_RUNNER stlink_gdbserver)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(auxdisplay)

target_sources(app PRIVATE src/main.c)
