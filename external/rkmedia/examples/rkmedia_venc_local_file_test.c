// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "rkmedia_api.h"
#include "rkmedia_venc.h"

static bool quit = false;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

static void *GetMediaBuffer(void *arg) {
  char *ot_path = (char *)arg;
  printf("#Start %s thread, arg:%p, out path: %s\n", __func__, arg, ot_path);
  FILE *save_file = fopen(ot_path, "w");
  if (!save_file)
    printf("ERROR: Open %s failed!\n", ot_path);

  MEDIA_BUFFER mb = NULL;
  while (!quit) {
    mb = RK_MPI_SYS_GetMediaBuffer(RK_ID_VENC, 0, -1);
    if (!mb) {
      printf("RK_MPI_SYS_GetMediaBuffer get null buffer!\n");
      break;
    }

    printf("Get packet:ptr:%p, fd:%d, size:%zu, mode:%d, channel:%d, "
           "timestamp:%lld\n",
           RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetFD(mb), RK_MPI_MB_GetSize(mb),
           RK_MPI_MB_GetModeID(mb), RK_MPI_MB_GetChannelID(mb),
           RK_MPI_MB_GetTimestamp(mb));

    if (save_file)
      fwrite(RK_MPI_MB_GetPtr(mb), 1, RK_MPI_MB_GetSize(mb), save_file);
    RK_MPI_MB_ReleaseBuffer(mb);
  }

  if (save_file)
    fclose(save_file);

  return NULL;
}

static RK_CHAR optstr[] = "?:i:o:w:h:t:c:";
static const struct option long_options[] = {
    {"input", required_argument, NULL, 'i'},
    {"output", required_argument, NULL, 'o'},
    {"width", required_argument, NULL, 'w'},
    {"height", required_argument, NULL, 'h'},
    {"type", required_argument, NULL, 't'},
    {"cnt", required_argument, NULL, 'c'},
    {"help", no_argument, NULL, '?'},
    {NULL, 0, NULL, 0},
};

static void print_usage(const RK_CHAR *name) {
  printf("usage example:\n");
  printf("\t%s [-i in.nv12] [-o out.h264] [-w 1920] [-h 1080] [-t h264]\n",
         name);
  printf("\t-i | --input: Input image file\n");
  printf("\t-o | --output: Output file\n");
  printf("\t-w | --width: Image width\n");
  printf("\t-h | --height: Image height\n");
  printf("\t-t | --height: encoder type, value: h264, h265, mjpeg\n");
  printf("\t-c | --cnt: encoder frame cnt. default: -1:end of file.\n");
  printf("\t-? | --help: show help\n");
}

int main(int argc, char *argv[]) {
  char *input_file = NULL;
  char *output_file = NULL;
  RK_U32 u32Width = 0;
  RK_U32 u32Height = 0;
  RK_S32 u32FrameCnt = -1;
  VENC_RC_MODE_E enEncoderMode = VENC_RC_MODE_H264CBR;
  RK_U32 u32Fps = 30;

  int c = 0;
  opterr = 1;
  while ((c = getopt_long(argc, argv, optstr, long_options, NULL)) != -1) {
    switch (c) {
    case 'i':
      input_file = optarg;
      break;
    case 'o':
      output_file = optarg;
      break;
    case 'w':
      u32Width = (RK_U32)atoi(optarg);
      break;
    case 'h':
      u32Height = (RK_U32)atoi(optarg);
      break;
    case 'c':
      u32FrameCnt = atoi(optarg);
      break;
    case 't':
      if (!strcmp(optarg, "h264")) {
        enEncoderMode = VENC_RC_MODE_H264CBR;
      } else if (!strcmp(optarg, "h265")) {
        enEncoderMode = VENC_RC_MODE_H265CBR;
      } else if (!strcmp(optarg, "mjpeg")) {
        enEncoderMode = VENC_RC_MODE_MJPEGCBR;
      } else {
        printf("ERROR: Invalid encoder type!\n");
        return -1;
      }
      break;
    case '?':
    default:
      print_usage(argv[0]);
      return 0;
    }
  }

  printf("#InFile:%s\n", input_file);
  printf("#OutFile:%s\n", output_file);
  printf("#ImgWidth:%u\n", u32Width);
  printf("#ImgHeight:%u\n", u32Height);
  printf("#FrameCnt:%d\n", u32FrameCnt);
  switch (enEncoderMode) {
  case VENC_RC_MODE_H264CBR:
    printf("#EncoderType:H264 Cbr\n");
    break;
  case VENC_RC_MODE_H265CBR:
    printf("#EncoderType:H265 Cbr\n");
    break;
  case VENC_RC_MODE_MJPEGCBR:
    printf("#EncoderType:Mjpeg Cbr\n");
    break;
  default:
    break;
  }

  if (!input_file || !u32Width || !u32Height || !u32FrameCnt) {
    printf("ERROR: Please check your input args!\n");
    return -1;
  }

  RK_S32 ret = 0;
  RK_MPI_SYS_Init();

  VENC_CHN_ATTR_S venc_chn_attr = {0};
  venc_chn_attr.stVencAttr.u32PicWidth = u32Width;
  venc_chn_attr.stVencAttr.u32PicHeight = u32Height;
  venc_chn_attr.stVencAttr.u32VirWidth = u32Width;
  venc_chn_attr.stVencAttr.u32VirHeight = u32Height;
  venc_chn_attr.stVencAttr.imageType = IMAGE_TYPE_NV12;
  if (enEncoderMode == VENC_RC_MODE_H264CBR) {
    venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_H264;
    venc_chn_attr.stVencAttr.u32Profile = 77;
    venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
    venc_chn_attr.stRcAttr.stH264Cbr.u32Gop = 2 * u32Fps;
    venc_chn_attr.stRcAttr.stH264Cbr.u32BitRate = u32Width * u32Height;
    venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateNum = u32Fps;
    venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateNum = u32Fps;
  } else if (enEncoderMode == VENC_RC_MODE_H265CBR) {
    venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_H265;
    venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
    venc_chn_attr.stRcAttr.stH265Cbr.u32Gop = 2 * u32Fps;
    venc_chn_attr.stRcAttr.stH265Cbr.u32BitRate = u32Width * u32Height;
    venc_chn_attr.stRcAttr.stH265Cbr.fr32DstFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH265Cbr.fr32DstFrameRateNum = u32Fps;
    venc_chn_attr.stRcAttr.stH265Cbr.u32SrcFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH265Cbr.u32SrcFrameRateNum = u32Fps;
  } else {
    venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_MJPEG;
    venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_MJPEGCBR;
    venc_chn_attr.stRcAttr.stMjpegCbr.fr32DstFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stMjpegCbr.fr32DstFrameRateNum = u32Fps;
    venc_chn_attr.stRcAttr.stMjpegCbr.u32SrcFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stMjpegCbr.u32SrcFrameRateNum = u32Fps;
    venc_chn_attr.stRcAttr.stMjpegCbr.u32BitRate = u32Width * u32Height * 8;
  }
  ret = RK_MPI_VENC_CreateChn(0, &venc_chn_attr);
  if (ret) {
    printf("ERROR: Create venc failed!\n");
    exit(0);
  }

  printf("%s initial finish\n", __func__);
  signal(SIGINT, sigterm_handler);

  FILE *read_file = fopen(input_file, "r");
  if (!read_file) {
    printf("ERROR: open %s failed!\n", input_file);
    exit(0);
  }

  pthread_t read_thread;
  pthread_create(&read_thread, NULL, GetMediaBuffer, output_file);

  RK_U32 u32FrameId = 0;
  RK_S32 s32ReadSize = 0;
  RK_S32 s32FrameSize = 0;
  RK_U64 u64TimePeriod = 1000000 / u32Fps; // us
  MB_IMAGE_INFO_S stImageInfo = {u32Width, u32Height, u32Width, u32Height,
                                 IMAGE_TYPE_NV12};

  while (!quit) {
    // Create dma buffer. Note that mpp encoder only support dma buffer.
    // As CPU write mb, Encoder IP read buffer, mb should be NOCACHED type.
    // This can avoid buffer synchronization problems.
    MEDIA_BUFFER mb =
        RK_MPI_MB_CreateImageBuffer(&stImageInfo, RK_TRUE, MB_FLAG_NOCACHED);
    if (!mb) {
      printf("ERROR: no space left!\n");
      break;
    }

    // One frame size for nv12 image.
    s32FrameSize = u32Width * u32Height * 3 / 2;
    s32ReadSize = fread(RK_MPI_MB_GetPtr(mb), 1, s32FrameSize, read_file);
    if (s32ReadSize != s32FrameSize) {
      printf("Get end of file!\n");
      quit = true;
    }
    RK_MPI_MB_SetSize(mb, s32ReadSize);
    RK_MPI_MB_SetTimestamp(mb, u32FrameId * u64TimePeriod);
    printf("#Send frame[%d] fd=%d to out...\n", u32FrameId++,
           RK_MPI_MB_GetFD(mb));
    RK_MPI_SYS_SendMediaBuffer(RK_ID_VENC, 0, mb);
    // mb must be release. The encoder has internal references to the data sent
    // in. Therefore, mb cannot be reused directly
    RK_MPI_MB_ReleaseBuffer(mb);

    if ((u32FrameCnt > 0) && ((RK_S32)u32FrameId >= u32FrameCnt))
      quit = true;

    usleep(u64TimePeriod);
  }

  printf("%s exit!\n", __func__);
  RK_MPI_VENC_DestroyChn(0);

  return 0;
}
