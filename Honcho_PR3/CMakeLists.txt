# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(Honcho_LLU)

#Early init file
zephyr_library()
zephyr_library_sources(board.c)

# header file 
zephyr_include_directories(src/drivers/include)
zephyr_include_directories(src/peripherals/include)
zephyr_include_directories(src/Bluetooth/include)
zephyr_include_directories(src/motor_control/include)
zephyr_include_directories(src/data_logging/include)
zephyr_include_directories(src/calibration/include)

# source file
FILE(GLOB app_sources src/*.c)
FILE(GLOB driver_sources src/drivers/*.c)
FILE(GLOB peripheral_sources src/peripherals/*.c)
FILE(GLOB ble_sources src/Bluetooth/*.c/)
FILE(GLOB motor_control_sources src/motor_control/*.c)
FILE(GLOB datalog_sources src/data_logging/*.c)
FILE(GLOB calib_sources src/calibration/*.c)
target_sources(app PRIVATE ${app_sources} ${driver_sources} ${peripheral_sources} ${ble_sources} 
                                        ${motor_control_sources} ${datalog_sources} ${calib_sources})