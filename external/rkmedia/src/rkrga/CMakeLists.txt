#
# Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

# vi: set noexpandtab syntax=cmake:

option(RKRGA "compile: rkrga wrapper" OFF)
if(RKRGA)

  set(EASY_MEDIA_RKRGA_SOURCE_FILES rkrga/rga.cc)
  set(EASY_MEDIA_SOURCE_FILES ${EASY_MEDIA_SOURCE_FILES}
                              ${EASY_MEDIA_RKRGA_SOURCE_FILES} PARENT_SCOPE)
  set(EASY_MEDIA_DEPENDENT_LIBS ${EASY_MEDIA_DEPENDENT_LIBS} rga PARENT_SCOPE)

  # cmake-format: off
  # option(RKRGA_TEST "compile: rkrga wrapper test" ON)
  # if (RKRGA_TEST)
  # add_subdirectory(test)
  # endif()
  # cmake-format: on

  if(COMPILES_MINIMEDIA)
    set(MINI_MEDIA_RKRGA_SOURCE_FILES rkrga/rga.cc)
    set(MINI_MEDIA_SOURCE_FILES ${MINI_MEDIA_SOURCE_FILES}
                                ${MINI_MEDIA_RKRGA_SOURCE_FILES} PARENT_SCOPE)
    set(MINI_MEDIA_DEPENDENT_LIBS ${MINI_MEDIA_DEPENDENT_LIBS} rga PARENT_SCOPE)
  endif()
endif()
