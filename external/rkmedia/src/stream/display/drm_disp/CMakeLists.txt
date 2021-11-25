#
# Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

# vi: set noexpandtab syntax=cmake:

option(DRM_DISPLAY "compile: drm display" OFF)
if(DRM_DISPLAY)
  set(EASY_MEDIA_STREAM_DISPLAY_SOURCE_FILES
      stream/display/drm_disp/drm_utils.cc stream/display/drm_disp/drm_stream.cc
      stream/display/drm_disp/drm_output_stream.cc
      PARENT_SCOPE)
  if(COMPILES_MINIMEDIA)
    set(MINI_MEDIA_STREAM_DISPLAY_SOURCE_FILES
        stream/display/drm_disp/drm_utils.cc stream/display/drm_disp/drm_stream.cc
        stream/display/drm_disp/drm_output_stream.cc
        PARENT_SCOPE)
  endif()
endif()
