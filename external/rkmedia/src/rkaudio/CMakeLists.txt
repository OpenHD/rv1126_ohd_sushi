#
# Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

# vi: set noexpandtab syntax=cmake:

option(RKAUDIO "compile: rkaudio wrapper" OFF)

set(EASY_MEDIA_RKAUDIO_SOURCE_FILES rkaudio/rkaudio_utils.cc)
if(MUXER)
  set(EASY_MEDIA_RKAUDIO_SOURCE_FILES ${EASY_MEDIA_RKAUDIO_SOURCE_FILES}
                                     rkaudio/rkaudio_muxer.cc)
endif()
if(ENCODER)
  set(EASY_MEDIA_RKAUDIO_SOURCE_FILES ${EASY_MEDIA_RKAUDIO_SOURCE_FILES}
                                     rkaudio/rkaudio_aud_encoder.cc)
  set(EASY_MEDIA_RKAUDIO_SOURCE_FILES ${EASY_MEDIA_RKAUDIO_SOURCE_FILES}
                                     rkaudio/rkaudio_vid_encoder.cc)
endif()

if(DECODER)
  set(EASY_MEDIA_RKAUDIO_SOURCE_FILES ${EASY_MEDIA_RKAUDIO_SOURCE_FILES}
                                     rkaudio/rkaudio_aud_decoder.cc)
  set(EASY_MEDIA_RKAUDIO_SOURCE_FILES ${EASY_MEDIA_RKAUDIO_SOURCE_FILES}
                                     rkaudio/rkaudio_vid_decoder.cc)
endif()

if(FILTER)
  set(EASY_MEDIA_RKAUDIO_SOURCE_FILES ${EASY_MEDIA_RKAUDIO_SOURCE_FILES}
                                     rkaudio/rkaudio_resample.cc
                                     rkaudio/rkaudio_audio_fifo.cc)
endif()

set(EASY_MEDIA_SOURCE_FILES ${EASY_MEDIA_SOURCE_FILES}
                            ${EASY_MEDIA_RKAUDIO_SOURCE_FILES} PARENT_SCOPE)
set(EASY_MEDIA_DEPENDENT_LIBS
    ${EASY_MEDIA_DEPENDENT_LIBS}
    avformat
    rkcodec
    swresample
    avutil
    PARENT_SCOPE)
