#
#Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd.All rights reserved.
#Use of this source code is governed by a BSD - style license that can be
#found in the LICENSE file.
#

#vi : set noexpandtab syntax = cmake:

option(ANR "compile: ANR filter" OFF)

if (ANR)
  set(EASY_MEDIA_FILTER_SOURCE_FILES
      filter/anr.cc)
  set(EASY_MEDIA_FILTER_DEPENDENT_LIBS
      RKAP_ANR
      RKAP_Common)
endif()

option(AEC "compile: AEC filter" OFF)

if (AEC)
  set(EASY_MEDIA_FILTER_SOURCE_FILES
      ${EASY_MEDIA_FILTER_SOURCE_FILES}
      filter/aec.cc)
  set(EASY_MEDIA_FILTER_DEPENDENT_LIBS
      ${EASY_MEDIA_FILTER_DEPENDENT_LIBS}
      RKAP_3A
      RKAP_Common)
endif()

set(EASY_MEDIA_SOURCE_FILES
    ${EASY_MEDIA_SOURCE_FILES}
    ${EASY_MEDIA_FILTER_SOURCE_FILES}
    PARENT_SCOPE)
set(EASY_MEDIA_DEPENDENT_LIBS
    ${EASY_MEDIA_DEPENDENT_LIBS}
    ${EASY_MEDIA_FILTER_DEPENDENT_LIBS}
    PARENT_SCOPE)
