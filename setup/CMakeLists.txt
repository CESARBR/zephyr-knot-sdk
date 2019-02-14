cmake_minimum_required(VERSION 3.8.2)

# Change dts with sub-partitions and use specific sub-partition to flash code
set(DTC_OVERLAY_MULTI_SLOT
	$ENV{KNOT_BASE}/core/boards/${BOARD}-sub-slots.overlay
)
set(DTC_OVERLAY_SET_SLOT
	${CMAKE_CURRENT_SOURCE_DIR}/boards/${BOARD}-code-slot.overlay
)
set(DTC_OVERLAY_FILE
	"${DTC_OVERLAY_FILE} ${DTC_OVERLAY_MULTI_SLOT} ${DTC_OVERLAY_SET_SLOT}"
)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(knot_setup)

FILE(GLOB app_sources src/*.c)
FILE(GLOB third_party_sources src/third_party/*.c)
set(core_sources
	$ENV{KNOT_BASE}/core/src/storage.c
	$ENV{KNOT_BASE}/core/src/storage.nrf52840)

target_sources(app PRIVATE
		${app_sources}
		${core_sources}
		${third_party_sources}
)

zephyr_include_directories($ENV{KNOT_BASE}/core/src/)
