/*
 * rk_aiq_algo_awdr_itf.c
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
#include "awdr/rk_aiq_algo_awdr_itf.h"
#include "xcam_log.h"
#include "awdr/rk_aiq_awdr_algo.h"
#include "awdr/rk_aiq_types_awdr_algo_prvt.h"


RKAIQ_BEGIN_DECLARE

static XCamReturn
create_context(RkAiqAlgoContext **context, const AlgoCtxInstanceCfg* cfg)
{
    LOG1_AWDR("%s:Enter!\n", __FUNCTION__);
    RESULT ret = AWDR_RET_SUCCESS;
    AlgoCtxInstanceCfgInt *cfgInt = (AlgoCtxInstanceCfgInt*)cfg;
    RkAiqAlgoContext *ctx = new RkAiqAlgoContext();
    if (ctx == NULL) {
        LOGE_AWDR( "%s: create awdr context fail!\n", __FUNCTION__);
        return XCAM_RETURN_ERROR_MEM;
    }
    memset(&ctx->AwdrInstConfig, 0x00, sizeof(AwdrInstanceConfig_t));
    ret = AwdrInit(&ctx->AwdrInstConfig, cfgInt->calib);

    if (ret != AWDR_RET_SUCCESS) {
        LOGE_AWDR("%s AWDR Init failed: %d", __FUNCTION__, ret);
        return(XCAM_RETURN_ERROR_FAILED);
    }
    *context = ctx;

    LOG1_AWDR("%s:Exit!\n", __FUNCTION__);
    return XCAM_RETURN_NO_ERROR;
}

static XCamReturn
destroy_context(RkAiqAlgoContext *context)
{
    LOG1_AWDR("%s:Enter!\n", __FUNCTION__);
    RESULT ret = AWDR_RET_SUCCESS;

    if(context != NULL) {
        AwdrHandle_t pAwdrCtx = (AwdrHandle_t)context->AwdrInstConfig.hAwdr;
        ret = AwdrRelease(pAwdrCtx);
        if (ret != AWDR_RET_SUCCESS) {
            LOGE_AWDR("%s Awdr Release failed: %d", __FUNCTION__, ret);
            return(XCAM_RETURN_ERROR_FAILED);
        }
        delete context;
        context = NULL;
    }

    LOG1_AWDR("%s:Exit!\n", __FUNCTION__);
    return(XCAM_RETURN_NO_ERROR);
}

static XCamReturn
prepare(RkAiqAlgoCom* params)
{
    LOG1_AWDR("%s:Enter!\n", __FUNCTION__);
    RESULT ret = AWDR_RET_SUCCESS;

    AwdrHandle_t pAwdrCtx = params->ctx->AwdrInstConfig.hAwdr;
    RkAiqAlgoConfigAwdrInt* AwdrCfgParam = (RkAiqAlgoConfigAwdrInt*)params; //come from params in html
    const CamCalibDbContext_t* pCalibDb = AwdrCfgParam->rk_com.u.prepare.calib;

    if (AwdrCfgParam->working_mode < RK_AIQ_WORKING_MODE_ISP_HDR2)
        pAwdrCtx->FrameNumber = 1;
    else if (AwdrCfgParam->working_mode < RK_AIQ_WORKING_MODE_ISP_HDR3 &&
             AwdrCfgParam->working_mode >= RK_AIQ_WORKING_MODE_ISP_HDR2)
        pAwdrCtx->FrameNumber = 2;
    else
        pAwdrCtx->FrameNumber = 3;

    if(!!(params->u.prepare.conf_type & RK_AIQ_ALGO_CONFTYPE_UPDATECALIB )) {
        LOGD_AWDR("%s: Awdr Reload Para!\n", __FUNCTION__);
        memcpy(&pAwdrCtx->pCalibDB, &pCalibDb->awdr, sizeof(CalibDb_Awdr_Para_t));//load iq paras
    }

    if (ret != AWDR_RET_SUCCESS) {
        LOGE_AWDR("%s AWDR UpdateConfig failed: %d", __FUNCTION__, ret);
        return(XCAM_RETURN_ERROR_FAILED);
    }

    if(/* !params->u.prepare.reconfig*/true) {
        AwdrStop(pAwdrCtx); // stop firstly for re-preapre
        ret = AwdrStart(pAwdrCtx);
        if (ret != AWDR_RET_SUCCESS) {
            LOGE_AHDR("%s AWDR Start failed: %d", __FUNCTION__, ret);
            return(XCAM_RETURN_ERROR_FAILED);
        }
    }

    LOG1_AWDR("%s:Exit!\n", __FUNCTION__);
    return(XCAM_RETURN_NO_ERROR);
}

static XCamReturn
pre_process(const RkAiqAlgoCom* inparams, RkAiqAlgoResCom* outparams)
{
    LOG1_AWDR("%s:Enter!\n", __FUNCTION__);
    RESULT ret = AWDR_RET_SUCCESS;

    AwdrHandle_t pAwdrCtx = inparams->ctx->AwdrInstConfig.hAwdr;
    RkAiqAlgoConfigAwdrInt* AwdrCfgParam = (RkAiqAlgoConfigAwdrInt*)inparams;

    // sence mode
    if (AwdrCfgParam->rk_com.u.proc.gray_mode)
        pAwdrCtx->sence_mode = AWDR_NIGHT;
    else if (pAwdrCtx->FrameNumber == 1)
        pAwdrCtx->sence_mode = AWDR_NORMAL;
    else
        pAwdrCtx->sence_mode = AWDR_HDR;

    LOGD_AWDR("%s:Current mode:%d\n", __FUNCTION__, pAwdrCtx->sence_mode);
    if(pAwdrCtx->wdrAttr.mode == WDR_OPMODE_AUTO)
        AwdrUpdateConfig(pAwdrCtx, &pAwdrCtx->wdrAttr.stAuto, pAwdrCtx->sence_mode);
    else
        AwdrUpdateConfig(pAwdrCtx, &pAwdrCtx->pCalibDB, pAwdrCtx->sence_mode);

    LOG1_AWDR("%s:Exit!\n", __FUNCTION__);
    return(XCAM_RETURN_NO_ERROR);
}

static XCamReturn
processing(const RkAiqAlgoCom* inparams, RkAiqAlgoResCom* outparams)
{
    LOG1_AWDR("%s:Enter!\n", __FUNCTION__);
    RESULT ret = AWDR_RET_SUCCESS;

    AwdrHandle_t pAwdrCtx = (AwdrHandle_t)inparams->ctx->AwdrInstConfig.hAwdr;
    RkAiqAlgoProcAwdrInt* AwdrParams = (RkAiqAlgoProcAwdrInt*)inparams;
    RkAiqAlgoProcResAwdrInt* AwdrProcResParams = (RkAiqAlgoProcResAwdrInt*)outparams;
    // pAwdrCtx->frameCnt = inparams->frame_id;

    RkAiqAlgoPreResAeInt* ae_pre_res_int =
        (RkAiqAlgoPreResAeInt*)(AwdrParams->rk_com.u.proc.pre_res_comb->ae_pre_res);
    if (ae_pre_res_int)
        AwdrProcessing(pAwdrCtx, ae_pre_res_int->ae_pre_res_rk);
    else {
        AecPreResult_t AecHdrPreResult;
        LOGW_AWDR("%s: ae Pre result is null!!!\n", __FUNCTION__);
        AwdrProcessing(pAwdrCtx, AecHdrPreResult);
    }

    AwdrSetProcRes(&AwdrProcResParams->AwdrProcRes, &pAwdrCtx->ProcRes);

    LOG1_AWDR("%s:Exit!\n", __FUNCTION__);
    return(XCAM_RETURN_NO_ERROR);
}

static XCamReturn
post_process(const RkAiqAlgoCom* inparams, RkAiqAlgoResCom* outparams)
{
    return XCAM_RETURN_NO_ERROR;
}

RkAiqAlgoDescription g_RkIspAlgoDescAwdr = {
    .common = {
        .version = RKISP_ALGO_AWDR_VERSION,
        .vendor  = RKISP_ALGO_AWDR_VENDOR,
        .description = RKISP_ALGO_AWDR_DESCRIPTION,
        .type    = RK_AIQ_ALGO_TYPE_AWDR,
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
