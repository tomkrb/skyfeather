# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# set(SHIELD adafruit_2900_oled adafruit_4565_9dof_imu adafruit_3632_joy)
set(SHIELD adafruit_2900_oled adafruit_4565_9dof_imu)


find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(skyfeather)

target_sources(app PRIVATE src/main.c)
