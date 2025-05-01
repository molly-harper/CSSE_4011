# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(base_node_observer2)

FILE(GLOB app_sources src/*.c)

FILE(GLOB lib_sources)



# Tell CMake to build with the app and lib sources
target_sources(app PRIVATE ${app_sources} ${lib_sources})

# Tell CMake where our header files are
target_include_directories(app PRIVATE ../../include)