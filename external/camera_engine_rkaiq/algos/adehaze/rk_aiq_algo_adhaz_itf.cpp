/*
 * rk_aiq_algo_adhaz_itf.c
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
/* for rockchip v2.0.0*/

#include "rk_aiq_algo_types_int.h"


#include "rk_aiq_algo_adhaz_itf.h"
#include "RkAiqCalibDbTypes.h"
#include "adehaze/rk_aiq_adehaze_algo.h"
#include "RkAiqCalibDbTypes.h"
#include "xcam_log.h"

RKAIQ_BEGIN_DECLARE

static XCamReturn
create_context(RkAiqAlgoContext **context, const AlgoCtxInstanceCfg* cfg)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    AdehazeHandle_t *AdehazeHandle = NULL;
    AlgoCtxInstanceCfgInt* instanc_int = (AlgoCtxInstanceCfgInt*)cfg;
    CamCalibDbContext_t* calib = instanc_int->calib;

    ret = AdehazeInit(&AdehazeHandle, calib);
    *context = (RkAiqAlgoContext *)(AdehazeHandle);
    return ret;
}

static XCamReturn
destroy_context(RkAiqAlgoContext *context)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    AdehazeHandle_t *AdehazeHandle = (AdehazeHandle_t *)context;

    ret = AdehazeRelease(AdehazeHandle);

    return ret;
}

static XCamReturn
prepare(RkAiqAlgoCom* params)
{

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoConfigAdhazInt* config = (RkAiqAlgoConfigAdhazInt*)params;
    AdehazeHandle_t * AdehazeHandle = (AdehazeHandle_t *)params->ctx;
    AdehazeHandle->working_mode = config->adhaz_config_com.com.u.prepare.working_mode;
    AdehazeHandle->prepare_type = params->u.prepare.conf_type;
    AdehazeHandle->width = config->rawWidth;
    AdehazeHandle->height = config->rawHeight;



    if(!!(params->u.prepare.conf_type & RK_AIQ_ALGO_CONFTYPE_UPDATECALIB )) {
        XCamReturn ret = AdehazeReloadPara(AdehazeHandle, config->rk_com.u.prepare.calib);
        if(ret != XCAM_RETURN_NO_ERROR) {
            ret = XCAM_RETURN_ERROR_FAILED;
            LOGE_ADPCC("%s: Adehaze Reload Para failed (%d)\n", __FUNCTION__, ret);
        }
    }

    return ret;
}

static XCamReturn
pre_process(const RkAiqAlgoCom* inparams, RkAiqAlgoResCom* outparams)
{
    RkAiqAlgoConfigAdhazInt* config = (RkAiqAlgoConfigAdhazInt*)inparams;
    AdehazeHandle_t * AdehazeHandle = (AdehazeHandle_t *)inparams->ctx;

    if (config->rk_com.u.proc.gray_mode)
        AdehazeHandle->Dehaze_Scene_mode = DEHAZE_NIGHT;
    else if (DEHAZE_NORMAL == AdehazeHandle->working_mode)
        AdehazeHandle->Dehaze_Scene_mode = DEHAZE_NORMAL;
    else
        AdehazeHandle->Dehaze_Scene_mode = DEHAZE_HDR;

    return XCAM_RETURN_NO_ERROR;
}

static XCamReturn
processing(const RkAiqAlgoCom* inparams, RkAiqAlgoResCom* outparams)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    int iso = 50;
    AdehazeHandle_t * AdehazeHandle = (AdehazeHandle_t *)inparams->ctx;
    RkAiqAlgoProcAdhazInt* procPara = (RkAiqAlgoProcAdhazInt*)inparams;
    RkAiqAlgoProcResAdhaz* procResPara = (RkAiqAlgoProcResAdhaz*)outparams;
    AdehazeExpInfo_t stExpInfo;
    memset(&stExpInfo, 0x00, sizeof(AdehazeExpInfo_t));

    stExpInfo.hdr_mode = 0;
    for(int i = 0; i < 3; i++) {
        stExpInfo.arIso[i] = 50;
        stExpInfo.arAGain[i] = 1.0;
        stExpInfo.arDGain[i] = 1.0;
        stExpInfo.arTime[i] = 0.01;
    }

    if(AdehazeHandle->working_mode == RK_AIQ_WORKING_MODE_NORMAL) {
        stExpInfo.hdr_mode = 0;
    } else if(RK_AIQ_HDR_GET_WORKING_MODE(AdehazeHandle->working_mode) == RK_AIQ_WORKING_MODE_ISP_HDR2) {
        stExpInfo.hdr_mode = 1;
    } else if(RK_AIQ_HDR_GET_WORKING_MODE(AdehazeHandle->working_mode) == RK_AIQ_WORKING_MODE_ISP_HDR3) {
        stExpInfo.hdr_mode = 2;
    }

    RKAiqAecExpInfo_t* pAERes = procPara->rk_com.u.proc.curExp;
    if (inparams->u.prepare.ae_algo_id != 0) {
        pAERes = procPara->adhaz_proc_com.com_ext.u.proc.curExp;
    } else {
        pAERes = procPara->rk_com.u.proc.curExp;
    }
    if(pAERes != NULL) {
        if(AdehazeHandle->working_mode == RK_AIQ_WORKING_MODE_NORMAL) {
            stExpInfo.arAGain[0] = pAERes->LinearExp.exp_real_params.analog_gain;
            stExpInfo.arDGain[0] = pAERes->LinearExp.exp_real_params.digital_gain;
            stExpInfo.arTime[0] = pAERes->LinearExp.exp_real_params.integration_time;
            stExpInfo.arIso[0] = stExpInfo.arAGain[0] * stExpInfo.arDGain[0] * 50;
        } else {
            for(int i = 0; i < 3; i++) {
                stExpInfo.arAGain[i] = pAERes->HdrExp[i].exp_real_params.analog_gain;
                stExpInfo.arDGain[i] = pAERes->HdrExp[i].exp_real_params.digital_gain;
                stExpInfo.arTime[i] = pAERes->HdrExp[i].exp_real_params.integration_time;
                stExpInfo.arIso[i] = stExpInfo.arAGain[i] * stExpInfo.arDGain[i] * 50;

                LOGD_ADEHAZE("index:%d again:%f dgain:%f time:%f iso:%d hdr_mode:%d\n",
                             i,
                             stExpInfo.arAGain[i],
                             stExpInfo.arDGain[i],
                             stExpInfo.arTime[i],
                             stExpInfo.arIso[i],
                             stExpInfo.hdr_mode);
            }
        }
    } else {
        LOGE_ADEHAZE("%s:%d pAERes is NULL, so use default instead \n", __FUNCTION__, __LINE__);
    }

    iso = stExpInfo.arIso[stExpInfo.hdr_mode];

    LOGD_ADEHAZE("hdr_mode=%d,iso=%d Scene mode =%d\n", stExpInfo.hdr_mode, iso, AdehazeHandle->Dehaze_Scene_mode);
    ret = AdehazeProcess(AdehazeHandle, iso, AdehazeHandle->Dehaze_Scene_mode);
    memcpy(&procResPara->adhaz_config, &AdehazeHandle->adhaz_config, sizeof(rk_aiq_dehaze_cfg_t));
    return ret;
}

static XCamReturn
post_process(const RkAiqAlgoCom* inparams, RkAiqAlgoResCom* outparams)
{
    return XCAM_RETURN_NO_ERROR;
}

RkAiqAlgoDescription g_RkIspAlgoDescAdhaz = {
    .common = {
        .version = RKISP_ALGO_ADHAZ_VERSION,
        .vendor  = RKISP_ALGO_ADHAZ_VENDOR,
        .description = RKISP_ALGO_ADHAZ_DESCRIPTION,
        .type    = RK_AIQ_ALGO_TYPE_ADHAZ,
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
