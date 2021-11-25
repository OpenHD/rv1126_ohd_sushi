#
#Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd.All rights reserved.
#Use of this source code is governed by a BSD - style license that can be
#found in the LICENSE file.
#

#vi : set noexpandtab syntax = cmake:

add_library(easymedia_client SHARED client.cc ../socket/socket.cc)
add_dependencies(easymedia_client easymedia)
target_link_libraries(easymedia_client pthread drm)
target_include_directories(easymedia_client PRIVATE ${CMAKE_SOURCE_DIR}/include)
install(TARGETS easymedia_client DESTINATION "lib")
