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

#include "rkadk_common.h"
#include "rkadk_log.h"
#include "rkadk_photo.h"
#include "rkadk_thumb.h"

extern int optind;
extern char *optarg;

static bool is_quit = false;
static RKADK_CHAR optstr[] = "i:t:f:h";

static void print_usage(const RKADK_CHAR *name) {
  printf("usage example:\n");
  printf("\t%s [-i /tmp/xxx.mp4] [-t 0]\n", name);
  printf("\t-i: test file\n");
  printf("\t-t: thumbnail type, default DCF, options: DCF, MPF1, MPF2\n");
  printf("\t-f: file type, default mp4, options: mp4, jpg\n");
}

static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  is_quit = true;
}

#define THUMB_TEST_SAVE_FILE
int main(int argc, char *argv[]) {
  int c;
  unsigned int count = 1;
  int buf_size = 30 * 1024;
  RKADK_CHAR *pInuptPath = "/userdata/RecordTest_0.mp4";
  RKADK_U32 size = buf_size; // 320 * 180 jpg
  RKADK_U8 buffer[size];
  char filePath[RKADK_MAX_FILE_PATH_LEN];
  RKADK_JPG_THUMB_TYPE_E eJpgThumbType = RKADK_JPG_THUMB_TYPE_DCF;
  bool bIsMp4 = true;

  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 'i':
      pInuptPath = optarg;
      break;
    case 'f':
      if (strstr(optarg, "jpg"))
        bIsMp4 = false;
      break;
    case 't':
      if (strstr(optarg, "MPF1"))
        eJpgThumbType = RKADK_JPG_THUMB_TYPE_MFP1;
      else if (strstr(optarg, "MPF2"))
        eJpgThumbType = RKADK_JPG_THUMB_TYPE_MFP2;
      break;
    case 'h':
    default:
      print_usage(argv[0]);
      optind = 0;
      return 0;
    }
  }
  optind = 0;

  if (!pInuptPath) {
    RKADK_LOGE("Please input test file");
    return -1;
  }

  RKADK_LOGD("#get thm file: %s", pInuptPath);

  signal(SIGINT, sigterm_handler);

  while (!is_quit) {
    size = buf_size;

    if (bIsMp4) {
      if (RKADK_GetThmInMp4(pInuptPath, buffer, &size)) {
        RKADK_LOGE("RKADK_GetThmInMp4 failed");
        return -1;
      }
    } else {
      RKADK_LOGD("eJpgThumbType: %d", eJpgThumbType);
      if (RKADK_PHOTO_GetThmInJpg(pInuptPath, eJpgThumbType, buffer, &size)) {
        RKADK_LOGE("RKADK_PHOTO_GetThmInJpg failed");
        return -1;
      }
    }
    RKADK_LOGD("jpg size: %d, count: %d", size, count);

#ifdef THUMB_TEST_SAVE_FILE
    if (size > 0) {
      FILE *file = NULL;
      sprintf(filePath, "/tmp/thm_test_%u.jpg", count);
      file = fopen(filePath, "w");
      if (!file) {
        RKADK_LOGE("Create file(%s) failed", filePath);
        return -1;
      }

      fwrite(buffer, 1, size, file);
      fclose(file);
    }
#endif

    count--;
    if (count <= 0)
      break;

    usleep(2 * 1000 * 1000);
  }

  return 0;
}
