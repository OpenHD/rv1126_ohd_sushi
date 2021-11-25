#
#Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd.All rights reserved.
#Use of this source code is governed by a BSD - style license that can be
#found in the LICENSE file.
#

#vi : set noexpandtab syntax = cmake:

set(EASY_MEDIA_SERVER_SOURCE_FILES server/server.cc
                                   socket/socket.cc)

set(EASY_MEDIA_SOURCE_FILES ${EASY_MEDIA_SOURCE_FILES}
                            ${EASY_MEDIA_SERVER_SOURCE_FILES} PARENT_SCOPE)
