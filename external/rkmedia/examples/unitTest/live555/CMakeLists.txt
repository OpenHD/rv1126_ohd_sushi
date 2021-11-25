#
# Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

# vi: set noexpandtab syntax=cmake:

project(easymedia_rtsp_server_test)

set(RTSP_SERVER_TEST_DEPENDENT_LIBS ${RTSP_SERVER_TEST_DEPENDENT_LIBS}
                                    easymedia)

set(CMAKE_CXX_STANDARD 11)

add_definitions(-DDEBUG)

option(RTSP_SERVER_TEST "compile: rtsp server test" ON)

if(RTSP_SERVER_TEST)
    set(RTSP_SERVER_TEST_SRC_FILES rtsp_server_test.cc)
    add_executable(rtsp_server_test ${RTSP_SERVER_TEST_SRC_FILES})
    target_link_libraries(rtsp_server_test easymedia)
    target_include_directories(rtsp_server_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_compile_features(rtsp_server_test PRIVATE cxx_std_11)
    install(TARGETS rtsp_server_test RUNTIME DESTINATION "bin")
endif()

option(RTSP_MULTI_SERVER_TEST "compile: multi rtsp server test" ON)

if(RTSP_MULTI_SERVER_TEST)
    set(RTSP_MULTI_SERVER_TEST_SRC_FILES rtsp_multi_server_test.cc)
    add_executable(rtsp_multi_server_test ${RTSP_MULTI_SERVER_TEST_SRC_FILES})
    target_link_libraries(rtsp_multi_server_test easymedia)
    target_include_directories(rtsp_multi_server_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_compile_features(rtsp_multi_server_test PRIVATE cxx_std_11)
    install(TARGETS rtsp_multi_server_test RUNTIME DESTINATION "bin")
endif()
