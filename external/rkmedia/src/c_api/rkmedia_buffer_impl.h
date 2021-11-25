// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __RK_BUFFER_IMPL_
#define __RK_BUFFER_IMPL_

#include "buffer.h"
#include "flow.h"

#include "rkmedia_common.h"

typedef struct _rkMEDIA_BUFFER_S {
  MB_TYPE_E type;
  void *ptr;         // Virtual address of buffer
  int fd;            // dma buffer fd
  int handle;        // dma buffer handle
  int dev_fd;        // dma device fd
  size_t size;       // The size of the buffer
  MOD_ID_E mode_id;  // The module to which the buffer belongs
  RK_U16 chn_id;     // The channel to which the buffer belongs
  RK_U64 timestamp;  // buffer timesatmp
  RK_U32 flag;       // buffer flag
  RK_U32 tsvc_level; // buffer level
  std::shared_ptr<easymedia::MediaBuffer> rkmedia_mb;
  union {
    MB_IMAGE_INFO_S stImageInfo;
    MB_AUDIO_INFO_S stAudioInfo;
  };

} MEDIA_BUFFER_IMPLE;

typedef struct _rkMEDIA_BUFFER_POOL_S {
  MB_TYPE_E enType;
  RK_U32 u32Cnt;
  RK_U32 u32Size;
  // media buffer pool from c++ code.
  std::shared_ptr<easymedia::BufferPool> rkmedia_mbp;
  // Store double information to avoid frequent conversion
  // between c structure and c++ structure.
  union {
    MB_IMAGE_INFO_S stImageInfo;
    MB_AUDIO_INFO_S stAudioInfo;
  };
  union {
    ImageInfo rkmediaImgInfo;
    SampleInfo rkmediaSampleInfo;
  };

} MEDIA_BUFFER_POOL_IMPLE;

#endif // __RK_BUFFER_IMPL_
