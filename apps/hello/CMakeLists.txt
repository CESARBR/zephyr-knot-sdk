cmake_minimum_required(VERSION 3.8.2)

macro(set_conf_file)
  if(EXISTS ${APPLICATION_SOURCE_DIR}/prj_${BOARD}.conf)
    set(CONF_FILE "${APPLICATION_SOURCE_DIR}/prj_${BOARD}.conf")
  elseif(EXISTS ${APPLICATION_SOURCE_DIR}/boards/${BOARD}.conf)
    set(CONF_FILE
      "prj.conf ${APPLICATION_SOURCE_DIR}/boards/${BOARD}.conf")
  else()
    set(CONF_FILE "prj.conf")
  endif()
endmacro()

if (NOT DEFINED ENV{KNOT_BASE})
    message(FATAL_ERROR "Source the KNoT shell initialize script!")
endif()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

FILE(GLOB app_sources src/*.c)
FILE(GLOB core_sources $ENV{KNOT_BASE}/core/src/*.c)
target_sources(app PRIVATE ${core_sources} ${app_sources})
target_include_directories(app PRIVATE $ENV{KNOT_BASE}/core/src)

include($ENV{ZEPHYR_BASE}/samples/net/common/common.cmake)
