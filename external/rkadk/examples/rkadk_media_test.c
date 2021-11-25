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
#include <pthread.h>
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
#include "rkadk_photo.h"
#include "rkadk_record.h"
#include "rkadk_stream.h"
#include "rkadk_thumb.h"

extern int optind;
extern char *optarg;

static RKADK_CHAR optstr[] = "I:a:i:t:h";
static FILE *g_vo_file = NULL;
static FILE *g_ao_file = NULL;
static FILE *g_pcm_file = NULL;
#define IQ_FILE_PATH "/oem/etc/iqfiles"

const char *g_thumb_file = "/tmp/test.mp4";

static bool is_quit = false;
static RKADK_U32 g_u32CamID = 0;

static void print_usage(const RKADK_CHAR *name) {
  printf("usage example:\n");
  printf("\t%s [-I 0] [-a /app] [-i /tmp/thumb.mp4] [-t 1000]\n", name);
  printf("\t-I: Camera id, Default:0\n");
  printf("\t-a: aiq file path, Default:/oem/etc/iqfiles\n");
  printf("\t-i: thumb test mp4 file, Default:/tmp/test.mp4\n");
  printf("\t-t: sleep time, Default:500ms\n");
}

static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  is_quit = true;
}

static RKADK_S32 VencDataCb(RKADK_VIDEO_STREAM_S *pVStreamData) {
  if (g_vo_file) {
    fwrite(pVStreamData->astPack.apu8Addr, 1, pVStreamData->astPack.au32Len,
           g_vo_file);
    // RKADK_LOGD("#Write seq: %d, pts: %lld, size: %zu", pVStreamData->u32Seq,
    //      pVStreamData->astPack.u64PTS, pVStreamData->astPack.au32Len);
  }

  return 0;
}

static int VideoStartTest(RKADK_U32 u32CameraID, RKADK_CHAR *pOutPath,
                          RKADK_CODEC_TYPE_E enCodecType) {
  RKADK_S32 ret;

  g_vo_file = fopen(pOutPath, "w");
  if (!g_vo_file) {
    RKADK_LOGE("open %s file failed, exit", pOutPath);
    return -1;
  }

  RKADK_STREAM_VencRegisterCallback(u32CameraID, VencDataCb);

  ret = RKADK_STREAM_VideoInit(u32CameraID, enCodecType);
  if (ret) {
    RKADK_LOGE("RKADK_STREAM_VideoInit failed = %d", ret);
    return -1;
  }

  ret = RKADK_STREAM_VencStart(u32CameraID, enCodecType, -1);
  if (ret) {
    RKADK_LOGE("RKADK_STREAM_VencStart failed");
    return -1;
  }

  RKADK_LOGD("initial finish\n");

  return 0;
}

static int VideoStopTest(RKADK_U32 u32CameraID) {
  int ret = 0;

  ret = RKADK_STREAM_VencStop(u32CameraID);
  if (ret)
    RKADK_LOGE("RKADK_STREAM_VencStop failed");

  ret = RKADK_STREAM_VideoDeInit(u32CameraID);
  if (ret)
    RKADK_LOGE("RKADK_STREAM_VideoDeInit failed = %d", ret);

  RKADK_STREAM_VencUnRegisterCallback(u32CameraID);

  if (g_vo_file) {
    fclose(g_vo_file);
    g_vo_file = NULL;
  }

  return ret;
}

static RKADK_CODEC_TYPE_E g_AudioCodecType = RKADK_CODEC_TYPE_PCM;
static RKADK_S32 AencDataCb(RKADK_AUDIO_STREAM_S *pAStreamData) {
  RKADK_U8 mp3_header[7];
  RKADK_AUDIO_INFO_S audioInfo;

  if (g_AudioCodecType == RKADK_CODEC_TYPE_MP3) {
    if (RKADK_STREAM_GetAudioInfo(&audioInfo)) {
      RKADK_LOGE("RKADK_STREAM_GetAudioInfo failed\n");
      return -1;
    }
  }

  if (g_ao_file) {
    if (g_AudioCodecType == RKADK_CODEC_TYPE_MP3) {
      GetMp3Header(mp3_header, audioInfo.u32SampleRate, audioInfo.u32ChnCnt,
                    pAStreamData->u32Len);
      fwrite(mp3_header, 1, 7, g_ao_file);
    }

    fwrite(pAStreamData->pStream, 1, pAStreamData->u32Len, g_ao_file);
    // RKADK_LOGD("#Write seq: %d, pts: %lld, size: %zu", pAStreamData->u32Seq,
    //           pAStreamData->u64TimeStamp, pAStreamData->u32Len);
  }

  return 0;
}

static RKADK_S32 PcmDataCb(RKADK_AUDIO_STREAM_S *pAStreamData) {

  if (g_pcm_file) {
    fwrite(pAStreamData->pStream, 1, pAStreamData->u32Len, g_pcm_file);
    // RKADK_LOGD("#Write seq: %d, pts: %lld, size: %zu", pAStreamData->u32Seq,
    //           pAStreamData->u64TimeStamp, pAStreamData->u32Len);
  }

  return 0;
}

static int AudioStartTest(RKADK_CHAR *pOutPath,
                          RKADK_CODEC_TYPE_E enCodecType) {
  RKADK_S32 ret;

  g_ao_file = fopen(pOutPath, "w");
  if (!g_ao_file) {
    RKADK_LOGE("open %s file failed, exit", pOutPath);
    return -1;
  }
  RKADK_STREAM_AencRegisterCallback(enCodecType, AencDataCb);

  if (enCodecType != RKADK_CODEC_TYPE_PCM) {
    g_pcm_file = fopen("/userdata/ai.pcm", "w");
    if (!g_pcm_file) {
      RKADK_LOGE("open %s file failed, exit", pOutPath);
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

  return 0;
}

static int AudioStopTest(RKADK_CODEC_TYPE_E enCodecType) {
  int ret = 0;

  ret = RKADK_STREAM_AencStop();
  if (ret) {
    RKADK_LOGE("RKADK_STREAM_AencStop failed");
  }

  ret = RKADK_STREAM_AudioDeInit(enCodecType);
  if (ret)
    RKADK_LOGE("RKADK_STREAM_AudioDeInit failed = %d", ret);

  RKADK_STREAM_AencUnRegisterCallback(enCodecType);
  if (g_ao_file) {
    fclose(g_ao_file);
    g_ao_file = NULL;
  }

  if (enCodecType != RKADK_CODEC_TYPE_PCM) {
    RKADK_STREAM_AencUnRegisterCallback(RKADK_CODEC_TYPE_PCM);
    if (g_pcm_file) {
      fclose(g_pcm_file);
      g_pcm_file = NULL;
    }
  }

  return ret;
}

static void PhotoDataRecv(RKADK_U8 *pu8DataBuf, RKADK_U32 u32DataLen) {
  static RKADK_U32 photoId = 0;
  char jpegPath[128];
  FILE *file = NULL;

  if (!pu8DataBuf || u32DataLen <= 0) {
    RKADK_LOGE("Invalid packet buffer");
    return;
  }

  if (photoId > 5)
    photoId = 0;

  memset(jpegPath, 0, 128);
  sprintf(jpegPath, "/userdata/PhotoTest_%d.jpeg", photoId);
  file = fopen(jpegPath, "w");
  if (!file) {
    RKADK_LOGE("Create jpeg file(%s) failed", jpegPath);
    return;
  }

  RKADK_LOGD("save jpeg to %s", jpegPath);

  fwrite(pu8DataBuf, 1, u32DataLen, file);
  fclose(file);

  photoId++;
  if(photoId > 10)
    photoId = 0;
}

void *TakePhotoThread(void *para) {
  int ret;
  RKADK_PHOTO_ATTR_S stPhotoAttr;

  memset(&stPhotoAttr, 0, sizeof(RKADK_PHOTO_ATTR_S));
  stPhotoAttr.u32CamID = g_u32CamID;
  stPhotoAttr.enPhotoType = RKADK_PHOTO_TYPE_SINGLE;
  stPhotoAttr.unPhotoTypeAttr.stSingleAttr.s32Time_sec = 0;
  stPhotoAttr.pfnPhotoDataProc = PhotoDataRecv;
  stPhotoAttr.stThumbAttr.bSupportDCF = RKADK_FALSE;
  stPhotoAttr.stThumbAttr.stMPFAttr.eMode = RKADK_PHOTO_MPF_SINGLE;
  stPhotoAttr.stThumbAttr.stMPFAttr.sCfg.u8LargeThumbNum = 1;
  stPhotoAttr.stThumbAttr.stMPFAttr.sCfg.astLargeThumbSize[0].u32Width = 320;
  stPhotoAttr.stThumbAttr.stMPFAttr.sCfg.astLargeThumbSize[0].u32Height = 180;

  ret = RKADK_PHOTO_Init(&stPhotoAttr);
  if (ret) {
    RKADK_LOGE("RKADK_PHOTO_Init failed(%d)", ret);
    return NULL;
  }

  while (!is_quit) {
    RKADK_PHOTO_TakePhoto(&stPhotoAttr);
    sleep(5);
  }

  RKADK_PHOTO_DeInit(stPhotoAttr.u32CamID);
  RKADK_LOGD("Exit Photo Thread");
  return NULL;
}

static void GetThumbTest() {
  char *filePath = "/tmp/thm_test.jpg";
  RKADK_U32 size = 50 * 1024;
  RKADK_U8 buffer[size];
  FILE *file = NULL;

  if (RKADK_GetThmInMp4(g_thumb_file, buffer, &size)) {
    RKADK_LOGE("RKADK_GetThmInMp4 failed");
    return;
  }

  if(!size) {
    RKADK_LOGE("RKADK_GetThmInMp4 faile, size = %d", size);
    return;
  }

  RKADK_LOGD("jpg size = %d", size);
  file = fopen(filePath, "w");
  if (!file) {
    RKADK_LOGE("Create file(%s) failed", filePath);
    return;
  }

  fwrite(buffer, 1, size, file);
  fclose(file);
}

static RKADK_S32
GetRecordFileName(RKADK_MW_PTR pRecorder, RKADK_U32 u32FileCnt,
                  RKADK_CHAR (*paszFilename)[RKADK_MAX_FILE_PATH_LEN]) {
  static RKADK_U32 u32FileIdx = 0;

  if (u32FileIdx > 5)
    u32FileIdx = 0;

  for (RKADK_U32 i = 0; i < u32FileCnt; i++) {
    sprintf(paszFilename[i], "/userdata/RecordTest_%u.mp4", u32FileIdx);
    u32FileIdx++;
    RKADK_LOGD("NewRecordFileName:[%s]", paszFilename[i]);
  }

  return 0;
}

static RKADK_VOID
RecordEventCallback(RKADK_MW_PTR pRecorder,
                    const RKADK_REC_EVENT_INFO_S *pstEventInfo) {
  switch (pstEventInfo->enEvent) {
  case MUXER_EVENT_STREAM_START:
    printf("+++++ MUXER_EVENT_STREAM_START +++++\n");
    break;
  case MUXER_EVENT_STREAM_STOP:
    printf("+++++ MUXER_EVENT_STREAM_STOP +++++\n");
    break;
  case MUXER_EVENT_FILE_BEGIN:
    printf("+++++ MUXER_EVENT_FILE_BEGIN +++++\n");
    printf("\tstFileInfo: %s\n",
           pstEventInfo->unEventInfo.stFileInfo.asFileName);
    printf("\tu32Duration: %d\n",
           pstEventInfo->unEventInfo.stFileInfo.u32Duration);
    break;
  case MUXER_EVENT_FILE_END:
    printf("+++++ MUXER_EVENT_FILE_END +++++\n");
    printf("\tstFileInfo: %s\n",
           pstEventInfo->unEventInfo.stFileInfo.asFileName);
    printf("\tu32Duration: %d\n",
           pstEventInfo->unEventInfo.stFileInfo.u32Duration);
    break;
  case MUXER_EVENT_MANUAL_SPLIT_END:
    printf("+++++ MUXER_EVENT_MANUAL_SPLIT_END +++++\n");
    printf("\tstFileInfo: %s\n",
           pstEventInfo->unEventInfo.stFileInfo.asFileName);
    printf("\tu32Duration: %d\n",
           pstEventInfo->unEventInfo.stFileInfo.u32Duration);
    break;
  case MUXER_EVENT_ERR_CREATE_FILE_FAIL:
    printf("+++++ MUXER_EVENT_ERR_CREATE_FILE_FAIL +++++\n");
    break;
  case MUXER_EVENT_ERR_WRITE_FILE_FAIL:
    printf("+++++ MUXER_EVENT_ERR_WRITE_FILE_FAIL +++++\n");
    break;
  default:
    printf("+++++ Unknown event(%d) +++++\n", pstEventInfo->enEvent);
    break;
  }
}

static int IspProcess(RKADK_S32 s32CamID) {
  int ret;
  bool mirror = false, flip = false;
  RKADK_U32 ldcLevel = 0;
  RKADK_U32 antifog = 0;
  RKADK_U32 wdrLevel = 0;

  // set mirror flip
  ret = RKADK_PARAM_GetCamParam(s32CamID, RKADK_PARAM_TYPE_MIRROR, &mirror);
  if (ret)
    RKADK_LOGW("RKADK_PARAM_GetCamParam mirror failed");

  ret = RKADK_PARAM_GetCamParam(s32CamID, RKADK_PARAM_TYPE_FLIP, &flip);
  if (ret)
    RKADK_LOGW("RKADK_PARAM_GetCamParam flip failed");

  if (mirror || flip) {
    ret = SAMPLE_COMM_ISP_SET_MirrorFlip(s32CamID, mirror, flip);
    if (ret)
      RKADK_LOGW("SAMPLE_COMM_ISP_SET_MirrorFlip failed");
  }

  // set antifog
  ret = RKADK_PARAM_GetCamParam(s32CamID, RKADK_PARAM_TYPE_ANTIFOG, &antifog);
  if (ret) {
    RKADK_LOGW("RKADK_PARAM_GetCamParam antifog failed");
  } else {
    if (antifog > 0) {
      ret = SAMPLE_COMM_ISP_EnableDefog(s32CamID, true, OP_MANUAL, antifog);
      if (ret)
        RKADK_LOGW("SAMPLE_COMM_ISP_SET_EnableDefog failed");
    }
  }

  // set LDCH
  ret = RKADK_PARAM_GetCamParam(s32CamID, RKADK_PARAM_TYPE_LDC, &ldcLevel);
  if (ret) {
    RKADK_LOGW("RKADK_PARAM_GetCamParam ldc failed");
  } else {
    if (ldcLevel > 0) {
      ret = SAMPLE_COMM_ISP_EnableLdch(s32CamID, true, ldcLevel);
      if (ret)
        RKADK_LOGW("SAMPLE_COMM_ISP_EnableLdch failed");
    }
  }

  // WDR
  ret = RKADK_PARAM_GetCamParam(s32CamID, RKADK_PARAM_TYPE_WDR, &wdrLevel);
  if (ret) {
    RKADK_LOGW("RKADK_PARAM_GetCamParam wdr failed");
  } else {
    if (wdrLevel > 0) {
      ret = SAMPLE_COMM_ISP_SetDarkAreaBoostStrth(s32CamID, wdrLevel);
      if (ret)
        RKADK_LOGW("SAMPLE_COMM_ISP_SetDarkAreaBoostStrth failed");
    }
  }

#ifdef RKADK_DUMP_ISP_RESULT
  // mirror flip
  ret = SAMPLE_COMM_ISP_GET_MirrorFlip(s32CamID, &mirror, &flip);
  if (ret)
    RKADK_LOGW("SAMPLE_COMM_ISP_GET_MirrorFlip failed");
  else
    RKADK_LOGD("GET mirror = %d, flip = %d", mirror, flip);

  // antifog
  bool on;
  ret = SAMPLE_COMM_ISP_GetMDhzStrth(s32CamID, &on, &ldcLevel);
  if (ret)
    RKADK_LOGW("SAMPLE_COMM_ISP_GetMDhzStrth failed");
  else
    RKADK_LOGD("GET antifog on = %d, ldcLevel = %d", on, ldcLevel);

  // LDCH
  rk_aiq_ldch_attrib_t attr;
  ret = SAMPLE_COMM_ISP_GetLdchAttrib(s32CamID, &attr);
  if(ret) {
    RKADK_LOGW("SAMPLE_COMM_ISP_GetLdchAttrib failed");
  } else {
    RKADK_LOGD("LDC attr.en = %d", attr.en);
    RKADK_LOGD("LDC attr.correct_level = %d", attr.correct_level);
  }

  //WDR
  ret = SAMPLE_COMM_ISP_GetDarkAreaBoostStrth(s32CamID, &wdrLevel);
  if(ret)
    RKADK_LOGW("SAMPLE_COMM_ISP_GetDarkAreaBoostStrth failed");
  else
    RKADK_LOGD("WDR wdrLevel = %d", wdrLevel);
#endif

  return 0;
}

int main(int argc, char *argv[]) {
  int c, ret;
  RKADK_RECORD_ATTR_S stRecAttr;
  RKADK_MW_PTR pRecorder = NULL;
  RKADK_CHAR *pIqfilesPath = IQ_FILE_PATH;
  pthread_t tid = 0;
  RKADK_REC_TYPE_E enRecType = RKADK_REC_TYPE_NORMAL;
  RKADK_U32 u32SleepTime = 500;

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
    case 'i':
      g_thumb_file = optarg;
      break;
    case 'I':
      g_u32CamID = atoi(optarg);
      break;
    case 't':
      u32SleepTime = atoi(optarg);
      break;
    case 'h':
    default:
      print_usage(argv[0]);
      optind = 0;
      return -1;
    }
  }
  optind = 0;

#ifdef RKAIQ
  int fps = 30;
  RK_BOOL fec_enable = RK_FALSE;
  rk_aiq_working_mode_t hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;

  RKADK_PARAM_Init();
  ret = RKADK_PARAM_GetCamParam(g_u32CamID, RKADK_PARAM_TYPE_FPS, &fps);
  if (ret) {
    RKADK_LOGE("RKADK_PARAM_GetCamParam fps failed");
    return -1;
  }

  SAMPLE_COMM_ISP_Start(g_u32CamID, hdr_mode, fec_enable, pIqfilesPath, fps);

  //IspProcess(g_u32CamID);
#endif

  ret = RKADK_PARAM_GetCamParam(
      stRecAttr.s32CamID, RKADK_PARAM_TYPE_RECORD_TYPE, &enRecType);
  if (ret)
    RKADK_LOGW("RKADK_PARAM_GetCamParam record type failed");

  // set default value
  stRecAttr.s32CamID = g_u32CamID;
  stRecAttr.enRecType = enRecType;
  stRecAttr.pfnRequestFileNames = GetRecordFileName;

  if (RKADK_RECORD_Create(&stRecAttr, &pRecorder)) {
    RKADK_LOGE("Create recorder failed");
#ifdef RKAIQ
    SAMPLE_COMM_ISP_Stop(g_u32CamID);
#endif
    return -1;
  }

  RKADK_RECORD_RegisterEventCallback(pRecorder, RecordEventCallback);
  RKADK_RECORD_Start(pRecorder);

  // stream video test
  VideoStartTest(g_u32CamID, "/userdata/stream.h264", RKADK_CODEC_TYPE_H264);

  // stream audio test
  g_AudioCodecType = RKADK_CODEC_TYPE_MP3;
  AudioStartTest("/userdata/aac.adts", RKADK_CODEC_TYPE_MP3);

  // take photo pre second
  if (pthread_create(&tid, NULL, TakePhotoThread, NULL))
    RKADK_LOGE("Create take photo thread failed");

  signal(SIGINT, sigterm_handler);

  while (!is_quit) {
    if (is_quit)
      break;

    GetThumbTest();
    usleep(u32SleepTime * 1000);
  }

  RKADK_RECORD_Stop(pRecorder);
  RKADK_RECORD_Destroy(pRecorder);

  VideoStopTest(g_u32CamID);
  AudioStopTest(g_AudioCodecType);

  if (tid) {
    ret = pthread_join(tid, NULL);
    if (ret) {
      RKADK_LOGE("thread exit failed = %d", ret);
    } else {
      RKADK_LOGI("thread exit successed");
    }
  }

#ifdef RKAIQ
  SAMPLE_COMM_ISP_Stop(g_u32CamID);
#endif

  RKADK_LOGD("exit!");
  return 0;
}
