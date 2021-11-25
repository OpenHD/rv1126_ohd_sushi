#
# Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

# vi: set noexpandtab syntax=cmake:

option(V4L2_OUTPUT "compile: v4l2 output" OFF)
option(V4L2_CAPTURE "compile: v4l2 capture" OFF)

if(V4L2_OUTPUT OR V4L2_CAPTURE)

  pkg_check_modules(LIBV4L2 libv4l2)
  if(LIBV4L2_FOUND)
    set(EASY_MEDIA_STREAM_COMPILE_DEFINITIONS
        ${EASY_MEDIA_STREAM_COMPILE_DEFINITIONS} -DHAVE_LIBV4L2
        PARENT_SCOPE)
    set(EASY_MEDIA_STREAM_V4L2_LIBS v4l2)
    if(COMPILES_MINIMEDIA)
      set(MINI_MEDIA_STREAM_COMPILE_DEFINITIONS
          ${MINI_MEDIA_STREAM_COMPILE_DEFINITIONS} -DHAVE_LIBV4L2
          PARENT_SCOPE)
      set(MINI_MEDIA_STREAM_V4L2_LIBS v4l2)
    endif()
  endif()

  set(EASY_MEDIA_STREAM_V4L2_SOURCE_FILES stream/camera/v4l2_utils.cc
                                          stream/camera/v4l2_stream.cc)

  # include aiq && v4l2 capture
  if(COMPILES_MINIMEDIA)
    set(MINI_MEDIA_STREAM_V4L2_SOURCE_FILES stream/camera/v4l2_utils.cc
                                            stream/camera/v4l2_stream.cc)
  endif()

  if(USE_RKAIQ)
    set(EASY_MEDIA_STREAM_V4L2_SOURCE_FILES
        ${EASY_MEDIA_STREAM_V4L2_SOURCE_FILES}
        stream/camera/rkaiq_media.cc)
    if(COMPILES_MINIMEDIA)
      set(MINI_MEDIA_STREAM_V4L2_SOURCE_FILES
        ${MINI_MEDIA_STREAM_V4L2_SOURCE_FILES}
        stream/camera/rkaiq_media.cc)
    endif()
  endif()

  if(V4L2_CAPTURE)
    set(EASY_MEDIA_STREAM_V4L2_SOURCE_FILES
        ${EASY_MEDIA_STREAM_V4L2_SOURCE_FILES}
        stream/camera/v4l2_capture_stream.cc)
    if(COMPILES_MINIMEDIA)
      set(MINI_MEDIA_STREAM_V4L2_SOURCE_FILES
        ${MINI_MEDIA_STREAM_V4L2_SOURCE_FILES}
        stream/camera/v4l2_capture_stream.cc)
    endif()

    set(EASY_MEDIA_STREAM_V4L2_SOURCE_FILES
        ${EASY_MEDIA_STREAM_V4L2_SOURCE_FILES}
        stream/camera/v4l2_output_stream.cc)
    if(COMPILES_MINIMEDIA)
      set(MINI_MEDIA_STREAM_V4L2_SOURCE_FILES
        ${MINI_MEDIA_STREAM_V4L2_SOURCE_FILES}
        stream/camera/v4l2_output_stream.cc)
    endif()
  endif()
endif()

set(EASY_MEDIA_STREAM_SOURCE_FILES
    ${EASY_MEDIA_STREAM_SOURCE_FILES} ${EASY_MEDIA_STREAM_V4L2_SOURCE_FILES}
    PARENT_SCOPE)
set(EASY_MEDIA_STREAM_LIBS ${EASY_MEDIA_STREAM_LIBS}
                           ${EASY_MEDIA_STREAM_V4L2_LIBS} PARENT_SCOPE)

if(COMPILES_MINIMEDIA)
  set(MINI_MEDIA_STREAM_SOURCE_FILES
      ${MINI_MEDIA_STREAM_SOURCE_FILES} ${MINI_MEDIA_STREAM_V4L2_SOURCE_FILES}
      PARENT_SCOPE)
  set(MINI_MEDIA_STREAM_LIBS ${MINI_MEDIA_STREAM_LIBS}
                            ${MINI_MEDIA_STREAM_V4L2_LIBS} PARENT_SCOPE)
endif()
