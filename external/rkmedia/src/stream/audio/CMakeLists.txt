#
# Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

# vi: set noexpandtab syntax=cmake:

option(ALSA_PLAYBACK "compile: alsa playback" OFF)
option(ALSA_CAPTURE "compile: alsa capture" OFF)

if(ALSA_PLAYBACK OR ALSA_CAPTURE)

  pkg_check_modules(ALSA REQUIRED alsa)
  set(EASY_MEDIA_STREAM_ALSA_LIBS asound)
  set(EASY_MEDIA_STREAM_ALSA_SOURCE_FILES
	  stream/audio/alsa/alsa_utils.cc
	  stream/audio/rk_audio.cc
	  stream/audio/alsa/alsa_volume.cc)

  if(ALSA_PLAYBACK)
    set(EASY_MEDIA_STREAM_ALSA_SOURCE_FILES
        ${EASY_MEDIA_STREAM_ALSA_SOURCE_FILES}
        stream/audio/alsa/alsa_playback_stream.cc)
  endif()

  if(ALSA_CAPTURE)
    set(EASY_MEDIA_STREAM_ALSA_SOURCE_FILES
        ${EASY_MEDIA_STREAM_ALSA_SOURCE_FILES}
        stream/audio/alsa/alsa_capture_stream.cc)
  endif()

  if(AUDIO_ALGORITHM)
    set(EASY_MEDIA_STREAM_AUDIO_ALGO_LIBS RKAP_3A RKAP_ANR RKAP_Common)
  endif()
endif()

set(EASY_MEDIA_STREAM_SOURCE_FILES
    ${EASY_MEDIA_STREAM_SOURCE_FILES} ${EASY_MEDIA_STREAM_ALSA_SOURCE_FILES}
    PARENT_SCOPE)
set(EASY_MEDIA_STREAM_LIBS ${EASY_MEDIA_STREAM_LIBS}
                           ${EASY_MEDIA_STREAM_ALSA_LIBS}
                           ${EASY_MEDIA_STREAM_AUDIO_ALGO_LIBS} PARENT_SCOPE)

# playback test ref to ogg/test/ogg_decode_test.cc

# capture test ref to ogg/test/ogg_encode_test.cc
