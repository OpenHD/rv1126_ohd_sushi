#
# Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

# vi: set noexpandtab syntax=cmake:

set(EASY_MEDIA_NN_SOURCE_FILES rknn/rknn_utils.cc)
set(EASY_MEDIA_NN_DEPENDENT_LIBS)

if(RKNN)
  include_directories(${RKNPU_HEADER_DIR})
  set(EASY_MEDIA_NN_SOURCE_FILES ${EASY_MEDIA_NN_SOURCE_FILES}
                                   rknn/rknn.cc)
  set(EASY_MEDIA_NN_DEPENDENT_LIBS ${EASY_MEDIA_NN_DEPENDENT_LIBS}
                                     rknn_runtime)
endif()

if(ROCKFACE)
  set(EASY_MEDIA_NN_SOURCE_FILES ${EASY_MEDIA_NN_SOURCE_FILES}
                                   rknn/nn_result_input.cc
                                   rknn/face_capture.cc
                                   rknn/draw_filter.cc
                                   rknn/rockface_detect.cc
                                   rknn/rockface_evaluate.cc
                                   rknn/rockface_bodydetect.cc)
  set(EASY_MEDIA_NN_DEPENDENT_LIBS ${EASY_MEDIA_NN_DEPENDENT_LIBS}
                                     rknn_api rockface)

  option(FACE_RECOGNIZE "compile: face recognize" OFF)
  if(FACE_RECOGNIZE)
    set(EASY_MEDIA_NN_SOURCE_FILES ${EASY_MEDIA_NN_SOURCE_FILES}
                                   rknn/rockface_recognition.cc
                                   rknn/rockface_db_manager.cc)
    set(EASY_MEDIA_NN_DEPENDENT_LIBS ${EASY_MEDIA_NN_DEPENDENT_LIBS}
                                     sqlite3)
  endif()
endif()

if(ROCKX)
  set(EASY_MEDIA_ROCKX_SOURCE_FILES rknn/rockx_filter.cc
                                    rknn/nn_result_input.cc
                                    rknn/draw_filter.cc)
  set(EASY_MEDIA_SOURCE_FILES ${EASY_MEDIA_SOURCE_FILES}
                                ${EASY_MEDIA_ROCKX_SOURCE_FILES})
endif()

set(EASY_MEDIA_SOURCE_FILES ${EASY_MEDIA_SOURCE_FILES}
                            ${EASY_MEDIA_NN_SOURCE_FILES} PARENT_SCOPE)
set(EASY_MEDIA_DEPENDENT_LIBS ${EASY_MEDIA_DEPENDENT_LIBS}
                              ${EASY_MEDIA_NN_DEPENDENT_LIBS} PARENT_SCOPE)
