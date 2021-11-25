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

#include "rkadk_param.h"
#include "rkadk_param_map.h"
#include "rkadk_struct2ini.h"
#include <signal.h>
#include <stdbool.h>

static bool is_quit = false;

static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  is_quit = true;
}

void dump_map(RKADK_SI_CONFIG_MAP_S *pmap, int cnt) {
  for (int i = 0; i < cnt; i++) {
    printf("\n the map id = %d", i);
    printf("\n struct_name = %s", pmap->structName);
    printf("\n struct_member = %s", pmap->structMember);
    printf("\n key_vlaue_type = %d", pmap->keyVlaueType);
    printf("\n string_length = %d", pmap->stringLength);
    printf("\n offset = %d", pmap->offset);
    pmap++;
  }
}

static void SetVersion() {
  RKADK_PARAM_VERSION_S stParamVersion;

  dump_map(g_stVersionMapTable,
           sizeof(g_stVersionMapTable) / sizeof(RKADK_SI_CONFIG_MAP_S));

  memset(&stParamVersion, 0, sizeof(RKADK_PARAM_VERSION_S));

  strcpy(stParamVersion.version, RKADK_PARAM_VERSION);

  RKADK_Struct2Ini(RKADK_DEFPARAM_PATH, &stParamVersion, g_stVersionMapTable,
                   sizeof(g_stVersionMapTable) / sizeof(RKADK_SI_CONFIG_MAP_S));

  RKADK_Struct2Ini(RKADK_PARAM_PATH, &stParamVersion, g_stVersionMapTable,
                   sizeof(g_stVersionMapTable) / sizeof(RKADK_SI_CONFIG_MAP_S));
}

static void SetCommCfg() {
  RKADK_PARAM_COMM_CFG_S stParamCommCfg;

  memset(&stParamCommCfg, 0, sizeof(RKADK_PARAM_COMM_CFG_S));
  stParamCommCfg.sensor_count = 1;
  stParamCommCfg.rec_unmute = true;
  stParamCommCfg.enable_speaker = true;
  stParamCommCfg.speaker_volume = 80;
  stParamCommCfg.mic_unmute = true;
  stParamCommCfg.mic_volume = 80;
  stParamCommCfg.osd_time_format = 0;
  stParamCommCfg.osd = true;
  stParamCommCfg.boot_sound = true;
  stParamCommCfg.osd_speed = false;

  RKADK_Struct2Ini(RKADK_DEFPARAM_PATH, &stParamCommCfg, g_stCommCfgMapTable,
                   sizeof(g_stCommCfgMapTable) / sizeof(RKADK_SI_CONFIG_MAP_S));

  RKADK_Struct2Ini(RKADK_PARAM_PATH, &stParamCommCfg, g_stCommCfgMapTable,
                   sizeof(g_stCommCfgMapTable) / sizeof(RKADK_SI_CONFIG_MAP_S));
}

static void SetSensorCfg() {
  RKADK_PARAM_SENSOR_CFG_S stSensorCfg;

  memset(&stSensorCfg, 0, sizeof(RKADK_PARAM_SENSOR_CFG_S));
  stSensorCfg.max_width = SENSOR_MAX_WIDTH;
  stSensorCfg.max_height = SENSOR_MAX_HEIGHT;
  stSensorCfg.framerate = VIDEO_FRAME_RATE;
  stSensorCfg.enable_record = true;
  stSensorCfg.enable_photo = true;
  stSensorCfg.flip = false;
  stSensorCfg.mirror = false;
  stSensorCfg.ldc = 0; //[0,255]
  stSensorCfg.wdr = 0;
  stSensorCfg.hdr = 0;
  stSensorCfg.antifog = 0;

  RKADK_Struct2Ini(RKADK_DEFPARAM_PATH, &stSensorCfg, g_stSensorCfgMapTable_0,
                   sizeof(g_stSensorCfgMapTable_0) /
                       sizeof(RKADK_SI_CONFIG_MAP_S));

  RKADK_Struct2Ini(RKADK_PARAM_PATH, &stSensorCfg, g_stSensorCfgMapTable_0,
                   sizeof(g_stSensorCfgMapTable_0) /
                       sizeof(RKADK_SI_CONFIG_MAP_S));
}

static void SetAudioCfg() {
  RKADK_PARAM_AUDIO_CFG_S stAudioCfg;

  memset(&stAudioCfg, 0, sizeof(RKADK_PARAM_AUDIO_CFG_S));
  strcpy(stAudioCfg.audio_node, AI_DEVICE_NAME);
  stAudioCfg.sample_format = AUDIO_SAMPLE_FORMAT;
  stAudioCfg.channels = AUDIO_CHANNEL;
  stAudioCfg.samplerate = AUDIO_SAMPLE_RATE;
  stAudioCfg.samples_per_frame = AUDIO_FRAME_COUNT;
  stAudioCfg.bitrate = AUDIO_BIT_REAT;
  stAudioCfg.ai_layout = AI_LAYOUT_NORMAL;
  stAudioCfg.vqe_mode = RKADK_VQE_MODE_BUTT;

  RKADK_Struct2Ini(RKADK_DEFPARAM_PATH, &stAudioCfg, g_stAudioCfgMapTable,
                   sizeof(g_stAudioCfgMapTable) /
                       sizeof(RKADK_SI_CONFIG_MAP_S));

  RKADK_Struct2Ini(RKADK_PARAM_PATH, &stAudioCfg, g_stAudioCfgMapTable,
                   sizeof(g_stAudioCfgMapTable) /
                       sizeof(RKADK_SI_CONFIG_MAP_S));
}

static void SetRecCfg() {
  RKADK_PARAM_REC_CFG_S stParamRecCfg;

  memset(&stParamRecCfg, 0, sizeof(RKADK_PARAM_REC_CFG_S));
  stParamRecCfg.record_type = RKADK_REC_TYPE_NORMAL;
  stParamRecCfg.pre_record_time = 0;
  stParamRecCfg.pre_record_mode = MUXER_PRE_RECORD_NONE;
  stParamRecCfg.lapse_multiple = 30;
  stParamRecCfg.file_num = 2;

  stParamRecCfg.record_time_cfg[0].record_time = 60;
  stParamRecCfg.record_time_cfg[0].splite_time = 60;
  stParamRecCfg.record_time_cfg[0].lapse_interval = 60;
  stParamRecCfg.attribute[0].width = RECORD_VIDEO_WIDTH;
  stParamRecCfg.attribute[0].height = RECORD_VIDEO_HEIGHT;
  stParamRecCfg.attribute[0].bitrate = 30 * 1024 * 1024;
  stParamRecCfg.attribute[0].gop = VIDEO_GOP;
  stParamRecCfg.attribute[0].profile = VIDEO_PROFILE;
  stParamRecCfg.attribute[0].codec_type = RKADK_CODEC_TYPE_H264;
  stParamRecCfg.attribute[0].venc_chn = 0;
  strcpy(stParamRecCfg.attribute[0].rc_mode, "CBR");
  stParamRecCfg.attribute[0].venc_param.max_qp = 48;
  stParamRecCfg.attribute[0].venc_param.min_qp = 8;
  stParamRecCfg.attribute[0].venc_param.full_range = true;
  stParamRecCfg.attribute[0].venc_param.scaling_list = true;
  stParamRecCfg.attribute[0].venc_param.hier_qp_en = true;
  strcpy(stParamRecCfg.attribute[0].venc_param.hier_qp_delta, "-3,0,0,0");
  strcpy(stParamRecCfg.attribute[0].venc_param.hier_frame_num, "3,0,0,0");


  stParamRecCfg.record_time_cfg[1].record_time = 60;
  stParamRecCfg.record_time_cfg[1].splite_time = 60;
  stParamRecCfg.record_time_cfg[1].lapse_interval = 60;
  stParamRecCfg.attribute[1].width = RECORD_VIDEO_WIDTH_S;
  stParamRecCfg.attribute[1].height = RECORD_VIDEO_HEIGHT_S;
  stParamRecCfg.attribute[1].bitrate = 4 * 1024 * 1024;
  stParamRecCfg.attribute[1].gop = VIDEO_GOP;
  stParamRecCfg.attribute[1].profile = VIDEO_PROFILE;
  stParamRecCfg.attribute[1].codec_type = RKADK_CODEC_TYPE_H264;
  stParamRecCfg.attribute[1].venc_chn = 1;
  strcpy(stParamRecCfg.attribute[1].rc_mode, "VBR");
  stParamRecCfg.attribute[1].venc_param.max_qp = 48;
  stParamRecCfg.attribute[1].venc_param.min_qp = 8;
  stParamRecCfg.attribute[1].venc_param.full_range = true;
  stParamRecCfg.attribute[1].venc_param.scaling_list = true;
  stParamRecCfg.attribute[1].venc_param.hier_qp_en = true;
  strcpy(stParamRecCfg.attribute[1].venc_param.hier_qp_delta, "-3,0,0,0");
  strcpy(stParamRecCfg.attribute[1].venc_param.hier_frame_num, "3,0,0,0");

  RKADK_Struct2Ini(RKADK_DEFPARAM_PATH, &stParamRecCfg, g_stRecCfgMapTable_0,
                   sizeof(g_stRecCfgMapTable_0) /
                       sizeof(RKADK_SI_CONFIG_MAP_S));
  RKADK_Struct2Ini(RKADK_DEFPARAM_PATH, &stParamRecCfg.attribute[0],
                   g_stRecCfgMapTable_0_0, sizeof(g_stRecCfgMapTable_0_0) /
                                               sizeof(RKADK_SI_CONFIG_MAP_S));
  RKADK_Struct2Ini(RKADK_DEFPARAM_PATH, &stParamRecCfg.attribute[1],
                   g_stRecCfgMapTable_0_1, sizeof(g_stRecCfgMapTable_0_1) /
                                               sizeof(RKADK_SI_CONFIG_MAP_S));
  RKADK_Struct2Ini(RKADK_DEFPARAM_PATH, &stParamRecCfg.attribute[0].venc_param,
                   g_stRecParamMapTable_0_0, sizeof(g_stRecParamMapTable_0_0) /
                                                 sizeof(RKADK_SI_CONFIG_MAP_S));
  RKADK_Struct2Ini(RKADK_DEFPARAM_PATH, &stParamRecCfg.attribute[1].venc_param,
                   g_stRecParamMapTable_0_1, sizeof(g_stRecParamMapTable_0_1) /
                                                 sizeof(RKADK_SI_CONFIG_MAP_S));

  RKADK_Struct2Ini(RKADK_PARAM_PATH, &stParamRecCfg, g_stRecCfgMapTable_0,
                   sizeof(g_stRecCfgMapTable_0) /
                       sizeof(RKADK_SI_CONFIG_MAP_S));
  RKADK_Struct2Ini(RKADK_PARAM_PATH, &stParamRecCfg.attribute[0],
                   g_stRecCfgMapTable_0_0, sizeof(g_stRecCfgMapTable_0_0) /
                                               sizeof(RKADK_SI_CONFIG_MAP_S));
  RKADK_Struct2Ini(RKADK_PARAM_PATH, &stParamRecCfg.attribute[1],
                   g_stRecCfgMapTable_0_1, sizeof(g_stRecCfgMapTable_0_1) /
                                               sizeof(RKADK_SI_CONFIG_MAP_S));
  RKADK_Struct2Ini(RKADK_PARAM_PATH, &stParamRecCfg.attribute[0].venc_param,
                   g_stRecParamMapTable_0_0, sizeof(g_stRecParamMapTable_0_0) /
                                                 sizeof(RKADK_SI_CONFIG_MAP_S));
  RKADK_Struct2Ini(RKADK_PARAM_PATH, &stParamRecCfg.attribute[1].venc_param,
                   g_stRecParamMapTable_0_1, sizeof(g_stRecParamMapTable_0_1) /
                                                 sizeof(RKADK_SI_CONFIG_MAP_S));
}

static void SetStreamCfg() {
  RKADK_PARAM_STREAM_CFG_S stStreamCfg;

  memset(&stStreamCfg, 0, sizeof(RKADK_PARAM_STREAM_CFG_S));
  stStreamCfg.attribute.width = STREAM_VIDEO_WIDTH;
  stStreamCfg.attribute.height = STREAM_VIDEO_HEIGHT;
  stStreamCfg.attribute.bitrate = 4 * 1024 * 1024;
  stStreamCfg.attribute.gop = VIDEO_GOP;
  stStreamCfg.attribute.profile = VIDEO_PROFILE;
  stStreamCfg.attribute.codec_type = RKADK_CODEC_TYPE_H264;
  stStreamCfg.attribute.venc_chn = 1;
  strcpy(stStreamCfg.attribute.rc_mode, "VBR");
  stStreamCfg.attribute.venc_param.max_qp = 48;
  stStreamCfg.attribute.venc_param.min_qp = 8;
  stStreamCfg.attribute.venc_param.full_range = true;
  stStreamCfg.attribute.venc_param.scaling_list = true;
  stStreamCfg.attribute.venc_param.hier_qp_en = true;
  strcpy(stStreamCfg.attribute.venc_param.hier_qp_delta, "-3,0,0,0");
  strcpy(stStreamCfg.attribute.venc_param.hier_frame_num, "3,0,0,0");

  RKADK_Struct2Ini(RKADK_DEFPARAM_PATH, &stStreamCfg, g_stPreviewCfgMapTable_0,
                   sizeof(g_stPreviewCfgMapTable_0) /
                       sizeof(RKADK_SI_CONFIG_MAP_S));

  RKADK_Struct2Ini(RKADK_PARAM_PATH, &stStreamCfg, g_stPreviewCfgMapTable_0,
                   sizeof(g_stPreviewCfgMapTable_0) /
                       sizeof(RKADK_SI_CONFIG_MAP_S));

  RKADK_Struct2Ini(RKADK_DEFPARAM_PATH, &stStreamCfg.attribute.venc_param,
                   g_stPreviewParamMapTable_0,
                   sizeof(g_stPreviewParamMapTable_0) /
                       sizeof(RKADK_SI_CONFIG_MAP_S));

  RKADK_Struct2Ini(RKADK_PARAM_PATH, &stStreamCfg.attribute.venc_param,
                   g_stPreviewParamMapTable_0,
                   sizeof(g_stPreviewParamMapTable_0) /
                       sizeof(RKADK_SI_CONFIG_MAP_S));
}

static void SetLiveCfg() {
  RKADK_PARAM_STREAM_CFG_S stLiveCfg;

  memset(&stLiveCfg, 0, sizeof(RKADK_PARAM_STREAM_CFG_S));
  stLiveCfg.attribute.width = STREAM_VIDEO_WIDTH;
  stLiveCfg.attribute.height = STREAM_VIDEO_HEIGHT;
  stLiveCfg.attribute.bitrate = 4 * 1024 * 1024;
  stLiveCfg.attribute.gop = VIDEO_GOP;
  stLiveCfg.attribute.profile = VIDEO_PROFILE;
  stLiveCfg.attribute.codec_type = RKADK_CODEC_TYPE_H264;
  stLiveCfg.attribute.venc_chn = 1;
  strcpy(stLiveCfg.attribute.rc_mode, "VBR");
  stLiveCfg.attribute.venc_param.max_qp = 48;
  stLiveCfg.attribute.venc_param.min_qp = 8;
  stLiveCfg.attribute.venc_param.full_range = true;
  stLiveCfg.attribute.venc_param.scaling_list = true;
  stLiveCfg.attribute.venc_param.hier_qp_en = true;
  strcpy(stLiveCfg.attribute.venc_param.hier_qp_delta, "-3,0,0,0");
  strcpy(stLiveCfg.attribute.venc_param.hier_frame_num, "3,0,0,0");

  RKADK_Struct2Ini(RKADK_DEFPARAM_PATH, &stLiveCfg, g_stLiveCfgMapTable_0,
                   sizeof(g_stLiveCfgMapTable_0) /
                       sizeof(RKADK_SI_CONFIG_MAP_S));

  RKADK_Struct2Ini(RKADK_PARAM_PATH, &stLiveCfg, g_stLiveCfgMapTable_0,
                   sizeof(g_stLiveCfgMapTable_0) /
                       sizeof(RKADK_SI_CONFIG_MAP_S));

  RKADK_Struct2Ini(RKADK_DEFPARAM_PATH, &stLiveCfg.attribute.venc_param,
                   g_stLiveParamMapTable_0, sizeof(g_stLiveParamMapTable_0) /
                                                sizeof(RKADK_SI_CONFIG_MAP_S));

  RKADK_Struct2Ini(RKADK_PARAM_PATH, &stLiveCfg.attribute.venc_param,
                   g_stLiveParamMapTable_0, sizeof(g_stLiveParamMapTable_0) /
                                                sizeof(RKADK_SI_CONFIG_MAP_S));
}

static void SetPhotoCfg() {
  RKADK_PARAM_PHOTO_CFG_S stPhotoCfg;

  memset(&stPhotoCfg, 0, sizeof(RKADK_PARAM_PHOTO_CFG_S));
  stPhotoCfg.image_width = PHOTO_VIDEO_WIDTH;
  stPhotoCfg.image_height = PHOTO_VIDEO_HEIGHT;
  stPhotoCfg.snap_num = 1;
  stPhotoCfg.venc_chn = 2;

  RKADK_Struct2Ini(RKADK_DEFPARAM_PATH, &stPhotoCfg, g_stPhotoCfgMapTable_0,
                   sizeof(g_stPhotoCfgMapTable_0) /
                       sizeof(RKADK_SI_CONFIG_MAP_S));

  RKADK_Struct2Ini(RKADK_PARAM_PATH, &stPhotoCfg, g_stPhotoCfgMapTable_0,
                   sizeof(g_stPhotoCfgMapTable_0) /
                       sizeof(RKADK_SI_CONFIG_MAP_S));
}

static void SetViCfg() {
  RKADK_PARAM_VI_CFG_S stViCfg;

  memset(&stViCfg, 0, sizeof(RKADK_PARAM_VI_CFG_S));
  stViCfg.chn_id = 0;
  strcpy(stViCfg.device_name, "rkispp_m_bypass");
  stViCfg.width = SENSOR_MAX_WIDTH;
  stViCfg.height = SENSOR_MAX_HEIGHT;
  stViCfg.buf_cnt = 4;
  strcpy(stViCfg.pix_fmt, "FBC0");
  strcpy(stViCfg.module, "RECORD_MAIN|PHOTO");
  RKADK_Struct2Ini(RKADK_DEFPARAM_PATH, &stViCfg, g_stViCfgMapTable_0,
                   sizeof(g_stViCfgMapTable_0) / sizeof(RKADK_SI_CONFIG_MAP_S));
  RKADK_Struct2Ini(RKADK_PARAM_PATH, &stViCfg, g_stViCfgMapTable_0,
                   sizeof(g_stViCfgMapTable_0) / sizeof(RKADK_SI_CONFIG_MAP_S));

  memset(&stViCfg, 0, sizeof(RKADK_PARAM_VI_CFG_S));
  stViCfg.chn_id = 1;
  strcpy(stViCfg.device_name, "rkispp_scale0");
  stViCfg.buf_cnt = 4;
  strcpy(stViCfg.pix_fmt, "NV12");
  strcpy(stViCfg.module, "RECORD_MAIN|PHOTO");
  RKADK_Struct2Ini(RKADK_DEFPARAM_PATH, &stViCfg, g_stViCfgMapTable_1,
                   sizeof(g_stViCfgMapTable_1) / sizeof(RKADK_SI_CONFIG_MAP_S));
  RKADK_Struct2Ini(RKADK_PARAM_PATH, &stViCfg, g_stViCfgMapTable_1,
                   sizeof(g_stViCfgMapTable_1) / sizeof(RKADK_SI_CONFIG_MAP_S));

  memset(&stViCfg, 0, sizeof(RKADK_PARAM_VI_CFG_S));
  stViCfg.chn_id = 2;
  strcpy(stViCfg.device_name, "rkispp_scale1");
  stViCfg.buf_cnt = 2;
  strcpy(stViCfg.pix_fmt, "NV12");
  strcpy(stViCfg.module, "NONE");
  RKADK_Struct2Ini(RKADK_DEFPARAM_PATH, &stViCfg, g_stViCfgMapTable_2,
                   sizeof(g_stViCfgMapTable_2) / sizeof(RKADK_SI_CONFIG_MAP_S));
  RKADK_Struct2Ini(RKADK_PARAM_PATH, &stViCfg, g_stViCfgMapTable_2,
                   sizeof(g_stViCfgMapTable_2) / sizeof(RKADK_SI_CONFIG_MAP_S));

  memset(&stViCfg, 0, sizeof(RKADK_PARAM_VI_CFG_S));
  stViCfg.chn_id = 3;
  strcpy(stViCfg.device_name, "rkispp_scale2");
  stViCfg.width = STREAM_VIDEO_WIDTH;
  stViCfg.height = STREAM_VIDEO_HEIGHT;
  stViCfg.buf_cnt = 4;
  strcpy(stViCfg.pix_fmt, "NV12");
  strcpy(stViCfg.module, "RECORD_SUB|PREVIEW|LIVE|DISP");
  RKADK_Struct2Ini(RKADK_DEFPARAM_PATH, &stViCfg, g_stViCfgMapTable_3,
                   sizeof(g_stViCfgMapTable_3) / sizeof(RKADK_SI_CONFIG_MAP_S));
  RKADK_Struct2Ini(RKADK_PARAM_PATH, &stViCfg, g_stViCfgMapTable_3,
                   sizeof(g_stViCfgMapTable_3) / sizeof(RKADK_SI_CONFIG_MAP_S));
}

static void SetThumbCfg() {
  RKADK_PARAM_THUMB_CFG_S stThumbCfg;

  memset(&stThumbCfg, 0, sizeof(RKADK_PARAM_THUMB_CFG_S));
  stThumbCfg.thumb_width = THUMB_WIDTH;
  stThumbCfg.thumb_height = THUMB_HEIGHT;
  stThumbCfg.venc_chn = THUMB_VENC_CHN;

  RKADK_Struct2Ini(RKADK_DEFPARAM_PATH, &stThumbCfg, g_stThumbCfgMapTable,
                   sizeof(g_stThumbCfgMapTable) /
                       sizeof(RKADK_SI_CONFIG_MAP_S));
  RKADK_Struct2Ini(RKADK_PARAM_PATH, &stThumbCfg, g_stThumbCfgMapTable,
                   sizeof(g_stThumbCfgMapTable) /
                       sizeof(RKADK_SI_CONFIG_MAP_S));
}

static void SetDispCfg() {
  RKADK_PARAM_DISP_CFG_S stDispCfg;

  memset(&stDispCfg, 0, sizeof(RKADK_PARAM_DISP_CFG_S));
  stDispCfg.width = DISP_WIDTH;
  stDispCfg.height = DISP_HEIGHT;
  // rga
  stDispCfg.enable_buf_pool = true;
  stDispCfg.buf_pool_cnt = 2;
  stDispCfg.rotaion = 90;
  stDispCfg.rga_chn = 0;
  // vo
  strcpy(stDispCfg.device_node, "/dev/dri/card0");
  stDispCfg.plane_type = VO_PLANE_PRIMARY;
  strcpy(stDispCfg.img_type, "RGB888");
  stDispCfg.z_pos = 0;
  stDispCfg.vo_chn = 0;
  RKADK_Struct2Ini(RKADK_DEFPARAM_PATH, &stDispCfg, g_stDispCfgMapTable_0,
                   sizeof(g_stDispCfgMapTable_0) /
                       sizeof(RKADK_SI_CONFIG_MAP_S));
  RKADK_Struct2Ini(RKADK_PARAM_PATH, &stDispCfg, g_stDispCfgMapTable_0,
                   sizeof(g_stDispCfgMapTable_0) /
                       sizeof(RKADK_SI_CONFIG_MAP_S));
}

int main(int argc, char *argv[]) {

  RKADK_LOGD("this is demo for struct2ini");

  signal(SIGINT, sigterm_handler);

  char cmd[64];
  printf("\n#Usage: input 'quit' to exit programe!\n"
         "input 'set' key to write config param to ini file\n"
         "input 'get' key to read config param from ini file\n");
  while (!is_quit) {
    fgets(cmd, sizeof(cmd), stdin);
    if (strstr(cmd, "quit") || is_quit) {
      RKADK_LOGD("#Get 'quit' cmd!");
      break;
    }

    if (strstr(cmd, "set")) {
      RKADK_LOGD("#Set version");
      SetVersion();
      RKADK_LOGD("SetVersion done");

      RKADK_LOGD("#Set config param");
      SetCommCfg();
      RKADK_LOGD("SetCommCfg done");

      SetAudioCfg();
      RKADK_LOGD("SetAudioCfg done");

      SetThumbCfg();
      RKADK_LOGD("SetThumbCfg done");

      SetSensorCfg();
      RKADK_LOGD("SetSensorCfg done");

      SetViCfg();
      RKADK_LOGD("SetViCfg done");

      SetRecCfg();
      RKADK_LOGD("SetRecCfg done");

      SetPhotoCfg();
      RKADK_LOGD("SetPhotoCfg done");

      SetStreamCfg();
      RKADK_LOGD("SetStreamCfg done");

      SetLiveCfg();
      RKADK_LOGD("SetLiveCfg done");

      SetDispCfg();
      RKADK_LOGD("SetDispCfg done");
    }

    if (strstr(cmd, "get")) {
      RKADK_LOGD("#Get config param");
      RKADK_PARAM_Init();
    }

    usleep(500000);
  }

  return 0;
}
