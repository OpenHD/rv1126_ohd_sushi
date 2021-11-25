/*
 * rk_aiq_algo_agamma_itf.c
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

#include "rk_aiq_algo_types_int.h"
#include "agamma/rk_aiq_algo_agamma_itf.h"
#include "agamma/rk_aiq_agamma_algo.h"
RKAIQ_BEGIN_DECLARE

typedef struct _RkAiqAlgoContext {
    void* place_holder[0];
} RkAiqAlgoContext;



static XCamReturn
create_context(RkAiqAlgoContext **context, const AlgoCtxInstanceCfg* cfg)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    AgammaHandle_t*AgammaHandle = NULL;
    AlgoCtxInstanceCfgInt* instanc_int = (AlgoCtxInstanceCfgInt*)cfg;
    CamCalibDbContext_t* calib = instanc_int->calib;
    ret = AgammaInit(&AgammaHandle, calib);
    *context = (RkAiqAlgoContext *)(AgammaHandle);
    return ret;
}

static XCamReturn
destroy_context(RkAiqAlgoContext *context)
{
    AgammaHandle_t*AgammaHandle = (AgammaHandle_t*)context;
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = AgammaRelease(AgammaHandle);

    return ret;
}

static XCamReturn
prepare(RkAiqAlgoCom* params)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    AgammaHandle_t * AgammaHandle = (AgammaHandle_t *)params->ctx;
    RkAiqAlgoConfigAgammaInt* pCfgParam = (RkAiqAlgoConfigAgammaInt*)params;
    rk_aiq_gamma_cfg_t *agamma_config = &AgammaHandle->agamma_config;
    AgammaHandle->working_mode = pCfgParam->agamma_config_com.com.u.prepare.working_mode;
    AgammaHandle->prepare_type = pCfgParam->agamma_config_com.com.u.prepare.conf_type;

    if(!!(AgammaHandle->prepare_type & RK_AIQ_ALGO_CONFTYPE_UPDATECALIB )) {
        AgammaHandle->pCalibDb = &pCfgParam->rk_com.u.prepare.calib->gamma;//reload iq
        LOGD_ADPCC("%s: Agamma Reload Para!!!\n", __FUNCTION__);
    }


    return ret;
}

static XCamReturn
pre_process(const RkAiqAlgoCom* inparams, RkAiqAlgoResCom* outparams)
{
    RkAiqAlgoPreAgammaInt* pAgammaPreParams = (RkAiqAlgoPreAgammaInt*)inparams;
    AgammaHandle_t * AgammaHandle = (AgammaHandle_t *)inparams->ctx;
    rk_aiq_gamma_cfg_t *agamma_config = &AgammaHandle->agamma_config;

    if (pAgammaPreParams->rk_com.u.proc.gray_mode)
        AgammaHandle->Scene_mode = GAMMA_OUT_NIGHT;
    else if (GAMMA_OUT_NORMAL == AgammaHandle->working_mode)
        AgammaHandle->Scene_mode = GAMMA_OUT_NORMAL;
    else
        AgammaHandle->Scene_mode = GAMMA_OUT_HDR;

    LOGD_AGAMMA(" %s: Gamma scene is:%d\n", __func__, AgammaHandle->Scene_mode);

    return XCAM_RETURN_NO_ERROR;
}

static XCamReturn
processing(const RkAiqAlgoCom* inparams, RkAiqAlgoResCom* outparams)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    AgammaHandle_t * AgammaHandle = (AgammaHandle_t *)inparams->ctx;
    RkAiqAlgoProcResAgamma* procResPara = (RkAiqAlgoProcResAgamma*)outparams;
    AgammaProcRes_t* AgammaProcRes = (AgammaProcRes_t*)&procResPara->agamma_proc_res;

    AgammaProcessing(AgammaHandle);

    //set proc res
    AgammaSetProcRes(AgammaProcRes, &AgammaHandle->agamma_config);
    return ret;
}

static XCamReturn
post_process(const RkAiqAlgoCom* inparams, RkAiqAlgoResCom* outparams)
{
    return XCAM_RETURN_NO_ERROR;
}

RkAiqAlgoDescription g_RkIspAlgoDescAgamma = {
    .common = {
        .version = RKISP_ALGO_AGAMMA_VERSION,
        .vendor  = RKISP_ALGO_AGAMMA_VENDOR,
        .description = RKISP_ALGO_AGAMMA_DESCRIPTION,
        .type    = RK_AIQ_ALGO_TYPE_AGAMMA,
        .id      = 0,
        .create_context  = create_context,
        .destroy_context = destroy_context,
    },
    .prepare = prepare,
    .pre_process = pre_process,
    .processing = processing,
    .post_process = post_process,
};

RKAIQ_END_DECLARE
