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

#ifndef _RK_AIQ_TYPES_ABLC_ALGO_PRVT_H_
#define _RK_AIQ_TYPES_ABLC_ALGO_PRVT_H_

#include "ablc/rk_aiq_types_ablc_algo_int.h"
#include "RkAiqCalibDbTypes.h"
#include "xcam_log.h"
#include "xcam_common.h"

RKAIQ_BEGIN_DECLARE

typedef struct AblcContext_s {
    AblcExpInfo_t stExpInfo;
    AblcState_t eState;
    AblcOPMode_t eMode;

    AblcAutoAttr_t stAuto;
    AblcManualAttr_t stManual;

    CalibDb_Blc_t stBlcCalib;
	AblcParamMode_t eParamMode;

	bool isIQParaUpdate;
	int prepare_type;
} AblcContext_t;



RKAIQ_END_DECLARE

#endif


