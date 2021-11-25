#
# Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

# vi: set noexpandtab syntax=cmake:

if(LIBDRM_FOUND)
  add_subdirectory(drm_disp)
endif()

set(EASY_MEDIA_STREAM_SOURCE_FILES
    ${EASY_MEDIA_STREAM_SOURCE_FILES} ${EASY_MEDIA_STREAM_DISPLAY_SOURCE_FILES}
    PARENT_SCOPE)
set(EASY_MEDIA_STREAM_LIBS ${EASY_MEDIA_STREAM_LIBS}
                           ${EASY_MEDIA_STREAM_DISPLAY_LIBS} PARENT_SCOPE)

if(COMPILES_MINIMEDIA)
  set(MINI_MEDIA_STREAM_SOURCE_FILES
      ${MINI_MEDIA_STREAM_SOURCE_FILES} ${MINI_MEDIA_STREAM_DISPLAY_SOURCE_FILES}
      PARENT_SCOPE)
  set(MINI_MEDIA_STREAM_LIBS ${MINI_MEDIA_STREAM_LIBS}
                            ${MINI_MEDIA_STREAM_DISPLAY_LIBS} PARENT_SCOPE)
endif()