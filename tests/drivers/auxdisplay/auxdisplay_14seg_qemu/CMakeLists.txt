# SPDX-License-Identifier: Apache-2.0

message(STATUS "Start of CMakeList.txt")

cmake_minimum_required(VERSION 3.20.0)

message(STATUS "CMake current source dir is: ${CMAKE_CURRENT_SOURCE_DIR}")
list(APPEND DTS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

message(STATUS "Zephyr base is: ${ZEPHYR_BASE}")
list(APPEND DTS_ROOT "${ZEPHYR_BASE}/drivers/auxdisplay")

project(auxdisplay_14seg_qemu)

include_directories(SYSTEM src)
target_compile_options(app PRIVATE "-include" "${CMAKE_CURRENT_SOURCE_DIR}/src/mock_stm32l4xx_hal.h")
if(TARGET zephyr_interface)
	target_compile_options(zephyr_interface INTERFACE "-include" "${CMAKE_CURRENT_SOURCE_DIR}/src/mock_stm32l4xx_hal.h")
endif()

target_sources(app PRIVATE
	src/main.c
	src/mock_pinctrl_soc.c
	src/mock_stm32_clock_control.c
	src/mock_stm32l4xx_hal.c)

# Driver files to test
zephyr_library_include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")
zephyr_library_sources(
	"${ZEPHYR_BASE}/drivers/auxdisplay/auxdisplay_slcd_14seg.c"
	"${ZEPHYR_BASE}/drivers/auxdisplay/auxdisplay_st_slcd.c")

message(STATUS "End of CMakeList.txt")
