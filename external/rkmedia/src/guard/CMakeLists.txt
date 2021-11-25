#
# Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

# vi: set noexpandtab syntax=cmake:

option(RKGUARD "compile: rkguard wrapper" OFF)
if(RKGUARD)
  set(EASY_MEDIA_RKGUARD_SOURCE_FILES guard/through_guard.cc)
  set(EASY_MEDIA_SOURCE_FILES ${EASY_MEDIA_SOURCE_FILES}
                              ${EASY_MEDIA_RKGUARD_SOURCE_FILES} PARENT_SCOPE)
  if(COMPILES_MINIMEDIA)
    set(MINI_MEDIA_RKGUARD_SOURCE_FILES guard/through_guard.cc)
    set(MINI_MEDIA_SOURCE_FILES ${MINI_MEDIA_SOURCE_FILES}
                                ${MINI_MEDIA_RKGUARD_SOURCE_FILES} PARENT_SCOPE)
  endif()
endif()
