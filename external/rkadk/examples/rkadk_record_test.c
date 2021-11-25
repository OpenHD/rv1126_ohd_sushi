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

#include "isp/sample_common.h"
#include "rkadk_common.h"
#include "rkadk_log.h"
#include "rkadk_param.h"
#include "rkadk_record.h"

extern int optind;
extern char *optarg;

static RKADK_CHAR optstr[] = "a:I:t:h";

static bool is_quit = false;
#define IQ_FILE_PATH "/etc/iqfiles"

static void print_usage(const RKADK_CHAR *name) {
  printf("usage example:\n");
  printf("\t%s [-a /etc/iqfiles] [-I 0] [-t 0]\n", name);
  printf("\t-a: enable aiq with dirpath provided, eg:-a "
         "/oem/etc/iqfiles/, Default /oem/etc/iqfiles,"
         "without this option aiq should run in other application\n");
  printf("\t-I: Camera id, Default:0\n");
  printf("\t-t: Record type, Default:0, Options: 0(normal), 1(lapse)\n");
}

static RKADK_S32
GetRecordFileName(RKADK_MW_PTR pRecorder, RKADK_U32 u32FileCnt,
                  RKADK_CHAR (*paszFilename)[RKADK_MAX_FILE_PATH_LEN]) {
  static RKADK_U32 u32FileIdx = 0;

  RKADK_LOGD("u32FileCnt:%d, pRecorder:%p", u32FileCnt, pRecorder);

  if (u32FileIdx >= 10)
    u32FileIdx = 0;

  for (RKADK_U32 i = 0; i < u32FileCnt; i++) {
    sprintf(paszFilename[i], "/userdata/RecordTest_%u.mp4", u32FileIdx);
    u32FileIdx++;
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
    RKADK_LOGE("RKADK_PARAM_GetCamParam mirror failed");

  ret = RKADK_PARAM_GetCamParam(s32CamID, RKADK_PARAM_TYPE_FLIP, &flip);
  if (ret)
    RKADK_LOGE("RKADK_PARAM_GetCamParam flip failed");

  if (mirror || flip) {
    ret = SAMPLE_COMM_ISP_SET_MirrorFlip(s32CamID, mirror, flip);
    if (ret)
      RKADK_LOGE("SAMPLE_COMM_ISP_SET_MirrorFlip failed");
  }

  // set antifog
  ret = RKADK_PARAM_GetCamParam(s32CamID, RKADK_PARAM_TYPE_ANTIFOG, &antifog);
  if (ret) {
    RKADK_LOGE("RKADK_PARAM_GetCamParam antifog failed");
  } else {
    if (antifog > 0) {
      ret = SAMPLE_COMM_ISP_EnableDefog(s32CamID, true, OP_MANUAL, antifog);
      if (ret)
        RKADK_LOGE("SAMPLE_COMM_ISP_SET_EnableDefog failed");
    }
  }

  // set LDCH
  ret = RKADK_PARAM_GetCamParam(s32CamID, RKADK_PARAM_TYPE_LDC, &ldcLevel);
  if (ret) {
    RKADK_LOGE("RKADK_PARAM_GetCamParam ldc failed");
  } else {
    if (ldcLevel > 0) {
      ret = SAMPLE_COMM_ISP_EnableLdch(s32CamID, true, ldcLevel);
      if (ret)
        RKADK_LOGE("SAMPLE_COMM_ISP_EnableLdch failed");
    }
  }

  // WDR
  ret = RKADK_PARAM_GetCamParam(s32CamID, RKADK_PARAM_TYPE_WDR, &wdrLevel);
  if (ret) {
    RKADK_LOGE("RKADK_PARAM_GetCamParam wdr failed");
  } else {
    if (wdrLevel > 0) {
      ret = SAMPLE_COMM_ISP_SetDarkAreaBoostStrth(s32CamID, wdrLevel);
      if (ret)
        RKADK_LOGE("SAMPLE_COMM_ISP_SetDarkAreaBoostStrth failed");
    }
  }

#ifdef RKADK_DUMP_ISP_RESULT
  // mirror flip
  ret = SAMPLE_COMM_ISP_GET_MirrorFlip(s32CamID, &mirror, &flip);
  if (ret)
    RKADK_LOGE("SAMPLE_COMM_ISP_GET_MirrorFlip failed");
  else
    RKADK_LOGD("GET mirror = %d, flip = %d", mirror, flip);

  // antifog
  bool on;
  ret = SAMPLE_COMM_ISP_GetMDhzStrth(s32CamID, &on, &ldcLevel);
  if (ret)
    RKADK_LOGE("SAMPLE_COMM_ISP_GetMDhzStrth failed");
  else
    RKADK_LOGD("GET antifog on = %d, ldcLevel = %d", on, ldcLevel);

  // LDCH
  rk_aiq_ldch_attrib_t attr;
  ret = SAMPLE_COMM_ISP_GetLdchAttrib(s32CamID, &attr);
  if (ret) {
    RKADK_LOGE("SAMPLE_COMM_ISP_GetLdchAttrib failed");
  } else {
    RKADK_LOGD("LDC attr.en = %d", attr.en);
    RKADK_LOGD("LDC attr.correct_level = %d", attr.correct_level);
  }

  // WDR
  ret = SAMPLE_COMM_ISP_GetDarkAreaBoostStrth(s32CamID, &wdrLevel);
  if (ret)
    RKADK_LOGE("SAMPLE_COMM_ISP_GetDarkAreaBoostStrth failed");
  else
    RKADK_LOGD("WDR wdrLevel = %d", wdrLevel);
#endif

  return 0;
}

static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  is_quit = true;
}

int main(int argc, char *argv[]) {
  int c, ret, fps;
  RKADK_RECORD_ATTR_S stRecAttr;
  RKADK_CHAR *pIqfilesPath = IQ_FILE_PATH;
  RKADK_MW_PTR pRecorder = NULL;
  RK_BOOL fec_enable = RK_FALSE;
  rk_aiq_working_mode_t hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;

  // set default value
  stRecAttr.s32CamID = 0;
  stRecAttr.enRecType = RKADK_REC_TYPE_NORMAL;
  stRecAttr.pfnRequestFileNames = GetRecordFileName;

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
      stRecAttr.s32CamID = atoi(optarg);
      break;
    case 't':
      stRecAttr.enRecType = atoi(optarg);
      break;
    case 'h':
    default:
      print_usage(argv[0]);
      optind = 0;
      return -1;
    }
  }
  optind = 0;

  RKADK_PARAM_Init();

record:
#ifdef RKAIQ
  ret = RKADK_PARAM_GetCamParam(stRecAttr.s32CamID, RKADK_PARAM_TYPE_FPS, &fps);
  if (ret) {
    RKADK_LOGE("RKADK_PARAM_GetCamParam fps failed");
    return -1;
  }

  SAMPLE_COMM_ISP_Start(stRecAttr.s32CamID, hdr_mode, fec_enable, pIqfilesPath,
                        fps);

  IspProcess(stRecAttr.s32CamID);
#endif

  ret = RKADK_PARAM_GetCamParam(
      stRecAttr.s32CamID, RKADK_PARAM_TYPE_RECORD_TYPE, &stRecAttr.enRecType);
  if (ret)
    RKADK_LOGW("RKADK_PARAM_GetCamParam record type failed");

  if (RKADK_RECORD_Create(&stRecAttr, &pRecorder)) {
    RKADK_LOGE("Create recorder failed");
#ifdef RKAIQ
    SAMPLE_COMM_ISP_Stop(stRecAttr.s32CamID);
#endif
    return -1;
  }

  RKADK_RECORD_RegisterEventCallback(pRecorder, RecordEventCallback);
  RKADK_LOGD("initial finish\n");

  RKADK_RECORD_Start(pRecorder);

  signal(SIGINT, sigterm_handler);
  char cmd[64];
  printf("\n#Usage: input 'quit' to exit programe!\n"
         "peress any other key to manual split file\n");
  while (!is_quit) {
    fgets(cmd, sizeof(cmd), stdin);
    if (strstr(cmd, "quit")) {
      RKADK_LOGD("#Get 'quit' cmd!");
      break;
    } else if (strstr(cmd, "ms")) { // manual-split
      RKADK_REC_MANUAL_SPLIT_ATTR_S stSplitAttr;

      stSplitAttr.enManualType = MUXER_PRE_MANUAL_SPLIT;

      RKADK_PARAM_GetCamParam(stRecAttr.s32CamID, RKADK_PARAM_TYPE_SPLITTIME,
                              &stSplitAttr.stPreSplitAttr.u32DurationSec);
      if (stSplitAttr.stPreSplitAttr.u32DurationSec <= 0)
        stSplitAttr.stPreSplitAttr.u32DurationSec = 30;

      RKADK_RECORD_ManualSplit(pRecorder, &stSplitAttr);
    } else if (strstr(cmd, "switch_res")) {
      RKADK_PARAM_RES_E enRes;
      if (strstr(cmd, "switch_res_1080"))
        enRes = RKADK_RES_1080P;
      else if (strstr(cmd, "switch_res_2K"))
        enRes = RKADK_RES_1520P;
      else
        enRes = RKADK_RES_2160P;

      RKADK_RECORD_Stop(pRecorder);
      RKADK_RECORD_Destroy(pRecorder);
      pRecorder = NULL;

      // must destroy recorder before switching the resolution
      RKADK_PARAM_SetCamParam(stRecAttr.s32CamID, RKADK_PARAM_TYPE_RES, &enRes);

#ifdef RKAIQ
      SAMPLE_COMM_ISP_Stop(stRecAttr.s32CamID);
#endif
      goto record;
    } else if (strstr(cmd, "flip-")) {
      bool flip = false;
      SAMPLE_COMM_ISP_SET_MirrorFlip(stRecAttr.s32CamID, false, flip);
      RKADK_PARAM_SetCamParam(stRecAttr.s32CamID, RKADK_PARAM_TYPE_FLIP, &flip);
    } else if (strstr(cmd, "flip+")) {
      bool flip = true;
      SAMPLE_COMM_ISP_SET_MirrorFlip(stRecAttr.s32CamID, false, flip);
      RKADK_PARAM_SetCamParam(stRecAttr.s32CamID, RKADK_PARAM_TYPE_FLIP, &flip);
    } else if (strstr(cmd, "mirror-")) {
      bool mirror = false;
      SAMPLE_COMM_ISP_SET_MirrorFlip(stRecAttr.s32CamID, mirror, false);
      RKADK_PARAM_SetCamParam(stRecAttr.s32CamID, RKADK_PARAM_TYPE_MIRROR,
                              &mirror);
    } else if (strstr(cmd, "mirror+")) {
      bool mirror = true;
      SAMPLE_COMM_ISP_SET_MirrorFlip(stRecAttr.s32CamID, mirror, false);
      RKADK_PARAM_SetCamParam(stRecAttr.s32CamID, RKADK_PARAM_TYPE_MIRROR,
                              &mirror);
    } else if (strstr(cmd, "mic_volume")) {
      RKADK_U32 volume = 80;
      RKADK_PARAM_SetCommParam(RKADK_PARAM_TYPE_MIC_VOLUME, &volume);
    } else if (strstr(cmd, "volume")) {
      RKADK_U32 volume = 80;
      RKADK_PARAM_SetCommParam(RKADK_PARAM_TYPE_VOLUME, &volume);
    } else if (strstr(cmd, "photo_res")) {
      RKADK_PARAM_RES_E enRes = RKADK_RES_720P;
      RKADK_PARAM_SetCamParam(stRecAttr.s32CamID, RKADK_PARAM_TYPE_PHOTO_RES,
                              &enRes);
    } else if (strstr(cmd, "wdr+")) {
      RKADK_U32 wdr = 9;
      SAMPLE_COMM_ISP_SetDarkAreaBoostStrth(stRecAttr.s32CamID, wdr);
      RKADK_PARAM_SetCamParam(stRecAttr.s32CamID, RKADK_PARAM_TYPE_WDR, &wdr);
    } else if (strstr(cmd, "wdr-")) {
      RKADK_U32 wdr = 0;
      SAMPLE_COMM_ISP_SetDarkAreaBoostStrth(stRecAttr.s32CamID, wdr);
      RKADK_PARAM_SetCamParam(stRecAttr.s32CamID, RKADK_PARAM_TYPE_WDR, &wdr);
    } else if (strstr(cmd, "ldc+")) {
      RKADK_U32 ldc = 200;
      SAMPLE_COMM_ISP_EnableLdch(stRecAttr.s32CamID, true, ldc);
      RKADK_PARAM_SetCamParam(stRecAttr.s32CamID, RKADK_PARAM_TYPE_LDC, &ldc);
    } else if (strstr(cmd, "ldc-")) {
      RKADK_U32 ldc = 0;
      SAMPLE_COMM_ISP_EnableLdch(stRecAttr.s32CamID, false, ldc);
      RKADK_PARAM_SetCamParam(stRecAttr.s32CamID, RKADK_PARAM_TYPE_LDC, &ldc);
    } else if (strstr(cmd, "antifog+")) {
      int antifog = 9;
      SAMPLE_COMM_ISP_EnableDefog(stRecAttr.s32CamID, true, OP_MANUAL, antifog);
      RKADK_PARAM_SetCamParam(stRecAttr.s32CamID, RKADK_PARAM_TYPE_ANTIFOG,
                              &antifog);
    } else if (strstr(cmd, "antifog-")) {
      int antifog = 0;
      SAMPLE_COMM_ISP_EnableDefog(stRecAttr.s32CamID, false, OP_MANUAL,
                                  antifog);
      RKADK_PARAM_SetCamParam(stRecAttr.s32CamID, RKADK_PARAM_TYPE_ANTIFOG,
                              &antifog);
    } else if (strstr(cmd, "get_codec_type")) {
      RKADK_PARAM_CODEC_CFG_S stCodecType;

      stCodecType.enStreamType = RKADK_STREAM_TYPE_VIDEO_MAIN;
      RKADK_PARAM_GetCamParam(stRecAttr.s32CamID, RKADK_PARAM_TYPE_CODEC_TYPE,
                              &stCodecType);
      RKADK_LOGD("main enCodecType = %d", stCodecType.enCodecType);
    } else if (strstr(cmd, "set_codec_type")) {
      RKADK_PARAM_CODEC_CFG_S stCodecType;

      stCodecType.enStreamType = RKADK_STREAM_TYPE_VIDEO_MAIN;
      stCodecType.enCodecType = RKADK_CODEC_TYPE_H265;
      RKADK_PARAM_SetCamParam(stRecAttr.s32CamID, RKADK_PARAM_TYPE_CODEC_TYPE,
                              &stCodecType);
    } else if (strstr(cmd, "switch_codec_type")) {
      RKADK_RECORD_Stop(pRecorder);
      RKADK_RECORD_Destroy(pRecorder);
      pRecorder = NULL;

      RKADK_PARAM_CODEC_CFG_S stCodecCfg;
      stCodecCfg.enStreamType = RKADK_STREAM_TYPE_VIDEO_MAIN;
      stCodecCfg.enCodecType = RKADK_CODEC_TYPE_H265;
      RKADK_PARAM_SetCamParam(stRecAttr.s32CamID, RKADK_PARAM_TYPE_CODEC_TYPE,
                              &stCodecCfg);

#ifdef RKAIQ
      SAMPLE_COMM_ISP_Stop(stRecAttr.s32CamID);
#endif

      goto record;
    } else if (strstr(cmd, "mic_unmute")) {
      bool ummute = true;
      RKADK_PARAM_SetCommParam(RKADK_PARAM_TYPE_MIC_UNMUTE, &ummute);
    } else if (strstr(cmd, "mic_mute")) {
      bool ummute = false;
      RKADK_PARAM_SetCommParam(RKADK_PARAM_TYPE_MIC_UNMUTE, &ummute);
    } else if (strstr(cmd, "stop")) {
      RKADK_RECORD_Stop(pRecorder);
    } else if (strstr(cmd, "start")) {
      RKADK_RECORD_Start(pRecorder);
    } else if (strstr(cmd, "set_main_time")) {
      RKADK_PARAM_REC_TIME_S stRecTime;

      stRecTime.enStreamType = RKADK_STREAM_TYPE_VIDEO_MAIN;
      stRecTime.time = 40;
      RKADK_PARAM_SetCamParam(stRecAttr.s32CamID, RKADK_PARAM_TYPE_RECORD_TIME,
                              &stRecTime);

      memset(&stRecTime, 0, sizeof(RKADK_PARAM_REC_TIME_S));
      RKADK_PARAM_GetCamParam(stRecAttr.s32CamID, RKADK_PARAM_TYPE_RECORD_TIME,
                              &stRecTime);
      RKADK_LOGD("Record Main record time: %d", stRecTime.time);
    } else if (strstr(cmd, "set_sun_time")) {
      RKADK_PARAM_REC_TIME_S stRecTime;

      stRecTime.enStreamType = RKADK_STREAM_TYPE_VIDEO_SUB;
      stRecTime.time = 40;
      RKADK_PARAM_SetCamParam(stRecAttr.s32CamID, RKADK_PARAM_TYPE_SPLITTIME,
                              &stRecTime);

      memset(&stRecTime, 0, sizeof(RKADK_PARAM_REC_TIME_S));
      stRecTime.enStreamType = RKADK_STREAM_TYPE_VIDEO_SUB;
      RKADK_PARAM_GetCamParam(stRecAttr.s32CamID, RKADK_PARAM_TYPE_SPLITTIME,
                              &stRecTime);
      RKADK_LOGD("Record Sub split time: %d", stRecTime.time);
    }

    if (is_quit)
      break;

    usleep(500000);
  }

  RKADK_RECORD_Stop(pRecorder);
  RKADK_RECORD_Destroy(pRecorder);

#ifdef RKAIQ
  SAMPLE_COMM_ISP_Stop(stRecAttr.s32CamID);
#endif

  RKADK_LOGD("exit!");
  return 0;
}
