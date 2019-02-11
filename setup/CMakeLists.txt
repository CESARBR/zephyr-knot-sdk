cmake_minimum_required(VERSION 3.8.2)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(knot_setup)

FILE(GLOB app_sources src/*.c)
set(core_sources
	$ENV{KNOT_BASE}/core/src/storage.c
	$ENV{KNOT_BASE}/core/src/storage.nrf52840)

target_sources(app PRIVATE ${app_sources} ${core_sources})
zephyr_include_directories($ENV{KNOT_BASE}/core/src/)
