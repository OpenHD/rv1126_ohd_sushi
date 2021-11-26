/*
 * Copyright 2015 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if defined(_WIN32)
#include "vld.h"
#endif

#define MODULE_TAG "mpi_get_frame_test"

#include <string.h>
#include "rk_mpi.h"

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_time.h"
#include "mpp_common.h"

#include "utils.h"
#include "mpi_enc_utils.h"
#include "camera_source.h"

static uint64_t getTimeMs(){
    struct timeval time;
    gettimeofday(&time, NULL);
    uint64_t millis = (time.tv_sec * ((uint64_t)1000)) + ((uint64_t)time.tv_usec / ((uint64_t)1000));
    return millis;
}

typedef struct {
    CamSource *cam_ctx;
    RK_U32 width;
    RK_U32 height;
    MppFrameFormat fmt;
} MpiGetFrameTestData;

// use ?V4l2? to pull camera frames

int main(int argc, char **argv)
{
//Consti10
    char* file_input="/dev/video0";
    int quit=0;
    MpiGetFrameTestData *p = NULL;
    p = mpp_calloc(MpiGetFrameTestData, 1);
    if (!p) {
        mpp_err_f("create MpiGetFrameTestData failed\n");
        return 0;
    }
    p->width=1280;
    p->height=720;
    p->fmt=0;
    mpp_log("open camera device");
    p->cam_ctx = camera_source_init(file_input, 4, p->width, p->height, p->fmt);
    mpp_log("new framecap ok");
    if (p->cam_ctx == NULL)
        mpp_err("open %s fail", file_input);

    RK_S32 cam_frm_idx = -1;
    RK_S32 cap_num=0;
    MppBuffer cam_buf = NULL;
    //
    uint64_t lastTs=getTimeMs();
    uint64_t frameDeltaAvgSum=0;
    uint64_t frameDeltaAvgCount=0;

    while(!quit){
        cam_frm_idx = camera_source_get_frame(p->cam_ctx);
        mpp_assert(cam_frm_idx >= 0);
        cap_num++;
        if(cap_num>1000){
            quit=1;
        }
        printf("Got camera frame %d cam_frm_idx: %d\n",cap_num,cam_frm_idx);

        cam_buf = camera_frame_to_buf(p->cam_ctx, cam_frm_idx);
        mpp_assert(cam_buf);

        // put it back in
        camera_source_put_frame(p->cam_ctx, cam_frm_idx);
        //
        uint64_t now=getTimeMs();
        uint64_t delta=now-lastTs;
        printf("Frame delta: %lld\n",delta);
        lastTs=getTimeMs();
        // calculate average
        frameDeltaAvgSum+=delta;
        frameDeltaAvgCount++;
        if(frameDeltaAvgCount>100){
            float avgFrameDelta=(float)((double)frameDeltaAvgSum / (double)frameDeltaAvgCount);
            printf("Avg of frame delta: %f\n",avgFrameDelta);
            frameDeltaAvgSum=0;
            frameDeltaAvgCount=0;
        }
        //
    }


    camera_source_deinit(p->cam_ctx);
    p->cam_ctx = NULL;

    MPP_FREE(p);
    return 0;
}

