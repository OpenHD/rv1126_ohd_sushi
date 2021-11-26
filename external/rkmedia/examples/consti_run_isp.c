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
#include <time.h>
#include <unistd.h>
#include <inttypes.h>

#include "common/sample_common.h"
#include "rkmedia_api.h"
#include "rkmedia_venc.h"

#include <sys/time.h>
//socket
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static bool quit = false;
static void sigterm_handler(int sig) {
    fprintf(stderr, "signal %d\n", sig);
    quit = true;
}

static RK_CHAR optstr[] = "?:f:s:g";
static const struct option long_options[] = {
        {"framerate", required_argument, NULL, 'f'},
        {"shutter", required_argument, NULL, 's'},
        {"gain", required_argument, NULL, 'g'},
        {"crop", required_argument, NULL, 'c'},
        {NULL, 0, NULL, 0},
};

static void print_usage(const RK_CHAR *name) {
    printf("usage example:\n");
    printf("\t%s "
           "[-f | --framerate 30] \n"
           "[-s | --shutter] \n"
           "[-g | --gain] \n"
           "[-c | --crop crop to 720p enable/disable] \n",
           name);
}


// start the ISP, I think this only has to run somewhere, not necessarily in the same thread
// maybe use rkisp_demo instead ?!
int main(int argc, char *argv[]) {
    RK_CHAR *iq_file_dir = NULL;
    RK_S32 s32CamId = 0;
    RK_U32 m_framerate=30;
    RK_U32 m_shutter=0;
    RK_U32 m_gain=0;
    iq_file_dir = "/oem/etc/iqfiles";
    int useManualShutterGain=0;
    bool m_TestCrop=false;

    int ret = 0;
    int c;
    while ((c = getopt_long(argc, argv, optstr, long_options, NULL)) != -1) {
        switch (c) {
            case 'f':
                m_framerate = atoi(optarg);
                break;
            case 's':
                m_shutter = atoi(optarg);
                useManualShutterGain=1;
                break;
            case 'g':
                m_gain = atoi(optarg);
                useManualShutterGain=1;
                break;
            case 'c':
                m_TestCrop=true;
                break;
            case '?':
            default:
                print_usage(argv[0]);
                return 0;
        }
    }

    printf("#Aiq xml dirpath: %s\n", iq_file_dir);
    rk_aiq_working_mode_t hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
    RK_BOOL fec_enable = RK_FALSE;
    int fps = m_framerate;
    ret=SAMPLE_COMM_ISP_Init(s32CamId,hdr_mode, fec_enable, iq_file_dir);
    printf("X1:%d\n",ret);
    // get and print the current crop
    rk_aiq_rect_t cropRect;
    ret=SAMPLE_COMM_ISP_GET_Crop(s32CamId,&cropRect);
    printf("Consti10: current crop is %d:%d:%d:%d\n",cropRect.left,cropRect.top,cropRect.width,cropRect.height);
    // if enabled, set a manual crop for testing
    if(m_TestCrop){
        cropRect.left = 0;
        cropRect.top = 0;
        cropRect.width = 1280;
        cropRect.height = 720;
        ret=SAMPLE_COMM_ISP_SET_Crop(s32CamId,cropRect);
        printf("Consti10: applying crop%d\n",ret);
    }
    ret=SAMPLE_COMM_ISP_Run(s32CamId);
    printf("X2:%d\n",ret);

    // test: disable stuff, as much as possible
    SAMPLE_COMM_ISP_Consti10_DisableStuff(s32CamId);

    ret=SAMPLE_COMM_ISP_SetFrameRate(s32CamId,fps);
    printf("X3:%d\n",ret);

    if(useManualShutterGain){
        ret=SAMPLE_COMM_ISP_SET_ManualExposureManualGain(s32CamId,m_shutter,m_gain);
        printf("Manual Exposure and Gain:%d\n",ret);
        //./consti_run_isp -s 19999 -f 90
    }

    printf("Done initializing, ISP should be running now\n");
    signal(SIGINT, sigterm_handler);

    while (!quit) {
        usleep(1*1000000); //x ms
        SAMPLE_COMM_ISP_DumpExpInfo(s32CamId,hdr_mode);
        // Integration time - gain | MeanLuma | stCCT.CCT
        // isp exp dump: M:29999-251.8 LM:2.3 CT:6610.5
    }

    printf("Stopping ISP\n");

    ret=SAMPLE_COMM_ISP_Stop(s32CamId); // isp aiq stop before vi streamoff
    printf("X5:%d",ret);

    printf("ISP stopped,DONE\n");
}
