/*
 * rk_aiq_algo_agic_itf.c
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
#include "agic/rk_aiq_algo_agic_itf.h"
#include "agic/rk_aiq_types_algo_agic_prvt.h"

RKAIQ_BEGIN_DECLARE


static XCamReturn
create_context(RkAiqAlgoContext **context, const AlgoCtxInstanceCfg* cfg)
{
    XCamReturn result = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoContext *ctx = new RkAiqAlgoContext();
    if (ctx == NULL) {
        LOGE_AGIC( "%s: create agic context fail!\n", __FUNCTION__);
        return XCAM_RETURN_ERROR_MEM;
    }
    LOGI_AGIC("%s: (enter)\n", __FUNCTION__ );
    AgicInit(&ctx->agicCtx);
    *context = ctx;
    LOGI_AGIC("%s: (exit)\n", __FUNCTION__ );
    return result;

}

static XCamReturn
destroy_context(RkAiqAlgoContext *context)
{
    XCamReturn result = XCAM_RETURN_NO_ERROR;

    LOGI_AGIC("%s: (enter)\n", __FUNCTION__ );
    AgicContext_t* pAgicCtx = (AgicContext_t*)&context->agicCtx;
    AgicRelease(pAgicCtx);
    delete context;
    LOGI_AGIC("%s: (exit)\n", __FUNCTION__ );
    return result;

}

static XCamReturn
prepare(RkAiqAlgoCom* params)
{
    XCamReturn result = XCAM_RETURN_NO_ERROR;

    LOGI_AGIC("%s: (enter)\n", __FUNCTION__ );
    AgicContext_t* pAgicCtx = (AgicContext_t *)&params->ctx->agicCtx;
    RkAiqAlgoConfigAgicInt* pCfgParam = (RkAiqAlgoConfigAgicInt*)params;
    pAgicCtx->pCalibDb = pCfgParam->rk_com.u.prepare.calib;
    AgicStart(pAgicCtx);
    LOGI_AGIC("%s: (exit)\n", __FUNCTION__ );
    return result;

}

static XCamReturn
pre_process(const RkAiqAlgoCom* inparams, RkAiqAlgoResCom* outparams)
{
    return XCAM_RETURN_NO_ERROR;
}

static XCamReturn
processing(const RkAiqAlgoCom* inparams, RkAiqAlgoResCom* outparams)
{
    XCamReturn result = XCAM_RETURN_NO_ERROR;
    int iso = 50;
    RkAiqAlgoProcAgicInt* pAgicProcParams = (RkAiqAlgoProcAgicInt*)inparams;
    RkAiqAlgoProcResAgicInt* pAgicProcResParams = (RkAiqAlgoProcResAgicInt*)outparams;
    AgicContext_t* pAgicCtx = (AgicContext_t *)&inparams->ctx->agicCtx;

    LOGI_AGIC("%s: (enter)\n", __FUNCTION__ );

    RKAiqAecExpInfo_t* pAERes = NULL;
    if (inparams->u.prepare.ae_algo_id != 0) {
        pAERes = pAgicProcParams->agic_proc_com.com_ext.u.proc.curExp;
    } else {
        pAERes = pAgicProcParams->rk_com.u.proc.curExp;
    }

    if(pAERes != NULL) {
        if(pAgicProcParams->hdr_mode == RK_AIQ_WORKING_MODE_NORMAL) {
            iso = pAERes->LinearExp.exp_real_params.analog_gain * 50;
            LOGD_AGIC("%s:NORMAL:iso=%d,again=%f\n", __FUNCTION__, iso,
                      pAERes->LinearExp.exp_real_params.analog_gain);
        } else if(RK_AIQ_HDR_GET_WORKING_MODE(pAgicProcParams->hdr_mode) == RK_AIQ_WORKING_MODE_ISP_HDR2) {
            iso = pAERes->HdrExp[1].exp_real_params.analog_gain * 50;
            LOGD_AGIC("%s:HDR2:iso=%d,again=%f\n", __FUNCTION__, iso,
                      pAERes->HdrExp[1].exp_real_params.analog_gain);
        } else if(RK_AIQ_HDR_GET_WORKING_MODE(pAgicProcParams->hdr_mode) == RK_AIQ_WORKING_MODE_ISP_HDR3) {
            iso = pAERes->HdrExp[2].exp_real_params.analog_gain * 50;
            LOGD_AGIC("%s:HDR3:iso=%d,again=%f\n", __FUNCTION__, iso,
                      pAERes->HdrExp[2].exp_real_params.analog_gain);
        }
    } else {
        LOGE_AGIC("%s: pAERes is NULL, so use default instead \n", __FUNCTION__);
    }

    AgicProcess(pAgicCtx, iso);
    AgicGetProcResult(pAgicCtx, &pAgicProcResParams->gicRes);

    LOGI_AGIC("%s: (exit)\n", __FUNCTION__ );
    return result;

}

static XCamReturn
post_process(const RkAiqAlgoCom* inparams, RkAiqAlgoResCom* outparams)
{
    return XCAM_RETURN_NO_ERROR;
}

RkAiqAlgoDescription g_RkIspAlgoDescAgic = {
    .common = {
        .version = RKISP_ALGO_AGIC_VERSION,
        .vendor  = RKISP_ALGO_AGIC_VENDOR,
        .description = RKISP_ALGO_AGIC_DESCRIPTION,
        .type    = RK_AIQ_ALGO_TYPE_AGIC,
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
