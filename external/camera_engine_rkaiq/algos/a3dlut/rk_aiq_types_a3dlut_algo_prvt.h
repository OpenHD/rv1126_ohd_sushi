/*
 * rk_aiq_a3dlut_algo_prvt.h
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

#ifndef _RK_AIQ_A3DLUT_ALGO_PRVT_H_
#define _RK_AIQ_A3DLUT_ALGO_PRVT_H_

#include "RkAiqCalibDbTypes.h"
#include "a3dlut/rk_aiq_types_a3dlut_algo_int.h"
#include "xcam_log.h"
#include "xcam_common.h"

RKAIQ_BEGIN_DECLARE


typedef struct alut3d_context_s {
    const CalibDb_Lut3d_t *calib_lut3d;
    rk_aiq_lut3d_cfg_t lut3d_hw_conf;
    //ctrl & api
    rk_aiq_lut3d_attrib_t mCurAtt;
    rk_aiq_lut3d_attrib_t mNewAtt;
    bool updateAtt;
    int prepare_type;
} alut3d_context_t ;

typedef alut3d_context_t* alut3d_handle_t ;

typedef struct _RkAiqAlgoContext {
    void* place_holder[0];
    alut3d_handle_t a3dlut_para;
} RkAiqAlgoContext;


RKAIQ_END_DECLARE

#endif

