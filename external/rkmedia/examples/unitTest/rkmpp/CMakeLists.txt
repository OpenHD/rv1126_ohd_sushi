#
# Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

# vi: set noexpandtab syntax=cmake:

project(easymedia_rkmpp_test)

set(RKMPP_TEST_DEPENDENT_LIBS easymedia)

set(CMAKE_CXX_STANDARD 11)

add_definitions(-DDEBUG)

if(RKMPP_ENCODER)
  set(RKMPP_ENC_TEST_SRC_FILES mpp_enc_test.cc)
  add_executable(rkmpp_enc_test ${RKMPP_ENC_TEST_SRC_FILES})
  add_dependencies(rkmpp_enc_test easymedia)
  target_link_libraries(rkmpp_enc_test ${RKMPP_TEST_DEPENDENT_LIBS})
  install(TARGETS rkmpp_enc_test RUNTIME DESTINATION "bin")
endif()

if(RKMPP_DECODER)
  set(RKMPP_DEC_TEST_SRC_FILES mpp_dec_test.cc)
  add_executable(rkmpp_dec_test ${RKMPP_DEC_TEST_SRC_FILES})
  add_dependencies(rkmpp_dec_test easymedia)
  target_link_libraries(rkmpp_dec_test ${RKMPP_TEST_DEPENDENT_LIBS})
  install(TARGETS rkmpp_dec_test RUNTIME DESTINATION "bin")
endif()
