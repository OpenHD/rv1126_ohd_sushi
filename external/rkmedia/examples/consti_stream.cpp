// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RKAIQ
#define RKAIQ
#endif

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

extern "C"{
#include "common/sample_common.h"
};
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
//

//Stuff added by Consti10
#include "common_consti/UDPSender.h"
#include "common_consti/make_unique.hpp"
#include "common_consti/rtp_enc.h"
//
#include "easymedia/utils.h"
#include "easymedia/codec.h"
// well just write helper functions for rkmedia into my own helper file
#include "common_consti/consti_rkmedia_helper.h"

static bool quit = false;
static void sigterm_handler(int sig) {
    fprintf(stderr, "signal %d\n", sig);
    quit = true;
}

static const int M_DESTINATION_PORT=5600;
static const int CHUNK_SIZE=1446;
static const int MY_WANTED_UDP_SENDBUFF_SIZE=1024*1024*10;
static std::unique_ptr<UDPSender> udpSender=nullptr;
static RK_CHAR *pOutPath = NULL;
static FILE *g_output_file=NULL;
//
static uint64_t lastTimeStamp=0;
static uint64_t frameDeltaAvgSum=0;
static uint64_t frameDeltaAvgCount=0;

static uint64_t bytesSinceLastCalculation=0;
static uint64_t lastBitrateCalculationMs=0;

// this is just a NALU with no content
static __attribute__((unused)) uint8_t fakeNALU[4]={0,0,0,1};
// this is the data for an h264 AUD unit
static __attribute__((unused)) uint8_t EXAMPLE_AUD[6]={0,0,0,1,9,48};

static const size_t RTP_PACKET_SIZE= 1466;
static std::vector<std::vector<uint8_t>> rtpPacketsBuffer(50,std::vector<uint8_t>(RTP_PACKET_SIZE));
rtp_enc rtpEnc;

static uint64_t getTimeMs(){
    struct timeval time;
    gettimeofday(&time, NULL);
    uint64_t millis = (time.tv_sec * ((uint64_t)1000)) + ((uint64_t)time.tv_usec / ((uint64_t)1000));
    return millis;
}

static uint64_t __attribute__((unused)) getTimeUs(){
    struct timeval time;
    gettimeofday(&time, NULL);
    uint64_t micros = (time.tv_sec * ((uint64_t)1000*1000)) + ((uint64_t)time.tv_usec);
    return micros;
}

static void __attribute__((unused)) writeToFileIfEnabled(const void* data, int data_length){
    // if enabled the file pointer is not null
    if (g_output_file) {
        size_t written=fwrite(data, 1,data_length, g_output_file);
        printf("#Wrote packet to file %d actual:%d\n",data_length,(int)written);
    }
}

static void sendViaUDPIfEnabled(const void* data,int data_length){
    if(udpSender!= nullptr){
        udpSender->splitAndSend((uint8_t*)data,data_length,CHUNK_SIZE);
    }
}

// check if there is more than one NALU inside this (MediaBuffer) packet
// apparently mpp gives us a sps,pps with every IDR frame (IDR prefixed by SPS/PPS)
static void __attribute__((unused)) sendAsRTP(const uint8_t* data,size_t data_length,bool isH265){
    const size_t n_packets=30;
    std::vector<uint8_t*> packets(n_packets);
    std::vector<int> packet_sizes(n_packets);
    for(size_t i=0;i<n_packets;i++){
        packets[i]=rtpPacketsBuffer[i].data();
        packet_sizes[i]=rtpPacketsBuffer[i].size();
    }
    int count=rtp_enc_h26X(isH265,&rtpEnc,data,data_length,0,packets.data(),packet_sizes.data());
    printf("Rtp packets: %d\n",count);
    std::size_t total=0;
    for(int i=0;i<count;i++){
        printf("Rtp packet: %d: %d\n",i,packet_sizes[i]);
        total+=packet_sizes[i];
        // xxx test
        //writeToFileIfEnabled(packets[i],packet_sizes[i]);
    }
    printf("Rtp packets total size: %d NALU size: %d\n",total,data_length);
}

static void __attribute__((unused)) sendAsRTP2(const uint8_t* data,size_t data_length,bool isH265){
    // one media buffer might contain more than one NALU -
    // or rather an IDR frame is prefixed by SPS/PPS
    std::list<std::shared_ptr<easymedia::MediaBuffer>> spspps;
    if(isH265){
        spspps = easymedia::split_h265_separate((const uint8_t *)data,data_length,0);
    }else{
        spspps = easymedia::split_h264_separate((const uint8_t *)data,data_length,0);
    }
    printf("Got %d NALUs\n",spspps.size());
}


void video_packet_cb(MEDIA_BUFFER mb) {
    const char *nalu_type = "Unknown";
    switch (RK_MPI_MB_GetFlag(mb)) {
        case VENC_NALU_IDRSLICE:
            nalu_type = "IDR Slice";
            break;
        case VENC_NALU_PSLICE:
            nalu_type = "P Slice";
            break;
        case VENC_NALU_SPS:
            nalu_type="SPS";
            break;
        case VENC_NALU_PPS:
            nalu_type="PPS";
            break;
        default:
            break;
    }
    const uint8_t * mb_data=(uint8_t*)RK_MPI_MB_GetPtr(mb);
    const size_t mb_data_size=RK_MPI_MB_GetSize(mb);
    printf("Get Video Encoded packet(%s):ptr:%p, fd:%d, size:%zu, mode:%d\n",
           nalu_type, RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetFD(mb),
           RK_MPI_MB_GetSize(mb), RK_MPI_MB_GetModeID(mb));
    // hm how exactly does one get the sps/pps ?!
    // send out NALU data via udp (raw)
    sendViaUDPIfEnabled(mb_data,mb_data_size);
    // send a "empty NALU" to decrease latency (obviosly this works, but we want RTP instead)
    sendViaUDPIfEnabled(EXAMPLE_AUD,sizeof(EXAMPLE_AUD));
    // confirmed, IDR frame comes prefixed with SPS/PPS
    //checkForAnotherNALSequence(mb_data,mb_data_size);
    //sendAsRTP(mb_data,mb_data_size, false);
    //
    //sendAsRTP2(mb_data,mb_data_size,false);

    //optionally write to file for debugging
    writeToFileIfEnabled(mb_data,mb_data_size);
    //writeToFileIfEnabled(EXAMPLE_AUD,sizeof(EXAMPLE_AUD));
    //Consti10: print time to check for fps
    uint64_t ts=getTimeMs();
    uint64_t delta=ts-lastTimeStamp;
    uint64_t buffer_ts=RK_MPI_MB_GetTimestamp(mb);
    //uint64_t mb_latency=ts-(buffer_ts/1000);
    frameDeltaAvgSum+=delta;
    frameDeltaAvgCount++;
    if(frameDeltaAvgCount>100){
        float avgFrameDelta=(float)((double)frameDeltaAvgSum / (double)frameDeltaAvgCount);
        printf("Avg of frame delta: %f\n",avgFrameDelta);
        frameDeltaAvgSum=0;
        frameDeltaAvgCount=0;
    }

    bytesSinceLastCalculation+=RK_MPI_MB_GetSize(mb);
    if(ts-lastBitrateCalculationMs>=1000){
        float avgBitrateMbits=(double)bytesSinceLastCalculation/1024.0f/1024.0f*8;
        printf("Avg bitrate is %f MBit/s\n",avgBitrateMbits);
        lastBitrateCalculationMs=ts;
        bytesSinceLastCalculation=0;
    }
    lastTimeStamp=ts;
    printf("Current time %" PRIu64 "(ms), delta %" PRIu64 "(ms) MB ts:%" PRIu64 "\n",ts,delta,buffer_ts);
    RK_MPI_MB_TsNodeDump(mb);
    //Consti10: add end
    RK_MPI_MB_ReleaseBuffer(mb);
}

static RK_CHAR optstr[] = "?:a::h:w:e:d:f:i:c:o:b:y:t:z::";
static const struct option long_options[] = {
        {"aiq", optional_argument, NULL, 'a'},
        {"height", required_argument, NULL, 'h'},
        {"width", required_argument, NULL, 'w'},
        {"encode", required_argument, NULL, 'e'},
        {"framerate", required_argument, NULL, 'f'},
        {"device_name", required_argument, NULL, 'd'},
        {"ip_address",required_argument,NULL, 'i'},
        {"crop",required_argument,NULL, 'c'},
        {"output",required_argument,NULL, 'o'},
        {"bitrate",required_argument,NULL, 'b'},
        {"hdr",required_argument,NULL, 'x'},
        {"cam_id",required_argument,NULL, 'y'},
        {"run_time",required_argument,NULL, 't'},
        {"disable_processing",optional_argument,NULL, 'z'},
        {NULL, 0, NULL, 0},
};

static void print_usage(const RK_CHAR *name) {
    printf("usage example:\n");
    printf("\t%s "
         "[-a | --aiq /oem/etc/iqfiles/] "
         "[-h | --height 1080] "
         "[-w | --width 1920] "
         "[-e | --encode 0] "
         "[-f | --framerate 30]"
         "[-d | --device_name rkispp_scale0]\n",
         name);
  printf("\t-a | --aiq: enable aiq with dirpath provided, eg:-a "
         "/oem/etc/iqfiles/, "
         "set dirpath empty to using path by default, without this option aiq "
         "should run in other application\n");
    printf("\t-h | --height: VI height, Default:1080\n");
    printf("\t-w | --width: VI width, Default:1920\n");
    printf("\t-e | --encode: encode type, Default:0, set 0 to use H264, set 1 to "
           "use H265\n");
    printf("\t-d | --device_name set pcDeviceName, Default:rkispp_m_bypass, "
           "Option:[rkispp_m_bypass, rkispp_scale0, rkispp_scale1, rkispp_scale2]\n");
    printf("\t-b | --bitrate Use custom bitrate, in MBit/s (e.g. 5==5 MBit/s)\n");
    printf("\t-o | --output Write raw data to file (optional)\n");
    printf("\t-x | --hdr HDR working mode. 0=NO_HDR\n");
    printf("\t-z | disable as much isp processing\n");
    printf("\t-t | specify how long to run (else infinite),in seconds\n");
}

int main(int argc, char *argv[]) {
    // I think this also affects child processes, but not sure - doesn't make a difference 
    // on this system though.
    RK_U32 u32Width = 1920;
    RK_U32 u32Height = 1080;
    RK_U32 encode_type = 0;
    RK_U32 m_framerate=30;
    std::string device_name = "rkispp_m_bypass";
    std::string iq_file_dir = "";
    RK_S32 s32CamId = 0;
    bool m_crop=false;
    std::string destinationIpAddress="192.168.0.13";
    int bitrateMbitps=5; // 5 MBit/s
    int m_HdrModeInt=0;
    bool disableAsMuchIspAsPossible=false;
    // default to -1 (infinite)
    int runTimeSeconds=-1;
    
    int ret = 0;
    int c;
    while ((c = getopt_long(argc, argv, optstr, long_options, NULL)) != -1) {
        const char *tmp_optarg = optarg;
        switch (c) {
            case 'a':
                if (!optarg && NULL != argv[optind] && '-' != argv[optind][0]) {
                    tmp_optarg = argv[optind++];
                }
                if (tmp_optarg) {
                    iq_file_dir = std::string((char *)tmp_optarg);
                } else {
                    iq_file_dir = std::string("/oem/etc/iqfiles");
                }
                break;
            case 'h':
                u32Height = atoi(optarg);
                break;
            case 'w':
                u32Width = atoi(optarg);
                break;
            case 'e':
                encode_type = atoi(optarg);
                break;
            case 'd':
                device_name = optarg;
                break;
            case 'f':
                m_framerate=atoi(optarg);
                break;
            case 'i':
                destinationIpAddress = optarg;
                break;
            case 'c':
                m_crop=true;
                break;
            case 'o':
                pOutPath = optarg;
                break;
            case 'b':
                bitrateMbitps = atoi(optarg);
                break;
            case 'x':
                m_HdrModeInt = atoi(optarg);
                break;
            case 'y':
                s32CamId = atoi(optarg);
                break;
            case 'z':
                disableAsMuchIspAsPossible=true;
                break;
            case 't':
                runTimeSeconds=atoi(optarg);
                break;
            case '?':
            default:
                print_usage(argv[0]);
                return 0;
        }
    }
    // consti10: with isp bypass we can use FBC0, else we have to use NV12
    IMAGE_TYPE_E m_image_type=IMAGE_TYPE_NV12;
    if(strcmp(device_name.c_str(),"rkispp_m_bypass")==0){
      m_image_type=IMAGE_TYPE_FBC0;
    }
    // optionally enable HDR
    rk_aiq_working_mode_t hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
    hdr_mode=convert_rk_aiq_working_mode_t(m_HdrModeInt);
    print_rk_aiq_working_mode_t(hdr_mode);


    udpSender=std::make_unique<UDPSender>(destinationIpAddress,M_DESTINATION_PORT,MY_WANTED_UDP_SENDBUFF_SIZE);
    rtp_enc_init(&rtpEnc);

    printf("device_name: %s\n", device_name.c_str());
    printf("#height: %d\n", u32Height);
    printf("#width: %d\n", u32Width);
    printf("#framerate: %d\n", m_framerate);
    printf("#encode_type: %d\n", encode_type);
    printf("#UDP Destination: %s %d\n",destinationIpAddress.c_str(),M_DESTINATION_PORT);
    printf("#image type:%d\n",(int)m_image_type);
    printf("#Bitrate:%d Mbit/s\n",(int)bitrateMbitps);
    printf("#Camera id %d\n",(int)s32CamId);
    if(pOutPath!=NULL){
        printf("#Output Path: %s\n", pOutPath);
    }else{
        printf("#File output disabled\n");
    }
    printf("Test isp processing disabled: %s\n",(disableAsMuchIspAsPossible ? "y":"n"));

    if (!iq_file_dir.empty()) {
#ifdef RKAIQ
        printf("#Aiq xml dirpath: %s\n", iq_file_dir.c_str());
        RK_BOOL fec_enable = RK_FALSE;
        int fps = m_framerate;
        SAMPLE_COMM_ISP_Init(s32CamId,hdr_mode, fec_enable, iq_file_dir.c_str());
        // no matter what, disable LSC. LSC gives errors when the resolution in the iqfile
        // does not match the exact isp input resolution.
        SAMPLE_COMM_ISP_Consti10_DisableLSC(s32CamId);
        // test
        if(disableAsMuchIspAsPossible){
            SAMPLE_COMM_ISP_Consti10_DisableStuff(s32CamId);
        }
        if(m_crop){
            // crop the image before it goes into the ISP
            rk_aiq_rect_t cropRect;
            cropRect.left = 0;
            cropRect.top = 0;
            cropRect.width = u32Width;
            cropRect.height = u32Height;
            ret=SAMPLE_COMM_ISP_SET_Crop(s32CamId,cropRect);
            printf("Consti10: applying crop%d\n",ret);
        }
        SAMPLE_COMM_ISP_Run(s32CamId);
        SAMPLE_COMM_ISP_SetFrameRate(s32CamId,fps);
#endif
    }
    if (pOutPath) {
        g_output_file = fopen(pOutPath, "w");
        if (!g_output_file) {
            printf("ERROR: open file: %s fail, exit\n", pOutPath);
            return 0;
        }
    }

    RK_MPI_SYS_Init();
    VI_CHN_ATTR_S vi_chn_attr;
    vi_chn_attr.pcVideoNode = device_name.c_str();
    vi_chn_attr.u32BufCnt = 6; // should be >= 4 | Consti10: was 4 by default
    vi_chn_attr.u32Width = u32Width;
    vi_chn_attr.u32Height = u32Height;
    vi_chn_attr.enPixFmt = m_image_type;
    vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
    // did I accidentally remove this ?!
    // somewhere it says VI_CHN_BUF_TYPE_DMA = 0, // Default , so perhaps I should have been using MMAP ?!
    //vi_chn_attr.enBufType = VI_CHN_BUF_TYPE_MMAP;
    printf("Consti10: Used vi_chn_attr.enBufType:%d\n",vi_chn_attr.enBufType);
    ret = RK_MPI_VI_SetChnAttr(0, 0, &vi_chn_attr);
    ret |= RK_MPI_VI_EnableChn(0, 0);
    if (ret) {
        printf("ERROR: create VI[0] error! ret=%d\n", ret);
        return 0;
    }

    VENC_CHN_ATTR_S venc_chn_attr;
    switch (encode_type) {
        case 0:
            venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_H264;
            venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
            // Consti10: isn't it best to manually set the level ?!
            venc_chn_attr.stVencAttr.stAttrH264e.u32Level = 51;
            break;
        case 1:
            venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_H265;
            venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
            break;
        default:
            venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_H264;
            venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
            break;
    }
    venc_chn_attr.stVencAttr.imageType = m_image_type;
    venc_chn_attr.stVencAttr.u32PicWidth = u32Width;
    venc_chn_attr.stVencAttr.u32PicHeight = u32Height;
    venc_chn_attr.stVencAttr.u32VirWidth = u32Width;
    venc_chn_attr.stVencAttr.u32VirHeight = u32Height;
    // Consti10: use baseline instead of main ?
    //venc_chn_attr.stVencAttr.u32Profile = 77;
    venc_chn_attr.stVencAttr.u32Profile = 66;
    // Consti10: if we use 1 as GOP, we obviously increase bit rate a lot, but each frame can be decoded independently.
    // however, we probably want to make this user-configurable later. For latency testing though it is nice to have it at 1,
    // since it makes everything more consistently
    // Note: Without gop==1 the size of NALUs fluctuates immensely
    venc_chn_attr.stRcAttr.stH264Cbr.u32Gop = 10;
    //venc_chn_attr.stRcAttr.stH264Cbr.u32Gop = 10;
    //venc_chn_attr.stRcAttr.stH264Cbr.u32BitRate = 1920 * 1080 * 30 / 14;
    //1920 * 1080 * 30 / 14 == 4443428.57143 (4.4 MBit/s)
    // use a fixed X MBit/s as bit rate
    // Weirdly, a higher bit rate actually seems to result in slightly lower or more consistently low latency.
    // Perhaps, if you decrease the bit rate too much, the decoder needs to do "too much" compression work
    //venc_chn_attr.stRcAttr.stH264Cbr.u32BitRate = 5*1000*1000;
    venc_chn_attr.stRcAttr.stH264Cbr.u32BitRate = bitrateMbitps*1024*1024; //convert from MBit/s to bit/s
    venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateDen = 0;
    venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateNum = m_framerate;
    venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateDen = 0;
    venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateNum = m_framerate;
    ret = RK_MPI_VENC_CreateChn(0, &venc_chn_attr);
    if (ret) {
        printf("ERROR: create VENC[0] error! ret=%d\n", ret);
        return 0;
    }

    MPP_CHN_S stEncChn;
    stEncChn.enModId = RK_ID_VENC;
    stEncChn.s32DevId = 0;
    stEncChn.s32ChnId = 0;
    ret = RK_MPI_SYS_RegisterOutCb(&stEncChn, video_packet_cb);
    if (ret) {
        printf("ERROR: register output callback for VENC[0] error! ret=%d\n", ret);
        return 0;
    }

    MPP_CHN_S stSrcChn;
    stSrcChn.enModId = RK_ID_VI;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = 0;
    MPP_CHN_S stDestChn;
    stDestChn.enModId = RK_ID_VENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = 0;
    ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (ret) {
        printf("ERROR: Bind VI[0] and VENC[0] error! ret=%d\n", ret);
        return 0;
    }

    printf("%s initial finish\n", __func__);
    signal(SIGINT, sigterm_handler);

    const unsigned long beginMs=getTimeMs();

    while (!quit) {
        usleep(100);
        if(runTimeSeconds>0){
            const unsigned long currentRunTimeSeconds=(getTimeMs()-beginMs)/1000;
            if(currentRunTimeSeconds>=(unsigned long)runTimeSeconds){
                printf("Run time elapsed\n");
                quit=true;
            }
        }
    }

    if (g_output_file){
        fclose(g_output_file);
    }

    if (!iq_file_dir.empty()) {
#ifdef RKAIQ
        SAMPLE_COMM_ISP_Stop(s32CamId); // isp aiq stop before vi streamoff
#endif
    }

    printf("%s exit!\n", __func__);
    // unbind first
    ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    if (ret) {
        printf("ERROR: UnBind VI[0] and VENC[0] error! ret=%d\n", ret);
        return 0;
    }
    // destroy venc before vi
    ret = RK_MPI_VENC_DestroyChn(0);
    if (ret) {
        printf("ERROR: Destroy VENC[0] error! ret=%d\n", ret);
        return 0;
    }
    // destroy vi
    ret = RK_MPI_VI_DisableChn(0, 0);
    if (ret) {
        printf("ERROR: Destroy VI[0] error! ret=%d\n", ret);
        return 0;
    }

    return 0;
}