#
# Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

# vi: set noexpandtab syntax=cmake:

#--------------------------
# drm_display_test
#--------------------------

project(easymedia_stream_test)

set(STREAM_TEST_DEPENDENT_LIBS easymedia)

set(CMAKE_CXX_STANDARD 11)

add_definitions(-DDEBUG)

if(LIBDRM_FOUND)
if(DRM_DISPLAY)
set(DRM_DISPLAY_TEST_SRC_FILES drm_display_test.cc)
add_executable(drm_display_test ${DRM_DISPLAY_TEST_SRC_FILES})
add_dependencies(drm_display_test easymedia)
target_link_libraries(drm_display_test ${STREAM_TEST_DEPENDENT_LIBS})
install(TARGETS drm_display_test RUNTIME DESTINATION "bin")
endif() #DRM_DISPLAY
endif() #LIBDRM_FOUND

#--------------------------
# camera_capture_test
#--------------------------
if(V4L2_CAPTURE)
  set(CAMERA_CAP_TEST_SRC_FILES camera_capture_test.cc)
  add_executable(camera_cap_test ${CAMERA_CAP_TEST_SRC_FILES})
  add_dependencies(camera_cap_test easymedia)
  target_link_libraries(camera_cap_test ${STREAM_TEST_DEPENDENT_LIBS})
  install(TARGETS camera_cap_test RUNTIME DESTINATION "bin")
endif()