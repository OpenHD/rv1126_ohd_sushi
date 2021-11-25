// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef __RK_BUFFER_
#define __RK_BUFFER_
#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>

#include "rkmedia_common.h"

typedef void *MEDIA_BUFFER_POOL;
typedef void *MEDIA_BUFFER;
typedef void (*OutCbFunc)(MEDIA_BUFFER mb);
typedef void (*OutCbFuncEx)(MEDIA_BUFFER mb, RK_VOID *pHandle);

#define MB_FLAG_NOCACHED 0x01             // no cached attrs
#define MB_FLAG_PHY_ADDR_CONSECUTIVE 0x02 // physical address consecutive

#define MB_TYPE_IMAGE_MASK 0x0100
#define MB_TYPE_VIDEO_MASK 0x0200
#define MB_TYPE_AUDIO_MASK 0x0400

typedef enum rkMB_TYPE {
  MB_TYPE_COMMON = 0,
  // Original image, such as NV12, RGB
  MB_TYPE_IMAGE = MB_TYPE_IMAGE_MASK | 0x0000,
  // Encoded video data. Treat JPEG as a video data.
  MB_TYPE_VIDEO = MB_TYPE_VIDEO_MASK | 0x0000,
  MB_TYPE_H264 = MB_TYPE_VIDEO_MASK | 0x0001,
  MB_TYPE_H265 = MB_TYPE_VIDEO_MASK | 0x0002,
  MB_TYPE_JPEG = MB_TYPE_VIDEO_MASK | 0x0003,
  MB_TYPE_MJPEG = MB_TYPE_VIDEO_MASK | 0x0004,
  // Audio data
  MB_TYPE_AUDIO = MB_TYPE_AUDIO_MASK | 0x0000,
} MB_TYPE_E;

typedef struct rkMB_IMAGE_INFO {
  RK_U32 u32Width;
  RK_U32 u32Height;
  RK_U32 u32HorStride;
  RK_U32 u32VerStride;
  IMAGE_TYPE_E enImgType;
} MB_IMAGE_INFO_S;

typedef struct rkMB_AUDIO_INFO {
  RK_U32 u32Channels;
  RK_U32 u32SampleRate;
  RK_U32 u32NBSamples;
  SAMPLE_FORMAT_E enSampleFmt;
} MB_AUDIO_INFO_S;

typedef struct rkMB_POOL_PARAM_S {
  MB_TYPE_E enMediaType;
  RK_U32 u32Cnt;
  RK_U32 u32Size;
  RK_BOOL bHardWare;
  RK_U16 u16Flag;
  union {
    MB_IMAGE_INFO_S stImageInfo;
  };
} MB_POOL_PARAM_S;

typedef struct rkHANDLE_MB {
  void *handle;
  void *mb;
} HANDLE_MB;

_CAPI void *RK_MPI_MB_GetPtr(MEDIA_BUFFER mb);
_CAPI int RK_MPI_MB_GetFD(MEDIA_BUFFER mb);
_CAPI int RK_MPI_MB_GetHandle(MEDIA_BUFFER mb);
_CAPI int RK_MPI_MB_GetDevFD(MEDIA_BUFFER mb);
_CAPI size_t RK_MPI_MB_GetSize(MEDIA_BUFFER mb);
_CAPI MOD_ID_E RK_MPI_MB_GetModeID(MEDIA_BUFFER mb);
_CAPI RK_S16 RK_MPI_MB_GetChannelID(MEDIA_BUFFER mb);
_CAPI RK_U64 RK_MPI_MB_GetTimestamp(MEDIA_BUFFER mb);
_CAPI RK_S32 RK_MPI_MB_ReleaseBuffer(MEDIA_BUFFER mb);
_CAPI MEDIA_BUFFER RK_MPI_MB_CreateBuffer(RK_U32 u32Size, RK_BOOL boolHardWare,
                                          RK_U8 u8Flag);
_CAPI MEDIA_BUFFER RK_MPI_MB_ConvertToImgBuffer(MEDIA_BUFFER mb,
                                                MB_IMAGE_INFO_S *pstImageInfo);
_CAPI MEDIA_BUFFER
RK_MPI_MB_ConvertToAudBufferExt(MEDIA_BUFFER mb, MB_AUDIO_INFO_S *pstAudioInfo);
_CAPI MEDIA_BUFFER RK_MPI_MB_ConvertToAudBuffer(MEDIA_BUFFER mb);
_CAPI MEDIA_BUFFER RK_MPI_MB_CreateImageBuffer(MB_IMAGE_INFO_S *pstImageInfo,
                                               RK_BOOL boolHardWare,
                                               RK_U8 u8Flag);
_CAPI MEDIA_BUFFER RK_MPI_MB_CreateAudioBufferExt(MB_AUDIO_INFO_S *pstAudioInfo,
                                                  RK_BOOL boolHardWare,
                                                  RK_U8 u8Flag);
_CAPI MEDIA_BUFFER RK_MPI_MB_CreateAudioBuffer(RK_U32 u32BufferSize,
                                               RK_BOOL boolHardWare);
_CAPI RK_S32 RK_MPI_MB_SetSize(MEDIA_BUFFER mb, RK_U32 size);
_CAPI RK_S32 RK_MPI_MB_SetTimestamp(MEDIA_BUFFER mb, RK_U64 timestamp);
_CAPI RK_S32 RK_MPI_MB_GetFlag(MEDIA_BUFFER mb);
_CAPI RK_S32 RK_MPI_MB_GetTsvcLevel(MEDIA_BUFFER mb);
_CAPI RK_BOOL RK_MPI_MB_IsViFrame(MEDIA_BUFFER mb);
_CAPI RK_S32 RK_MPI_MB_GetImageInfo(MEDIA_BUFFER mb,
                                    MB_IMAGE_INFO_S *pstImageInfo);
_CAPI RK_S32 RK_MPI_MB_GetAudioInfo(MEDIA_BUFFER mb,
                                    MB_AUDIO_INFO_S *pstAudioInfo);
_CAPI RK_S32 RK_MPI_MB_BeginCPUAccess(MEDIA_BUFFER mb, RK_BOOL bReadonly);
_CAPI RK_S32 RK_MPI_MB_EndCPUAccess(MEDIA_BUFFER mb, RK_BOOL bReadonly);
_CAPI RK_S32 RK_MPI_MB_TsNodeDump(MEDIA_BUFFER mb);
_CAPI MEDIA_BUFFER RK_MPI_MB_Copy(MEDIA_BUFFER mb, RK_BOOL bZeroCopy);

_CAPI MEDIA_BUFFER_POOL RK_MPI_MB_POOL_Create(MB_POOL_PARAM_S *pstPoolParam);
_CAPI RK_S32 RK_MPI_MB_POOL_Destroy(MEDIA_BUFFER_POOL MBPHandle);
_CAPI MEDIA_BUFFER RK_MPI_MB_POOL_GetBuffer(MEDIA_BUFFER_POOL MBPHandle,
                                            RK_BOOL bIsBlock);
#ifdef __cplusplus
}
#endif
#endif // #ifndef __RK_BUFFER_
