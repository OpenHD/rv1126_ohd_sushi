#
# Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

# vi: set noexpandtab syntax=cmake:

project(easymedia_rkaudio_test)

set(RKAUDIO_TEST_DEPENDENT_LIBS easymedia)

set(CMAKE_CXX_STANDARD 11)

add_definitions(-DDEBUG)

add_executable(rkaudio_enc_mux_test rkaudio_enc_mux_test.cc)
target_compile_features(rkaudio_enc_mux_test PRIVATE cxx_std_17)
add_dependencies(rkaudio_enc_mux_test easymedia)
target_link_libraries(rkaudio_enc_mux_test ${RKAUDIO_TEST_DEPENDENT_LIBS})
install(TARGETS rkaudio_enc_mux_test RUNTIME DESTINATION "bin")
