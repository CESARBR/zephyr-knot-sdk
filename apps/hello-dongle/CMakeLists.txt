cmake_minimum_required(VERSION 3.8.2)

if (NOT DEFINED ENV{KNOT_BASE})
    message(FATAL_ERROR "Source the KNoT shell initialize script!")
endif()

include($ENV{KNOT_BASE}/core/CMakeLists.txt)
project(Hello)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

include($ENV{ZEPHYR_BASE}/samples/net/common/common.cmake)
