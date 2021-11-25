#
#Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd.All rights reserved.
#Use of this source code is governed by a BSD - style license that can be
#found in the LICENSE file.
#

#vi : set noexpandtab syntax = cmake:

set(EASY_MEDIA_CAPI_SOURCE_FILES c_api/rkmedia_api.cc
								 c_api/rkmedia_utils.cc
								 c_api/rkmedia_buffer.cc
								 c_api/osd/color_table.cc)

set(EASY_MEDIA_SOURCE_FILES ${EASY_MEDIA_SOURCE_FILES}
                            ${EASY_MEDIA_CAPI_SOURCE_FILES} PARENT_SCOPE)

if(COMPILES_MINIMEDIA)
	set(MINI_MEDIA_CAPI_SOURCE_FILES c_api/rkmedia_api.cc
									c_api/rkmedia_utils.cc
									c_api/rkmedia_buffer.cc
									c_api/osd/color_table.cc)

	set(MINI_MEDIA_SOURCE_FILES ${MINI_MEDIA_SOURCE_FILES}
								${MINI_MEDIA_CAPI_SOURCE_FILES} PARENT_SCOPE)
endif()