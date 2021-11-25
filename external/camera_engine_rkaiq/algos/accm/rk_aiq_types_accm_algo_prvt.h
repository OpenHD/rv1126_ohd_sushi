/*
 *rk_aiq_types_accm_algo_prvt.h
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

#ifndef _RK_AIQ_TYPES_ACCM_ALGO_PRVT_H_
#define _RK_AIQ_TYPES_ACCM_ALGO_PRVT_H_
#include "RkAiqCalibDbTypes.h"
#include "accm/rk_aiq_types_accm_algo_int.h"
#include "xcam_log.h"
#include "xcam_common.h"
#include "list.h"


RKAIQ_BEGIN_DECLARE
#define CCM_CURVE_DOT_NUM 17


typedef struct accm_rest_s {
    float fSaturation;
    List dominateIlluList;//to record domain illuminant
    int dominateIlluProfileIdx;
    const CalibDb_CcmMatrixProfile_t *pCcmProfile1;
    const CalibDb_CcmMatrixProfile_t *pCcmProfile2;
    Cam3x3FloatMatrix_t undampedCcmMatrix;
    Cam3x3FloatMatrix_t dampedCcmMatrix;
    Cam1x3FloatMatrix_t undampedCcOffset;
    Cam1x3FloatMatrix_t dampedCcOffset;
    float color_inhibition_level;
    float color_saturation_level;
    CalibDb_CcmHdrNormalMode_t currentHdrNormalMode;
} accm_rest_t;

typedef struct illu_node_s {
    void*        p_next;       /**< for adding to a list */
    unsigned int value;
} illu_node_t;

typedef struct accm_context_s {
    const CalibDb_Ccm_t *calibCcm;//profile para
    const CalibDb_CcmMatrixProfile_t *pCcmMatrixAll[CCM_FOR_MODE_MAX][CCM_ILLUMINATION_MAX][CCM_PROFILES_NUM_MAX];// reorder para //to do, change to pointer
    accm_sw_info_t accmSwInfo;
    accm_rest_t accmRest;
    rk_aiq_ccm_cfg_t ccmHwConf; //hw para
    unsigned int count;
    //ctrl & api
    rk_aiq_ccm_attrib_t mCurAtt;
    rk_aiq_ccm_attrib_t mNewAtt;
    bool updateAtt;
    //char lsForFirstFrame[CALD_AWB_ILLUMINATION_NAME];
} accm_context_t ;

typedef accm_context_t* accm_handle_t ;

typedef struct _RkAiqAlgoContext {
    accm_handle_t accm_para;
} RkAiqAlgoContext;

RKAIQ_END_DECLARE

#endif

