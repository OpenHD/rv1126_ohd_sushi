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

#define MODULE_TAG "mpi_stream"

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

#include "../../rkmedia/examples/common_consti/TimeHelper.hpp"
#include "../../rkmedia/examples/common_consti/UDPSender.h"
#include "../../rkmedia/examples/common_consti/Helper.hpp"
#include <queue>
#include <mutex>
#include <thread>

static const int M_DESTINATION_PORT=5600;
static const int CHUNK_SIZE=1446;
static const int MY_WANTED_UDP_SENDBUFF_SIZE=1024*1024*10;
static std::unique_ptr<UDPSender> udpSender=nullptr;
AvgCalculator avgEncodeDelay;
AvgCalculator avgCameraFrameDelay;
AvgCalculator avgFrameDelta;
AvgCalculator avgConsumerThreadDelay;

static void sendViaUDPIfEnabled(const void* data,int data_length){
    if(udpSender!= nullptr){
        udpSender->splitAndSend((uint8_t*)data,data_length,CHUNK_SIZE);
    }
    //std::cout<<"Sent\n";
}

typedef struct {
    // global flow control flag
    RK_U32 frm_eos;
    RK_U32 pkt_eos;
    RK_U32 frm_pkt_cnt;
    RK_S32 frame_count;
    RK_U64 stream_size;

    // src and dst
    FILE *fp_input;
    FILE *fp_output=NULL;

    // base flow context
    MppCtx ctx;
    MppApi *mpi;
    MppEncCfg cfg;
    MppEncPrepCfg prep_cfg;
    MppEncRcCfg rc_cfg;
    MppEncCodecCfg codec_cfg;
    MppEncSliceSplit split_cfg;
    MppEncOSDPltCfg osd_plt_cfg;
    MppEncOSDPlt    osd_plt;
    MppEncOSDData   osd_data;
    MppEncROIRegion roi_region[3];
    MppEncROICfg    roi_cfg;

    // input / output
    MppBufferGroup buf_grp;
    MppBuffer frm_buf;
    MppBuffer pkt_buf;
    MppEncSeiMode sei_mode;
    MppEncHeaderMode header_mode;

    // paramter for resource malloc
    RK_U32 width;
    RK_U32 height;
    RK_U32 hor_stride;
    RK_U32 ver_stride;
    MppFrameFormat fmt;
    MppCodingType type;
    RK_S32 num_frames;
    RK_S32 loop_times;
    CamSource *cam_ctx;

    // resources
    size_t header_size;
    size_t frame_size;
    /* NOTE: packet buffer may overflow */
    size_t packet_size;

    RK_U32 split_mode;
    RK_U32 split_arg;

    // rate control runtime parameter

    RK_S32 fps_in_flex;
    RK_S32 fps_in_den;
    RK_S32 fps_in_num;
    RK_S32 fps_out_flex;
    RK_S32 fps_out_den;
    RK_S32 fps_out_num;
    RK_S32 bps;
    RK_S32 bps_max;
    RK_S32 bps_min;
    RK_S32 rc_mode;
    RK_S32 gop_mode;
    RK_S32 gop_len;
    RK_S32 vi_len;
} MpiEncTestData;


struct BufferWithTimestamp{
    std::vector<uint8_t> data;
    std::chrono::steady_clock::time_point timeStamp;
    // make it non copyable
    //BufferWithTimestamp(const BufferWithTimestamp&) = delete;
    //BufferWithTimestamp& operator = (const BufferWithTimestamp&) = delete;
};
static bool consumerThreadRun=true;
std::queue<std::shared_ptr<BufferWithTimestamp>> consumerThreadQueue;
std::mutex consumerThreadMutex;

// Consti10
static void processEncodedPacket(MpiEncTestData *p, MppPacket packet){
    // write packet to file here
    const auto timestampBegin=std::chrono::steady_clock::now();
    void *ptr   = mpp_packet_get_pos(packet);
    const size_t len  = mpp_packet_get_length(packet);
    /*if (p->fp_output){
        fwrite(ptr, 1, len, p->fp_output);
    }
    sendViaUDPIfEnabled(ptr,len);
    sendViaUDPIfEnabled(EXAMPLE_AUD,sizeof(EXAMPLE_AUD));*/
    std::shared_ptr<BufferWithTimestamp> buff=std::make_shared<BufferWithTimestamp>();
    buff->data.resize(len);
    buff->timeStamp=timestampBegin;
    std::memcpy(buff->data.data(),ptr,len);
    consumerThreadMutex.lock();
    consumerThreadQueue.push(std::move(buff));
    consumerThreadMutex.unlock();
}

static void consumerThreadLoop(MpiEncTestData *p){
    std::cout<<"Consumer thread begin\n";
    while (consumerThreadRun){
        std::shared_ptr<BufferWithTimestamp> buf;
        consumerThreadMutex.lock();
        if(!consumerThreadQueue.empty()){
            buf=consumerThreadQueue.front();
            consumerThreadQueue.pop();
        }
        consumerThreadMutex.unlock();
        if(buf){
            const auto delay=std::chrono::steady_clock::now()-buf->timeStamp;
            std::cout<<"Consumer thread buffer delay:"<<MyTimeHelper::R(delay)<<"\n";
            avgConsumerThreadDelay.add(delay);
            if(avgConsumerThreadDelay.getNSamples()>100){
                std::cout<<"Avg Consumer Thread delay:"<<avgConsumerThreadDelay.getAvgReadable()<<"\n";
                avgConsumerThreadDelay.reset();
            }
            sendViaUDPIfEnabled(buf->data.data(),buf->data.size());
            sendViaUDPIfEnabled(EXAMPLE_AUD,sizeof(EXAMPLE_AUD));
        }
    }
    std::cout<<"Consumer thread end\n";
}


MPP_RET test_ctx_init(MpiEncTestData **data, MpiEncTestArgs *cmd)
{
    MpiEncTestData *p = NULL;
    MPP_RET ret = MPP_OK;

    if (!data || !cmd) {
        mpp_err_f("invalid input data %p cmd %p\n", data, cmd);
        return MPP_ERR_NULL_PTR;
    }

    p = mpp_calloc(MpiEncTestData, 1);
    if (!p) {
        mpp_err_f("create MpiEncTestData failed\n");
        ret = MPP_ERR_MALLOC;
        *data = p;
        return ret;
    }

    // get paramter from cmd
    p->width        = cmd->width;
    p->height       = cmd->height;
    p->hor_stride   = (cmd->hor_stride) ? (cmd->hor_stride) :
                      (MPP_ALIGN(cmd->width, 16));
    p->ver_stride   = (cmd->ver_stride) ? (cmd->ver_stride) :
                      (MPP_ALIGN(cmd->height, 16));
    p->fmt          = cmd->format;
    p->type         = cmd->type;
    p->bps          = cmd->bps_target;
    p->bps_min      = cmd->bps_min;
    p->bps_max      = cmd->bps_max;
    p->rc_mode      = cmd->rc_mode;
    p->num_frames   = cmd->num_frames;
    if (cmd->type == MPP_VIDEO_CodingMJPEG && p->num_frames == 0) {
        mpp_log("jpege default encode only one frame. Use -n [num] for rc case\n");
        p->num_frames   = 1;
    }
    p->gop_mode     = cmd->gop_mode;
    p->gop_len      = cmd->gop_len;
    p->vi_len       = cmd->vi_len;

    p->fps_in_flex  = cmd->fps_in_flex;
    p->fps_in_den   = cmd->fps_in_den;
    p->fps_in_num   = cmd->fps_in_num;
    p->fps_out_flex = cmd->fps_out_flex;
    p->fps_out_den  = cmd->fps_out_den;
    p->fps_out_num  = cmd->fps_out_num;

    printf("gop mode:%d gop_len:%d vi_len:%d\n",p->gop_mode,p->gop_len,p->vi_len);

    if (cmd->file_input) {
        if (!strncmp(cmd->file_input, "/dev/video", 10)) {
            mpp_log("open camera device");
            p->cam_ctx = camera_source_init(cmd->file_input, 4, p->width, p->height, p->fmt);
            mpp_log("new framecap ok");
            if (p->cam_ctx == NULL)
                mpp_err("open %s fail", cmd->file_input);
        } else {
            p->fp_input = fopen(cmd->file_input, "rb");
            if (NULL == p->fp_input) {
                mpp_err("failed to open input file %s\n", cmd->file_input);
                mpp_err("create default yuv image for test\n");
            }
        }
    }

    if (cmd->file_output) {
        if(strstr(cmd->file_output,"null")==NULL){
            p->fp_output = fopen(cmd->file_output, "w+b");
            if (NULL == p->fp_output) {
                mpp_err("failed to open output file %s\n", cmd->file_output);
                ret = MPP_ERR_OPEN_FILE;
            }
        }else{
            std::cout<<"File output disabled\n";
        }
    }

    // update resource parameter
    switch (p->fmt & MPP_FRAME_FMT_MASK) {
    case MPP_FMT_YUV420SP:
    case MPP_FMT_YUV420P: {
        p->frame_size = MPP_ALIGN(p->hor_stride, 64) * MPP_ALIGN(p->ver_stride, 64) * 3 / 2;
    } break;

    case MPP_FMT_YUV422_YUYV :
    case MPP_FMT_YUV422_YVYU :
    case MPP_FMT_YUV422_UYVY :
    case MPP_FMT_YUV422_VYUY :
    case MPP_FMT_YUV422P :
    case MPP_FMT_YUV422SP :
    case MPP_FMT_RGB444 :
    case MPP_FMT_BGR444 :
    case MPP_FMT_RGB555 :
    case MPP_FMT_BGR555 :
    case MPP_FMT_RGB565 :
    case MPP_FMT_BGR565 : {
        p->frame_size = MPP_ALIGN(p->hor_stride, 64) * MPP_ALIGN(p->ver_stride, 64) * 2;
    } break;

    default: {
        p->frame_size = MPP_ALIGN(p->hor_stride, 64) * MPP_ALIGN(p->ver_stride, 64) * 4;
    } break;
    }

    if (MPP_FRAME_FMT_IS_FBC(p->fmt))
        p->header_size = MPP_ALIGN(MPP_ALIGN(p->width, 16) * MPP_ALIGN(p->height, 16) / 16, SZ_4K);
    else
        p->header_size = 0;

    *data = p;
    return ret;
}

MPP_RET test_ctx_deinit(MpiEncTestData **data)
{
    MpiEncTestData *p = NULL;

    if (!data) {
        mpp_err_f("invalid input data %p\n", data);
        return MPP_ERR_NULL_PTR;
    }
    p = *data;
    if (p) {
        if (p->cam_ctx) {
            camera_source_deinit(p->cam_ctx);
            p->cam_ctx = NULL;
        }
        if (p->fp_input) {
            fclose(p->fp_input);
            p->fp_input = NULL;
        }
        if (p->fp_output) {
            fclose(p->fp_output);
            p->fp_output = NULL;
        }
        MPP_FREE(p);
        *data = NULL;
    }
    return MPP_OK;
}

MPP_RET test_mpp_enc_cfg_setup(MpiEncTestData *p)
{
    MPP_RET ret;
    MppApi *mpi;
    MppCtx ctx;
    MppEncCfg cfg;
    RK_U32 gop_mode = p->gop_mode;

    if (NULL == p)
        return MPP_ERR_NULL_PTR;

    mpi = p->mpi;
    ctx = p->ctx;
    cfg = p->cfg;

    /* setup default parameter */
    if (p->fps_in_den == 0)
        p->fps_in_den = 1;
    if (p->fps_in_num == 0)
        p->fps_in_num = 30;
    if (p->fps_out_den == 0)
        p->fps_out_den = 1;
    if (p->fps_out_num == 0)
        p->fps_out_num = 30;

    if (!p->bps)
        p->bps = p->width * p->height / 8 * (p->fps_out_num / p->fps_out_den);

    mpp_enc_cfg_set_s32(cfg, "prep:width", p->width);
    mpp_enc_cfg_set_s32(cfg, "prep:height", p->height);
    mpp_enc_cfg_set_s32(cfg, "prep:hor_stride", p->hor_stride);
    mpp_enc_cfg_set_s32(cfg, "prep:ver_stride", p->ver_stride);
    mpp_enc_cfg_set_s32(cfg, "prep:format", p->fmt);

    mpp_enc_cfg_set_s32(cfg, "rc:mode", p->rc_mode);

    /* fix input / output frame rate */
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_flex", p->fps_in_flex);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_num", p->fps_in_num);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_denorm", p->fps_in_den);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_flex", p->fps_out_flex);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_num", p->fps_out_num);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_denorm", p->fps_out_den);
    mpp_enc_cfg_set_s32(cfg, "rc:gop", p->gop_len ? p->gop_len : p->fps_out_num * 2);

    /* drop frame or not when bitrate overflow */
    mpp_enc_cfg_set_u32(cfg, "rc:drop_mode", MPP_ENC_RC_DROP_FRM_DISABLED);
    mpp_enc_cfg_set_u32(cfg, "rc:drop_thd", 20);        /* 20% of max bps */
    mpp_enc_cfg_set_u32(cfg, "rc:drop_gap", 1);         /* Do not continuous drop frame */

    /* setup bitrate for different rc_mode */
    mpp_enc_cfg_set_s32(cfg, "rc:bps_target", p->bps);
    switch (p->rc_mode) {
    case MPP_ENC_RC_MODE_FIXQP : {
        /* do not setup bitrate on FIXQP mode */
    } break;
    case MPP_ENC_RC_MODE_CBR : {
        /* CBR mode has narrow bound */
        mpp_enc_cfg_set_s32(cfg, "rc:bps_max", p->bps_max ? p->bps_max : p->bps * 17 / 16);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_min", p->bps_min ? p->bps_min : p->bps * 15 / 16);
    } break;
    case MPP_ENC_RC_MODE_VBR :
    case MPP_ENC_RC_MODE_AVBR : {
        /* VBR mode has wide bound */
        mpp_enc_cfg_set_s32(cfg, "rc:bps_max", p->bps_max ? p->bps_max : p->bps * 17 / 16);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_min", p->bps_min ? p->bps_min : p->bps * 1 / 16);
    } break;
    default : {
        /* default use CBR mode */
        mpp_enc_cfg_set_s32(cfg, "rc:bps_max", p->bps_max ? p->bps_max : p->bps * 17 / 16);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_min", p->bps_min ? p->bps_min : p->bps * 15 / 16);
    } break;
    }

    /* setup qp for different codec and rc_mode */
    switch (p->type) {
    case MPP_VIDEO_CodingAVC :
    case MPP_VIDEO_CodingHEVC : {
        switch (p->rc_mode) {
        case MPP_ENC_RC_MODE_FIXQP : {
            mpp_enc_cfg_set_s32(cfg, "rc:qp_init", 20);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max", 20);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min", 20);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", 20);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", 20);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 2);
        } break;
        case MPP_ENC_RC_MODE_CBR :
        case MPP_ENC_RC_MODE_VBR :
        case MPP_ENC_RC_MODE_AVBR : {
            mpp_enc_cfg_set_s32(cfg, "rc:qp_init", 26);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max", 51);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min", 10);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", 51);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", 10);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 2);
        } break;
        default : {
            mpp_err_f("unsupport encoder rc mode %d\n", p->rc_mode);
        } break;
        }
    } break;
    case MPP_VIDEO_CodingVP8 : {
        /* vp8 only setup base qp range */
        mpp_enc_cfg_set_s32(cfg, "rc:qp_init", 40);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_max",  127);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_min",  0);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", 127);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", 0);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 6);
    } break;
    case MPP_VIDEO_CodingMJPEG : {
        /* jpeg use special codec config to control qtable */
        mpp_enc_cfg_set_s32(cfg, "jpeg:q_factor", 80);
        mpp_enc_cfg_set_s32(cfg, "jpeg:qf_max", 99);
        mpp_enc_cfg_set_s32(cfg, "jpeg:qf_min", 1);
    } break;
    default : {
    } break;
    }

    /* setup codec  */
    mpp_enc_cfg_set_s32(cfg, "codec:type", p->type);
    switch (p->type) {
    case MPP_VIDEO_CodingAVC : {
        /*
         * H.264 profile_idc parameter
         * 66  - Baseline profile
         * 77  - Main profile
         * 100 - High profile
         */
        mpp_enc_cfg_set_s32(cfg, "h264:profile", 100);
        /*
         * H.264 level_idc parameter
         * 10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
         * 20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
         * 30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
         * 40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
         * 50 / 51 / 52         - 4K@30fps
         */
        mpp_enc_cfg_set_s32(cfg, "h264:level", 40);
        mpp_enc_cfg_set_s32(cfg, "h264:cabac_en", 1);
        mpp_enc_cfg_set_s32(cfg, "h264:cabac_idc", 0);
        mpp_enc_cfg_set_s32(cfg, "h264:trans8x8", 1);
    } break;
    case MPP_VIDEO_CodingHEVC :
    case MPP_VIDEO_CodingMJPEG :
    case MPP_VIDEO_CodingVP8 : {
    } break;
    default : {
        mpp_err_f("unsupport encoder coding type %d\n", p->type);
    } break;
    }

    p->split_mode = 0;
    p->split_arg = 0;

    mpp_env_get_u32("split_mode", &p->split_mode, MPP_ENC_SPLIT_NONE);
    mpp_env_get_u32("split_arg", &p->split_arg, 0);

    if (p->split_mode) {
        mpp_log("%p split_mode %d split_arg %d\n", ctx, p->split_mode, p->split_arg);
        mpp_enc_cfg_set_s32(cfg, "split:mode", p->split_mode);
        mpp_enc_cfg_set_s32(cfg, "split:arg", p->split_arg);
    }

    ret = mpi->control(ctx, MPP_ENC_SET_CFG, cfg);
    if (ret) {
        mpp_err("mpi control enc set cfg failed ret %d\n", ret);
        return ret;
    }

    /* optional */
    p->sei_mode = MPP_ENC_SEI_MODE_ONE_FRAME;
    ret = mpi->control(ctx, MPP_ENC_SET_SEI_CFG, &p->sei_mode);
    if (ret) {
        mpp_err("mpi control enc set sei cfg failed ret %d\n", ret);
        return ret;
    }

    if (p->type == MPP_VIDEO_CodingAVC || p->type == MPP_VIDEO_CodingHEVC) {
        p->header_mode = MPP_ENC_HEADER_MODE_EACH_IDR;
        ret = mpi->control(ctx, MPP_ENC_SET_HEADER_MODE, &p->header_mode);
        if (ret) {
            mpp_err("mpi control enc set header mode failed ret %d\n", ret);
            return ret;
        }
    }
    //Consti10 use -g input only
    //mpp_env_get_u32("gop_mode", &gop_mode, gop_mode);
    if (gop_mode) {
        MppEncRefCfg ref;

        mpp_enc_ref_cfg_init(&ref);

        if (p->gop_mode < 4)
            mpi_enc_gen_ref_cfg(ref, gop_mode);
        else
            mpi_enc_gen_smart_gop_ref_cfg(ref, p->gop_len, p->vi_len);

        ret = mpi->control(ctx, MPP_ENC_SET_REF_CFG, ref);
        if (ret) {
            mpp_err("mpi control enc set ref cfg failed ret %d\n", ret);
            return ret;
        }
        mpp_enc_ref_cfg_deinit(&ref);
    }
    return ret;
}

MPP_RET test_mpp_run(MpiEncTestData *p)
{
    MPP_RET ret = MPP_OK;
    MppApi *mpi;
    MppCtx ctx;
    RK_U32 cap_num = 0;
    std::chrono::steady_clock::time_point lastTs=std::chrono::steady_clock::now();

    if (NULL == p)
        return MPP_ERR_NULL_PTR;

    mpi = p->mpi;
    ctx = p->ctx;

    consumerThreadRun=true;
    std::thread consumerThread(consumerThreadLoop,p);

    if (p->type == MPP_VIDEO_CodingAVC || p->type == MPP_VIDEO_CodingHEVC) {
        MppPacket packet = NULL;

        /*
         * Can use packet with normal malloc buffer as input not pkt_buf.
         * Please refer to vpu_api_legacy.cpp for normal buffer case.
         * Using pkt_buf buffer here is just for simplifing demo.
         */
        mpp_packet_init_with_buffer(&packet, p->pkt_buf);
        /* NOTE: It is important to clear output packet length!! */
        mpp_packet_set_length(packet, 0);

        ret = mpi->control(ctx, MPP_ENC_GET_HDR_SYNC, packet);
        if (ret) {
            mpp_err("mpi control enc get extra info failed\n");
            return ret;
        } else {
            /* get and write sps/pps for H.264 */

            /*void *ptr   = mpp_packet_get_pos(packet);
            size_t len  = mpp_packet_get_length(packet);

            if (p->fp_output)
                fwrite(ptr, 1, len, p->fp_output);
            sendViaUDPIfEnabled(ptr,len);*/
            processEncodedPacket(p,packet);
        }

        mpp_packet_deinit(&packet);
    }

    lastTs=std::chrono::steady_clock::now();

    while (!p->pkt_eos) {
        MppMeta meta = NULL;
        MppFrame frame = NULL;
        MppPacket packet = NULL;
        void *buf = mpp_buffer_get_ptr(p->frm_buf);
        RK_S32 cam_frm_idx = -1;
        MppBuffer cam_buf = NULL;
        RK_U32 eoi = 1;

        const auto now=std::chrono::steady_clock::now();
        const auto delta=now-lastTs;
        std::cout<<"Frame delta: "<<MyTimeHelper::R(delta)<<"\n";
        lastTs=now;
        avgFrameDelta.add(delta);
        if(avgFrameDelta.getNSamples()>100){
            std::cout<<"Avg frame delta:"<<avgFrameDelta.getAvgReadable()<<"\n";
            avgFrameDelta.reset();
        }
        if (p->fp_input) {
            ret = read_image((RK_U8*)buf, p->fp_input, p->width, p->height,
                             p->hor_stride, p->ver_stride, p->fmt);
            if (ret == MPP_NOK || feof(p->fp_input)) {
                p->frm_eos = 1;

                if (p->num_frames < 0 || p->frame_count < p->num_frames) {
                    clearerr(p->fp_input);
                    rewind(p->fp_input);
                    p->frm_eos = 0;
                    mpp_log("%p loop times %d\n", ctx, ++p->loop_times);
                    continue;
                }
                mpp_log("%p found last frame. feof %d\n", ctx, feof(p->fp_input));
            } else if (ret == MPP_ERR_VALUE)
                return ret;
        } else {
            if (p->cam_ctx == NULL) {
                uint64_t before=getTimeMs();
                //Consti10
                ret = fill_image_consti((RK_U8*)buf, p->width, p->height, p->hor_stride,
                                 p->ver_stride, p->fmt, p->frame_count);
                uint64_t deltaX=getTimeMs()-before;
                printf("Filling frame took: %lld ms\n",deltaX);
                if (ret)
                    return ret;
            } else {
                const auto before=std::chrono::steady_clock::now();
                cam_frm_idx = camera_source_get_frame(p->cam_ctx);
                mpp_assert(cam_frm_idx >= 0);
                // xxx

                /* skip unstable frames */
                if (cap_num++ < 50) {
                    camera_source_put_frame(p->cam_ctx, cam_frm_idx);
                    continue;
                }

                cam_buf = camera_frame_to_buf(p->cam_ctx, cam_frm_idx);
                mpp_assert(cam_buf);
                const auto delta=std::chrono::steady_clock::now()-before;
                const auto cameraFrameTsUs=consti_get_timestamp(p->cam_ctx,cam_frm_idx);
                const auto cameraFrameDelayUs=getTimeUs()- cameraFrameTsUs;
                std::cout<<"Camera get frame took:"<<MyTimeHelper::R(delta)<<" age:"<<cameraFrameDelayUs/1000.0f<<"ms\n";
                avgCameraFrameDelay.addUs(cameraFrameDelayUs);
                if(avgCameraFrameDelay.getNSamples()>=120){
                    std::cout<<"Avg Camera frame delay:"<<avgCameraFrameDelay.getAvgReadable()<<"\n";
                    avgCameraFrameDelay.reset();
                }
            }
        }
        const auto encodeBegin=std::chrono::steady_clock::now();
        ret = mpp_frame_init(&frame);
        if (ret) {
            mpp_err_f("mpp_frame_init failed\n");
            return ret;
        }

        mpp_frame_set_width(frame, p->width);
        mpp_frame_set_height(frame, p->height);
        mpp_frame_set_hor_stride(frame, p->hor_stride);
        mpp_frame_set_ver_stride(frame, p->ver_stride);
        mpp_frame_set_fmt(frame, p->fmt);
        mpp_frame_set_eos(frame, p->frm_eos);

        if (p->fp_input && feof(p->fp_input))
            mpp_frame_set_buffer(frame, NULL);
        else if (cam_buf)
            mpp_frame_set_buffer(frame, cam_buf);
        else
            mpp_frame_set_buffer(frame, p->frm_buf);

        meta = mpp_frame_get_meta(frame);
        mpp_packet_init_with_buffer(&packet, p->pkt_buf);
        /* NOTE: It is important to clear output packet length!! */
        mpp_packet_set_length(packet, 0);
        mpp_meta_set_packet(meta, KEY_OUTPUT_PACKET, packet);

        /*
         * NOTE: in non-block mode the frame can be resent.
         * The default input timeout mode is block.
         *
         * User should release the input frame to meet the requirements of
         * resource creator must be the resource destroyer.
         */
        ret = mpi->encode_put_frame(ctx, frame);
        if (ret) {
            mpp_err("mpp encode put frame failed\n");
            mpp_frame_deinit(&frame);
            return ret;
        }
        mpp_frame_deinit(&frame);

        do {
            ret = mpi->encode_get_packet(ctx, &packet);
            if (ret) {
                mpp_err("mpp encode get packet failed\n");
                return ret;
            }

            mpp_assert(packet);

            if (packet) {
                // write packet to file here
                void *ptr   = mpp_packet_get_pos(packet);
                size_t len  = mpp_packet_get_length(packet);
                char log_buf[256];
                RK_S32 log_size = sizeof(log_buf) - 1;
                RK_S32 log_len = 0;

                p->pkt_eos = mpp_packet_get_eos(packet);

                const auto encodeDelay=std::chrono::steady_clock::now()-encodeBegin;
                std::cout<<"Encode took:"<<MyTimeHelper::R(encodeDelay)<<"\n";
                avgEncodeDelay.add(encodeDelay);
                if(avgEncodeDelay.getNSamples()>=120){
                    std::cout<<"Avg encode delay:"<<avgEncodeDelay.getAvgReadable()<<"\n";
                    avgEncodeDelay.reset();
                }

                /*if (p->fp_output)
                    fwrite(ptr, 1, len, p->fp_output);
                sendViaUDPIfEnabled(ptr,len);*/
                processEncodedPacket(p,packet);

                log_len += snprintf(log_buf + log_len, log_size - log_len,
                                    "encoded frame %-4d", p->frame_count);

                /* for low delay partition encoding */
                if (mpp_packet_is_partition(packet)) {
                    eoi = mpp_packet_is_eoi(packet);

                    log_len += snprintf(log_buf + log_len, log_size - log_len,
                                        " pkt %d", p->frm_pkt_cnt);
                    p->frm_pkt_cnt = (eoi) ? (0) : (p->frm_pkt_cnt + 1);
                }

                log_len += snprintf(log_buf + log_len, log_size - log_len,
                                    " size %-7zu", len);

                if (mpp_packet_has_meta(packet)) {
                    meta = mpp_packet_get_meta(packet);
                    RK_S32 temporal_id = 0;
                    RK_S32 lt_idx = -1;
                    RK_S32 avg_qp = -1;

                    if (MPP_OK == mpp_meta_get_s32(meta, KEY_TEMPORAL_ID, &temporal_id))
                        log_len += snprintf(log_buf + log_len, log_size - log_len,
                                            " tid %d", temporal_id);

                    if (MPP_OK == mpp_meta_get_s32(meta, KEY_LONG_REF_IDX, &lt_idx))
                        log_len += snprintf(log_buf + log_len, log_size - log_len,
                                            " lt %d", lt_idx);

                    if (MPP_OK == mpp_meta_get_s32(meta, KEY_ENC_AVERAGE_QP, &avg_qp))
                        log_len += snprintf(log_buf + log_len, log_size - log_len,
                                            " qp %d", avg_qp);
                }

                //mpp_log("%p %s\n", ctx, log_buf);
                printf("%s\n",log_buf);

                mpp_packet_deinit(&packet);

                p->stream_size += len;
                p->frame_count += eoi;

                if (p->pkt_eos) {
                    mpp_log("%p found last packet\n", ctx);
                    mpp_assert(p->frm_eos);
                }
            }
        } while (!eoi);

        if (cam_frm_idx >= 0)
            camera_source_put_frame(p->cam_ctx, cam_frm_idx);

        if (p->num_frames > 0 && p->frame_count >= p->num_frames) {
            mpp_log("%p encode max %d frames", ctx, p->frame_count);
            break;
        }

        if (p->frm_eos && p->pkt_eos)
            break;
    }
    consumerThreadRun=false;
    if(consumerThread.joinable()){
        consumerThread.join();
    }
    return ret;
}

int mpi_enc_test(MpiEncTestArgs *cmd)
{
    MPP_RET ret = MPP_OK;
    MpiEncTestData *p = NULL;
    MppPollType timeout = MPP_POLL_BLOCK;

    mpp_log("mpi_enc_test start\n");

    ret = test_ctx_init(&p, cmd);
    if (ret) {
        mpp_err_f("test data init failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = mpp_buffer_group_get_internal(&p->buf_grp, MPP_BUFFER_TYPE_DRM);
    if (ret) {
        mpp_err_f("failed to get mpp buffer group ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = mpp_buffer_get(p->buf_grp, &p->frm_buf, p->frame_size + p->header_size);
    if (ret) {
        mpp_err_f("failed to get buffer for input frame ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = mpp_buffer_get(p->buf_grp, &p->pkt_buf, p->frame_size);
    if (ret) {
        mpp_err_f("failed to get buffer for output packet ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    // encoder demo
    ret = mpp_create(&p->ctx, &p->mpi);
    if (ret) {
        mpp_err("mpp_create failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    mpp_log("%p mpi_enc_test encoder test start w %d h %d type %d\n",
            p->ctx, p->width, p->height, p->type);

    ret = p->mpi->control(p->ctx, MPP_SET_OUTPUT_TIMEOUT, &timeout);
    if (MPP_OK != ret) {
        mpp_err("mpi control set output timeout %d ret %d\n", timeout, ret);
        goto MPP_TEST_OUT;
    }

    ret = mpp_init(p->ctx, MPP_CTX_ENC, p->type);
    if (ret) {
        mpp_err("mpp_init failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = mpp_enc_cfg_init(&p->cfg);
    if (ret) {
        mpp_err_f("mpp_enc_cfg_init failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = test_mpp_enc_cfg_setup(p);
    if (ret) {
        mpp_err_f("test mpp setup failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = test_mpp_run(p);
    if (ret) {
        mpp_err_f("test mpp run failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = p->mpi->reset(p->ctx);
    if (ret) {
        mpp_err("mpi->reset failed\n");
        goto MPP_TEST_OUT;
    }

MPP_TEST_OUT:
    if (MPP_OK == ret)
        mpp_log("%p mpi_enc_test success total frame %d bps %lld\n",
                p->ctx, p->frame_count,
                (RK_U64)((p->stream_size * 8 * (p->fps_out_num / p->fps_out_den)) / p->frame_count));
    else
        mpp_err("%p mpi_enc_test failed ret %d\n", p->ctx, ret);

    if (p->ctx) {
        mpp_destroy(p->ctx);
        p->ctx = NULL;
    }

    if (p->cfg) {
        mpp_enc_cfg_deinit(p->cfg);
        p->cfg = NULL;
    }

    if (p->frm_buf) {
        mpp_buffer_put(p->frm_buf);
        p->frm_buf = NULL;
    }

    if (p->pkt_buf) {
        mpp_buffer_put(p->pkt_buf);
        p->pkt_buf = NULL;
    }

    if (p->osd_data.buf) {
        mpp_buffer_put(p->osd_data.buf);
        p->osd_data.buf = NULL;
    }

    if (p->buf_grp) {
        mpp_buffer_group_put(p->buf_grp);
        p->buf_grp = NULL;
    }

    test_ctx_deinit(&p);

    return ret;
}

int main(int argc, char **argv)
{
    RK_S32 ret = MPP_NOK;
    MpiEncTestArgs* cmd = mpi_enc_test_cmd_get();

    memset((void*)cmd, 0, sizeof(*cmd));

    // parse the cmd option
    if (argc > 1){
        ret = mpi_enc_test_cmd_update_by_args(cmd, argc, argv);
        std::cout<<"Parse returned:"<<ret<<"\n";
    }

    if (ret) {
        mpi_enc_test_help();
        mpi_enc_test_cmd_put(cmd);
        return ret;
    }

    mpi_enc_test_cmd_show_opt(cmd);

    std::string destinationIpAddress="169.254.210.192";
    udpSender=std::make_unique<UDPSender>(destinationIpAddress,M_DESTINATION_PORT,MY_WANTED_UDP_SENDBUFF_SIZE);

    ret = mpi_enc_test(cmd);

    mpi_enc_test_cmd_put(cmd);

    return ret;
}

