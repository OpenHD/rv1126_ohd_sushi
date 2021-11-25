#
# Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

add_subdirectory(stream)
add_subdirectory(flow)
add_subdirectory(buffer)

if(RKAUDIO)
add_subdirectory(rkaudio)
endif()

if(LIVE555)
add_subdirectory(live555)
endif()

if(OGG)
add_subdirectory(ogg)
endif()

if(RKMPP)
add_subdirectory(rkmpp)
if(UVC)
add_subdirectory(uvc)
endif()
endif()
