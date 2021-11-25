/*
 * Copyright (c) 2021 Rockchip, Inc. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#include "rkadk_thumb.h"
#include "RTMediaBuffer.h"
#include "RTMetadataRetriever.h"
#include "rkadk_param.h"
#include "rkmedia_api.h"
#include <malloc.h>
#include <rga/RgaApi.h>
#include <rga/rga.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

//#define DUMP_RUN_TIME
//#define THUMB_SAVE_FILE
#define THM_BOX_HEADER_LEN 8 /* size: 4byte, type: 4byte */

typedef struct {
  RKADK_U32 width;
  RKADK_U32 height;
  RKADK_U32 strideWidth;
  RKADK_U32 strideHeight;
} THUMB_RECT_S;

static int YuvScale(RTMediaBuffer *buffer, char *scaleBuffer,
                    THUMB_RECT_S stScaleRect) {
  int ret = 0, error;
  int strideWidth = 0, strideHeight = 0;
  int width = 0, height = 0;
  rga_info_t srcInfo;
  rga_info_t dstInfo;

#ifdef DUMP_RUN_TIME
  struct timeval tv_start, tv_end;
  long start, end;

  gettimeofday(&tv_start, NULL);
  start = ((long)tv_start.tv_sec) * 1000 + (long)tv_start.tv_usec / 1000;
#endif

  RKADK_CHECK_POINTER(buffer, RKADK_FAILURE);
  RKADK_CHECK_POINTER(scaleBuffer, RKADK_FAILURE);

  RtMetaData *meta = buffer->getMetaData();
  meta->findInt32(kKeyFrameError, &error);
  if (error) {
    RKADK_LOGE("frame error");
    return -1;
  }

  // stride of width and height
  if (!meta->findInt32(kKeyFrameW, &strideWidth)) {
    RKADK_LOGW("not find width stride in meta");
    return -1;
  }
  if (!meta->findInt32(kKeyFrameH, &strideHeight)) {
    RKADK_LOGW("not find height stride in meta");
    return -1;
  }

  // valid width and height of datas
  if (!meta->findInt32(kKeyVCodecWidth, &width)) {
    RKADK_LOGW("not find width in meta");
    return -1;
  }
  if (!meta->findInt32(kKeyVCodecHeight, &height)) {
    RKADK_LOGW("not find height in meta");
    return -1;
  }

  RKADK_LOGD("src width = %d, height = %d", width, height);
  RKADK_LOGD("src strideWidth = %d, strideHeight = %d", strideWidth,
             strideHeight);
  RKADK_LOGD("src buffer size = %d", buffer->getSize());

  RKADK_LOGD("dts width = %d, height = %d", stScaleRect.width,
             stScaleRect.height);
  RKADK_LOGD("dts strideWidth = %d, strideHeight = %d", stScaleRect.strideWidth,
             stScaleRect.strideHeight);

  ret = c_RkRgaInit();
  if (ret < 0) {
    RKADK_LOGE("c_RkRgaInit failed(%d)", ret);
    return -1;
  }

  memset(&srcInfo, 0, sizeof(rga_info_t));
  srcInfo.fd = -1;
  srcInfo.virAddr = buffer->getData();
  srcInfo.mmuFlag = 1;
  srcInfo.rotation = 0;
  srcInfo.blend = 0xFF0405;
  rga_set_rect(&srcInfo.rect, 0, 0, width, height, strideWidth, strideHeight,
               RK_FORMAT_YCbCr_420_SP);

  memset(&dstInfo, 0, sizeof(rga_info_t));
  dstInfo.fd = -1;
  dstInfo.virAddr = scaleBuffer;
  dstInfo.mmuFlag = 1;
  rga_set_rect(&dstInfo.rect, 0, 0, stScaleRect.width, stScaleRect.height,
               stScaleRect.strideWidth, stScaleRect.strideHeight,
               RK_FORMAT_YCbCr_420_SP);

  ret = c_RkRgaBlit(&srcInfo, &dstInfo, NULL);
  if (ret)
    RKADK_LOGE("c_RkRgaBlit scale failed(%d)", ret);

  c_RkRgaDeInit();

#ifdef DUMP_RUN_TIME
  gettimeofday(&tv_end, NULL);
  end = ((long)tv_end.tv_sec) * 1000 + (long)tv_end.tv_usec / 1000;
  RKADK_LOGD("rga scale run time: %ld\n", end - start);
#endif

  return ret;
}

static int YuvToJpg(RTMediaBuffer *buffer, RKADK_U8 *pu8Buf,
                    RKADK_U32 *pu32Size) {
  int ret = 0;
  char *scaleBuffer;
  RKADK_U32 scaleBufferLen = 0;
  RKADK_U32 jpgSize = 0;
  THUMB_RECT_S stScaleRect;
  VENC_CHN_ATTR_S vencChnAttr;
  MEDIA_BUFFER mb = NULL;
  MEDIA_BUFFER jpg_mb = NULL;

#ifdef THUMB_SAVE_FILE
  FILE *file = NULL;
#endif

  RKADK_CHECK_POINTER(buffer, RKADK_FAILURE);
  RKADK_CHECK_POINTER(pu8Buf, RKADK_FAILURE);

  memset(pu8Buf, 0, *pu32Size);

  RKADK_PARAM_THUMB_CFG_S *ptsThumbCfg = RKADK_PARAM_GetThumbCfg();
  if (!ptsThumbCfg) {
    RKADK_LOGE("RKADK_PARAM_GetThumbCfg failed");
    return -1;
  }

  stScaleRect.width = ptsThumbCfg->thumb_width;
  stScaleRect.height = ptsThumbCfg->thumb_height;
  stScaleRect.strideWidth = UPALIGNTO(ptsThumbCfg->thumb_width, 16);
  stScaleRect.strideHeight = UPALIGNTO(ptsThumbCfg->thumb_height, 16);
  scaleBufferLen =
      stScaleRect.strideWidth * stScaleRect.strideHeight * 3 / 2; // NV12

  MB_IMAGE_INFO_S stImageInfo = {stScaleRect.width, stScaleRect.height,
                                 stScaleRect.strideWidth,
                                 stScaleRect.strideHeight, IMAGE_TYPE_NV12};

  mb = RK_MPI_MB_CreateImageBuffer(&stImageInfo, RK_TRUE, MB_FLAG_NOCACHED);
  if (!mb) {
    RKADK_LOGE("no space left");
    return -1;
  }

  scaleBuffer = (char *)RK_MPI_MB_GetPtr(mb);
  ret = YuvScale(buffer, scaleBuffer, stScaleRect);
  if (ret) {
    RKADK_LOGE("YuvScale failed(%d)", ret);
    goto free_mb;
  }

#ifdef THUMB_SAVE_FILE
  file = fopen("/userdata/scale.yuv", "w");
  if (!file) {
    RKADK_LOGE("Create /userdata/scale.yuv failed");
  } else {
    fwrite(scaleBuffer, 1, scaleBufferLen, file);
    fclose(file);
    RKADK_LOGD("fwrite scale.yuv done");
  }
#endif

  RK_MPI_SYS_Init();

  memset(&vencChnAttr, 0, sizeof(vencChnAttr));
  vencChnAttr.stVencAttr.enType = RK_CODEC_TYPE_JPEG;
  vencChnAttr.stVencAttr.imageType = IMAGE_TYPE_NV12;
  vencChnAttr.stVencAttr.u32PicWidth = stScaleRect.width;
  vencChnAttr.stVencAttr.u32PicHeight = stScaleRect.height;
  vencChnAttr.stVencAttr.u32VirWidth = stScaleRect.strideWidth;
  vencChnAttr.stVencAttr.u32VirHeight = stScaleRect.strideHeight;
  ret = RK_MPI_VENC_CreateChn(ptsThumbCfg->venc_chn, &vencChnAttr);
  if (ret) {
    printf("Create Thumb Venc failed! ret=%d\n", ret);
    goto free_mb;
  }

  RK_MPI_MB_SetSize(mb, scaleBufferLen);
  RK_MPI_SYS_SendMediaBuffer(RK_ID_VENC, ptsThumbCfg->venc_chn, mb);

  jpg_mb = RK_MPI_SYS_GetMediaBuffer(RK_ID_VENC, ptsThumbCfg->venc_chn, -1);
  if (!jpg_mb) {
    RKADK_LOGE("get null jpg buffer");
    ret = -1;
    goto exit;
  }

  jpgSize = RK_MPI_MB_GetSize(jpg_mb);
  if (*pu32Size < jpgSize)
    RKADK_LOGW("pu32Size(%d) < jpgSize(%d)", *pu32Size, jpgSize);
  else
    *pu32Size = jpgSize;
  memcpy(pu8Buf, RK_MPI_MB_GetPtr(jpg_mb), *pu32Size);

  RK_MPI_SYS_StopGetMediaBuffer(RK_ID_VENC, ptsThumbCfg->venc_chn);

#ifdef THUMB_SAVE_FILE
  RKADK_LOGD("Get packet:ptr:%p, fd:%d, size:%zu, mode:%d, channel:%d, "
             "timestamp:%lld\n",
             RK_MPI_MB_GetPtr(jpg_mb), RK_MPI_MB_GetFD(jpg_mb),
             RK_MPI_MB_GetSize(jpg_mb), RK_MPI_MB_GetModeID(jpg_mb),
             RK_MPI_MB_GetChannelID(jpg_mb), RK_MPI_MB_GetTimestamp(jpg_mb));

  file = fopen("/userdata/thumb.jpg", "w");
  if (!file) {
    RKADK_LOGE("Create /userdata/thumb.jpg failed");
  } else {
    fwrite(RK_MPI_MB_GetPtr(jpg_mb), 1, jpgSize, file);
    fclose(file);
    RKADK_LOGD("fwrite thumb.jpg done");
  }
#endif

exit:
  RK_MPI_VENC_DestroyChn(ptsThumbCfg->venc_chn);

free_mb:
  if (jpg_mb)
    RK_MPI_MB_ReleaseBuffer(jpg_mb);

  if (mb)
    RK_MPI_MB_ReleaseBuffer(mb);

  return ret;
}

static RKADK_S32 BuildInThm(RKADK_CHAR *pszFileName, RKADK_U8 *pu8Buf,
                            RKADK_U32 u32Size) {
  FILE *fd = NULL;
  int ret = -1;
  RKADK_U32 u32BoxSize;
  char boxHeader[THM_BOX_HEADER_LEN];

  fd = fopen(pszFileName, "r+");
  if (!fd) {
    RKADK_LOGE("open %s failed", pszFileName);
    return -1;
  }

  if (fseek(fd, 0, SEEK_END)) {
    RKADK_LOGE("seek file end failed");
    goto exit;
  }

  u32BoxSize = u32Size + THM_BOX_HEADER_LEN;
  boxHeader[0] = u32BoxSize >> 24;
  boxHeader[1] = (u32BoxSize & 0x00FF0000) >> 16;
  boxHeader[2] = (u32BoxSize & 0x0000FF00) >> 8;
  boxHeader[3] = u32BoxSize & 0x000000FF;
  boxHeader[4] = 't';
  boxHeader[5] = 'h';
  boxHeader[6] = 'm';
  boxHeader[7] = 0x20;

  if (fwrite(boxHeader, THM_BOX_HEADER_LEN, 1, fd) != 1) {
    RKADK_LOGE("write thm box header failed");
    goto exit;
  }

  if (fwrite(pu8Buf, u32Size, 1, fd) != 1) {
    RKADK_LOGE("write thm box body failed");
    goto exit;
  }

  ret = 0;

exit:
  if (fd)
    fclose(fd);

  return ret;
}

static RKADK_S32 GetThmInBox(RKADK_CHAR *pszFileName, RKADK_U8 *pu8Buf,
                             RKADK_U32 *pu32Size) {
  FILE *fd = NULL;
  int ret = -1;
  RKADK_U64 u32BoxSize;
  char boxHeader[THM_BOX_HEADER_LEN];
  char largeSize[THM_BOX_HEADER_LEN];

  fd = fopen(pszFileName, "r");
  if (!fd) {
    RKADK_LOGE("open %s failed", pszFileName);
    return -1;
  }

  while (!feof(fd)) {
    if (fread(boxHeader, THM_BOX_HEADER_LEN, 1, fd) != 1) {
      RKADK_LOGI("Can't read box header");
      break;
    }

    u32BoxSize = boxHeader[0] << 24 | boxHeader[1] << 16 | boxHeader[2] << 8 |
                 boxHeader[3];
    if (u32BoxSize <= 0) {
      RKADK_LOGI("last one box, not find thm box");
      break;
    }

    if (u32BoxSize == 1) {
      if (fread(largeSize, THM_BOX_HEADER_LEN, 1, fd) != 1) {
        RKADK_LOGE("read largeSize failed");
        break;
      }

      u32BoxSize = (RKADK_U64)largeSize[0] << 56 |
                   (RKADK_U64)largeSize[1] << 48 |
                   (RKADK_U64)largeSize[2] << 40 |
                   (RKADK_U64)largeSize[3] << 32 | largeSize[4] << 24 |
                   largeSize[5] << 16 | largeSize[6] << 8 | largeSize[7];

      if (fseek(fd, u32BoxSize - (THM_BOX_HEADER_LEN * 2), SEEK_CUR)) {
        RKADK_LOGE("largeSize seek failed");
        break;
      }

      continue;
    }

    if (boxHeader[4] == 't' && boxHeader[5] == 'h' && boxHeader[6] == 'm' &&
        boxHeader[7] == 0x20) {
      RKADK_U32 u32JpgSize = u32BoxSize - THM_BOX_HEADER_LEN;
      if (*pu32Size < u32JpgSize)
        RKADK_LOGW("pu32Size(%d) < u32JpgSize(%d)", *pu32Size, u32JpgSize);
      else
        *pu32Size = u32JpgSize;

      if (fread(pu8Buf, *pu32Size, 1, fd) != 1)
        RKADK_LOGE("read thm box body failed");
      else
        ret = 0;

      break;
    } else {
      if (fseek(fd, u32BoxSize - THM_BOX_HEADER_LEN, SEEK_CUR)) {
        RKADK_LOGE("seek failed");
        break;
      }
    }
  }

  if (fd)
    fclose(fd);

  return ret;
}

RKADK_S32 RKADK_GetThmInMp4(RKADK_CHAR *pszFileName, RKADK_U8 *pu8Buf,
                            RKADK_U32 *pu32Size) {
  int ret = 0;
  RTMediaBuffer *buffer = NULL;
  RtMetaData metaData;

#ifdef THUMB_SAVE_FILE
  FILE *file = NULL;
#endif

#ifdef DUMP_RUN_TIME
  struct timeval tv_begin, tv_end, tv_frame_end, tv_jpg_end;
  long start, end, frame_end, jpg_end;

  gettimeofday(&tv_begin, NULL);
  start = ((long)tv_begin.tv_sec) * 1000 + (long)tv_begin.tv_usec / 1000;
#endif

  RKADK_CHECK_POINTER(pszFileName, RKADK_FAILURE);
  RKADK_CHECK_POINTER(pu8Buf, RKADK_FAILURE);

  if (!GetThmInBox(pszFileName, pu8Buf, pu32Size))
    return 0;

  RKADK_PARAM_Init();

  RTMetadataRetriever *retriever = new RTMetadataRetriever();
  if (!retriever) {
    RKADK_LOGE("new RTMetadataRetriever failed");
    return -1;
  }

  ret = retriever->setDataSource(pszFileName, NULL);
  if (ret) {
    RKADK_LOGE("setDataSource failed(%d)", ret);
    goto exit;
  }

  metaData.setInt64(kRetrieverFrameAtTime, 0);
  buffer = retriever->getSingleFrameAtTime(&metaData);
  if (buffer == NULL || buffer->getMetaData() == NULL) {
    RKADK_LOGE("getSingleFrameAtTime failed");
    ret = -1;
    goto exit;
  }

#ifdef DUMP_RUN_TIME
  gettimeofday(&tv_frame_end, NULL);
  frame_end =
      ((long)tv_frame_end.tv_sec) * 1000 + (long)tv_frame_end.tv_usec / 1000;
  RKADK_LOGD("get yuv frame run time: %ld\n", frame_end - start);
#endif

#ifdef THUMB_SAVE_FILE
  file = fopen("/userdata/frame.yuv", "w");
  if (!file) {
    RKADK_LOGE("Create /userdata/frame.yuv failed");
  } else {
    fwrite(buffer->getData(), 1, buffer->getSize(), file);
    fclose(file);
    RKADK_LOGD("fwrite frame.yuv done");
  }
#endif

  ret = YuvToJpg(buffer, pu8Buf, pu32Size);
  if (ret) {
    RKADK_LOGE("YuvToJpg failed");
    goto exit;
  }

  if (BuildInThm(pszFileName, pu8Buf, *pu32Size))
    RKADK_LOGE("BuildInThm failed");

#ifdef DUMP_RUN_TIME
  gettimeofday(&tv_jpg_end, NULL);
  jpg_end = ((long)tv_jpg_end.tv_sec) * 1000 + (long)tv_jpg_end.tv_usec / 1000;
  RKADK_LOGD("yuv to jpg run time: %ld\n", jpg_end - frame_end);
#endif

exit:
  if (buffer)
    buffer->release();

  if (retriever)
    delete retriever;

#ifdef DUMP_RUN_TIME
  gettimeofday(&tv_end, NULL);
  end = ((long)tv_end.tv_sec) * 1000 + (long)tv_end.tv_usec / 1000;
  RKADK_LOGD("release run time: %ld\n", end - jpg_end);
  RKADK_LOGD("get thumb run time: %ld\n", end - start);
#endif

  malloc_trim(0);
  return ret;
}
