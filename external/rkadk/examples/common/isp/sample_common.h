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

#ifndef __SAMPLE_COMMON_H__
#define __SAMPLE_COMMON_H__

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#ifdef RKAIQ

#include <rk_aiq_user_api_imgproc.h>
#include <rk_aiq_user_api_sysctl.h>
/*
 * stream on:
 * 1) ISP Init
 * 2) ISP Run
 * 3) VI enable and stream on
 *
 * stream off:
 * 4) VI disable
 * 5) ISP Stop
 */
/*
typedef enum {
 RK_AIQ_WORKING_MODE_NORMAL,
 RK_AIQ_WORKING_MODE_ISP_HDR2    = 0x10,
 RK_AIQ_WORKING_MODE_ISP_HDR3    = 0x20,
 //RK_AIQ_WORKING_MODE_SENSOR_HDR = 10, // sensor built-in hdr mode
} rk_aiq_working_mode_t;
*/
int SAMPLE_COMM_ISP_Start(int CamId, rk_aiq_working_mode_t WDRMode,
                          bool MultiCam, const char *iq_file_dir, int fps);
int SAMPLE_COMM_ISP_Init(int CamId, rk_aiq_working_mode_t WDRMode,
                         bool MultiCam, const char *iq_file_dir);
int SAMPLE_COMM_ISP_UpdateIq(int CamId, char *iqfile);
int SAMPLE_COMM_ISP_Stop(int CamId);
int SAMPLE_COMM_ISP_SetFecEn(int CamId, bool bFECEnable);
int SAMPLE_COMM_ISP_SetFecBypass(int CamId, bool bypass);
int SAMPLE_COMM_ISP_GetFecAttrib(int CamId, rk_aiq_fec_attrib_t *attr);
int SAMPLE_COMM_ISP_Run(int CamId); // isp stop before vi streamoff
int SAMPLE_COMM_ISP_DumpExpInfo(int CamId, rk_aiq_working_mode_t WDRMode);
int SAMPLE_COMM_ISP_SetFrameRate(int CamId, unsigned int uFps);
int SAMPLE_COMM_ISP_EnableLdch(int CamId, bool on, unsigned int level);
int SAMPLE_COMM_ISP_GetLdchAttrib(int CamId, rk_aiq_ldch_attrib_t *attr);
int SAMPLE_COMM_ISP_SetWDRModeDyn(int CamId, rk_aiq_working_mode_t WDRMode);
int SAMPLE_COMM_ISP_SET_Brightness(int CamId, unsigned int value);
int SAMPLE_COMM_ISP_SET_Contrast(int CamId, unsigned int value);
int SAMPLE_COMM_ISP_SET_Saturation(int CamId, unsigned int value);
int SAMPLE_COMM_ISP_SET_Sharpness(int CamId, unsigned int value);
int SAMPLE_COMM_ISP_SET_ManualExposureAutoGain(int CamId,
                                               unsigned int u32Shutter);
int SAMPLE_COMM_ISP_SET_ManualExposureManualGain(int CamId,
                                                 unsigned int u32Shutter,
                                                 unsigned int u32Gain);
int SAMPLE_COMM_ISP_SET_AutoExposure(int CamId);
int SAMPLE_COMM_ISP_SET_Exposure(int CamId, bool bIsAutoExposure,
                                 unsigned int bIsAGC,
                                 unsigned int u32ElectronicShutter,
                                 unsigned int u32Agc);
int SAMPLE_COMM_ISP_SET_BackLight(int CamId, bool bEnable,
                                  unsigned int u32Strength);
int SAMPLE_COMM_ISP_SetDarkAreaBoostStrth(int CamId, unsigned int level);
int SAMPLE_COMM_ISP_GetDarkAreaBoostStrth(int CamId, unsigned int *level);
int SAMPLE_COMM_ISP_SET_LightInhibition(int CamId, bool bEnable,
                                        unsigned char u8Strength,
                                        unsigned char u8Level);
int SAMPLE_COMM_ISP_SET_CPSL_CFG(int CamId, rk_aiq_cpsl_cfg_t *cpsl);
int SAMPLE_COMM_ISP_SET_OpenColorCloseLed(int CamId);
int SAMPLE_COMM_ISP_SET_GrayOpenLed(int CamId, unsigned char u8Strength);
int SAMPLE_COMMON_ISP_SET_AutoWhiteBalance(int CamId);
int SAMPLE_COMMON_ISP_SET_ManualWhiteBalance(int CamId, unsigned int u32RGain,
                                             unsigned int u32GGain,
                                             unsigned int u32BGain);
int SAMPLE_COMMON_ISP_GET_WhiteBalanceGain(int CamId, rk_aiq_wb_gain_t *gain);
int SAMPLE_COMMON_ISP_SET_DNRStrength(int CamId, unsigned int u32Mode,
                                      unsigned int u322DValue,
                                      unsigned int u323Dvalue);
int SAMPLE_COMMON_ISP_GET_DNRStrength(int CamId, unsigned int *u322DValue,
                                      unsigned int *u323Dvalue);

int SAMPLE_COMMON_ISP_SET_Flicker(int CamId, unsigned char u32Flicker);
int SAMPLE_COMM_ISP_SET_HDR(int CamId, rk_aiq_working_mode_t mode);
int SAMPLE_COMM_ISP_EnableDefog(int CamId, bool on, opMode_t mode, unsigned int level);
int SAMPLE_COMM_ISP_GetMDhzStrth(int CamId, bool *on, unsigned int *level);
int SAMPLE_COMM_ISP_SET_Correction(int CamId, unsigned int u32Mode,
                                   unsigned int u32Value);
int SAMPLE_COMM_ISP_SET_MirrorFlip(int CamId, bool mirror, bool flip);
int SAMPLE_COMM_ISP_GET_MirrorFlip(int CamId, bool *mirror, bool *flip);

int SAMPLE_COMM_ISP_SET_BypassStreamRotation(int CamId, int S32Rotation);
int SAMPLE_COMM_ISP_SET_Crop(int CamId, rk_aiq_rect_t rect);
#endif

#endif
