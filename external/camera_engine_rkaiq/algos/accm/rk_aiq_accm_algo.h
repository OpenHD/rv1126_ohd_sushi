/*
 *rk_aiq_accm_algo.h
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

#ifndef _RK_AIQ_ACCM_ALGO_H_
#define _RK_AIQ_ACCM_ALGO_H_
#include "accm/rk_aiq_types_accm_algo_prvt.h"
#include "RkAiqCalibDbTypes.h"

RKAIQ_BEGIN_DECLARE

XCamReturn AccmInit(accm_handle_t *hAccm, const CamCalibDbContext_t* calib);
XCamReturn AccmRelease(accm_handle_t hAccm);
XCamReturn AccmPrepare(accm_handle_t hAccm);
XCamReturn AccmConfig(accm_handle_t hAccm);
XCamReturn AccmPreProc(accm_handle_t hAccm);
XCamReturn AccmProcessing(accm_handle_t hAccm);
XCamReturn illuminant_index_estimation_ccm(int light_num, const CalibDb_AccmCof_ill_t illAll[], float awbGain[2], int* illuminant_index);
XCamReturn CamCalibDbGetCcmProfileByName(const CalibDb_Ccm_ModeCell_t *calibCcm, char* name, const CalibDb_CcmMatrixProfile_t **pCcmMatrixProfile);
XCamReturn Swinfo_wbgain_init(float awbGain[2], const CalibDb_Ccm_ModeCell_t *pCalib, const char* illuName);
XCamReturn pCcmMatrixAll_init(accm_context_t* accm_context, const CalibDb_Ccm_ModeCell_t *pCalib, int idx);

RKAIQ_END_DECLARE

#endif

