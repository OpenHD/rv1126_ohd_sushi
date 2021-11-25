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
#include "rkadk_rtsp.h"

extern int optind;
extern char *optarg;

#define IQ_FILE_PATH "/etc/iqfiles"

static bool is_quit = false;
static RKADK_CHAR optstr[] = "a:I";

static void print_usage(const RKADK_CHAR *name) {
  printf("usage example:\n");
  printf("\t%s [-a /etc/iqfiles] [-I 0]\n", name);
  printf("\t-a: enable aiq with dirpath provided, eg:-a "
         "/oem/etc/iqfiles/, Default /oem/etc/iqfiles,"
         "without this option aiq should run in other application\n");
  printf("\t-I: Camera id, Default:0\n");
}

static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  is_quit = true;
}

int main(int argc, char *argv[]) {
  int c, ret, fps;
  RKADK_U32 u32CamId = 0;
  RKADK_CHAR *pIqfilesPath = IQ_FILE_PATH;
  RKADK_MW_PTR pHandle = NULL;

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
    case '?':
    default:
      print_usage(argv[0]);
      optind = 0;
      return 0;
    }
  }
  optind = 0;

  RKADK_LOGD("#camera id: %d", u32CamId);

  RKADK_PARAM_Init();

#ifdef RKAIQ
  ret = RKADK_PARAM_GetCamParam(u32CamId, RKADK_PARAM_TYPE_FPS, &fps);
  if (ret) {
    RKADK_LOGE("RKADK_PARAM_GetCamParam fps failed");
    return -1;
  }

  rk_aiq_working_mode_t hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
  RKADK_BOOL fec_enable = RKADK_FALSE;
  SAMPLE_COMM_ISP_Start(u32CamId, hdr_mode, fec_enable, pIqfilesPath, fps);
#endif

  ret = RKADK_RTSP_Init(u32CamId, 554, "/live/main_stream", &pHandle);
  if (ret) {
    RKADK_LOGE("RKADK_RTSP_Init failed(%d)", ret);
#ifdef RKAIQ
    SAMPLE_COMM_ISP_Stop(u32CamId);
#endif
    return -1;
  }

  RKADK_RTSP_Start(pHandle);

  signal(SIGINT, sigterm_handler);

  char cmd[64];
  printf("\n#Usage: input 'quit' to exit programe!\n"
         "peress any other key to capture one picture to file\n");
  while (!is_quit) {
    fgets(cmd, sizeof(cmd), stdin);
    if (strstr(cmd, "quit") || is_quit) {
      RKADK_LOGD("#Get 'quit' cmd!");
      break;
    } else if (strstr(cmd, "start")) {
      RKADK_RTSP_Start(pHandle);
    } else if (strstr(cmd, "stop")) {
      RKADK_RTSP_Stop(pHandle);
    }

    usleep(500000);
  }

  RKADK_RTSP_DeInit(pHandle);
  pHandle = NULL;

#ifdef RKAIQ
  SAMPLE_COMM_ISP_Stop(u32CamId);
#endif
  return 0;
}
