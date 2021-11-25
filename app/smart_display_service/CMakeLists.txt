# ----------------------------------------------------------------------------
#
# johnny.zhang@rock-chips.com
#
# SPDX-License-Identifier: Apache-2.0
#
# ----------------------------------------------------------------------------

cmake_minimum_required (VERSION 3.1.0 FATAL_ERROR)

project(smartservice)
include(GNUInstallDirs)

include(FindPkgConfig)
pkg_check_modules (GLIB REQUIRED IMPORTED_TARGET glib-2.0)
pkg_check_modules (DBUS REQUIRED IMPORTED_TARGET dbus-1)

set(CMAKE_CXX_STANDARD 11)
#add_definitions(-fno-rtti)
#add_definitions(-fexceptions -finstrument-functions -funwind-tables -g -rdynamic -O0)
add_definitions(-O2)
add_definitions("-fpermissive -g")
include_directories(./)
include_directories(./net)
include_directories(./shmc)
include_directories(./event)
include_directories(./dbus)
include_directories(./utils)
option(NDEBUG OFF)
if(NDEBUG)
  add_definitions("-DNDEBUG")
endif()
set(RK_NPU_RNDIS_NET_SRC
      main.cpp
      net/ccond.c
      net/cmutex.c
      net/cthread.c
      net/tcp_control.cpp
      net/tcp_server.cpp
      net/shm_control.cpp
      net/tcp_response.cpp
      event/event_handle.cpp
      dbus/dbus_manage.c
      dbus/dbus_helps.cpp
    )

find_package(Protobuf REQUIRED)
if(PROTOBUF_FOUND)
    message(STATUS "protobuf library found")
    include_directories(${PROTOBUF_INCLUDE_DIRS})
    include_directories(${CMAKE_CURRENT_BINARY_DIR})
    protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/net/uvc_connect.proto)
    include_directories(${PROTO_HDRS})
    set(RK_NPU_RNDIS_NET_SRC ${RK_NPU_RNDIS_NET_SRC} ${PROTO_SRCS})
endif()

option(ENABLE_MINILOGGER  "enbale minilogger" ON)
if (${ENABLE_MINILOGGER})
    find_package(MiniLogger REQUIRED)
    add_definitions(-DENABLE_MINILOGGER)
    set(SMART_DISPLAY_SERVICE_LIB ${SMART_DISPLAY_SERVICE_LIB} MiniLogger::MiniLogger)
endif()

#set(DBUS_TEST test/dbus_test.c)
#add_executable(dbus_test ${DBUS_TEST})
#target_link_libraries(dbus_test pthread gdbus PkgConfig::GLIB PkgConfig::DBUS)
#install(TARGETS dbus_test RUNTIME DESTINATION "bin")

#Add Marco for update
option(TMP_UPDATE_LOCATION "enable save Update to tmp" ON)
if (${TMP_UPDATE_LOCATION})
    add_definitions("-DTMP_UPDATE_LOCATION")
endif()

add_executable(smart_display_service ${RK_NPU_RNDIS_NET_SRC})
target_link_libraries(smart_display_service ${SMART_DISPLAY_SERVICE_LIB} pthread protobuf-lite crypto event_core event_extra  event_pthreads gdbus PkgConfig::GLIB PkgConfig::DBUS)
install(TARGETS smart_display_service RUNTIME DESTINATION "bin")

