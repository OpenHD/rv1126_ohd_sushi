#
# Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

# vi: set noexpandtab syntax=cmake:

project(easymedia_buffer_test)

set(FLOW_TEST_DEPENDENT_LIBS easymedia)

set(CMAKE_CXX_STANDARD 11)

add_definitions(-DDEBUG)

#--------------------------
# buffer_pool_test
#--------------------------
add_executable(buffer_pool_test buffer_pool_test.cc)
target_link_libraries(buffer_pool_test easymedia)
target_include_directories(buffer_pool_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_features(buffer_pool_test PRIVATE cxx_std_11)
install(TARGETS buffer_pool_test RUNTIME DESTINATION "bin")

