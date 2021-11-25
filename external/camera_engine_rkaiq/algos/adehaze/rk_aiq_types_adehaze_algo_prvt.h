/*
 *rk_aiq_types_adehaze_algo_prvt.h
 *
 *  Copyright (c) 2019 Rockchip Corporation
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
 *
 */

#ifndef _RK_AIQ_TYPES_ADEHAZE_ALGO_PRVT_H_
#define _RK_AIQ_TYPES_ADEHAZE_ALGO_PRVT_H_

#include "adehaze/rk_aiq_types_adehaze_algo_int.h"
#include "RkAiqCalibDbTypes.h"

#define DEHAZEBIGMODE     (2560)


typedef struct AdehazeHandle_s {
    rk_aiq_dehaze_cfg_t adhaz_config;
    CalibDb_Dehaze_t calib_dehaz;
    CamCalibDbContext_t* pCalibDb;
    int strength;
    int working_mode;
    int Dehaze_Scene_mode;
    adehaze_sw_t AdehazeAtrr;
    int prepare_type;
    uint32_t width;
    uint32_t height;
} AdehazeHandle_t;


#endif

