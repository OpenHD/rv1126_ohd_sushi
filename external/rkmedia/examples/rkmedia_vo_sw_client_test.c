// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "rkmedia_api.h"

int main(int argc, char *argv[]) {
  int ret;
  MPP_CHN_S stSrcChn = {0};
  MPP_CHN_S stDestChn = {0};

  if (argc == 2) {
    int sw = atoi(argv[1]);
    if (sw) {
      printf("#Bind VI[0] to RGA[0]....\n");
      stSrcChn.enModId = RK_ID_VI;
      stSrcChn.s32ChnId = 0;
      stDestChn.enModId = RK_ID_RGA;
      stDestChn.s32ChnId = 0;
      ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
      if (ret) {
        printf("Bind vi[0] to rga[0] failed! ret=%d\n", ret);
        return -1;
      }
    } else {
      printf("#UnBind VI[0] to RGA[0]....\n");
      stSrcChn.enModId = RK_ID_VI;
      stSrcChn.s32ChnId = 0;
      stDestChn.enModId = RK_ID_RGA;
      stDestChn.s32ChnId = 0;
      ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
      if (ret) {
        printf("UnBind vi[0] to rga[0] failed! ret=%d\n", ret);
        return -1;
      }
    }
  }
  return 0;
}
