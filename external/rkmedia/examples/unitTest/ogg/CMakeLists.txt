#
# Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

# vi: set noexpandtab syntax=cmake:

project(easymedia_ogg_test)

set(CMAKE_CXX_STANDARD 11)

add_definitions(-DDEBUG)

option(OGG_DECODE_TEST "compile: ogg decode test" ON)
if(OGG_DECODE_TEST)
  set(DEC_TEST_SRC_FILES ogg_decode_test.cc)
  add_executable(ogg_decode_test ${DEC_TEST_SRC_FILES})
  add_dependencies(ogg_decode_test easymedia)
  target_link_libraries(ogg_decode_test easymedia)
  install(TARGETS ogg_decode_test RUNTIME DESTINATION "bin")
endif()

option(OGG_ENCODE_TEST "compile: ogg encode test" ON)
if(OGG_ENCODE_TEST)
  set(ENC_TEST_SRC_FILES ogg_encode_test.cc)
  add_executable(ogg_encode_test ${ENC_TEST_SRC_FILES})
  add_dependencies(ogg_encode_test easymedia)
  target_link_libraries(ogg_encode_test easymedia)
  install(TARGETS ogg_encode_test RUNTIME DESTINATION "bin")
endif()
