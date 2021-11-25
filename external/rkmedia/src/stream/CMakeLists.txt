#
# Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

# vi: set noexpandtab syntax=cmake:

set(EASY_MEDIA_STREAM_SOURCE_FILES stream/file_stream.cc)
set(EASY_MEDIA_STREAM_COMPILE_DEFINITIONS)
set(EASY_MEDIA_STREAM_LIBS)

if(COMPILES_MINIMEDIA)
    set(MINI_MEDIA_STREAM_SOURCE_FILES stream/file_stream.cc)
    set(MINI_MEDIA_STREAM_COMPILE_DEFINITIONS)
    set(MINI_MEDIA_STREAM_LIBS)
endif()

add_subdirectory(audio)
add_subdirectory(camera)
add_subdirectory(display)

set(EASY_MEDIA_SOURCE_FILES ${EASY_MEDIA_SOURCE_FILES}
                            ${EASY_MEDIA_STREAM_SOURCE_FILES} PARENT_SCOPE)
set(EASY_MEDIA_COMPILE_DEFINITIONS
    ${EASY_MEDIA_STREAM_COMPILE_DEFINITIONS}
    PARENT_SCOPE)
set(EASY_MEDIA_DEPENDENT_LIBS ${EASY_MEDIA_DEPENDENT_LIBS}
                              ${EASY_MEDIA_STREAM_LIBS} PARENT_SCOPE)

if(COMPILES_MINIMEDIA)
    set(MINI_MEDIA_SOURCE_FILES ${MINI_MEDIA_SOURCE_FILES}
                                ${MINI_MEDIA_STREAM_SOURCE_FILES} PARENT_SCOPE)
    set(MINI_MEDIA_COMPILE_DEFINITIONS
        ${MINI_MEDIA_STREAM_COMPILE_DEFINITIONS}
        PARENT_SCOPE)
    set(MINI_MEDIA_DEPENDENT_LIBS ${MINI_MEDIA_DEPENDENT_LIBS}
                                ${MINI_MEDIA_STREAM_LIBS} PARENT_SCOPE)
endif()