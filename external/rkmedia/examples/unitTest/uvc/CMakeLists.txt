#
# Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

# vi: set noexpandtab syntax=cmake:

project(easymedia_uvc_test)

set(RKMPP_TEST_DEPENDENT_LIBS easymedia)

add_definitions(-DDEBUG)

#--------------------------
# uvc_flow_test
#--------------------------
if(UVC)
  set(UVC_FLOW_TEST_SRC_FILES uvc_flow_test.cc)
  add_executable(uvc_flow_test ${UVC_FLOW_TEST_SRC_FILES})
  add_dependencies(uvc_flow_test easymedia)
  target_link_libraries(uvc_flow_test easymedia)
  target_include_directories(uvc_flow_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
  target_compile_features(uvc_flow_test PRIVATE cxx_std_11)
  install(TARGETS uvc_flow_test RUNTIME DESTINATION "bin")
endif()#UVC

