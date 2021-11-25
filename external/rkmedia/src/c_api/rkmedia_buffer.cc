// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdlib.h>

#include "image.h"
#include "rkmedia_buffer.h"
#include "rkmedia_buffer_impl.h"
#include "rkmedia_utils.h"
#include "rkmedia_venc.h"

#ifdef MOD_TAG
#undef MOD_TAG
#endif
#define MOD_TAG RK_ID_VB

std::list<HANDLE_MB *> g_handle_mb;
std::mutex g_handle_mb_mutex;

void *RK_MPI_MB_GetPtr(MEDIA_BUFFER mb) {
  if (!mb)
    return NULL;

  MEDIA_BUFFER_IMPLE *mb_impl = (MEDIA_BUFFER_IMPLE *)mb;
  return mb_impl->ptr;
}

int RK_MPI_MB_GetFD(MEDIA_BUFFER mb) {
  if (!mb)
    return 0;

  MEDIA_BUFFER_IMPLE *mb_impl = (MEDIA_BUFFER_IMPLE *)mb;
  return mb_impl->fd;
}

int RK_MPI_MB_GetHandle(MEDIA_BUFFER mb) {
  if (!mb)
    return 0;

  MEDIA_BUFFER_IMPLE *mb_impl = (MEDIA_BUFFER_IMPLE *)mb;
  return mb_impl->handle;
}

int RK_MPI_MB_GetDevFD(MEDIA_BUFFER mb) {
  if (!mb)
    return 0;

  MEDIA_BUFFER_IMPLE *mb_impl = (MEDIA_BUFFER_IMPLE *)mb;
  return mb_impl->dev_fd;
}

size_t RK_MPI_MB_GetSize(MEDIA_BUFFER mb) {
  if (!mb)
    return 0;

  MEDIA_BUFFER_IMPLE *mb_impl = (MEDIA_BUFFER_IMPLE *)mb;
  return mb_impl->size;
}

MOD_ID_E RK_MPI_MB_GetModeID(MEDIA_BUFFER mb) {
  if (!mb)
    return RK_ID_UNKNOW;

  MEDIA_BUFFER_IMPLE *mb_impl = (MEDIA_BUFFER_IMPLE *)mb;
  return mb_impl->mode_id;
}

RK_S16 RK_MPI_MB_GetChannelID(MEDIA_BUFFER mb) {
  if (!mb)
    return -RK_ERR_SYS_ILLEGAL_PARAM;

  MEDIA_BUFFER_IMPLE *mb_impl = (MEDIA_BUFFER_IMPLE *)mb;
  return mb_impl->chn_id;
}

RK_U64 RK_MPI_MB_GetTimestamp(MEDIA_BUFFER mb) {
  if (!mb)
    return 0;

  MEDIA_BUFFER_IMPLE *mb_impl = (MEDIA_BUFFER_IMPLE *)mb;
  return mb_impl->timestamp;
}

RK_S32 RK_MPI_MB_ReleaseBuffer(MEDIA_BUFFER mb) {
  MEDIA_BUFFER_IMPLE *mb_impl = (MEDIA_BUFFER_IMPLE *)mb;
  if (!mb)
    return -RK_ERR_SYS_ILLEGAL_PARAM;

  if (mb_impl->rkmedia_mb)
    mb_impl->rkmedia_mb.reset();

  delete mb_impl;

  g_handle_mb_mutex.lock();
  for (auto it = g_handle_mb.begin(); it != g_handle_mb.end(); ++it) {
    auto p = *it;
    if (p->mb == mb) {
      g_handle_mb.erase(it);
      free(p);
      break;
    }
  }
  g_handle_mb_mutex.unlock();

  return RK_ERR_SYS_OK;
}

RK_S32 RK_MPI_MB_BeginCPUAccess(MEDIA_BUFFER mb, RK_BOOL bReadonly) {
  MEDIA_BUFFER_IMPLE *mb_impl = (MEDIA_BUFFER_IMPLE *)mb;
  if (!mb)
    return -RK_ERR_SYS_ILLEGAL_PARAM;

  if (mb_impl->rkmedia_mb)
    mb_impl->rkmedia_mb->BeginCPUAccess(bReadonly);

  return RK_ERR_SYS_OK;
}

RK_S32 RK_MPI_MB_EndCPUAccess(MEDIA_BUFFER mb, RK_BOOL bReadonly) {
  MEDIA_BUFFER_IMPLE *mb_impl = (MEDIA_BUFFER_IMPLE *)mb;
  if (!mb)
    return -RK_ERR_SYS_ILLEGAL_PARAM;

  if (mb_impl->rkmedia_mb)
    mb_impl->rkmedia_mb->EndCPUAccess(bReadonly);

  return RK_ERR_SYS_OK;
}

MEDIA_BUFFER RK_MPI_MB_CreateAudioBufferExt(MB_AUDIO_INFO_S *pstAudioInfo,
                                            RK_BOOL boolHardWare,
                                            RK_U8 u8Flag) {
  if (!pstAudioInfo || !pstAudioInfo->u32Channels ||
      !pstAudioInfo->u32SampleRate || !pstAudioInfo->u32NBSamples)
    return NULL;

  std::string strSampleFormat = SampleFormatToString(pstAudioInfo->enSampleFmt);
  SampleFormat rkmediaSampleFormat = StringToSampleFmt(strSampleFormat.c_str());
  if (rkmediaSampleFormat == SAMPLE_FMT_NONE) {
    RKMEDIA_LOGE("%s: unsupport sample format!\n", __func__);
    return NULL;
  }

  SampleInfo rkmediaSampleInfo = {
      rkmediaSampleFormat, (int)pstAudioInfo->u32Channels,
      (int)pstAudioInfo->u32SampleRate, (int)pstAudioInfo->u32NBSamples};
  RK_U32 buf_size =
      GetSampleSize(rkmediaSampleInfo) * pstAudioInfo->u32NBSamples;
  if (buf_size == 0)
    return NULL;

  MEDIA_BUFFER_IMPLE *mb = new MEDIA_BUFFER_IMPLE;
  if (!mb) {
    RKMEDIA_LOGE("%s: no space left!\n", __func__);
    return NULL;
  }

  RK_U32 u32RkmediaBufFlag = 2; // cached buffer type default
  if (u8Flag == MB_FLAG_NOCACHED)
    u32RkmediaBufFlag = 0;
  else if (u8Flag == MB_FLAG_PHY_ADDR_CONSECUTIVE)
    u32RkmediaBufFlag = 1;

  auto &&rkmedia_mb = easymedia::MediaBuffer::Alloc(
      buf_size,
      boolHardWare ? easymedia::MediaBuffer::MemType::MEM_HARD_WARE
                   : easymedia::MediaBuffer::MemType::MEM_COMMON,
      u32RkmediaBufFlag);
  if (!rkmedia_mb) {
    delete mb;
    RKMEDIA_LOGE("%s: no space left!\n", __func__);
    return NULL;
  }

  mb->rkmedia_mb = std::make_shared<easymedia::SampleBuffer>(
      *(rkmedia_mb.get()), rkmediaSampleInfo);
  mb->ptr = mb->rkmedia_mb->GetPtr();
  mb->fd = mb->rkmedia_mb->GetFD();
  mb->size = 0;
  mb->type = MB_TYPE_AUDIO;
  mb->stAudioInfo = *pstAudioInfo;
  mb->timestamp = 0;
  mb->mode_id = RK_ID_UNKNOW;
  mb->chn_id = 0;
  mb->flag = 0;
  mb->tsvc_level = 0;
  return mb;
}

MEDIA_BUFFER RK_MPI_MB_CreateAudioBuffer(RK_U32 u32BufferSize,
                                         RK_BOOL boolHardWare) {
  std::shared_ptr<easymedia::MediaBuffer> rkmedia_mb;
  if (u32BufferSize == 0) {
    rkmedia_mb = std::make_shared<easymedia::MediaBuffer>();
  } else {
    rkmedia_mb = easymedia::MediaBuffer::Alloc(
        u32BufferSize, boolHardWare
                           ? easymedia::MediaBuffer::MemType::MEM_HARD_WARE
                           : easymedia::MediaBuffer::MemType::MEM_COMMON);
  }
  MEDIA_BUFFER_IMPLE *mb = new MEDIA_BUFFER_IMPLE;
  if (!mb) {
    RKMEDIA_LOGE("%s: no space left!\n", __func__);
    return NULL;
  }

  if (!rkmedia_mb) {
    delete mb;
    RKMEDIA_LOGE("%s: no space left!\n", __func__);
    return NULL;
  }
  mb->rkmedia_mb = rkmedia_mb;
  mb->ptr = rkmedia_mb->GetPtr();
  mb->fd = rkmedia_mb->GetFD();
  mb->handle = rkmedia_mb->GetHandle();
  mb->dev_fd = rkmedia_mb->GetDevFD();
  mb->size = 0;
  mb->type = MB_TYPE_AUDIO;
  mb->timestamp = 0;
  mb->mode_id = RK_ID_UNKNOW;
  mb->chn_id = 0;
  mb->flag = 0;
  mb->tsvc_level = 0;

  return mb;
}

MEDIA_BUFFER RK_MPI_MB_CreateImageBuffer(MB_IMAGE_INFO_S *pstImageInfo,
                                         RK_BOOL boolHardWare, RK_U8 u8Flag) {
  if (!pstImageInfo || !pstImageInfo->u32Height || !pstImageInfo->u32Width ||
      !pstImageInfo->u32VerStride || !pstImageInfo->u32HorStride)
    return NULL;

  std::string strPixFormat = ImageTypeToString(pstImageInfo->enImgType);
  PixelFormat rkmediaPixFormat = StringToPixFmt(strPixFormat.c_str());
  if (rkmediaPixFormat == PIX_FMT_NONE) {
    RKMEDIA_LOGE("%s: unsupport pixformat!\n", __func__);
    return NULL;
  }
  RK_U32 buf_size = CalPixFmtSize(rkmediaPixFormat, pstImageInfo->u32HorStride,
                                  pstImageInfo->u32VerStride, 16);
  if (buf_size == 0)
    return NULL;

  MEDIA_BUFFER_IMPLE *mb = new MEDIA_BUFFER_IMPLE;
  if (!mb) {
    RKMEDIA_LOGE("%s: no space left!\n", __func__);
    return NULL;
  }

  RK_U32 u32RkmediaBufFlag = 2; // cached buffer type default
  if (u8Flag == MB_FLAG_NOCACHED)
    u32RkmediaBufFlag = 0;
  else if (u8Flag == MB_FLAG_PHY_ADDR_CONSECUTIVE)
    u32RkmediaBufFlag = 1;

  auto &&rkmedia_mb = easymedia::MediaBuffer::Alloc(
      buf_size,
      boolHardWare ? easymedia::MediaBuffer::MemType::MEM_HARD_WARE
                   : easymedia::MediaBuffer::MemType::MEM_COMMON,
      u32RkmediaBufFlag);
  if (!rkmedia_mb) {
    delete mb;
    RKMEDIA_LOGE("%s: no space left!\n", __func__);
    return NULL;
  }

  ImageInfo rkmediaImageInfo = {rkmediaPixFormat, (int)pstImageInfo->u32Width,
                                (int)pstImageInfo->u32Height,
                                (int)pstImageInfo->u32HorStride,
                                (int)pstImageInfo->u32VerStride};
  mb->rkmedia_mb = std::make_shared<easymedia::ImageBuffer>(*(rkmedia_mb.get()),
                                                            rkmediaImageInfo);
  mb->ptr = mb->rkmedia_mb->GetPtr();
  mb->fd = mb->rkmedia_mb->GetFD();
  mb->handle = mb->rkmedia_mb->GetHandle();
  mb->dev_fd = mb->rkmedia_mb->GetDevFD();
  mb->size = 0;
  mb->type = MB_TYPE_IMAGE;
  mb->stImageInfo = *pstImageInfo;
  mb->timestamp = 0;
  mb->mode_id = RK_ID_UNKNOW;
  mb->chn_id = 0;
  mb->flag = 0;
  mb->tsvc_level = 0;

  return mb;
}

RK_S32 RK_MPI_MB_SetSize(MEDIA_BUFFER mb, RK_U32 size) {
  MEDIA_BUFFER_IMPLE *mb_impl = (MEDIA_BUFFER_IMPLE *)mb;
  if (!mb_impl || !mb_impl->rkmedia_mb)
    return -RK_ERR_SYS_ILLEGAL_PARAM;

  mb_impl->size = size;
  if (mb_impl->rkmedia_mb)
    mb_impl->rkmedia_mb->SetValidSize(size);

  return RK_ERR_SYS_OK;
}

RK_S32 RK_MPI_MB_SetTimestamp(MEDIA_BUFFER mb, RK_U64 timestamp) {
  if (!mb)
    return -RK_ERR_SYS_ILLEGAL_PARAM;

  MEDIA_BUFFER_IMPLE *mb_impl = (MEDIA_BUFFER_IMPLE *)mb;
  mb_impl->timestamp = timestamp;
  if (mb_impl->rkmedia_mb)
    mb_impl->rkmedia_mb->SetUSTimeStamp(timestamp);

  return RK_ERR_SYS_OK;
}

RK_S32 RK_MPI_MB_GetFlag(MEDIA_BUFFER mb) {
  if (!mb)
    return -RK_ERR_SYS_ILLEGAL_PARAM;

  MEDIA_BUFFER_IMPLE *mb_impl = (MEDIA_BUFFER_IMPLE *)mb;

  return mb_impl->flag;
}

RK_S32 RK_MPI_MB_GetTsvcLevel(MEDIA_BUFFER mb) {
  if (!mb)
    return -RK_ERR_SYS_ILLEGAL_PARAM;

  MEDIA_BUFFER_IMPLE *mb_impl = (MEDIA_BUFFER_IMPLE *)mb;

  return mb_impl->tsvc_level;
}

RK_BOOL RK_MPI_MB_IsViFrame(MEDIA_BUFFER mb) {
  if (!mb)
    return RK_FALSE;

  MEDIA_BUFFER_IMPLE *mb_impl = (MEDIA_BUFFER_IMPLE *)mb;
  if ((mb_impl->type != MB_TYPE_H264) && (mb_impl->type != MB_TYPE_H265))
    return RK_FALSE;

  if ((mb_impl->flag == VENC_NALU_PSLICE) && (mb_impl->tsvc_level == 0))
    return RK_TRUE;

  return RK_FALSE;
}

MEDIA_BUFFER RK_MPI_MB_CreateBuffer(RK_U32 u32Size, RK_BOOL boolHardWare,
                                    RK_U8 u8Flag) {
  if (!u32Size) {
    RKMEDIA_LOGE("%s: Invalid buffer size!\n", __func__);
    return NULL;
  }

  MEDIA_BUFFER_IMPLE *mb = new MEDIA_BUFFER_IMPLE;
  if (!mb) {
    RKMEDIA_LOGE("%s: no space left!\n", __func__);
    return NULL;
  }

  RK_U32 u32RkmediaBufFlag = 2; // cached buffer type default
  if (u8Flag == MB_FLAG_NOCACHED)
    u32RkmediaBufFlag = 0;
  else if (u8Flag == MB_FLAG_PHY_ADDR_CONSECUTIVE)
    u32RkmediaBufFlag = 1;

  mb->rkmedia_mb = easymedia::MediaBuffer::Alloc(
      u32Size,
      boolHardWare ? easymedia::MediaBuffer::MemType::MEM_HARD_WARE
                   : easymedia::MediaBuffer::MemType::MEM_COMMON,
      u32RkmediaBufFlag);
  if (!mb->rkmedia_mb) {
    delete mb;
    RKMEDIA_LOGE("%s: no space left!\n", __func__);
    return NULL;
  }

  mb->ptr = mb->rkmedia_mb->GetPtr();
  mb->fd = mb->rkmedia_mb->GetFD();
  mb->handle = mb->rkmedia_mb->GetHandle();
  mb->dev_fd = mb->rkmedia_mb->GetDevFD();
  mb->size = 0;
  mb->type = MB_TYPE_COMMON;
  mb->timestamp = 0;
  mb->mode_id = RK_ID_UNKNOW;
  mb->chn_id = 0;
  mb->flag = 0;
  mb->tsvc_level = 0;

  return mb;
}

MEDIA_BUFFER RK_MPI_MB_ConvertToImgBuffer(MEDIA_BUFFER mb,
                                          MB_IMAGE_INFO_S *pstImageInfo) {
  if (!mb || !pstImageInfo || !pstImageInfo->u32Height ||
      !pstImageInfo->u32Width || !pstImageInfo->u32VerStride ||
      !pstImageInfo->u32HorStride) {
    RKMEDIA_LOGE("%s: invalid args!\n", __func__);
    return NULL;
  }

  MEDIA_BUFFER_IMPLE *mb_impl = (MEDIA_BUFFER_IMPLE *)mb;
  if (!mb_impl->rkmedia_mb) {
    RKMEDIA_LOGE("%s: mediabuffer not init yet!\n", __func__);
    return NULL;
  }

  std::string strPixFormat = ImageTypeToString(pstImageInfo->enImgType);
  PixelFormat rkmediaPixFormat = StringToPixFmt(strPixFormat.c_str());
  if (rkmediaPixFormat == PIX_FMT_NONE) {
    RKMEDIA_LOGE("%s: unsupport pixformat!\n", __func__);
    return NULL;
  }

  RK_U32 buf_size = CalPixFmtSize(rkmediaPixFormat, pstImageInfo->u32HorStride,
                                  pstImageInfo->u32VerStride, 1);
  if (buf_size > mb_impl->rkmedia_mb->GetSize()) {
    RKMEDIA_LOGE("%s: buffer size:%zu do not match imgInfo(%dx%d, %s)!\n",
                 __func__, mb_impl->rkmedia_mb->GetSize(),
                 pstImageInfo->u32HorStride, pstImageInfo->u32VerStride,
                 strPixFormat.c_str());
    return NULL;
  }

  ImageInfo rkmediaImageInfo = {rkmediaPixFormat, (int)pstImageInfo->u32Width,
                                (int)pstImageInfo->u32Height,
                                (int)pstImageInfo->u32HorStride,
                                (int)pstImageInfo->u32VerStride};
  mb_impl->rkmedia_mb = std::make_shared<easymedia::ImageBuffer>(
      *(mb_impl->rkmedia_mb.get()), rkmediaImageInfo);
  mb_impl->type = MB_TYPE_IMAGE;
  mb_impl->stImageInfo = *pstImageInfo;
  return mb_impl;
}

MEDIA_BUFFER RK_MPI_MB_ConvertToAudBufferExt(MEDIA_BUFFER mb,
                                             MB_AUDIO_INFO_S *pstAudioInfo) {
  if (!mb || !pstAudioInfo || !pstAudioInfo->u32Channels ||
      !pstAudioInfo->u32NBSamples || !pstAudioInfo->u32SampleRate) {
    RKMEDIA_LOGE("%s: invalid args!\n", __func__);
    return NULL;
  }

  MEDIA_BUFFER_IMPLE *mb_impl = (MEDIA_BUFFER_IMPLE *)mb;
  if (!mb_impl->rkmedia_mb) {
    RKMEDIA_LOGE("%s: mediabuffer not init yet!\n", __func__);
    return NULL;
  }

  std::string strSampleFormat = SampleFormatToString(pstAudioInfo->enSampleFmt);
  SampleFormat rkmediaSampleFormat = StringToSampleFmt(strSampleFormat.c_str());
  if (rkmediaSampleFormat == SAMPLE_FMT_NONE) {
    RKMEDIA_LOGE("%s: unsupport sample format!\n", __func__);
    return NULL;
  }

  SampleInfo rkmediaSampleInfo = {
      rkmediaSampleFormat, (int)pstAudioInfo->u32Channels,
      (int)pstAudioInfo->u32SampleRate, (int)pstAudioInfo->u32NBSamples};
  RK_U32 buf_size =
      GetSampleSize(rkmediaSampleInfo) * pstAudioInfo->u32NBSamples;
  if (buf_size == 0)
    return NULL;

  if (buf_size > mb_impl->rkmedia_mb->GetSize()) {
    RKMEDIA_LOGE("%s: buffer size:%zu do not match AudioInfo(%dx%d%d, %s)!\n",
                 __func__, mb_impl->rkmedia_mb->GetSize(),
                 pstAudioInfo->u32Channels, pstAudioInfo->u32SampleRate,
                 pstAudioInfo->u32NBSamples, strSampleFormat.c_str());
    return NULL;
  }

  mb_impl->rkmedia_mb = std::make_shared<easymedia::SampleBuffer>(
      *(mb_impl->rkmedia_mb.get()), rkmediaSampleInfo);
  mb_impl->type = MB_TYPE_AUDIO;
  mb_impl->stAudioInfo = *pstAudioInfo;
  return mb_impl;
}

MEDIA_BUFFER RK_MPI_MB_ConvertToAudBuffer(MEDIA_BUFFER mb) {
  if (!mb) {
    RKMEDIA_LOGE("%s: invalid args!\n", __func__);
    return NULL;
  }
  MEDIA_BUFFER_IMPLE *mb_impl = (MEDIA_BUFFER_IMPLE *)mb;
  if (!mb_impl->rkmedia_mb) {
    RKMEDIA_LOGE("%s: mediabuffer not init yet!\n", __func__);
    return NULL;
  }

  mb_impl->type = MB_TYPE_AUDIO;
  return mb_impl;
}

RK_S32 RK_MPI_MB_GetImageInfo(MEDIA_BUFFER mb, MB_IMAGE_INFO_S *pstImageInfo) {
  if (!mb || !pstImageInfo)
    return -RK_ERR_SYS_ILLEGAL_PARAM;

  MEDIA_BUFFER_IMPLE *mb_impl = (MEDIA_BUFFER_IMPLE *)mb;
  if (mb_impl->type != MB_TYPE_IMAGE)
    return -RK_ERR_SYS_NOT_PERM;

  *pstImageInfo = mb_impl->stImageInfo;
  return RK_ERR_SYS_OK;
}

RK_S32 RK_MPI_MB_GetAudioInfo(MEDIA_BUFFER mb, MB_AUDIO_INFO_S *pstAudioInfo) {
  if (!mb || !pstAudioInfo)
    return -RK_ERR_SYS_ILLEGAL_PARAM;

  MEDIA_BUFFER_IMPLE *mb_impl = (MEDIA_BUFFER_IMPLE *)mb;
  if (mb_impl->type != MB_TYPE_IMAGE)
    return -RK_ERR_SYS_NOT_PERM;

  *pstAudioInfo = mb_impl->stAudioInfo;
  return RK_ERR_SYS_OK;
}

RK_S32 RK_MPI_MB_TsNodeDump(MEDIA_BUFFER mb) {
  if (!mb)
    return -RK_ERR_SYS_ILLEGAL_PARAM;

#ifdef RKMEDIA_TIMESTAMP_DEBUG
  MEDIA_BUFFER_IMPLE *mb_impl = (MEDIA_BUFFER_IMPLE *)mb;
  if (mb_impl->rkmedia_mb)
    mb_impl->rkmedia_mb->TimeStampDump();
  return RK_ERR_SYS_OK;
#else
  return -RK_ERR_SYS_NOT_SUPPORT;
#endif
}

MEDIA_BUFFER RK_MPI_MB_Copy(MEDIA_BUFFER mb, RK_BOOL bZeroCopy) {
  MEDIA_BUFFER_IMPLE *mb_old = (MEDIA_BUFFER_IMPLE *)mb;
  MEDIA_BUFFER_IMPLE *mb_new = new MEDIA_BUFFER_IMPLE;
  if (!mb_new) {
    RKMEDIA_LOGE("%s: no space left!\n", __func__);
    return NULL;
  }
  if (bZeroCopy) {
    *mb_new = *mb_old;
  } else {
    RKMEDIA_LOGE("%s: not support DeepCopy\n", __func__);
    delete mb_new;
    return NULL;
  }

  return mb_new;
}

/****************************************************
 * MBP: Media Buffer Pool
 * **************************************************/
MEDIA_BUFFER_POOL RK_MPI_MB_POOL_Create(MB_POOL_PARAM_S *pstPoolParam) {
  RK_U32 u32Cnt;
  RK_U32 u32Size;
  RK_BOOL bIsHardWare;
  RK_U16 u16Flag;
  MB_TYPE_E enMediaType;
  MB_IMAGE_INFO_S stImageInfo;
  // c++ api struct
  PixelFormat rkmediaPixFormat;

  if (!pstPoolParam) {
    RKMEDIA_LOGE("%s: param is NULL!\n", __func__);
    return NULL;
  }

  u32Cnt = pstPoolParam->u32Cnt;
  u32Size = pstPoolParam->u32Size;
  bIsHardWare = pstPoolParam->bHardWare;
  u16Flag = pstPoolParam->u16Flag;
  enMediaType = pstPoolParam->enMediaType;

  if (enMediaType & MB_TYPE_VIDEO) {
    stImageInfo = pstPoolParam->stImageInfo;
    if ((stImageInfo.enImgType <= IMAGE_TYPE_UNKNOW) ||
        (stImageInfo.enImgType > IMAGE_TYPE_BUTT) || (!stImageInfo.u32Width) ||
        (!stImageInfo.u32Height) ||
        (stImageInfo.u32HorStride < stImageInfo.u32Width) ||
        (stImageInfo.u32VerStride < stImageInfo.u32Height)) {
      RKMEDIA_LOGE("%s: Invalid param!\n", __func__);
      return NULL;
    }

    std::string strPixFormat = ImageTypeToString(stImageInfo.enImgType);
    rkmediaPixFormat = StringToPixFmt(strPixFormat.c_str());
    if (rkmediaPixFormat == PIX_FMT_NONE) {
      RKMEDIA_LOGE("%s: unsupport pixformat!\n", __func__);
      return NULL;
    }

    u32Size = CalPixFmtSize(rkmediaPixFormat, stImageInfo.u32HorStride,
                            stImageInfo.u32VerStride, 16);
  }

  if (!u32Cnt || !u32Size) {
    RKMEDIA_LOGE("%s: Invalid cnt(%d) or size(%d)!\n", __func__, u32Cnt,
                 u32Size);
    return NULL;
  }

  MEDIA_BUFFER_POOL_IMPLE *pstMBPImpl = new MEDIA_BUFFER_POOL_IMPLE;
  if (!pstMBPImpl) {
    RKMEDIA_LOGE("%s: No space left!\n", __func__);
    return NULL;
  }

  RK_U32 u32RkmediaBufFlag = 2; // cached buffer type default
  if (u16Flag == MB_FLAG_NOCACHED)
    u32RkmediaBufFlag = 0;
  else if (u16Flag == MB_FLAG_PHY_ADDR_CONSECUTIVE)
    u32RkmediaBufFlag = 1;

  pstMBPImpl->rkmedia_mbp = std::make_shared<easymedia::BufferPool>(
      u32Cnt, u32Size,
      bIsHardWare ? easymedia::MediaBuffer::MemType::MEM_HARD_WARE
                  : easymedia::MediaBuffer::MemType::MEM_COMMON,
      u32RkmediaBufFlag);

  pstMBPImpl->enType = enMediaType;
  pstMBPImpl->u32Size = u32Size;
  pstMBPImpl->u32Cnt = u32Cnt;
  if (enMediaType & MB_TYPE_VIDEO) {
    // save c api imgInfo
    pstMBPImpl->stImageInfo = stImageInfo;
    // convert to c++ api imgInfo type
    pstMBPImpl->rkmediaImgInfo.pix_fmt = rkmediaPixFormat;
    pstMBPImpl->rkmediaImgInfo.width = stImageInfo.u32Width;
    pstMBPImpl->rkmediaImgInfo.height = stImageInfo.u32Height;
    pstMBPImpl->rkmediaImgInfo.vir_width = stImageInfo.u32HorStride;
    pstMBPImpl->rkmediaImgInfo.vir_height = stImageInfo.u32VerStride;
  }

  return (MEDIA_BUFFER_POOL)pstMBPImpl;
}

MEDIA_BUFFER RK_MPI_MB_POOL_GetBuffer(MEDIA_BUFFER_POOL MBPHandle,
                                      RK_BOOL bIsBlock) {
  if (!MBPHandle) {
    RKMEDIA_LOGE("%s: handle is null!\n", __func__);
    return NULL;
  }

  MEDIA_BUFFER_POOL_IMPLE *pstMBPImpl;
  pstMBPImpl = (MEDIA_BUFFER_POOL_IMPLE *)MBPHandle;

  if (!pstMBPImpl->rkmedia_mbp) {
    RKMEDIA_LOGE("%s: invalid handle!\n", __func__);
    return NULL;
  }

  bool boolBlock = (bIsBlock == RK_TRUE) ? true : false;
  auto rkmedia_mb = pstMBPImpl->rkmedia_mbp->GetBuffer(boolBlock);
  if (!rkmedia_mb) {
    RKMEDIA_LOGW("%s: No buffer available!\n", __func__);
    return NULL;
  }

  MEDIA_BUFFER_IMPLE *mb = new MEDIA_BUFFER_IMPLE;
  if (!mb) {
    RKMEDIA_LOGE("%s: no space left!\n", __func__);
    return NULL;
  }

  if (pstMBPImpl->enType & MB_TYPE_VIDEO) {
    // use c++ ImageInfo struct to convert img informations.
    auto img_mb = std::make_shared<easymedia::ImageBuffer>(
        *(rkmedia_mb.get()), pstMBPImpl->rkmediaImgInfo);
    mb->type = pstMBPImpl->enType;
    // use c MB_IMAGE_INFO_S struct to show this buffer's img info.
    mb->stImageInfo = pstMBPImpl->stImageInfo;
    mb->rkmedia_mb = img_mb;
  } else if (pstMBPImpl->enType & MB_TYPE_AUDIO) {
    mb->type = pstMBPImpl->enType;
    mb->rkmedia_mb = rkmedia_mb;
  } else {
    mb->type = MB_TYPE_COMMON;
    mb->rkmedia_mb = rkmedia_mb;
  }
  mb->ptr = mb->rkmedia_mb->GetPtr();
  mb->fd = mb->rkmedia_mb->GetFD();
  mb->handle = mb->rkmedia_mb->GetHandle();
  mb->dev_fd = mb->rkmedia_mb->GetDevFD();
  mb->size = 0;
  mb->timestamp = 0;
  mb->mode_id = RK_ID_UNKNOW;
  mb->chn_id = 0;
  mb->flag = 0;
  mb->tsvc_level = 0;

  return mb;
}

RK_S32 RK_MPI_MB_POOL_Destroy(MEDIA_BUFFER_POOL MBPHandle) {
  if (!MBPHandle)
    return -RK_ERR_SYS_ILLEGAL_PARAM;

  MEDIA_BUFFER_POOL_IMPLE *pstMBPImpl;
  pstMBPImpl = (MEDIA_BUFFER_POOL_IMPLE *)MBPHandle;

  if (pstMBPImpl->rkmedia_mbp) {
    pstMBPImpl->rkmedia_mbp.reset();
  }

  delete pstMBPImpl;

  return RK_ERR_SYS_OK;
}
