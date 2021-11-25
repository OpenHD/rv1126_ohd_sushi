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

#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mp3_header/mp3_header.h"
#include "isp/sample_common.h"
#include "rkadk_common.h"
#include "rkadk_log.h"
#include "rkadk_param.h"
#include "rkadk_stream.h"

extern int optind;
extern char *optarg;

#define IQ_FILE_PATH "/oem/etc/iqfiles"

static FILE *g_output_file = NULL;
static FILE *g_pcm_file = NULL;
static bool is_quit = false;
static RKADK_CODEC_TYPE_E g_enCodecType = RKADK_CODEC_TYPE_PCM;

static RKADK_CHAR optstr[] = "a:I:m:e:o:h";

static void print_usage(const RKADK_CHAR *name) {
  printf("usage example:\n");
  printf("\t%s [-a /oem/etc/iqfiles] [-I 0] [-m audio] [-e g711a] [-o "
         "/tmp/aenc.g711a]\n",
         name);
  printf("\t-a: enable aiq with dirpath provided, eg:-a "
         "/oem/etc/iqfiles/, Default /oem/etc/iqfiles,"
         "without this option aiq should run in other application\n");
  printf("\t-I: Camera id, Default:0\n");
  printf("\t-m: Test mode, Value: audio, video, Default:\"audio\"\n");
  printf("\t-e: Encode type, Value:pcm, g711a, g711u, h264, h265, mjpeg, "
         "jpeg, Default:pcm\n");
  printf("\t-o: Output path, Default:\"/tmp/ai.pcm\"\n");
  ;
}

static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  is_quit = true;
}

static RKADK_S32 VencDataCb(RKADK_VIDEO_STREAM_S *pVStreamData) {
  if (g_output_file) {
    fwrite(pVStreamData->astPack.apu8Addr, 1, pVStreamData->astPack.au32Len,
           g_output_file);
    RKADK_LOGD("#Write seq: %d, pts: %lld, size: %zu", pVStreamData->u32Seq,
               pVStreamData->astPack.u64PTS, pVStreamData->astPack.au32Len);
  }

  return 0;
}

static int VideoTest(RKADK_U32 u32CamID, RKADK_CHAR *pOutPath,
                     RKADK_CODEC_TYPE_E enCodecType, RKADK_CHAR *pIqfilesPath) {
  RKADK_S32 ret, fps;

  g_output_file = fopen(pOutPath, "w");
  if (!g_output_file) {
    RKADK_LOGE("open %s file failed, exit", pOutPath);
    return -1;
  }

#ifdef RKAIQ
  RKADK_PARAM_Init();
  ret = RKADK_PARAM_GetCamParam(u32CamID, RKADK_PARAM_TYPE_FPS, &fps);
  if (ret) {
    RKADK_LOGE("RKADK_PARAM_GetCamParam fps failed");
    return -1;
  }

  rk_aiq_working_mode_t hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
  RKADK_BOOL fec_enable = RKADK_FALSE;
  SAMPLE_COMM_ISP_Start(u32CamID, hdr_mode, fec_enable, pIqfilesPath, fps);
#endif

  RKADK_STREAM_VencRegisterCallback(u32CamID, VencDataCb);

  ret = RKADK_STREAM_VideoInit(u32CamID, enCodecType);
  if (ret) {
    RKADK_LOGE("RKADK_STREAM_VideoInit failed = %d", ret);
#ifdef RKAIQ
    SAMPLE_COMM_ISP_Stop(u32CamID);
#endif
    return -1;
  }

  ret = RKADK_STREAM_VencStart(u32CamID, enCodecType, -1);
  if (ret) {
    RKADK_LOGE("RKADK_STREAM_VencStart failed");
#ifdef RKAIQ
    SAMPLE_COMM_ISP_Stop(u32CamID);
#endif
    return -1;
  }

  RKADK_LOGD("initial finish\n");
  signal(SIGINT, sigterm_handler);

  while (!is_quit) {
    usleep(500000);
  }

  RKADK_LOGD("exit!");

  ret = RKADK_STREAM_VencStop(u32CamID);
  if (ret)
    RKADK_LOGE("RKADK_STREAM_VencStop failed");

  ret = RKADK_STREAM_VideoDeInit(u32CamID);
  if (ret)
    RKADK_LOGE("RKADK_STREAM_VideoDeInit failed = %d", ret);

  RKADK_STREAM_VencUnRegisterCallback(u32CamID);

#ifdef RKAIQ
  SAMPLE_COMM_ISP_Stop(u32CamID);
#endif

  if (g_output_file) {
    fclose(g_output_file);
    g_output_file = NULL;
  }

  return 0;
}

static RKADK_S32 AencDataCb(RKADK_AUDIO_STREAM_S *pAStreamData) {
  RKADK_U8 mp3_header[7];
  RKADK_AUDIO_INFO_S audioInfo;

  if (g_enCodecType == RKADK_CODEC_TYPE_MP3) {
    if (RKADK_STREAM_GetAudioInfo(&audioInfo)) {
      RKADK_LOGE("RKADK_STREAM_GetAudioInfo failed\n");
      return -1;
    }
  }

  if (g_output_file) {
    if (g_enCodecType == RKADK_CODEC_TYPE_MP3) {
      GetMp3Header(mp3_header, audioInfo.u32SampleRate, audioInfo.u32ChnCnt,
                    pAStreamData->u32Len);
      fwrite(mp3_header, 1, 7, g_output_file);
    }

    fwrite(pAStreamData->pStream, 1, pAStreamData->u32Len, g_output_file);
    RKADK_LOGD("#Write seq: %d, pts: %lld, size: %zu", pAStreamData->u32Seq,
               pAStreamData->u64TimeStamp, pAStreamData->u32Len);
  }

  return 0;
}

static RKADK_S32 PcmDataCb(RKADK_AUDIO_STREAM_S *pAStreamData) {

  if (g_pcm_file) {
    fwrite(pAStreamData->pStream, 1, pAStreamData->u32Len, g_pcm_file);
    RKADK_LOGD("#Write seq: %d, pts: %lld, size: %zu", pAStreamData->u32Seq,
               pAStreamData->u64TimeStamp, pAStreamData->u32Len);
  }

  return 0;
}

static int AudioTest(RKADK_CHAR *pOutPath, RKADK_CODEC_TYPE_E enCodecType) {
  RKADK_S32 ret;

  g_output_file = fopen(pOutPath, "w");
  if (!g_output_file) {
    RKADK_LOGE("open %s file failed, exit", pOutPath);
    return -1;
  }
  RKADK_STREAM_AencRegisterCallback(enCodecType, AencDataCb);

  if (enCodecType != RKADK_CODEC_TYPE_PCM) {
    g_pcm_file = fopen("/data/ai.pcm", "w");
    if (!g_pcm_file) {
      RKADK_LOGE("open /data/ai.pcm file failed, exit");
      return -1;
    }

    RKADK_STREAM_AencRegisterCallback(RKADK_CODEC_TYPE_PCM, PcmDataCb);
  }

  ret = RKADK_STREAM_AudioInit(enCodecType);
  if (ret) {
    RKADK_LOGE("RKADK_STREAM_AudioInit failed = %d", ret);
    return ret;
  }

  ret = RKADK_STREAM_AencStart();
  if (ret) {
    RKADK_LOGE("RKADK_STREAM_AencStart failed");
    return -1;
  }

  RKADK_LOGD("initial finish");
  signal(SIGINT, sigterm_handler);

  char cmd[64];
  while (!is_quit) {
    fgets(cmd, sizeof(cmd), stdin);
    if (strstr(cmd, "quit")) {
      RKADK_LOGD("#Get 'quit' cmd!");
      break;
    } else if (strstr(cmd, "start")) {
      RKADK_STREAM_AencStart();
    } else if (strstr(cmd, "stop")) {
      RKADK_STREAM_AencStop();
    }

    usleep(500000);
  }

  ret = RKADK_STREAM_AencStop();
  if (ret) {
    RKADK_LOGE("RKADK_STREAM_AencStop failed");
  }

  ret = RKADK_STREAM_AudioDeInit(enCodecType);
  if (ret)
    RKADK_LOGE("RKADK_STREAM_AudioDeInit failed = %d", ret);

  RKADK_STREAM_AencUnRegisterCallback(enCodecType);
  if (g_output_file) {
    fclose(g_output_file);
    g_output_file = NULL;
  }

  if (enCodecType != RKADK_CODEC_TYPE_PCM) {
    RKADK_STREAM_AencUnRegisterCallback(RKADK_CODEC_TYPE_PCM);
    if (g_pcm_file) {
      fclose(g_pcm_file);
      g_pcm_file = NULL;
    }
  }

  return 0;
}

static RKADK_CODEC_TYPE_E GetEncoderMode(char *encoder) {
  RKADK_CODEC_TYPE_E enCodecType = RKADK_CODEC_TYPE_PCM;
  RKADK_CHAR *pCodecName = "PCM";

  if (!strcmp(encoder, "pcm")) {
    enCodecType = RKADK_CODEC_TYPE_PCM;
    pCodecName = "PCM";
  } else if (!strcmp(encoder, "mp3")) {
    enCodecType = RKADK_CODEC_TYPE_MP3;
    pCodecName = "MP3";
  } else if (!strcmp(encoder, "g711a")) {
    enCodecType = RKADK_CODEC_TYPE_G711A;
    pCodecName = "G711A";
  } else if (!strcmp(encoder, "g711u")) {
    enCodecType = RKADK_CODEC_TYPE_G711U;
    pCodecName = "G711U";
  } else if (!strcmp(encoder, "h264")) {
    enCodecType = RKADK_CODEC_TYPE_H264;
    pCodecName = "H264";
  } else if (!strcmp(encoder, "h265")) {
    enCodecType = RKADK_CODEC_TYPE_H265;
    pCodecName = "H265";
  } else if (!strcmp(encoder, "mjpeg")) {
    enCodecType = RKADK_CODEC_TYPE_MJPEG;
    pCodecName = "MJPEG";
  } else if (!strcmp(encoder, "jpeg")) {
    enCodecType = RKADK_CODEC_TYPE_JPEG;
    pCodecName = "JPEG";
  }

  RKADK_LOGD("#CodecName: %s", pCodecName);
  return enCodecType;
}

int main(int argc, char *argv[]) {
  RKADK_U32 u32CamId = 0;
  RKADK_CHAR *pOutPath = "/tmp/ai.pcm";
  RKADK_CHAR *pMode = "audio";
  RKADK_CHAR *pIqfilesPath = IQ_FILE_PATH;
  int c;

  while ((c = getopt(argc, argv, optstr)) != -1) {
    const char *tmp_optarg = optarg;
    switch (c) {
    case 'a':
      if (!optarg && NULL != argv[optind] && '-' != argv[optind][0]) {
        tmp_optarg = argv[optind++];
      }

      if (tmp_optarg)
        pIqfilesPath = (char *)tmp_optarg;
      break;
    case 'I':
      u32CamId = atoi(optarg);
      break;
    case 'm':
      pMode = optarg;
      break;
    case 'e':
      g_enCodecType = GetEncoderMode(optarg);
      break;
    case 'o':
      pOutPath = optarg;
      break;
    case 'h':
    default:
      print_usage(argv[0]);
      optind = 0;
      return 0;
    }
  }
  optind = 0;

  RKADK_LOGD("#Test mode: %s", pMode);
  RKADK_LOGD("#Out path: %s", pOutPath);

  if (!strcmp(pMode, "audio"))
    AudioTest(pOutPath, g_enCodecType);
  else if (!strcmp(pMode, "video"))
    VideoTest(u32CamId, pOutPath, g_enCodecType, pIqfilesPath);
  else {
    RKADK_LOGE("Invalid test mode: %s", pMode);
    return -1;
  }

  return 0;
}
