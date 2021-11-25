#
# Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
cmake_minimum_required(VERSION 3.5)

project(dbtool)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2 -pthread")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -O2 -pthread")

aux_source_directory(. dir_src)
aux_source_directory(thirdparty/sqlite3 sql_src)
aux_source_directory(thirdparty/nlohmann_json json_src)

add_executable(dbtool
    ${dir_src} ${sql_src} ${json_src})

target_link_libraries(dbtool
    PRIVATE
	dl)


install(TARGETS dbtool
	RUNTIME DESTINATION bin)
