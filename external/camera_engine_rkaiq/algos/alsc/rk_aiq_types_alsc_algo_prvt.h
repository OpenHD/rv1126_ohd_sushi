/*
 *rk_aiq_types_alsc_algo_prvt.h
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

#ifndef _RK_AIQ_TYPES_ALSC_ALGO_PRVT_H_
#define _RK_AIQ_TYPES_ALSC_ALGO_PRVT_H_

#include "alsc/rk_aiq_types_alsc_algo_int.h"
#include "RkAiqCalibDbTypes.h"
#include "xcam_log.h"
#include "xcam_common.h"
#include "list.h"

RKAIQ_BEGIN_DECLARE

typedef struct alsc_rest_s {
    int caseIndex;
    float fVignetting;
    List dominateIlluList;//to record domain illuminant
    int dominateIlluProfileIdx;
    int resIdx;
    const CalibDb_LscTableProfile_t *pLscProfile1;
    const CalibDb_LscTableProfile_t *pLscProfile2;
    CamLscMatrix_t undampedLscMatrixTable;
    CamLscMatrix_t dampedLscMatrixTable;
} alsc_rest_t;

typedef struct illu_node_s {
    void*        p_next;       /**< for adding to a list */
    unsigned int value;
} illu_node_t;

typedef const CalibDb_LscTableProfile_t* pLscTableProfile_t;
typedef pLscTableProfile_t*  pLscTableProfileVig_t;
typedef pLscTableProfileVig_t*  pLscTableProfileVigIll_t;
typedef pLscTableProfileVigIll_t*  pLscTableProfileVigIllRes_t;
typedef pLscTableProfileVigIllRes_t*  pLscTableProfileVigIllResUC_t;

typedef struct alsc_context_s {
    const CalibDb_Lsc_t *calibLsc;//profile para

    int _lscResNum_backup;
    int _illuNum_backup[USED_FOR_CASE_MAX];

    pLscTableProfileVigIllResUC_t pLscTableAll;// reorder para , const CalibDb_LscTableProfile_t *pLscTableAll[USED_FOR_CASE_MAX][LSC_RESOLUTIONS_NUM_MAX][LSC_ILLUMINATION_MAX][LSC_PROFILES_NUM_MAX];
    CalibDb_ResolutionName_t  curResName;
    alsc_sw_info_t alscSwInfo;
    alsc_rest_t alscRest;
    rk_aiq_lsc_cfg_t lscHwConf; //hw para
    unsigned int count;
    //ctrl & api
    rk_aiq_lsc_attrib_t mCurAtt;
    rk_aiq_lsc_attrib_t mNewAtt;
    bool updateAtt;
} alsc_context_t ;

typedef alsc_context_t* alsc_handle_t ;

typedef struct _RkAiqAlgoContext {
    alsc_handle_t alsc_para;
} RkAiqAlgoContext;


RKAIQ_END_DECLARE

#endif

