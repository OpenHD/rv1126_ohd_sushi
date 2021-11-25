/*
 * RkAiqHandle.h
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

#include "RkAiqHandle.h"
#include "RkAiqCore.h"

namespace RkCam {

RkAiqHandle::RkAiqHandle(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
    : mDes(des)
    , mAiqCore(aiqCore)
    , mEnable(true)
    , mReConfig(false)
{
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    mDes->create_context(&mAlgoCtx,
                         (const _AlgoCtxInstanceCfg*)(&shared->ctxCfigs[des->type]));
    mConfig = NULL;
    mPreInParam = NULL;
    mPreOutParam = NULL;
    mProcInParam = NULL;
    mProcOutParam = NULL;
    mPostInParam = NULL;
    mPostOutParam = NULL;
}

RkAiqHandle::~RkAiqHandle()
{
    if (mDes)
        mDes->destroy_context(mAlgoCtx);
}

XCamReturn RkAiqHandle::configInparamsCom(RkAiqAlgoCom* com, int type)
{
    ENTER_ANALYZER_FUNCTION();

    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    xcam_mem_clear(*com);

    if (type == RKAIQ_CONFIG_COM_PREPARE) {
        com->ctx = mAlgoCtx;
        com->frame_id = shared->frameId;
        com->u.prepare.working_mode = shared->working_mode;
        com->u.prepare.sns_op_width = shared->snsDes.isp_acq_width;
        com->u.prepare.sns_op_height = shared->snsDes.isp_acq_height;
        com->u.prepare.conf_type = shared->conf_type;
    } else {
        RkAiqHandle* handle =
            const_cast<RkAiqHandle*>(mAiqCore->getAiqAlgoHandle(RK_AIQ_ALGO_TYPE_AE));
        com->u.prepare.ae_algo_id = handle->getAlgoId();
        com->ctx = mAlgoCtx;
        com->frame_id = shared->frameId;
        com->u.proc.init = shared->init;

        RkAiqAlgoComExt* ext_com = NULL;
#define GET_EXT_PROC_COM(algo) \
        { \
            if (type == RKAIQ_CONFIG_COM_PROC) \
                ext_com = &(((RkAiqAlgoProc##algo *)com)->com_ext); \
        } \

#define GET_EXT_PRE_COM(algo) \
        { \
            if (type == RKAIQ_CONFIG_COM_PRE) \
                ext_com = &(((RkAiqAlgoPre##algo *)com)->com_ext); \
        } \

        switch (mDes->type) {
        case RK_AIQ_ALGO_TYPE_AE:
            GET_EXT_PROC_COM(Ae);
            break;
        case RK_AIQ_ALGO_TYPE_AFD:
            //GET_EXT_PROC_COM(Afd);
            break;
        case RK_AIQ_ALGO_TYPE_AWB:
            //GET_EXT_PROC_COM(Awb);
            break;
        case RK_AIQ_ALGO_TYPE_AF:
            //GET_EXT_PROC_COM(Af);
            break;
        case RK_AIQ_ALGO_TYPE_ABLC:
            GET_EXT_PROC_COM(Ablc);
            break;
        case RK_AIQ_ALGO_TYPE_ADPCC:
            GET_EXT_PROC_COM(Adpcc);
            break;
        case RK_AIQ_ALGO_TYPE_AHDR:
            GET_EXT_PROC_COM(Ahdr);
            break;
        case RK_AIQ_ALGO_TYPE_ANR:
            GET_EXT_PROC_COM(Anr);
            break;
        case RK_AIQ_ALGO_TYPE_ALSC:
            //GET_EXT_PROC_COM(Alsc);
            break;
        case RK_AIQ_ALGO_TYPE_AGIC:
            GET_EXT_PROC_COM(Agic);
            break;
        case RK_AIQ_ALGO_TYPE_ADEBAYER:
            GET_EXT_PROC_COM(Adebayer);
            break;
        case RK_AIQ_ALGO_TYPE_ACCM:
            //GET_EXT_PROC_COM(Accm);
            break;
        case RK_AIQ_ALGO_TYPE_AGAMMA:
            //GET_EXT_PROC_COM(Agamma);
            break;
        case RK_AIQ_ALGO_TYPE_ADEGAMMA:
            //GET_EXT_PROC_COM(Adegamma);
            break;
        case RK_AIQ_ALGO_TYPE_AWDR:
            //GET_EXT_PROC_COM(Awdr);
            break;
        case RK_AIQ_ALGO_TYPE_ADHAZ:
            GET_EXT_PROC_COM(Adhaz);
            break;
        case RK_AIQ_ALGO_TYPE_A3DLUT:
            //GET_EXT_PROC_COM(A3dlut);
            break;
        case RK_AIQ_ALGO_TYPE_ALDCH:
            //GET_EXT_PROC_COM(Aldch);
            break;
        case RK_AIQ_ALGO_TYPE_AR2Y:
            //GET_EXT_PROC_COM(Ar2y);
            break;
        case RK_AIQ_ALGO_TYPE_ACP:
            //GET_EXT_PROC_COM(Acp);
            break;
        case RK_AIQ_ALGO_TYPE_AIE:
            //GET_EXT_PROC_COM(Aie);
            break;
        case RK_AIQ_ALGO_TYPE_ASHARP:
            GET_EXT_PROC_COM(Asharp);
            break;
        case RK_AIQ_ALGO_TYPE_AORB:
            //GET_EXT_PROC_COM(Aorb);
            break;
        case RK_AIQ_ALGO_TYPE_AFEC:
            //GET_EXT_PROC_COM(Afec);
            break;
        case RK_AIQ_ALGO_TYPE_ACGC:
            //GET_EXT_PROC_COM(Acgc);
            break;
        case RK_AIQ_ALGO_TYPE_ASD:
            GET_EXT_PRE_COM(Asd);
            break;

        default:
            LOGE_ANALYZER("wrong algo type 0x%x, des type: %d !", type, mDes->type);
        }

        if (!ext_com)
            goto out;

        xcam_mem_clear(*ext_com);

        ext_com->u.proc.pre_res_comb = &shared->preResComb;
        ext_com->u.proc.proc_res_comb = &shared->procResComb;
        ext_com->u.proc.post_res_comb = &shared->postResComb;
        ext_com->u.proc.fill_light_on = shared->fill_light_on;
        ext_com->u.proc.gray_mode = shared->gray_mode;
        ext_com->u.proc.is_bw_sensor = shared->is_bw_sensor;
        ext_com->u.proc.iso = shared->iso;
        ext_com->u.proc.preExp = &shared->preExp;
        ext_com->u.proc.curExp = &shared->curExp;
    }
out:
    EXIT_ANALYZER_FUNCTION();

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqHandle::prepare()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    if (mConfig == NULL)
        init();
    // build common configs
    RkAiqAlgoCom* cfgParam = mConfig;
    configInparamsCom(cfgParam, RKAIQ_CONFIG_COM_PREPARE);

    EXIT_ANALYZER_FUNCTION();

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqHandle::preProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoCom* preParam = mPreInParam;

    configInparamsCom(preParam, RKAIQ_CONFIG_COM_PRE);

    EXIT_ANALYZER_FUNCTION();

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqHandle::processing()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoCom* procParam = mProcInParam;

    configInparamsCom(procParam, RKAIQ_CONFIG_COM_PROC);

    EXIT_ANALYZER_FUNCTION();

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqHandle::postProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoCom* postParam = mPostInParam;

    configInparamsCom(postParam, RKAIQ_CONFIG_COM_POST);

    EXIT_ANALYZER_FUNCTION();

    return XCAM_RETURN_NO_ERROR;
}

void
RkAiqHandle::deInit()
{
    ENTER_ANALYZER_FUNCTION();

#define RKAIQ_DELLET(a) \
    if (a) { \
        delete a; \
        a = NULL; \
    }

    RKAIQ_DELLET(mConfig);
    RKAIQ_DELLET(mPreInParam);
    RKAIQ_DELLET(mPreOutParam);
    RKAIQ_DELLET(mProcInParam);
    RKAIQ_DELLET(mProcOutParam);
    RKAIQ_DELLET(mPostInParam);
    RKAIQ_DELLET(mPostOutParam);

    EXIT_ANALYZER_FUNCTION();
}

void
RkAiqAeHandle::init()
{
    ENTER_ANALYZER_FUNCTION();

    deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAe());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAe());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAe());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAe());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAe());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAe());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAe());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAeHandle::prepare()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;

    ret = RkAiqHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "ae handle prepare failed");

    // TODO config ae common params:
    RkAiqAlgoConfigAe* ae_config = (RkAiqAlgoConfigAe*)mConfig;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;

    /*****************AecConfig Sensor Exp related info*****************/
    ae_config->LinePeriodsPerField = (float)shared->snsDes.frame_length_lines;
    ae_config->PixelPeriodsPerLine = (float)shared->snsDes.line_length_pck;
    ae_config->PixelClockFreqMHZ = (float) shared->snsDes.pixel_clock_freq_mhz;
    ae_config->nr_switch = shared->snsDes.nr_switch;

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->prepare(mConfig);
        RKAIQCORE_CHECK_RET(ret, "ae algo prepare failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAeHandle::preProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPreAe* ae_pre = (RkAiqAlgoPreAe*)mPreInParam;
    RkAiqAlgoPreResAe* ae_pre_res = (RkAiqAlgoPreResAe*)mPreOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;
    RkAiqPreResComb* comb = &shared->preResComb;

    ret = RkAiqHandle::preProcess();
    RKAIQCORE_CHECK_RET(ret, "ae handle preProcess failed");

    if (!ispStats->aec_stats_valid && !shared->init) {
        LOGE("no aec stats, ignore!");
        return XCAM_RETURN_BYPASS;
    }

    // TODO config common ae preprocess params
    ae_pre->ispAeStats = &ispStats->aec_stats;
    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->pre_process(mPreInParam, mPreOutParam);
        RKAIQCORE_CHECK_RET(ret, "ae handle pre_process failed");
        // set result to mAiqCore
        if (shared->mCustomAlgoRunningMode == CUSTOM_ALGO_RUNNING_MODE_SINGLE) {
            comb->ae_pre_res = (RkAiqAlgoPreResAe*)ae_pre_res;
        } else {
           // run after rk ae, replace rk ae result
            if (!comb->ae_pre_res) {
                LOGE("no rk ae pre res, failed !");
                return ret;
            }
            RkAiqAlgoPreResAeInt* pre_res_ae_rk = (RkAiqAlgoPreResAeInt*)(comb->ae_pre_res);
            pre_res_ae_rk->ae_pre_res_rk.LinearExp = ae_pre_res->ae_pre_res.LinearExp;
        }
        EXIT_ANALYZER_FUNCTION();
    }
    return ret;
}

XCamReturn
RkAiqAeHandle::processing()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoProcAe* ae_proc = (RkAiqAlgoProcAe*)mProcInParam;
    RkAiqAlgoProcResAe* ae_proc_res = (RkAiqAlgoProcResAe*)mProcOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;
    RkAiqProcResComb* comb = &shared->procResComb;

    ret = RkAiqHandle::processing();
    RKAIQCORE_CHECK_RET(ret, "ae handle processing failed");

    if (!ispStats->aec_stats_valid && !shared->init) {
        LOGE("no aec stats, ignore!");
        return XCAM_RETURN_BYPASS;
    }

    // TODO config common ae processing params
    ae_proc->ispAeStats = &ispStats->aec_stats;
    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->processing(mProcInParam, mProcOutParam);
        RKAIQCORE_CHECK_RET(ret, "ae algo processing failed");
        if (shared->mCustomAlgoRunningMode == CUSTOM_ALGO_RUNNING_MODE_SINGLE) {
            comb->ae_proc_res = (RkAiqAlgoProcResAe*)ae_proc_res;
        } else {
           // run after rk ae, replace rk ae result
            if (!comb->ae_proc_res) {
                LOGE("no rk ae proc res, failed !");
                return ret;
            }
            RkAiqAlgoProcResAeInt* proc_res_ae_rk = (RkAiqAlgoProcResAeInt*)(comb->ae_proc_res);
            proc_res_ae_rk->ae_proc_res_com = *ae_proc_res;
            proc_res_ae_rk->ae_proc_res_rk.exp_set_cnt = ae_proc_res->ae_proc_res.exp_set_cnt;
            proc_res_ae_rk->ae_proc_res_rk.exp_set_tbl[0] =
                        ae_proc_res->ae_proc_res.exp_set_tbl[0];
#if 0 // custom ae has no these result now
            memcpy(proc_res_ae_rk->ae_proc_res_rk.exp_set_tbl,
                   ae_proc_res->ae_proc_res.exp_set_tbl,
                   sizeof(proc_res_ae_rk->ae_proc_res_rk.exp_set_tbl));
            proc_res_ae_rk->ae_proc_res_rk.IsConverged = ae_proc_res->ae_proc_res.IsConverged;
            proc_res_ae_rk->ae_proc_res_rk.LumaDeviation = ae_proc_res->ae_proc_res.LumaDeviation;
            proc_res_ae_rk->ae_proc_res_rk.MeanLuma = ae_proc_res->ae_proc_res.MeanLuma;
#endif
        }
        EXIT_ANALYZER_FUNCTION();
    }
    return ret;
}

XCamReturn
RkAiqAeHandle::postProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPostAe* ae_post = (RkAiqAlgoPostAe*)mPostInParam;
    RkAiqAlgoPostResAe* ae_post_res = (RkAiqAlgoPostResAe*)mPostOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;
    RkAiqPostResComb* comb = &shared->postResComb;

    ret = RkAiqHandle::postProcess();
    RKAIQCORE_CHECK_RET(ret, "ae handle postProcess failed");

    if (!ispStats->aec_stats_valid && !shared->init) {
        LOGE("no aec stats, ignore!");
        return XCAM_RETURN_BYPASS;
    }

    // TODO config common ae postProcess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->post_process(mPostInParam, mPostOutParam);
        RKAIQCORE_CHECK_RET(ret, "ae algo postProcess failed");
        // set result to mAiqCore
        comb->ae_post_res = (RkAiqAlgoPostResAe*)ae_post_res;
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}
void
RkAiqAfdHandle::init()
{
    ENTER_ANALYZER_FUNCTION();

    deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAfd());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAfd());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAfd());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAfd());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAfd());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAfd());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAfd());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAfdHandle::prepare()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;

    ret = RkAiqHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "afd handle prepare failed");

    // TODO config ae common params:
    RkAiqAlgoConfigAfd* afd_config = (RkAiqAlgoConfigAfd*)mConfig;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;

    /*****************AfdConfig Sensor Exp related info*****************/
    afd_config->LinePeriodsPerField = (float)shared->snsDes.frame_length_lines;
    afd_config->PixelPeriodsPerLine = (float)shared->snsDes.line_length_pck;
    afd_config->PixelClockFreqMHZ = (float) shared->snsDes.pixel_clock_freq_mhz;

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->prepare(mConfig);
        RKAIQCORE_CHECK_RET(ret, "afd algo prepare failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}
XCamReturn
RkAiqAfdHandle::preProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPreAfd* afd_pre = (RkAiqAlgoPreAfd*)mPreInParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqHandle::preProcess();
    RKAIQCORE_CHECK_RET(ret, "afd handle preProcess failed");

    // TODO config common afd preprocess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->pre_process(mPreInParam, mPreOutParam);
        RKAIQCORE_CHECK_RET(ret, "afd handle pre_process failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfdHandle::processing()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoProcAfd* afd_proc = (RkAiqAlgoProcAfd*)mProcInParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqHandle::processing();
    RKAIQCORE_CHECK_RET(ret, "afd handle processing failed");

    // TODO config common afd processing params


    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->processing(mProcInParam, mProcOutParam);
        RKAIQCORE_CHECK_RET(ret, "afd algo processing failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfdHandle::postProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPostAfd* afd_post = (RkAiqAlgoPostAfd*)mPostInParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqHandle::postProcess();
    RKAIQCORE_CHECK_RET(ret, "afd handle postProcess failed");

    // TODO config common afd postProcess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->post_process(mPostInParam, mPostOutParam);
        RKAIQCORE_CHECK_RET(ret, "afd algo postProcess failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAwbHandle::init()
{
    ENTER_ANALYZER_FUNCTION();

    deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAwb());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAwb());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAwb());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAwb());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAwb());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAwb());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAwb());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAwbHandle::prepare()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;

    ret = RkAiqHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "awb handle prepare failed");

    // TODO config awb common params
    RkAiqAlgoConfigAwb* awb_config = (RkAiqAlgoConfigAwb*)mConfig;

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->prepare(mConfig);
        RKAIQCORE_CHECK_RET(ret, "awb algo prepare failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAwbHandle::preProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPreAwb* awb_pre = (RkAiqAlgoPreAwb*)mPreInParam;


    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqHandle::preProcess();
    RKAIQCORE_CHECK_RET(ret, "awb handle preProcess failed");

    if (!ispStats->awb_stats_valid && !shared->init) {
        LOGE("no awb stats, ignore!");
        return XCAM_RETURN_BYPASS;
    }
    if (!ispStats->awb_cfg_effect_valid && !shared->init) {
        LOGE("no effective awb cfg, ignore!");
        return XCAM_RETURN_BYPASS;
    }
    // TODO config common awb preprocess params


    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->pre_process(mPreInParam, mPreOutParam);
        RKAIQCORE_CHECK_RET(ret, "awb handle pre_process failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAwbHandle::processing()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoProcAwb* awb_pre = (RkAiqAlgoProcAwb*)mProcInParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqHandle::processing();
    RKAIQCORE_CHECK_RET(ret, "awb handle processing failed");

    if (!ispStats->awb_stats_valid && !shared->init) {
        LOGE("no awb stats, ignore!");
        return XCAM_RETURN_BYPASS;
    }
    if (!ispStats->awb_cfg_effect_valid && !shared->init) {
        LOGE("no effective awb cfg, ignore!");
        return XCAM_RETURN_BYPASS;
    }

    // TODO config common awb processing params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->processing(mProcInParam, mProcOutParam);
        RKAIQCORE_CHECK_RET(ret, "awb algo processing failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAwbHandle::postProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPostAwb* awb_pre = (RkAiqAlgoPostAwb*)mPostInParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqHandle::postProcess();
    RKAIQCORE_CHECK_RET(ret, "awb handle postProcess failed");

    if (!ispStats->awb_stats_valid && !shared->init) {
        LOGE("no awb stats, ignore!");
        return XCAM_RETURN_BYPASS;
    }
    if (!ispStats->awb_cfg_effect_valid && !shared->init) {
        LOGE("no effective awb cfg, ignore!");
        return XCAM_RETURN_BYPASS;
    }

    // TODO config common awb postProcess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->post_process(mPostInParam, mPostOutParam);
        RKAIQCORE_CHECK_RET(ret, "awb algo postProcess failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAfHandle::init()
{
    ENTER_ANALYZER_FUNCTION();

    deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAf());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAf());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAf());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAf());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAf());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAf());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAf());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAfHandle::prepare()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;

    ret = RkAiqHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "af handle prepare failed");

    // TODO config af common params
    RkAiqAlgoConfigAf* af_config = (RkAiqAlgoConfigAf*)mConfig;

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->prepare(mConfig);
        RKAIQCORE_CHECK_RET(ret, "af algo prepare failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfHandle::preProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPreAf* af_pre = (RkAiqAlgoPreAf*)mPreInParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqHandle::preProcess();
    RKAIQCORE_CHECK_RET(ret, "af handle preProcess failed");

    if (!ispStats->af_stats_valid && !shared->init) {
        LOGD("no af stats, ignore!");
        return XCAM_RETURN_BYPASS;
    }

    // TODO config common af preprocess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->pre_process(mPreInParam, mPreOutParam);
        RKAIQCORE_CHECK_RET(ret, "af handle pre_process failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfHandle::processing()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoProcAf* af_pre = (RkAiqAlgoProcAf*)mProcInParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqHandle::processing();
    RKAIQCORE_CHECK_RET(ret, "af handle processing failed");

    if (!ispStats->af_stats_valid && !shared->init) {
        LOGD("no af stats, ignore!");
        return XCAM_RETURN_BYPASS;
    }

    // TODO config common af processing params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->processing(mProcInParam, mProcOutParam);
        RKAIQCORE_CHECK_RET(ret, "af algo processing failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfHandle::postProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPostAf* af_pre = (RkAiqAlgoPostAf*)mPostInParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqHandle::postProcess();
    RKAIQCORE_CHECK_RET(ret, "af handle postProcess failed");

    if (!ispStats->af_stats_valid && !shared->init) {
        LOGD("no af stats, ignore!");
        return XCAM_RETURN_BYPASS;
    }

    // TODO config common af postProcess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->post_process(mPostInParam, mPostOutParam);
        RKAIQCORE_CHECK_RET(ret, "af algo postProcess failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAhdrHandle::init()
{
    ENTER_ANALYZER_FUNCTION();

    deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAhdr());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAhdr());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAhdr());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAhdr());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAhdr());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAhdr());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAhdr());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAhdrHandle::prepare()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;

    ret = RkAiqHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "ahdr handle prepare failed");

    // TODO config ahdr common params
    RkAiqAlgoConfigAhdr* ahdr_config = (RkAiqAlgoConfigAhdr*)mConfig;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->prepare(mConfig);
        RKAIQCORE_CHECK_RET(ret, "ahdr algo prepare failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAhdrHandle::preProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPreAhdr* ahdr_pre = (RkAiqAlgoPreAhdr*)mPreInParam;

    ret = RkAiqHandle::preProcess();
    RKAIQCORE_CHECK_RET(ret, "ahdr handle preProcess failed");

    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;

    if(!shared->ispStats.ahdr_stats_valid && !shared->init) {
        LOGD("no ahdr stats, ignore!");
        // TODO: keep last result ?
        //         comb->ahdr_proc_res = NULL;
        //
        return XCAM_RETURN_BYPASS;
    }
    // TODO config common ahdr preprocess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->pre_process(mPreInParam, mPreOutParam);
        RKAIQCORE_CHECK_RET(ret, "ahdr handle pre_process failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAhdrHandle::processing()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoProcAhdr* ahdr_pre = (RkAiqAlgoProcAhdr*)mProcInParam;

    ret = RkAiqHandle::processing();
    RKAIQCORE_CHECK_RET(ret, "ahdr handle processing failed");

    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;

    if(!shared->ispStats.ahdr_stats_valid && !shared->init) {
        LOGD("no ahdr stats, ignore!");
        // TODO: keep last result ?
        //         comb->ahdr_proc_res = NULL;
        //
        return XCAM_RETURN_BYPASS;
    }
    // TODO config common ahdr processing params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->processing(mProcInParam, mProcOutParam);
        RKAIQCORE_CHECK_RET(ret, "ahdr algo processing failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAhdrHandle::postProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPostAhdr* ahdr_pre = (RkAiqAlgoPostAhdr*)mPostInParam;

    ret = RkAiqHandle::postProcess();
    RKAIQCORE_CHECK_RET(ret, "ahdr handle postProcess failed");

    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;

    if(!shared->ispStats.ahdr_stats_valid && !shared->init) {
        LOGD("no ahdr stats, ignore!");
        // TODO: keep last result ?
        //         comb->ahdr_proc_res = NULL;
        //
        return XCAM_RETURN_BYPASS;
    }
    // TODO config common ahdr postProcess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->post_process(mPostInParam, mPostOutParam);
        RKAIQCORE_CHECK_RET(ret, "ahdr algo postProcess failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAnrHandle::init()
{
    ENTER_ANALYZER_FUNCTION();

    deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAnr());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAnr());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAnr());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAnr());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAnr());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAnr());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAnr());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAnrHandle::prepare()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;

    ret = RkAiqHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "anr handle prepare failed");

    // TODO config anr common params
    RkAiqAlgoConfigAnr* anr_config = (RkAiqAlgoConfigAnr*)mConfig;

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->prepare(mConfig);
        RKAIQCORE_CHECK_RET(ret, "anr algo prepare failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAnrHandle::preProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPreAnr* anr_pre = (RkAiqAlgoPreAnr*)mPreInParam;

    ret = RkAiqHandle::preProcess();
    RKAIQCORE_CHECK_RET(ret, "anr handle preProcess failed");

    // TODO config common anr preprocess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->pre_process(mPreInParam, mPreOutParam);
        RKAIQCORE_CHECK_RET(ret, "anr handle pre_process failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAnrHandle::processing()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoProcAnr* anr_pre = (RkAiqAlgoProcAnr*)mProcInParam;

    ret = RkAiqHandle::processing();
    RKAIQCORE_CHECK_RET(ret, "anr handle processing failed");

    // TODO config common anr processing params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->processing(mProcInParam, mProcOutParam);
        RKAIQCORE_CHECK_RET(ret, "anr algo processing failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAnrHandle::postProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPostAnr* anr_pre = (RkAiqAlgoPostAnr*)mPostInParam;

    ret = RkAiqHandle::postProcess();
    RKAIQCORE_CHECK_RET(ret, "anr handle postProcess failed");

    // TODO config common anr postProcess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->post_process(mPostInParam, mPostOutParam);
        RKAIQCORE_CHECK_RET(ret, "anr algo postProcess failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAlscHandle::init()
{
    ENTER_ANALYZER_FUNCTION();

    deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAlsc());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAlsc());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAlsc());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAlsc());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAlsc());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAlsc());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAlsc());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAlscHandle::prepare()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;

    ret = RkAiqHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "alsc handle prepare failed");

    // TODO config alsc common params
    RkAiqAlgoConfigAlsc* alsc_config = (RkAiqAlgoConfigAlsc*)mConfig;

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->prepare(mConfig);
        RKAIQCORE_CHECK_RET(ret, "alsc algo prepare failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAlscHandle::preProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPreAlsc* alsc_pre = (RkAiqAlgoPreAlsc*)mPreInParam;

    ret = RkAiqHandle::preProcess();
    RKAIQCORE_CHECK_RET(ret, "alsc handle preProcess failed");

    // TODO config common alsc preprocess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->pre_process(mPreInParam, mPreOutParam);
        RKAIQCORE_CHECK_RET(ret, "alsc handle pre_process failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAlscHandle::processing()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoProcAlsc* alsc_pre = (RkAiqAlgoProcAlsc*)mProcInParam;

    ret = RkAiqHandle::processing();
    RKAIQCORE_CHECK_RET(ret, "alsc handle processing failed");

    // TODO config common alsc processing params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->processing(mProcInParam, mProcOutParam);
        RKAIQCORE_CHECK_RET(ret, "alsc algo processing failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAlscHandle::postProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPostAlsc* alsc_pre = (RkAiqAlgoPostAlsc*)mPostInParam;

    ret = RkAiqHandle::postProcess();
    RKAIQCORE_CHECK_RET(ret, "alsc handle postProcess failed");

    // TODO config common alsc postProcess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->post_process(mPostInParam, mPostOutParam);
        RKAIQCORE_CHECK_RET(ret, "alsc algo postProcess failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAsharpHandle::init()
{
    ENTER_ANALYZER_FUNCTION();

    deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAsharp());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAsharp());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAsharp());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAsharp());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAsharp());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAsharp());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAsharp());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAsharpHandle::prepare()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;

    ret = RkAiqHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "asharp handle prepare failed");

    // TODO config asharp common params
    RkAiqAlgoConfigAsharp* asharp_config = (RkAiqAlgoConfigAsharp*)mConfig;

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->prepare(mConfig);
        RKAIQCORE_CHECK_RET(ret, "asharp algo prepare failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAsharpHandle::preProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPreAsharp* asharp_pre = (RkAiqAlgoPreAsharp*)mPreInParam;

    ret = RkAiqHandle::preProcess();
    RKAIQCORE_CHECK_RET(ret, "asharp handle preProcess failed");

    // TODO config common asharp preprocess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->pre_process(mPreInParam, mPreOutParam);
        RKAIQCORE_CHECK_RET(ret, "asharp handle pre_process failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAsharpHandle::processing()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoProcAsharp* asharp_pre = (RkAiqAlgoProcAsharp*)mProcInParam;

    ret = RkAiqHandle::processing();
    RKAIQCORE_CHECK_RET(ret, "asharp handle processing failed");

    // TODO config common asharp processing params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->processing(mProcInParam, mProcOutParam);
        RKAIQCORE_CHECK_RET(ret, "asharp algo processing failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAsharpHandle::postProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPostAsharp* asharp_pre = (RkAiqAlgoPostAsharp*)mPostInParam;

    ret = RkAiqHandle::postProcess();
    RKAIQCORE_CHECK_RET(ret, "asharp handle postProcess failed");

    // TODO config common asharp postProcess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->post_process(mPostInParam, mPostOutParam);
        RKAIQCORE_CHECK_RET(ret, "asharp algo postProcess failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAdhazHandle::init()
{
    ENTER_ANALYZER_FUNCTION();

    deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAdhaz());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAdhaz());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAdhaz());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAdhaz());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAdhaz());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAdhaz());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAdhaz());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAdhazHandle::prepare()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;

    ret = RkAiqHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "adhaz handle prepare failed");

    // TODO config adhaz common params
    RkAiqAlgoConfigAdhaz* adhaz_config = (RkAiqAlgoConfigAdhaz*)mConfig;

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->prepare(mConfig);
        RKAIQCORE_CHECK_RET(ret, "adhaz algo prepare failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAdhazHandle::preProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPreAdhaz* adhaz_pre = (RkAiqAlgoPreAdhaz*)mPreInParam;

    ret = RkAiqHandle::preProcess();
    RKAIQCORE_CHECK_RET(ret, "adhaz handle preProcess failed");

    // TODO config common adhaz preprocess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->pre_process(mPreInParam, mPreOutParam);
        RKAIQCORE_CHECK_RET(ret, "adhaz handle pre_process failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAdhazHandle::processing()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoProcAdhaz* adhaz_pre = (RkAiqAlgoProcAdhaz*)mProcInParam;

    ret = RkAiqHandle::processing();
    RKAIQCORE_CHECK_RET(ret, "adhaz handle processing failed");

    // TODO config common adhaz processing params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->processing(mProcInParam, mProcOutParam);
        RKAIQCORE_CHECK_RET(ret, "adhaz algo processing failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAdhazHandle::postProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPostAdhaz* adhaz_pre = (RkAiqAlgoPostAdhaz*)mPostInParam;

    ret = RkAiqHandle::postProcess();
    RKAIQCORE_CHECK_RET(ret, "adhaz handle postProcess failed");

    // TODO config common adhaz postProcess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->post_process(mPostInParam, mPostOutParam);
        RKAIQCORE_CHECK_RET(ret, "adhaz algo postProcess failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAsdHandle::init()
{
    ENTER_ANALYZER_FUNCTION();

    deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAsd());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAsd());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAsd());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAsd());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAsd());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAsd());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAsd());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAsdHandle::prepare()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;

    ret = RkAiqHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "asd handle prepare failed");

    // TODO config asd common params
    RkAiqAlgoConfigAsd* asd_config = (RkAiqAlgoConfigAsd*)mConfig;

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->prepare(mConfig);
        RKAIQCORE_CHECK_RET(ret, "asd algo prepare failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAsdHandle::preProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPreAsd* asd_pre = (RkAiqAlgoPreAsd*)mPreInParam;

    ret = RkAiqHandle::preProcess();
    RKAIQCORE_CHECK_RET(ret, "asd handle preProcess failed");

    // TODO config common asd preprocess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->pre_process(mPreInParam, mPreOutParam);
        RKAIQCORE_CHECK_RET(ret, "asd handle pre_process failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAsdHandle::processing()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoProcAsd* asd_pre = (RkAiqAlgoProcAsd*)mProcInParam;

    ret = RkAiqHandle::processing();
    RKAIQCORE_CHECK_RET(ret, "asd handle processing failed");

    // TODO config common asd processing params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->processing(mProcInParam, mProcOutParam);
        RKAIQCORE_CHECK_RET(ret, "asd algo processing failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAsdHandle::postProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPostAsd* asd_pre = (RkAiqAlgoPostAsd*)mPostInParam;

    ret = RkAiqHandle::postProcess();
    RKAIQCORE_CHECK_RET(ret, "asd handle postProcess failed");

    // TODO config common asd postProcess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->post_process(mPostInParam, mPostOutParam);
        RKAIQCORE_CHECK_RET(ret, "asd algo postProcess failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAcpHandle::init()
{
    ENTER_ANALYZER_FUNCTION();

    deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAcp());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAcp());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAcp());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAcp());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAcp());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAcp());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAcp());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAcpHandle::prepare()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;

    ret = RkAiqHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "acp handle prepare failed");

    // TODO config acp common params
    RkAiqAlgoConfigAcp* acp_config = (RkAiqAlgoConfigAcp*)mConfig;

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->prepare(mConfig);
        RKAIQCORE_CHECK_RET(ret, "acp algo prepare failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAcpHandle::preProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPreAcp* acp_pre = (RkAiqAlgoPreAcp*)mPreInParam;

    ret = RkAiqHandle::preProcess();
    RKAIQCORE_CHECK_RET(ret, "acp handle preProcess failed");

    // TODO config common acp preprocess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->pre_process(mPreInParam, mPreOutParam);
        RKAIQCORE_CHECK_RET(ret, "acp handle pre_process failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAcpHandle::processing()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoProcAcp* acp_pre = (RkAiqAlgoProcAcp*)mProcInParam;

    ret = RkAiqHandle::processing();
    RKAIQCORE_CHECK_RET(ret, "acp handle processing failed");

    // TODO config common acp processing params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->processing(mProcInParam, mProcOutParam);
        RKAIQCORE_CHECK_RET(ret, "acp algo processing failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAcpHandle::postProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPostAcp* acp_pre = (RkAiqAlgoPostAcp*)mPostInParam;

    ret = RkAiqHandle::postProcess();
    RKAIQCORE_CHECK_RET(ret, "acp handle postProcess failed");

    // TODO config common acp postProcess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->post_process(mPostInParam, mPostOutParam);
        RKAIQCORE_CHECK_RET(ret, "acp algo postProcess failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqA3dlutHandle::init()
{
    ENTER_ANALYZER_FUNCTION();

    deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigA3dlut());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreA3dlut());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResA3dlut());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcA3dlut());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResA3dlut());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostA3dlut());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResA3dlut());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqA3dlutHandle::prepare()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;

    ret = RkAiqHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "a3dlut handle prepare failed");

    // TODO config a3dlut common params
    RkAiqAlgoConfigA3dlut* a3dlut_config = (RkAiqAlgoConfigA3dlut*)mConfig;

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->prepare(mConfig);
        RKAIQCORE_CHECK_RET(ret, "a3dlut algo prepare failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqA3dlutHandle::preProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPreA3dlut* a3dlut_pre = (RkAiqAlgoPreA3dlut*)mPreInParam;

    ret = RkAiqHandle::preProcess();
    RKAIQCORE_CHECK_RET(ret, "a3dlut handle preProcess failed");

    // TODO config common a3dlut preprocess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->pre_process(mPreInParam, mPreOutParam);
        RKAIQCORE_CHECK_RET(ret, "a3dlut handle pre_process failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqA3dlutHandle::processing()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoProcA3dlut* a3dlut_pre = (RkAiqAlgoProcA3dlut*)mProcInParam;

    ret = RkAiqHandle::processing();
    RKAIQCORE_CHECK_RET(ret, "a3dlut handle processing failed");

    // TODO config common a3dlut processing params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->processing(mProcInParam, mProcOutParam);
        RKAIQCORE_CHECK_RET(ret, "a3dlut algo processing failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqA3dlutHandle::postProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPostA3dlut* a3dlut_pre = (RkAiqAlgoPostA3dlut*)mPostInParam;

    ret = RkAiqHandle::postProcess();
    RKAIQCORE_CHECK_RET(ret, "a3dlut handle postProcess failed");

    // TODO config common a3dlut postProcess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->post_process(mPostInParam, mPostOutParam);
        RKAIQCORE_CHECK_RET(ret, "a3dlut algo postProcess failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAblcHandle::init()
{
    ENTER_ANALYZER_FUNCTION();

    deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAblc());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAblc());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAblc());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAblc());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAblc());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAblc());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAblc());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAblcHandle::prepare()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;

    ret = RkAiqHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "ablc handle prepare failed");

    // TODO config ablc common params
    RkAiqAlgoConfigAblc* ablc_config = (RkAiqAlgoConfigAblc*)mConfig;

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->prepare(mConfig);
        RKAIQCORE_CHECK_RET(ret, "ablc algo prepare failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAblcHandle::preProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPreAblc* ablc_pre = (RkAiqAlgoPreAblc*)mPreInParam;

    ret = RkAiqHandle::preProcess();
    RKAIQCORE_CHECK_RET(ret, "ablc handle preProcess failed");

    // TODO config common ablc preprocess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->pre_process(mPreInParam, mPreOutParam);
        RKAIQCORE_CHECK_RET(ret, "ablc handle pre_process failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAblcHandle::processing()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoProcAblc* ablc_pre = (RkAiqAlgoProcAblc*)mProcInParam;

    ret = RkAiqHandle::processing();
    RKAIQCORE_CHECK_RET(ret, "ablc handle processing failed");

    // TODO config common ablc processing params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->processing(mProcInParam, mProcOutParam);
        RKAIQCORE_CHECK_RET(ret, "ablc algo processing failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAblcHandle::postProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPostAblc* ablc_pre = (RkAiqAlgoPostAblc*)mPostInParam;

    ret = RkAiqHandle::postProcess();
    RKAIQCORE_CHECK_RET(ret, "ablc handle postProcess failed");

    // TODO config common ablc postProcess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->post_process(mPostInParam, mPostOutParam);
        RKAIQCORE_CHECK_RET(ret, "ablc algo postProcess failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAccmHandle::init()
{
    ENTER_ANALYZER_FUNCTION();

    deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAccm());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAccm());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAccm());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAccm());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAccm());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAccm());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAccm());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAccmHandle::prepare()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;

    ret = RkAiqHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "accm handle prepare failed");

    // TODO config accm common params
    RkAiqAlgoConfigAccm* accm_config = (RkAiqAlgoConfigAccm*)mConfig;

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->prepare(mConfig);
        RKAIQCORE_CHECK_RET(ret, "accm algo prepare failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAccmHandle::preProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPreAccm* accm_pre = (RkAiqAlgoPreAccm*)mPreInParam;

    ret = RkAiqHandle::preProcess();
    RKAIQCORE_CHECK_RET(ret, "accm handle preProcess failed");

    // TODO config common accm preprocess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->pre_process(mPreInParam, mPreOutParam);
        RKAIQCORE_CHECK_RET(ret, "accm handle pre_process failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAccmHandle::processing()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoProcAccm* accm_pre = (RkAiqAlgoProcAccm*)mProcInParam;

    ret = RkAiqHandle::processing();
    RKAIQCORE_CHECK_RET(ret, "accm handle processing failed");

    // TODO config common accm processing params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->processing(mProcInParam, mProcOutParam);
        RKAIQCORE_CHECK_RET(ret, "accm algo processing failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAccmHandle::postProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPostAccm* accm_pre = (RkAiqAlgoPostAccm*)mPostInParam;

    ret = RkAiqHandle::postProcess();
    RKAIQCORE_CHECK_RET(ret, "accm handle postProcess failed");

    // TODO config common accm postProcess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->post_process(mPostInParam, mPostOutParam);
        RKAIQCORE_CHECK_RET(ret, "accm algo postProcess failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAcgcHandle::init()
{
    ENTER_ANALYZER_FUNCTION();

    deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAcgc());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAcgc());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAcgc());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAcgc());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAcgc());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAcgc());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAcgc());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAcgcHandle::prepare()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;

    ret = RkAiqHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "acgc handle prepare failed");

    // TODO config acgc common params
    RkAiqAlgoConfigAcgc* acgc_config = (RkAiqAlgoConfigAcgc*)mConfig;

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->prepare(mConfig);
        RKAIQCORE_CHECK_RET(ret, "acgc algo prepare failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAcgcHandle::preProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPreAcgc* acgc_pre = (RkAiqAlgoPreAcgc*)mPreInParam;

    ret = RkAiqHandle::preProcess();
    RKAIQCORE_CHECK_RET(ret, "acgc handle preProcess failed");

    // TODO config common acgc preprocess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->pre_process(mPreInParam, mPreOutParam);
        RKAIQCORE_CHECK_RET(ret, "acgc handle pre_process failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAcgcHandle::processing()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoProcAcgc* acgc_pre = (RkAiqAlgoProcAcgc*)mProcInParam;

    ret = RkAiqHandle::processing();
    RKAIQCORE_CHECK_RET(ret, "acgc handle processing failed");

    // TODO config common acgc processing params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->processing(mProcInParam, mProcOutParam);
        RKAIQCORE_CHECK_RET(ret, "acgc algo processing failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAcgcHandle::postProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPostAcgc* acgc_pre = (RkAiqAlgoPostAcgc*)mPostInParam;

    ret = RkAiqHandle::postProcess();
    RKAIQCORE_CHECK_RET(ret, "acgc handle postProcess failed");

    // TODO config common acgc postProcess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->post_process(mPostInParam, mPostOutParam);
        RKAIQCORE_CHECK_RET(ret, "acgc algo postProcess failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAdebayerHandle::init()
{
    ENTER_ANALYZER_FUNCTION();

    deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAdebayer());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAdebayer());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAdebayer());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAdebayer());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAdebayer());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAdebayer());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAdebayer());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAdebayerHandle::prepare()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;

    ret = RkAiqHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "adebayer handle prepare failed");

    // TODO config adebayer common params
    RkAiqAlgoConfigAdebayer* adebayer_config = (RkAiqAlgoConfigAdebayer*)mConfig;

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->prepare(mConfig);
        RKAIQCORE_CHECK_RET(ret, "adebayer algo prepare failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAdebayerHandle::preProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPreAdebayer* adebayer_pre = (RkAiqAlgoPreAdebayer*)mPreInParam;

    ret = RkAiqHandle::preProcess();
    RKAIQCORE_CHECK_RET(ret, "adebayer handle preProcess failed");

    // TODO config common adebayer preprocess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->pre_process(mPreInParam, mPreOutParam);
        RKAIQCORE_CHECK_RET(ret, "adebayer handle pre_process failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAdebayerHandle::processing()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoProcAdebayer* adebayer_pre = (RkAiqAlgoProcAdebayer*)mProcInParam;

    ret = RkAiqHandle::processing();
    RKAIQCORE_CHECK_RET(ret, "adebayer handle processing failed");

    // TODO config common adebayer processing params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->processing(mProcInParam, mProcOutParam);
        RKAIQCORE_CHECK_RET(ret, "adebayer algo processing failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAdebayerHandle::postProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPostAdebayer* adebayer_pre = (RkAiqAlgoPostAdebayer*)mPostInParam;

    ret = RkAiqHandle::postProcess();
    RKAIQCORE_CHECK_RET(ret, "adebayer handle postProcess failed");

    // TODO config common adebayer postProcess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->post_process(mPostInParam, mPostOutParam);
        RKAIQCORE_CHECK_RET(ret, "adebayer algo postProcess failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAdpccHandle::init()
{
    ENTER_ANALYZER_FUNCTION();

    deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAdpcc());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAdpcc());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAdpcc());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAdpcc());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAdpcc());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAdpcc());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAdpcc());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAdpccHandle::prepare()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;

    ret = RkAiqHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "adpcc handle prepare failed");

    // TODO config adpcc common params
    RkAiqAlgoConfigAdpcc* adpcc_config = (RkAiqAlgoConfigAdpcc*)mConfig;

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->prepare(mConfig);
        RKAIQCORE_CHECK_RET(ret, "adpcc algo prepare failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAdpccHandle::preProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPreAdpcc* adpcc_pre = (RkAiqAlgoPreAdpcc*)mPreInParam;

    ret = RkAiqHandle::preProcess();
    RKAIQCORE_CHECK_RET(ret, "adpcc handle preProcess failed");

    // TODO config common adpcc preprocess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->pre_process(mPreInParam, mPreOutParam);
        RKAIQCORE_CHECK_RET(ret, "adpcc handle pre_process failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAdpccHandle::processing()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoProcAdpcc* adpcc_pre = (RkAiqAlgoProcAdpcc*)mProcInParam;

    ret = RkAiqHandle::processing();
    RKAIQCORE_CHECK_RET(ret, "adpcc handle processing failed");

    // TODO config common adpcc processing params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->processing(mProcInParam, mProcOutParam);
        RKAIQCORE_CHECK_RET(ret, "adpcc algo processing failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAdpccHandle::postProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPostAdpcc* adpcc_pre = (RkAiqAlgoPostAdpcc*)mPostInParam;

    ret = RkAiqHandle::postProcess();
    RKAIQCORE_CHECK_RET(ret, "adpcc handle postProcess failed");

    // TODO config common adpcc postProcess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->post_process(mPostInParam, mPostOutParam);
        RKAIQCORE_CHECK_RET(ret, "adpcc algo postProcess failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAfecHandle::init()
{
    ENTER_ANALYZER_FUNCTION();

    deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAfec());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAfec());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAfec());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAfec());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAfec());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAfec());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAfec());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAfecHandle::prepare()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;

    ret = RkAiqHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "afec handle prepare failed");

    // TODO config afec common params
    RkAiqAlgoConfigAfec* afec_config = (RkAiqAlgoConfigAfec*)mConfig;

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->prepare(mConfig);
        RKAIQCORE_CHECK_RET(ret, "afec algo prepare failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfecHandle::preProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPreAfec* afec_pre = (RkAiqAlgoPreAfec*)mPreInParam;

    ret = RkAiqHandle::preProcess();
    RKAIQCORE_CHECK_RET(ret, "afec handle preProcess failed");

    // TODO config common afec preprocess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->pre_process(mPreInParam, mPreOutParam);
        RKAIQCORE_CHECK_RET(ret, "afec handle pre_process failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfecHandle::processing()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoProcAfec* afec_pre = (RkAiqAlgoProcAfec*)mProcInParam;

    ret = RkAiqHandle::processing();
    RKAIQCORE_CHECK_RET(ret, "afec handle processing failed");

    // TODO config common afec processing params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->processing(mProcInParam, mProcOutParam);
        RKAIQCORE_CHECK_RET(ret, "afec algo processing failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfecHandle::postProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPostAfec* afec_pre = (RkAiqAlgoPostAfec*)mPostInParam;

    ret = RkAiqHandle::postProcess();
    RKAIQCORE_CHECK_RET(ret, "afec handle postProcess failed");

    // TODO config common afec postProcess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->post_process(mPostInParam, mPostOutParam);
        RKAIQCORE_CHECK_RET(ret, "afec algo postProcess failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAgammaHandle::init()
{
    ENTER_ANALYZER_FUNCTION();

    deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAgamma());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAgamma());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAgamma());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAgamma());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAgamma());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAgamma());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAgamma());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAgammaHandle::prepare()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;

    ret = RkAiqHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "agamma handle prepare failed");

    // TODO config agamma common params
    RkAiqAlgoConfigAgamma* agamma_config = (RkAiqAlgoConfigAgamma*)mConfig;

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->prepare(mConfig);
        RKAIQCORE_CHECK_RET(ret, "agamma algo prepare failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAgammaHandle::preProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPreAgamma* agamma_pre = (RkAiqAlgoPreAgamma*)mPreInParam;

    ret = RkAiqHandle::preProcess();
    RKAIQCORE_CHECK_RET(ret, "agamma handle preProcess failed");

    // TODO config common agamma preprocess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->pre_process(mPreInParam, mPreOutParam);
        RKAIQCORE_CHECK_RET(ret, "agamma handle pre_process failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAgammaHandle::processing()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoProcAgamma* agamma_pre = (RkAiqAlgoProcAgamma*)mProcInParam;

    ret = RkAiqHandle::processing();
    RKAIQCORE_CHECK_RET(ret, "agamma handle processing failed");

    // TODO config common agamma processing params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->processing(mProcInParam, mProcOutParam);
        RKAIQCORE_CHECK_RET(ret, "agamma algo processing failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAgammaHandle::postProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPostAgamma* agamma_pre = (RkAiqAlgoPostAgamma*)mPostInParam;

    ret = RkAiqHandle::postProcess();
    RKAIQCORE_CHECK_RET(ret, "agamma handle postProcess failed");

    // TODO config common agamma postProcess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->post_process(mPostInParam, mPostOutParam);
        RKAIQCORE_CHECK_RET(ret, "agamma algo postProcess failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAdegammaHandle::init()
{
    ENTER_ANALYZER_FUNCTION();

    deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAdegamma());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAdegamma());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAdegamma());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAdegamma());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAdegamma());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAdegamma());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAdegamma());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAdegammaHandle::prepare()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;

    ret = RkAiqHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "adegamma handle prepare failed");

    // TODO config agamma common params
    RkAiqAlgoConfigAdegamma* adegamma_config = (RkAiqAlgoConfigAdegamma*)mConfig;

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->prepare(mConfig);
        RKAIQCORE_CHECK_RET(ret, "adegamma algo prepare failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAdegammaHandle::preProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPreAdegamma* agamma_pre = (RkAiqAlgoPreAdegamma*)mPreInParam;

    ret = RkAiqHandle::preProcess();
    RKAIQCORE_CHECK_RET(ret, "adegamma handle preProcess failed");

    // TODO config common agamma preprocess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->pre_process(mPreInParam, mPreOutParam);
        RKAIQCORE_CHECK_RET(ret, "adegamma handle pre_process failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAdegammaHandle::processing()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoProcAdegamma* adegamma_pre = (RkAiqAlgoProcAdegamma*)mProcInParam;

    ret = RkAiqHandle::processing();
    RKAIQCORE_CHECK_RET(ret, "adegamma handle processing failed");

    // TODO config common agamma processing params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->processing(mProcInParam, mProcOutParam);
        RKAIQCORE_CHECK_RET(ret, "adegamma algo processing failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAdegammaHandle::postProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPostAdegamma* agamma_pre = (RkAiqAlgoPostAdegamma*)mPostInParam;

    ret = RkAiqHandle::postProcess();
    RKAIQCORE_CHECK_RET(ret, "adegamma handle postProcess failed");

    // TODO config common agamma postProcess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->post_process(mPostInParam, mPostOutParam);
        RKAIQCORE_CHECK_RET(ret, "adegamma algo postProcess failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAgicHandle::init()
{
    ENTER_ANALYZER_FUNCTION();

    deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAgic());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAgic());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAgic());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAgic());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAgic());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAgic());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAgic());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAgicHandle::prepare()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;

    ret = RkAiqHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "agic handle prepare failed");

    // TODO config agic common params
    RkAiqAlgoConfigAgic* agic_config = (RkAiqAlgoConfigAgic*)mConfig;

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->prepare(mConfig);
        RKAIQCORE_CHECK_RET(ret, "agic algo prepare failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAgicHandle::preProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPreAgic* agic_pre = (RkAiqAlgoPreAgic*)mPreInParam;

    ret = RkAiqHandle::preProcess();
    RKAIQCORE_CHECK_RET(ret, "agic handle preProcess failed");

    // TODO config common agic preprocess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->pre_process(mPreInParam, mPreOutParam);
        RKAIQCORE_CHECK_RET(ret, "agic handle pre_process failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAgicHandle::processing()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoProcAgic* agic_pre = (RkAiqAlgoProcAgic*)mProcInParam;

    ret = RkAiqHandle::processing();
    RKAIQCORE_CHECK_RET(ret, "agic handle processing failed");

    // TODO config common agic processing params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->processing(mProcInParam, mProcOutParam);
        RKAIQCORE_CHECK_RET(ret, "agic algo processing failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAgicHandle::postProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPostAgic* agic_pre = (RkAiqAlgoPostAgic*)mPostInParam;

    ret = RkAiqHandle::postProcess();
    RKAIQCORE_CHECK_RET(ret, "agic handle postProcess failed");

    // TODO config common agic postProcess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->post_process(mPostInParam, mPostOutParam);
        RKAIQCORE_CHECK_RET(ret, "agic algo postProcess failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAieHandle::init()
{
    ENTER_ANALYZER_FUNCTION();

    deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAie());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAie());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAie());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAie());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAie());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAie());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAie());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAieHandle::prepare()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;

    ret = RkAiqHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "aie handle prepare failed");

    // TODO config aie common params
    RkAiqAlgoConfigAie* aie_config = (RkAiqAlgoConfigAie*)mConfig;

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->prepare(mConfig);
        RKAIQCORE_CHECK_RET(ret, "aie algo prepare failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAieHandle::preProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPreAie* aie_pre = (RkAiqAlgoPreAie*)mPreInParam;

    ret = RkAiqHandle::preProcess();
    RKAIQCORE_CHECK_RET(ret, "aie handle preProcess failed");

    // TODO config common aie preprocess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->pre_process(mPreInParam, mPreOutParam);
        RKAIQCORE_CHECK_RET(ret, "aie handle pre_process failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAieHandle::processing()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoProcAie* aie_pre = (RkAiqAlgoProcAie*)mProcInParam;

    ret = RkAiqHandle::processing();
    RKAIQCORE_CHECK_RET(ret, "aie handle processing failed");

    // TODO config common aie processing params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->processing(mProcInParam, mProcOutParam);
        RKAIQCORE_CHECK_RET(ret, "aie algo processing failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAieHandle::postProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPostAie* aie_pre = (RkAiqAlgoPostAie*)mPostInParam;

    ret = RkAiqHandle::postProcess();
    RKAIQCORE_CHECK_RET(ret, "aie handle postProcess failed");

    // TODO config common aie postProcess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->post_process(mPostInParam, mPostOutParam);
        RKAIQCORE_CHECK_RET(ret, "aie algo postProcess failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAldchHandle::init()
{
    ENTER_ANALYZER_FUNCTION();

    deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAldch());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAldch());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAldch());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAldch());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAldch());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAldch());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAldch());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAldchHandle::prepare()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;

    ret = RkAiqHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "aldch handle prepare failed");

    // TODO config aldch common params
    RkAiqAlgoConfigAldch* aldch_config = (RkAiqAlgoConfigAldch*)mConfig;

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->prepare(mConfig);
        RKAIQCORE_CHECK_RET(ret, "aldch algo prepare failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAldchHandle::preProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPreAldch* aldch_pre = (RkAiqAlgoPreAldch*)mPreInParam;

    ret = RkAiqHandle::preProcess();
    RKAIQCORE_CHECK_RET(ret, "aldch handle preProcess failed");

    // TODO config common aldch preprocess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->pre_process(mPreInParam, mPreOutParam);
        RKAIQCORE_CHECK_RET(ret, "aldch handle pre_process failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAldchHandle::processing()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoProcAldch* aldch_pre = (RkAiqAlgoProcAldch*)mProcInParam;

    ret = RkAiqHandle::processing();
    RKAIQCORE_CHECK_RET(ret, "aldch handle processing failed");

    // TODO config common aldch processing params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->processing(mProcInParam, mProcOutParam);
        RKAIQCORE_CHECK_RET(ret, "aldch algo processing failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAldchHandle::postProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPostAldch* aldch_pre = (RkAiqAlgoPostAldch*)mPostInParam;

    ret = RkAiqHandle::postProcess();
    RKAIQCORE_CHECK_RET(ret, "aldch handle postProcess failed");

    // TODO config common aldch postProcess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->post_process(mPostInParam, mPostOutParam);
        RKAIQCORE_CHECK_RET(ret, "aldch algo postProcess failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}


void
RkAiqAr2yHandle::init()
{
    ENTER_ANALYZER_FUNCTION();

    deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAr2y());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAr2y());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAr2y());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAr2y());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAr2y());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAr2y());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAr2y());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAr2yHandle::prepare()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;

    ret = RkAiqHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "ar2y handle prepare failed");

    // TODO config ar2y common params
    RkAiqAlgoConfigAr2y* ar2y_config = (RkAiqAlgoConfigAr2y*)mConfig;

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->prepare(mConfig);
        RKAIQCORE_CHECK_RET(ret, "ar2y algo prepare failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAr2yHandle::preProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPreAr2y* ar2y_pre = (RkAiqAlgoPreAr2y*)mPreInParam;

    ret = RkAiqHandle::preProcess();
    RKAIQCORE_CHECK_RET(ret, "ar2y handle preProcess failed");

    // TODO config common ar2y preprocess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->pre_process(mPreInParam, mPreOutParam);
        RKAIQCORE_CHECK_RET(ret, "ar2y handle pre_process failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAr2yHandle::processing()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoProcAr2y* ar2y_pre = (RkAiqAlgoProcAr2y*)mProcInParam;

    ret = RkAiqHandle::processing();
    RKAIQCORE_CHECK_RET(ret, "ar2y handle processing failed");

    // TODO config common ar2y processing params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->processing(mProcInParam, mProcOutParam);
        RKAIQCORE_CHECK_RET(ret, "ar2y algo processing failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAr2yHandle::postProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPostAr2y* ar2y_pre = (RkAiqAlgoPostAr2y*)mPostInParam;

    ret = RkAiqHandle::postProcess();
    RKAIQCORE_CHECK_RET(ret, "ar2y handle postProcess failed");

    // TODO config common ar2y postProcess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->post_process(mPostInParam, mPostOutParam);
        RKAIQCORE_CHECK_RET(ret, "ar2y algo postProcess failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAwdrHandle::init()
{
    ENTER_ANALYZER_FUNCTION();

    deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAwdr());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAwdr());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAwdr());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAwdr());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAwdr());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAwdr());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAwdr());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAwdrHandle::prepare()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;

    ret = RkAiqHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "awdr handle prepare failed");

    // TODO config awdr common params
    RkAiqAlgoConfigAwdr* awdr_config = (RkAiqAlgoConfigAwdr*)mConfig;

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->prepare(mConfig);
        RKAIQCORE_CHECK_RET(ret, "awdr algo prepare failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAwdrHandle::preProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPreAwdr* awdr_pre = (RkAiqAlgoPreAwdr*)mPreInParam;

    ret = RkAiqHandle::preProcess();
    RKAIQCORE_CHECK_RET(ret, "awdr handle preProcess failed");

    // TODO config common awdr preprocess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->pre_process(mPreInParam, mPreOutParam);
        RKAIQCORE_CHECK_RET(ret, "awdr handle pre_process failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAwdrHandle::processing()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoProcAwdr* awdr_pre = (RkAiqAlgoProcAwdr*)mProcInParam;

    ret = RkAiqHandle::processing();
    RKAIQCORE_CHECK_RET(ret, "awdr handle processing failed");

    // TODO config common awdr processing params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->processing(mProcInParam, mProcOutParam);
        RKAIQCORE_CHECK_RET(ret, "awdr algo processing failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAwdrHandle::postProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPostAwdr* awdr_pre = (RkAiqAlgoPostAwdr*)mPostInParam;

    ret = RkAiqHandle::postProcess();
    RKAIQCORE_CHECK_RET(ret, "awdr handle postProcess failed");

    // TODO config common awdr postProcess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->post_process(mPostInParam, mPostOutParam);
        RKAIQCORE_CHECK_RET(ret, "awdr algo postProcess failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAorbHandle::init()
{
    ENTER_ANALYZER_FUNCTION();

    deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAorb());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAorb());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAorb());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAorb());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAorb());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAorb());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAorb());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAorbHandle::prepare()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;

    ret = RkAiqHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "aorb handle prepare failed");

    // TODO config aorb common params
    RkAiqAlgoConfigAorb* aorb_config = (RkAiqAlgoConfigAorb*)mConfig;

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->prepare(mConfig);
        RKAIQCORE_CHECK_RET(ret, "aorb algo prepare failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAorbHandle::preProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPreAorb* aorb_pre = (RkAiqAlgoPreAorb*)mPreInParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqHandle::preProcess();
    RKAIQCORE_CHECK_RET(ret, "aorb handle preProcess failed");

    if (!ispStats->orb_stats_valid && !shared->init) {
        LOGE("no orb stats, ignore!");
        return XCAM_RETURN_BYPASS;
    }

    // TODO config common aorb preprocess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->pre_process(mPreInParam, mPreOutParam);
        RKAIQCORE_CHECK_RET(ret, "aorb handle pre_process failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAorbHandle::processing()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoProcAorb* aorb_pre = (RkAiqAlgoProcAorb*)mProcInParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqHandle::processing();
    RKAIQCORE_CHECK_RET(ret, "aorb handle processing failed");

    if (!ispStats->orb_stats_valid && !shared->init) {
        LOGE("no orb stats, ignore!");
        return XCAM_RETURN_BYPASS;
    }

    // TODO config common aorb processing params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->processing(mProcInParam, mProcOutParam);
        RKAIQCORE_CHECK_RET(ret, "aorb algo processing failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAorbHandle::postProcess()
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    RkAiqAlgoPostAorb* aorb_pre = (RkAiqAlgoPostAorb*)mPostInParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqHandle::postProcess();
    RKAIQCORE_CHECK_RET(ret, "aorb handle postProcess failed");

    if (!ispStats->orb_stats_valid && !shared->init) {
        LOGE("no orb stats, ignore!");
        return XCAM_RETURN_BYPASS;
    }

    // TODO config common aorb postProcess params

    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        ret = des->post_process(mPostInParam, mPostOutParam);
        RKAIQCORE_CHECK_RET(ret, "aorb algo postProcess failed");
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

}; //namespace RkCam
