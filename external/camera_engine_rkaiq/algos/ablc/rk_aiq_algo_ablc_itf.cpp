/*
 * rk_aiq_algo_ablc_itf.c
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
#include "ablc/rk_aiq_algo_ablc_itf.h"
#include "ablc/rk_aiq_ablc_algo.h"

RKAIQ_BEGIN_DECLARE

typedef struct _RkAiqAlgoContext {
    void* place_holder[0];
} RkAiqAlgoContext;


static XCamReturn
create_context(RkAiqAlgoContext **context, const AlgoCtxInstanceCfg* cfg)
{   XCamReturn result = XCAM_RETURN_NO_ERROR;

    LOGI_ABLC("%s: (enter)\n", __FUNCTION__ );
    AlgoCtxInstanceCfgInt *cfgInt = (AlgoCtxInstanceCfgInt*)cfg;
#if 1
    AblcContext_t* pAblcCtx = NULL;
    AblcResult_t ret = AblcInit(&pAblcCtx, cfgInt->calib);
    if(ret != ABLC_RET_SUCCESS) {
        result = XCAM_RETURN_ERROR_FAILED;
        LOGE_ABLC("%s: Initializaion Ablc failed (%d)\n", __FUNCTION__, ret);
    } else {
        *context = (RkAiqAlgoContext *)(pAblcCtx);
    }
#endif

    LOGI_ABLC("%s: (exit)\n", __FUNCTION__ );
    return result;

}

static XCamReturn
destroy_context(RkAiqAlgoContext *context)
{
    XCamReturn result = XCAM_RETURN_NO_ERROR;

    LOGI_ABLC("%s: (enter)\n", __FUNCTION__ );

#if 1
    AblcContext_t* pAblcCtx = (AblcContext_t*)context;
    AblcResult_t ret = AblcRelease(pAblcCtx);
    if(ret != ABLC_RET_SUCCESS) {
        result = XCAM_RETURN_ERROR_FAILED;
        LOGE_ABLC("%s: release Ablc failed (%d)\n", __FUNCTION__, ret);
    }
#endif

    LOGI_ABLC("%s: (exit)\n", __FUNCTION__ );
    return result;

}

static XCamReturn
prepare(RkAiqAlgoCom* params)
{
    XCamReturn result = XCAM_RETURN_NO_ERROR;

    LOGI_ABLC("%s: (enter)\n", __FUNCTION__ );

#if 1
    AblcContext_t* pAblcCtx = (AblcContext_t *)params->ctx;
    RkAiqAlgoConfigAblcInt* pCfgParam = (RkAiqAlgoConfigAblcInt*)params;
    AblcConfig_t *pAblc_config = &pCfgParam->ablc_config;
	pAblcCtx->prepare_type = params->u.prepare.conf_type;

	if(!!(params->u.prepare.conf_type & RK_AIQ_ALGO_CONFTYPE_UPDATECALIB )){
        pAblcCtx->stBlcCalib = pCfgParam->rk_com.u.prepare.calib->blc;
		pAblcCtx->isIQParaUpdate = true;
    }

    AblcResult_t ret = AblcConfig(pAblcCtx, pAblc_config);
    if(ret != ABLC_RET_SUCCESS) {
        result = XCAM_RETURN_ERROR_FAILED;
        LOGE_ABLC("%s: config Ablc failed (%d)\n", __FUNCTION__, ret);
    }

#endif

    LOGI_ABLC("%s: (exit)\n", __FUNCTION__ );
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
    int iso;

    LOGI_ABLC("%s: (enter)\n", __FUNCTION__ );

#if 1
    RkAiqAlgoProcAblcInt* pAblcProcParams = (RkAiqAlgoProcAblcInt*)inparams;
    RkAiqAlgoProcResAblcInt* pAblcProcResParams = (RkAiqAlgoProcResAblcInt*)outparams;
    AblcContext_t* pAblcCtx = (AblcContext_t *)inparams->ctx;
    AblcExpInfo_t stExpInfo;
    memset(&stExpInfo, 0x00, sizeof(AblcExpInfo_t));

    LOGD_ABLC("%s:%d init:%d hdr mode:%d  \n",
              __FUNCTION__, __LINE__,
              inparams->u.proc.init,
              pAblcProcParams->hdr_mode);

    stExpInfo.hdr_mode = 0; //pAnrProcParams->hdr_mode;
    for(int i = 0; i < 3; i++) {
        stExpInfo.arIso[i] = 50;
        stExpInfo.arAGain[i] = 1.0;
        stExpInfo.arDGain[i] = 1.0;
        stExpInfo.arTime[i] = 0.01;
    }

    if(pAblcProcParams->hdr_mode == RK_AIQ_WORKING_MODE_NORMAL) {
        stExpInfo.hdr_mode = 0;
    } else if(pAblcProcParams->hdr_mode == RK_AIQ_ISP_HDR_MODE_2_FRAME_HDR
              || pAblcProcParams->hdr_mode == RK_AIQ_ISP_HDR_MODE_2_LINE_HDR ) {
        stExpInfo.hdr_mode = 1;
    } else if(pAblcProcParams->hdr_mode == RK_AIQ_ISP_HDR_MODE_3_FRAME_HDR
              || pAblcProcParams->hdr_mode == RK_AIQ_ISP_HDR_MODE_3_LINE_HDR ) {
        stExpInfo.hdr_mode = 2;
    }

#if 1
    RKAiqAecExpInfo_t *curExp = NULL;
    if (inparams->u.prepare.ae_algo_id != 0) {
        curExp = pAblcProcParams->ablc_proc_com.com_ext.u.proc.curExp;
    } else {
        curExp = pAblcProcParams->rk_com.u.proc.curExp;
    }
    if(curExp != NULL) {
        if(pAblcProcParams->hdr_mode == RK_AIQ_WORKING_MODE_NORMAL) {
            stExpInfo.arAGain[0] = curExp->LinearExp.exp_real_params.analog_gain;
            stExpInfo.arDGain[0] = curExp->LinearExp.exp_real_params.digital_gain;
            stExpInfo.arTime[0] = curExp->LinearExp.exp_real_params.integration_time;
            stExpInfo.arIso[0] = stExpInfo.arAGain[0] * stExpInfo.arDGain[0] * 50;
        } else {
            for(int i = 0; i < 3; i++) {
                stExpInfo.arAGain[i] = curExp->HdrExp[i].exp_real_params.analog_gain;
                stExpInfo.arDGain[i] = curExp->HdrExp[i].exp_real_params.digital_gain;
                stExpInfo.arTime[i] = curExp->HdrExp[i].exp_real_params.integration_time;
                stExpInfo.arIso[i] = stExpInfo.arAGain[i] * stExpInfo.arDGain[i] * 50;

                LOGD_ABLC("%s:%d index:%d again:%f dgain:%f time:%f iso:%d hdr_mode:%d\n",
                          __FUNCTION__, __LINE__,
                          i,
                          stExpInfo.arAGain[i],
                          stExpInfo.arDGain[i],
                          stExpInfo.arTime[i],
                          stExpInfo.arIso[i],
                          stExpInfo.hdr_mode);
            }
        }
    } else {
        LOGE_ABLC("%s:%d curExp is NULL, so use default instead \n", __FUNCTION__, __LINE__);
    }

#endif

    AblcResult_t ret = AblcProcess(pAblcCtx, &stExpInfo);
    if(ret != ABLC_RET_SUCCESS) {
        result = XCAM_RETURN_ERROR_FAILED;
        LOGE_ABLC("%s: processing ABLC failed (%d)\n", __FUNCTION__, ret);
    }

    AblcGetProcResult(pAblcCtx, &pAblcProcResParams->ablc_proc_res);
#endif

    LOGI_ABLC("%s: (exit)\n", __FUNCTION__ );
    return XCAM_RETURN_NO_ERROR;
}

static XCamReturn
post_process(const RkAiqAlgoCom* inparams, RkAiqAlgoResCom* outparams)
{
    return XCAM_RETURN_NO_ERROR;
}

RkAiqAlgoDescription g_RkIspAlgoDescAblc = {
    .common = {
        .version = RKISP_ALGO_ABLC_VERSION,
        .vendor  = RKISP_ALGO_ABLC_VENDOR,
        .description = RKISP_ALGO_ABLC_DESCRIPTION,
        .type    = RK_AIQ_ALGO_TYPE_ABLC,
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
