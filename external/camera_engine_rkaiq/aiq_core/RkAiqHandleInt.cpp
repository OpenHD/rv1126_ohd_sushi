/*
 * RkAiqHandleInt.cpp
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

#include "RkAiqHandleInt.h"
#include "RkAiqCore.h"
#include <fcntl.h>

namespace RkCam {

XCamReturn RkAiqHandleIntCom::configInparamsCom(RkAiqAlgoCom* com, int type)
{
    ENTER_ANALYZER_FUNCTION();

    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqAlgoComInt* rk_com = NULL;

#define GET_RK_COM(algo) \
    { \
        if (type == RKAIQ_CONFIG_COM_PREPARE) \
            rk_com = &(((RkAiqAlgoConfig##algo##Int*)com)->rk_com); \
        else if (type == RKAIQ_CONFIG_COM_PRE) \
            rk_com = &(((RkAiqAlgoPre##algo##Int*)com)->rk_com); \
        else if (type == RKAIQ_CONFIG_COM_PROC) \
            rk_com = &(((RkAiqAlgoProc##algo##Int*)com)->rk_com); \
        else if (type == RKAIQ_CONFIG_COM_POST) \
            rk_com = &(((RkAiqAlgoPost##algo##Int*)com)->rk_com); \
    } \

    switch (mDes->type) {
    case RK_AIQ_ALGO_TYPE_AE:
        GET_RK_COM(Ae);
        break;
    case RK_AIQ_ALGO_TYPE_AWB:
        GET_RK_COM(Awb);
        break;
    case RK_AIQ_ALGO_TYPE_AF:
        GET_RK_COM(Af);
        break;
    case RK_AIQ_ALGO_TYPE_ABLC:
        GET_RK_COM(Ablc);
        break;
    case RK_AIQ_ALGO_TYPE_ADPCC:
        GET_RK_COM(Adpcc);
        break;
    case RK_AIQ_ALGO_TYPE_AHDR:
        GET_RK_COM(Ahdr);
        break;
    case RK_AIQ_ALGO_TYPE_ANR:
        GET_RK_COM(Anr);
        break;
    case RK_AIQ_ALGO_TYPE_ALSC:
        GET_RK_COM(Alsc);
        break;
    case RK_AIQ_ALGO_TYPE_AGIC:
        GET_RK_COM(Agic);
        break;
    case RK_AIQ_ALGO_TYPE_ADEBAYER:
        GET_RK_COM(Adebayer);
        break;
    case RK_AIQ_ALGO_TYPE_ACCM:
        GET_RK_COM(Accm);
        break;
    case RK_AIQ_ALGO_TYPE_AGAMMA:
        GET_RK_COM(Agamma);
        break;
    case RK_AIQ_ALGO_TYPE_ADEGAMMA:
        GET_RK_COM(Adegamma);
        break;
    case RK_AIQ_ALGO_TYPE_AWDR:
        GET_RK_COM(Awdr);
        break;
    case RK_AIQ_ALGO_TYPE_ADHAZ:
        GET_RK_COM(Adhaz);
        break;
    case RK_AIQ_ALGO_TYPE_A3DLUT:
        GET_RK_COM(A3dlut);
        break;
    case RK_AIQ_ALGO_TYPE_ALDCH:
        GET_RK_COM(Aldch);
        break;
    case RK_AIQ_ALGO_TYPE_AR2Y:
        GET_RK_COM(Ar2y);
        break;
    case RK_AIQ_ALGO_TYPE_ACP:
        GET_RK_COM(Acp);
        break;
    case RK_AIQ_ALGO_TYPE_AIE:
        GET_RK_COM(Aie);
        break;
    case RK_AIQ_ALGO_TYPE_ASHARP:
        GET_RK_COM(Asharp);
        break;
    case RK_AIQ_ALGO_TYPE_AORB:
        GET_RK_COM(Aorb);
        break;
    case RK_AIQ_ALGO_TYPE_AFEC:
        GET_RK_COM(Afec);
        break;
    case RK_AIQ_ALGO_TYPE_ACGC:
        GET_RK_COM(Acgc);
        break;
    case RK_AIQ_ALGO_TYPE_ASD:
        GET_RK_COM(Asd);
        break;
    case RK_AIQ_ALGO_TYPE_AFD:
        GET_RK_COM(Afd);
        break;
    default:
        LOGE_ANALYZER("wrong algo type !");
    }

    if (!rk_com)
        goto out;

    xcam_mem_clear(*rk_com);

    if (type == RkAiqHandle::RKAIQ_CONFIG_COM_PREPARE) {
        rk_com->u.prepare.calib = (CamCalibDbContext_t*)(shared->calib);
    } else {
        rk_com->u.proc.pre_res_comb = &shared->preResComb;
        rk_com->u.proc.proc_res_comb = &shared->procResComb;
        rk_com->u.proc.post_res_comb = &shared->postResComb;
        rk_com->u.proc.iso = shared->iso;
        rk_com->u.proc.fill_light_on = shared->fill_light_on;
        rk_com->u.proc.gray_mode = shared->gray_mode;
        rk_com->u.proc.is_bw_sensor = shared->is_bw_sensor;
        rk_com->u.proc.preExp = &shared->preExp;
        rk_com->u.proc.curExp = &shared->curExp;
    }

    EXIT_ANALYZER_FUNCTION();
out:
    return RkAiqHandle::configInparamsCom(com, type);
}

void
RkAiqHandleIntCom::waitSignal()
{
    if (mAiqCore->isRunningState()) {
        mUpdateCond.timedwait(mCfgMutex, 100000);
    } else {
        updateConfig(false);
    }
}

void
RkAiqHandleIntCom::sendSignal()
{
    if (mAiqCore->isRunningState())
        mUpdateCond.signal();
}

void
RkAiqAeHandleInt::init()
{
    ENTER_ANALYZER_FUNCTION();

    RkAiqAeHandle::deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAeInt());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAeInt());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAeInt());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAeInt());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAeInt());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAeInt());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAeInt());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAeHandleInt::updateConfig(bool needSync)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    if (needSync)
        mCfgMutex.lock();
    // if something changed, api will modify aecCfg in mAlgoCtx
    if (updateExpSwAttr) {
        mCurExpSwAttr = mNewExpSwAttr;
        updateExpSwAttr = false;
        updateAtt = true;
        rk_aiq_uapi_ae_setExpSwAttr(mAlgoCtx, &mCurExpSwAttr, false);
        sendSignal();
    }
    if (updateLinExpAttr) {
        mCurLinExpAttr = mNewLinExpAttr;
        updateLinExpAttr = false;
        updateAtt = true;
        rk_aiq_uapi_ae_setLinExpAttr(mAlgoCtx, &mCurLinExpAttr, false);
        sendSignal();
    }
    if (updateHdrExpAttr) {
        mCurHdrExpAttr = mNewHdrExpAttr;
        updateHdrExpAttr = false;
        updateAtt = true;
        rk_aiq_uapi_ae_setHdrExpAttr(mAlgoCtx, &mCurHdrExpAttr, false);
        sendSignal();
    }
    if (updateLinAeDayRouteAttr) {
        mCurLinAeDayRouteAttr = mNewLinAeDayRouteAttr;
        updateLinAeDayRouteAttr = false;
        updateAtt = true;
        rk_aiq_uapi_ae_setLinAeDayRouteAttr(mAlgoCtx, &mCurLinAeDayRouteAttr, false);
        sendSignal();
    }
    if (updateHdrAeDayRouteAttr) {
        mCurHdrAeDayRouteAttr = mNewHdrAeDayRouteAttr;
        updateHdrAeDayRouteAttr = false;
        updateAtt = true;
        rk_aiq_uapi_ae_setHdrAeDayRouteAttr(mAlgoCtx, &mCurHdrAeDayRouteAttr, false);
        sendSignal();
    }
    if (updateLinAeNightRouteAttr) {
        mCurLinAeNightRouteAttr = mNewLinAeNightRouteAttr;
        updateLinAeNightRouteAttr = false;
        updateAtt = true;
        rk_aiq_uapi_ae_setLinAeNightRouteAttr(mAlgoCtx, &mCurLinAeNightRouteAttr, false);
        sendSignal();
    }
    if (updateHdrAeNightRouteAttr) {
        mCurHdrAeNightRouteAttr = mNewHdrAeNightRouteAttr;
        updateHdrAeNightRouteAttr = false;
        updateAtt = true;
        rk_aiq_uapi_ae_setHdrAeNightRouteAttr(mAlgoCtx, &mCurHdrAeNightRouteAttr, false);
        sendSignal();
    }
    if (updateExpWinAttr) {
        mCurExpWinAttr = mNewExpWinAttr;
        updateExpWinAttr = false;
        updateAtt = true;
        rk_aiq_uapi_ae_setExpWinAttr(mAlgoCtx, &mCurExpWinAttr, false);
        sendSignal();
    }

    // once any params are changed, run reconfig to convert aecCfg to paectx
    AeInstanceConfig_t* pAeInstConfig = (AeInstanceConfig_t*)mAlgoCtx;
    AeConfig_t pAecCfg = pAeInstConfig->aecCfg;
    pAecCfg->IsReconfig |= updateAtt;
    updateAtt = false;
    if (needSync)
        mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}
XCamReturn
RkAiqAeHandleInt::setExpSwAttr(Uapi_ExpSwAttr_t ExpSwAttr)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore

    // if something changed
    if (0 != memcmp(&mCurExpSwAttr, &ExpSwAttr, sizeof(Uapi_ExpSwAttr_t))) {
        mNewExpSwAttr = ExpSwAttr;
        updateExpSwAttr = true;
        waitSignal();
    }
    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}
XCamReturn
RkAiqAeHandleInt::getExpSwAttr(Uapi_ExpSwAttr_t* pExpSwAttr)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_ae_getExpSwAttr(mAlgoCtx, pExpSwAttr);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAeHandleInt::setLinExpAttr(Uapi_LinExpAttr_t LinExpAttr)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore

    // if something changed
    if (0 != memcmp(&mCurLinExpAttr, &LinExpAttr, sizeof(Uapi_LinExpAttr_t))) {
        mNewLinExpAttr = LinExpAttr;
        updateLinExpAttr = true;
        waitSignal();
    }
    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}
XCamReturn
RkAiqAeHandleInt::getLinExpAttr(Uapi_LinExpAttr_t* pLinExpAttr)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_ae_getLinExpAttr(mAlgoCtx, pLinExpAttr);

    EXIT_ANALYZER_FUNCTION();
    return ret;

}
XCamReturn
RkAiqAeHandleInt::setHdrExpAttr(Uapi_HdrExpAttr_t HdrExpAttr)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore

    // if something changed
    if (0 != memcmp(&mCurHdrExpAttr, &HdrExpAttr, sizeof(Uapi_HdrExpAttr_t))) {
        mNewHdrExpAttr = HdrExpAttr;
        updateHdrExpAttr = true;
        waitSignal();
    }
    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}
XCamReturn
RkAiqAeHandleInt::getHdrExpAttr (Uapi_HdrExpAttr_t* pHdrExpAttr)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_ae_getHdrExpAttr(mAlgoCtx, pHdrExpAttr);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}
XCamReturn
RkAiqAeHandleInt::setLinAeDayRouteAttr(Uapi_LinAeRouteAttr_t LinAeDayRouteAttr)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore

    // if something changed

    if (0 != memcmp(&mCurLinAeDayRouteAttr, &LinAeDayRouteAttr, sizeof(Uapi_LinAeRouteAttr_t))) {
        mNewLinAeDayRouteAttr = LinAeDayRouteAttr;
        updateLinAeDayRouteAttr = true;
        waitSignal();
    }
    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}
XCamReturn
RkAiqAeHandleInt::getLinAeDayRouteAttr(Uapi_LinAeRouteAttr_t* pLinAeDayRouteAttr)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_ae_getLinAeDayRouteAttr(mAlgoCtx, pLinAeDayRouteAttr);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}
XCamReturn
RkAiqAeHandleInt::setHdrAeDayRouteAttr(Uapi_HdrAeRouteAttr_t HdrAeDayRouteAttr)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore

    // if something changed
    if (0 != memcmp(&mCurHdrAeDayRouteAttr, &HdrAeDayRouteAttr, sizeof(Uapi_HdrAeRouteAttr_t))) {
        mNewHdrAeDayRouteAttr = HdrAeDayRouteAttr;
        updateHdrAeDayRouteAttr = true;
        waitSignal();
    }
    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}
XCamReturn
RkAiqAeHandleInt::getHdrAeDayRouteAttr(Uapi_HdrAeRouteAttr_t* pHdrAeDayRouteAttr)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_ae_getHdrAeDayRouteAttr(mAlgoCtx, pHdrAeDayRouteAttr);

    EXIT_ANALYZER_FUNCTION();
    return ret;

}

XCamReturn
RkAiqAeHandleInt::setLinAeNightRouteAttr(Uapi_LinAeRouteAttr_t LinAeNightRouteAttr)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore

    // if something changed

    if (0 != memcmp(&mCurLinAeNightRouteAttr, &LinAeNightRouteAttr, sizeof(Uapi_LinAeRouteAttr_t))) {
        mNewLinAeNightRouteAttr = LinAeNightRouteAttr;
        updateLinAeNightRouteAttr = true;
        waitSignal();
    }
    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}
XCamReturn
RkAiqAeHandleInt::getLinAeNightRouteAttr(Uapi_LinAeRouteAttr_t* pLinAeNightRouteAttr)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_ae_getLinAeNightRouteAttr(mAlgoCtx, pLinAeNightRouteAttr);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}
XCamReturn
RkAiqAeHandleInt::setHdrAeNightRouteAttr(Uapi_HdrAeRouteAttr_t HdrAeNightRouteAttr)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore

    // if something changed
    if (0 != memcmp(&mCurHdrAeNightRouteAttr, &HdrAeNightRouteAttr, sizeof(Uapi_HdrAeRouteAttr_t))) {
        mNewHdrAeNightRouteAttr = HdrAeNightRouteAttr;
        updateHdrAeNightRouteAttr = true;
        waitSignal();
    }
    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}
XCamReturn
RkAiqAeHandleInt::getHdrAeNightRouteAttr(Uapi_HdrAeRouteAttr_t* pHdrAeNightRouteAttr)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_ae_getHdrAeNightRouteAttr(mAlgoCtx, pHdrAeNightRouteAttr);

    EXIT_ANALYZER_FUNCTION();
    return ret;

}

XCamReturn
RkAiqAeHandleInt::setExpWinAttr(Uapi_ExpWin_t ExpWinAttr)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore

    // if something changed

    if (0 != memcmp(&mCurExpWinAttr, &ExpWinAttr, sizeof(Uapi_ExpWin_t))) {
        mNewExpWinAttr = ExpWinAttr;
        updateExpWinAttr = true;
        waitSignal();
    }

    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}
XCamReturn
RkAiqAeHandleInt::getExpWinAttr(Uapi_ExpWin_t * pExpWinAttr)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_ae_getExpWinAttr(mAlgoCtx, pExpWinAttr);

    EXIT_ANALYZER_FUNCTION();
    return ret;

}
XCamReturn
RkAiqAeHandleInt::queryExpInfo(Uapi_ExpQueryInfo_t* pExpQueryInfo)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_ae_queryExpInfo(mAlgoCtx, pExpQueryInfo);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}
XCamReturn
RkAiqAeHandleInt::prepare()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = RkAiqAeHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "ae handle prepare failed");

    RkAiqAlgoConfigAeInt* ae_config_int = (RkAiqAlgoConfigAeInt*)mConfig;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;

    /*****************AecConfig pic-info params*****************/
    ae_config_int->RawWidth = shared->snsDes.isp_acq_width;
    ae_config_int->RawHeight = shared->snsDes.isp_acq_height;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->prepare(mConfig);
    RKAIQCORE_CHECK_RET(ret, "ae algo prepare failed");

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAeHandleInt::preProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPreAeInt* ae_pre_int = (RkAiqAlgoPreAeInt*)mPreInParam;
    RkAiqAlgoPreResAeInt* ae_pre_res_int = (RkAiqAlgoPreResAeInt*)mPreOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;
    RkAiqPreResComb* comb = &shared->preResComb;

    ret = RkAiqAeHandle::preProcess();
    if (ret) {
        comb->ae_pre_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "ae handle preProcess failed");
    }

    comb->ae_pre_res = NULL;

    ae_pre_int->ae_pre_com.ispAeStats = &ispStats->aec_stats;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->pre_process(mPreInParam, mPreOutParam);
    RKAIQCORE_CHECK_RET(ret, "ae algo pre_process failed");

    // set result to mAiqCore
    comb->ae_pre_res = (RkAiqAlgoPreResAe*)ae_pre_res_int;

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAeHandleInt::processing()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoProcAeInt* ae_proc_int = (RkAiqAlgoProcAeInt*)mProcInParam;
    RkAiqAlgoProcResAeInt* ae_proc_res_int = (RkAiqAlgoProcResAeInt*)mProcOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqProcResComb* comb = &shared->procResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAeHandle::processing();
    if (ret) {
        comb->ae_proc_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "ae handle processing failed");
    }

    comb->ae_proc_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->processing(mProcInParam, mProcOutParam);
    RKAIQCORE_CHECK_RET(ret, "ae algo processing failed");

    comb->ae_proc_res = (RkAiqAlgoProcResAe*)ae_proc_res_int;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAeHandleInt::postProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPostAeInt* ae_post_int = (RkAiqAlgoPostAeInt*)mPostInParam;
    RkAiqAlgoPostResAeInt* ae_post_res_int = (RkAiqAlgoPostResAeInt*)mPostOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqPostResComb* comb = &shared->postResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAeHandle::postProcess();
    if (ret) {
        comb->ae_post_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "ae handle postProcess failed");
        return ret;
    }

    comb->ae_post_res = NULL;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->post_process(mPostInParam, mPostOutParam);
    RKAIQCORE_CHECK_RET(ret, "ae algo post_process failed");
    // set result to mAiqCore
    comb->ae_post_res = (RkAiqAlgoPostResAe*)ae_post_res_int ;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}
void
RkAiqAfdHandleInt::init()
{
    ENTER_ANALYZER_FUNCTION();

    RkAiqAfdHandle::deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAfdInt());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAfdInt());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAfdInt());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAfdInt());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAfdInt());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAfdInt());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAfdInt());

    EXIT_ANALYZER_FUNCTION();
}
XCamReturn
RkAiqAfdHandleInt::prepare()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = RkAiqAfdHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "afd handle prepare failed");

    RkAiqAlgoConfigAfdInt* afd_config_int = (RkAiqAlgoConfigAfdInt*)mConfig;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;

    /*****************AfdConfig pic-info params*****************/
    afd_config_int->RawWidth = shared->snsDes.isp_acq_width;
    afd_config_int->RawHeight = shared->snsDes.isp_acq_height;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->prepare(mConfig);
    RKAIQCORE_CHECK_RET(ret, "afd algo prepare failed");

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAfdHandleInt::preProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPreAfdInt* afd_pre_int = (RkAiqAlgoPreAfdInt*)mPreInParam;
    RkAiqAlgoPreResAfdInt* afd_pre_res_int = (RkAiqAlgoPreResAfdInt*)mPreOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;
    RkAiqPreResComb* comb = &shared->preResComb;
    afd_pre_int->tx_buf = &shared->tx_buf;

    ret = RkAiqAfdHandle::preProcess();
    if (ret) {
        comb->ae_pre_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "afd handle preProcess failed");
    }

    comb->afd_pre_res = NULL;


    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->pre_process(mPreInParam, mPreOutParam);
    RKAIQCORE_CHECK_RET(ret, "afd algo pre_process failed");

    // set result to mAiqCore
    comb->afd_pre_res = (RkAiqAlgoPreResAfd*)afd_pre_res_int;

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAfdHandleInt::processing()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoProcAfdInt* afd_proc_int = (RkAiqAlgoProcAfdInt*)mProcInParam;
    RkAiqAlgoProcResAfdInt* afd_proc_res_int = (RkAiqAlgoProcResAfdInt*)mProcOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqProcResComb* comb = &shared->procResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAfdHandle::processing();
    if (ret) {
        comb->afd_proc_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "afd handle processing failed");
    }

    // comb->afd_proc_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->processing(mProcInParam, mProcOutParam);
    RKAIQCORE_CHECK_RET(ret, "afd algo processing failed");

    comb->afd_proc_res = (RkAiqAlgoProcResAfd*)afd_proc_res_int;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}
XCamReturn
RkAiqAfdHandleInt::postProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPostAfdInt* afd_post_int = (RkAiqAlgoPostAfdInt*)mPostInParam;
    RkAiqAlgoPostResAfdInt* afd_post_res_int = (RkAiqAlgoPostResAfdInt*)mPostOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqPostResComb* comb = &shared->postResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAfdHandle::postProcess();
    if (ret) {
        comb->afd_post_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "afd handle postProcess failed");
        return ret;
    }

    comb->afd_post_res = NULL;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->post_process(mPostInParam, mPostOutParam);
    RKAIQCORE_CHECK_RET(ret, "afd algo post_process failed");
    // set result to mAiqCore
    comb->afd_post_res = (RkAiqAlgoPostResAfd*)afd_post_res_int ;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAwbHandleInt::init()
{
    ENTER_ANALYZER_FUNCTION();

    RkAiqAwbHandle::deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAwbInt());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAwbInt());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAwbInt());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAwbInt());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAwbInt());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAwbInt());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAwbInt());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAwbHandleInt::updateConfig(bool needSync)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (needSync)
        mCfgMutex.lock();
    // if something changed
    if (updateAtt) {
        mCurAtt = mNewAtt;
        updateAtt = false;
        rk_aiq_uapi_awb_SetAttrib(mAlgoCtx, mCurAtt, false);
        sendSignal();
    }
    if (needSync)
        mCfgMutex.unlock();


    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAwbHandleInt::setAttrib(rk_aiq_wb_attrib_t att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore

    // if something changed
    if (0 != memcmp(&mCurAtt, &att, sizeof(rk_aiq_wb_attrib_t))) {
        mNewAtt = att;
        updateAtt = true;
        waitSignal();
    }

    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAwbHandleInt::getAttrib(rk_aiq_wb_attrib_t *att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_awb_GetAttrib(mAlgoCtx, att);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAwbHandleInt::getCct(rk_aiq_wb_cct_t *cct)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_awb_GetCCT(mAlgoCtx, cct);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAwbHandleInt::queryWBInfo(rk_aiq_wb_querry_info_t *wb_querry_info )
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_awb_QueryWBInfo(mAlgoCtx, wb_querry_info);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAwbHandleInt::lock()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_awb_Lock(mAlgoCtx);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAwbHandleInt::unlock()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_awb_Unlock(mAlgoCtx);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAwbHandleInt::prepare()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = RkAiqAwbHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "awb handle prepare failed");

    RkAiqAlgoConfigAwbInt* awb_config_int = (RkAiqAlgoConfigAwbInt*)mConfig;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    // TODO
    //awb_config_int->rawBit;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->prepare(mConfig);
    RKAIQCORE_CHECK_RET(ret, "ae algo post_process failed");

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAwbHandleInt::preProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPreAwbInt* awb_pre_int = (RkAiqAlgoPreAwbInt*)mPreInParam;
    RkAiqAlgoPreResAwbInt* awb_pre_res_int = (RkAiqAlgoPreResAwbInt*)mPreOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqPreResComb* comb = &shared->preResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAwbHandle::preProcess();
    if (ret) {
        comb->awb_pre_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "awb handle preProcess failed");
    }
    comb->awb_pre_res = NULL;
#ifdef RK_SIMULATOR_HW
    if(shared->hardware_version == 0) {
        awb_pre_int->awb_hw0_statis = ispStats->awb_stats;
        awb_pre_int->awb_cfg_effect_v200 = ispStats->awb_cfg_effect_v200;
    } else {
        awb_pre_int->awb_hw1_statis = ispStats->awb_stats_v201;
        awb_pre_int->awb_cfg_effect_v201 = ispStats->awb_cfg_effect_v201;
    }
#else
    if(shared->hardware_version == 0) {
        awb_pre_int->awb_cfg_effect_v200 = ispStats->awb_cfg_effect_v200;

        for(int i = 0; i < awb_pre_int->awb_cfg_effect_v200.lightNum; i++) {
            //mAwbPreParam.awb_hw0_statis.light,
            for(int j = 0; j < RK_AIQ_AWB_XY_TYPE_MAX_V200; j++) {
                awb_pre_int->awb_hw0_statis.light[i].xYType[j].WpNo
                    = ispStats->awb_stats.light[i].xYType[j].WpNo;
                awb_pre_int->awb_hw0_statis.light[i].xYType[j].Rvalue
                    = ispStats->awb_stats.light[i].xYType[j].Rvalue;
                awb_pre_int->awb_hw0_statis.light[i].xYType[j].Gvalue
                    = ispStats->awb_stats.light[i].xYType[j].Gvalue;
                awb_pre_int->awb_hw0_statis.light[i].xYType[j].Bvalue
                    = ispStats->awb_stats.light[i].xYType[j].Bvalue;

                awb_pre_int->awb_hw0_statis.multiwindowLightResult[i].xYType[j].WpNo
                    = ispStats->awb_stats.multiwindowLightResult[i].xYType[j].WpNo;
                awb_pre_int->awb_hw0_statis.multiwindowLightResult[i].xYType[j].Rvalue
                    = ispStats->awb_stats.multiwindowLightResult[i].xYType[j].Rvalue;
                awb_pre_int->awb_hw0_statis.multiwindowLightResult[i].xYType[j].Gvalue
                    = ispStats->awb_stats.multiwindowLightResult[i].xYType[j].Gvalue;
                awb_pre_int->awb_hw0_statis.multiwindowLightResult[i].xYType[j].Bvalue
                    = ispStats->awb_stats.multiwindowLightResult[i].xYType[j].Bvalue;

            }
        }
        for(int i = 0; i < RK_AIQ_AWB_GRID_NUM_TOTAL; i++) {
            awb_pre_int->awb_hw0_statis.blockResult[i].Rvalue
                = ispStats->awb_stats.blockResult[i].Rvalue;
            awb_pre_int->awb_hw0_statis.blockResult[i].Gvalue
                = ispStats->awb_stats.blockResult[i].Gvalue;
            awb_pre_int->awb_hw0_statis.blockResult[i].Bvalue
                = ispStats->awb_stats.blockResult[i].Bvalue;
            memcpy(awb_pre_int->awb_hw0_statis.blockResult[i].isWP,
                   ispStats->awb_stats.blockResult[i].isWP,
                   sizeof(awb_pre_int->awb_hw0_statis.blockResult[i].isWP));
        }

        for(int i = 0; i < RK_AIQ_AWB_STAT_WP_RANGE_NUM_V200; i++) {
            awb_pre_int->awb_hw0_statis.excWpRangeResult[i].Rvalue
                = ispStats->awb_stats.excWpRangeResult[i].Rvalue;
            awb_pre_int->awb_hw0_statis.excWpRangeResult[i].Gvalue
                = ispStats->awb_stats.excWpRangeResult[i].Gvalue;
            awb_pre_int->awb_hw0_statis.excWpRangeResult[i].Bvalue
                = ispStats->awb_stats.excWpRangeResult[i].Bvalue;
            awb_pre_int->awb_hw0_statis.excWpRangeResult[i].WpNo
                = ispStats->awb_stats.excWpRangeResult[i].WpNo;
        }


    } else {
        //V201 TO DO
        awb_pre_int->awb_cfg_effect_v201 = ispStats->awb_cfg_effect_v201;
    }
#endif
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->pre_process(mPreInParam, mPreOutParam);
    RKAIQCORE_CHECK_RET(ret, "awb algo pre_process failed");
    // set result to mAiqCore
    comb->awb_pre_res = (RkAiqAlgoPreResAwb*)awb_pre_res_int;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAwbHandleInt::processing()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoProcAwbInt* awb_proc_int = (RkAiqAlgoProcAwbInt*)mProcInParam;
    RkAiqAlgoProcResAwbInt* awb_proc_res_int = (RkAiqAlgoProcResAwbInt*)mProcOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqProcResComb* comb = &shared->procResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAwbHandle::processing();
    if (ret) {
        comb->awb_proc_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "awb handle processing failed");
    }

    comb->awb_proc_res = NULL;


    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->processing(mProcInParam, mProcOutParam);
    RKAIQCORE_CHECK_RET(ret, "awb algo processing failed");
    // set result to mAiqCore
    comb->awb_proc_res = (RkAiqAlgoProcResAwb*)awb_proc_res_int;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAwbHandleInt::postProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPostAwbInt* awb_post_int = (RkAiqAlgoPostAwbInt*)mPostInParam;
    RkAiqAlgoPostResAwbInt* awb_post_res_int = (RkAiqAlgoPostResAwbInt*)mPostOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqPostResComb* comb = &shared->postResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAwbHandle::postProcess();
    if (ret) {
        comb->awb_post_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "awb handle postProcess failed");
        return ret;
    }

    comb->awb_post_res = NULL;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->post_process(mPostInParam, mPostOutParam);
    RKAIQCORE_CHECK_RET(ret, "awb algo post_process failed");
    // set result to mAiqCore
    comb->awb_post_res = (RkAiqAlgoPostResAwb*)awb_post_res_int ;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAfHandleInt::init()
{
    ENTER_ANALYZER_FUNCTION();

    RkAiqAfHandle::deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAfInt());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAfInt());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAfInt());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAfInt());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAfInt());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAfInt());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAfInt());
    mLastZoomIndex = 0;

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAfHandleInt::updateConfig(bool needSync)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (needSync)
        mCfgMutex.lock();
    // if something changed
    if (updateAtt) {
        rk_aiq_uapi_af_SetAttrib(mAlgoCtx, mNewAtt, false);
        isUpdateAttDone = true;
    }
    if (needSync)
        mCfgMutex.unlock();


    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfHandleInt::setAttrib(rk_aiq_af_attrib_t *att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore

    // if something changed
    if (0 != memcmp(&mCurAtt, att, sizeof(rk_aiq_af_attrib_t))) {
        mNewAtt = *att;
        updateAtt = true;
        isUpdateAttDone = false;
        waitSignal();
    }

    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfHandleInt::getAttrib(rk_aiq_af_attrib_t *att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_af_GetAttrib(mAlgoCtx, att);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfHandleInt::lock()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_af_Lock(mAlgoCtx);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfHandleInt::unlock()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_af_Unlock(mAlgoCtx);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfHandleInt::Oneshot()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_af_Oneshot(mAlgoCtx);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfHandleInt::ManualTriger()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_af_ManualTriger(mAlgoCtx);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfHandleInt::Tracking()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_af_Tracking(mAlgoCtx);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfHandleInt::setZoomIndex(int index)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    mCfgMutex.lock();
    rk_aiq_uapi_af_setZoomIndex(mAlgoCtx, index);
    isUpdateZoomPosDone = true;
    waitSignal();
    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfHandleInt::getZoomIndex(int *index)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_af_getZoomIndex(mAlgoCtx, index);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfHandleInt::endZoomChg()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    mCfgMutex.lock();
    rk_aiq_uapi_af_endZoomChg(mAlgoCtx);
    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfHandleInt::startZoomCalib()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    mCfgMutex.lock();
    rk_aiq_uapi_af_startZoomCalib(mAlgoCtx);
    isUpdateZoomPosDone = true;
    waitSignal();
    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfHandleInt::resetZoom()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    mCfgMutex.lock();
    rk_aiq_uapi_af_resetZoom(mAlgoCtx);
    isUpdateZoomPosDone = true;
    waitSignal();
    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfHandleInt::GetSearchPath(rk_aiq_af_sec_path_t* path)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_af_getSearchPath(mAlgoCtx, path);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfHandleInt::GetSearchResult(rk_aiq_af_result_t* result)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_af_getSearchResult(mAlgoCtx, result);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfHandleInt::GetFocusRange(rk_aiq_af_focusrange* range)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_af_getFocusRange(mAlgoCtx, range);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfHandleInt::prepare()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = RkAiqAfHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "af handle prepare failed");

    RkAiqAlgoConfigAfInt* af_config_int = (RkAiqAlgoConfigAfInt*)mConfig;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;

    af_config_int->af_config_com.af_mode = 6;
    af_config_int->af_config_com.win_h_offs = 0;
    af_config_int->af_config_com.win_v_offs = 0;
    af_config_int->af_config_com.win_h_size = 0;
    af_config_int->af_config_com.win_v_size = 0;
    af_config_int->af_config_com.lens_des = shared->snsDes.lens_des;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->prepare(mConfig);
    RKAIQCORE_CHECK_RET(ret, "af algo prepare failed");

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAfHandleInt::preProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPreAfInt* af_pre_int = (RkAiqAlgoPreAfInt*)mPreInParam;
    RkAiqAlgoPreResAfInt* af_pre_res_int = (RkAiqAlgoPreResAfInt*)mPreOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqPreResComb* comb = &shared->preResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAfHandle::preProcess();
    if (ret) {
        comb->af_pre_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "af handle preProcess failed");
    }

    comb->af_pre_res = NULL;
    af_pre_int->af_stats = &ispStats->af_stats;
    af_pre_int->aec_stats = &ispStats->aec_stats;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->pre_process(mPreInParam, mPreOutParam);
    RKAIQCORE_CHECK_RET(ret, "af algo pre_process failed");
    // set result to mAiqCore
    comb->af_pre_res = (RkAiqAlgoPreResAf*)af_pre_res_int;

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

bool RkAiqAfHandleInt::getValueFromFile(const char* path, int *pos)
{
    const char *delim = " ";
    char buffer[16] = {0};
    int fp;

    fp = open(path, O_RDONLY | O_SYNC);
    if (fp != -1) {
        if (read(fp, buffer, sizeof(buffer)) <= 0) {
            LOGE_AF("%s read %s failed!", __func__, path);
            goto OUT;
        } else {
            char *p = nullptr;

            p = strtok(buffer, delim);
            if (p != nullptr) {
                *pos = atoi(p);
            }
        }
        close(fp);
        return true;
    }

OUT:
    return false;
}

XCamReturn
RkAiqAfHandleInt::processing()
{
    ENTER_ANALYZER_FUNCTION();


    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoProcAfInt* af_proc_int = (RkAiqAlgoProcAfInt*)mProcInParam;
    RkAiqAlgoProcResAfInt* af_proc_res_int = (RkAiqAlgoProcResAfInt*)mProcOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqProcResComb* comb = &shared->procResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

#define ZOOM_MOVE_DEBUG
#ifdef ZOOM_MOVE_DEBUG
    int zoom_index = 0;

    if (getValueFromFile("/tmp/.zoom_pos", &zoom_index) == true) {
        if (mLastZoomIndex != zoom_index) {
            setZoomIndex(zoom_index);
            endZoomChg();
            mLastZoomIndex = zoom_index;
        }
    }
#endif

    ret = RkAiqAfHandle::processing();
    if (ret) {
        comb->af_proc_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "af handle processing failed");
    }

    comb->af_proc_res = NULL;
    af_proc_int->af_proc_com.af_stats = &ispStats->af_stats;
    af_proc_int->af_proc_com.aec_stats = &ispStats->aec_stats;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->processing(mProcInParam, mProcOutParam);
    RKAIQCORE_CHECK_RET(ret, "af algo processing failed");

    comb->af_proc_res = (RkAiqAlgoProcResAf*)af_proc_res_int;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfHandleInt::postProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPostAfInt* af_post_int = (RkAiqAlgoPostAfInt*)mPostInParam;
    RkAiqAlgoPostResAfInt* af_post_res_int = (RkAiqAlgoPostResAfInt*)mPostOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqPostResComb* comb = &shared->postResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAfHandle::postProcess();
    if (ret) {
        comb->af_post_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "af handle postProcess failed");
        return ret;
    }

    comb->af_post_res = NULL;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->post_process(mPostInParam, mPostOutParam);
    RKAIQCORE_CHECK_RET(ret, "af algo post_process failed");
    // set result to mAiqCore
    comb->af_post_res = (RkAiqAlgoPostResAf*)af_post_res_int ;

    if (updateAtt && isUpdateAttDone) {
        mCurAtt = mNewAtt;
        updateAtt = false;
        isUpdateAttDone = false;
        sendSignal();
    }

    if (isUpdateZoomPosDone) {
        isUpdateZoomPosDone = false;
        sendSignal();
    }

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAnrHandleInt::init()
{
    ENTER_ANALYZER_FUNCTION();

    RkAiqAnrHandle::deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAnrInt());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAnrInt());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAnrInt());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAnrInt());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAnrInt());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAnrInt());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAnrInt());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAnrHandleInt::updateConfig(bool needSync)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (needSync)
        mCfgMutex.lock();
    // if something changed
    if (updateAtt) {
        mCurAtt = mNewAtt;
        updateAtt = false;
        // TODO
        rk_aiq_uapi_anr_SetAttrib(mAlgoCtx, &mCurAtt, false);
        sendSignal();
    }

    if(UpdateIQpara) {
        mCurIQpara = mNewIQpara;
        UpdateIQpara = false;
        rk_aiq_uapi_anr_SetIQPara(mAlgoCtx, &mCurIQpara, false);
        sendSignal();
    }

    if (needSync)
        mCfgMutex.unlock();


    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAnrHandleInt::setAttrib(rk_aiq_nr_attrib_t *att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore

    // if something changed
    if (0 != memcmp(&mCurAtt, att, sizeof(rk_aiq_nr_attrib_t))) {
        RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
        if (shared->calib->mfnr.enable && shared->calib->mfnr.motion_detect_en) {
            if ((att->eMode == ANR_OP_MODE_AUTO) && (!att->stAuto.mfnrEn)) {
                att->stAuto.mfnrEn = !att->stAuto.mfnrEn;
                LOGE("motion detect is running, operate not permit!");
                goto EXIT;
            } else if ((att->eMode == ANR_OP_MODE_MANUAL) && (!att->stManual.mfnrEn)) {
                att->stManual.mfnrEn = !att->stManual.mfnrEn;
                LOGE("motion detect is running, operate not permit!");
                goto EXIT;
            }
        }
        mNewAtt = *att;
        updateAtt = true;
        waitSignal();
    }
EXIT:
    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAnrHandleInt::getAttrib(rk_aiq_nr_attrib_t *att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = rk_aiq_uapi_anr_GetAttrib(mAlgoCtx, att);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAnrHandleInt::setIQPara(rk_aiq_nr_IQPara_t *para)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore

    // if something changed
    if (0 != memcmp(&mCurIQpara, para, sizeof(rk_aiq_nr_IQPara_t))) {
        RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
        if (shared->calib->mfnr.enable && shared->calib->mfnr.motion_detect_en) {
            if((para->module_bits & (1 << ANR_MODULE_MFNR)) && !para->stMfnrPara.enable) {
                para->stMfnrPara.enable = !para->stMfnrPara.enable;
                LOGE("motion detect is running, disable mfnr is not permit!");
            }
        }
        mNewIQpara = *para;
        UpdateIQpara = true;
        waitSignal();
    }
EXIT:
    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAnrHandleInt::getIQPara(rk_aiq_nr_IQPara_t *para)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = rk_aiq_uapi_anr_GetIQPara(mAlgoCtx, para);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}


XCamReturn
RkAiqAnrHandleInt::setLumaSFStrength(float fPercent)
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ret = rk_aiq_uapi_anr_SetLumaSFStrength(mAlgoCtx, fPercent);
    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAnrHandleInt::setLumaTFStrength(float fPercent)
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ret = rk_aiq_uapi_anr_SetLumaTFStrength(mAlgoCtx, fPercent);
    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAnrHandleInt::getLumaSFStrength(float *pPercent)
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ret = rk_aiq_uapi_anr_GetLumaSFStrength(mAlgoCtx, pPercent);
    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAnrHandleInt::getLumaTFStrength(float *pPercent)
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ret = rk_aiq_uapi_anr_GetLumaTFStrength(mAlgoCtx, pPercent);
    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAnrHandleInt::setChromaSFStrength(float fPercent)
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ret = rk_aiq_uapi_anr_SetChromaSFStrength(mAlgoCtx, fPercent);
    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAnrHandleInt::setChromaTFStrength(float fPercent)
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ret = rk_aiq_uapi_anr_SetChromaTFStrength(mAlgoCtx, fPercent);
    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAnrHandleInt::getChromaSFStrength(float *pPercent)
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ret = rk_aiq_uapi_anr_GetChromaSFStrength(mAlgoCtx, pPercent);
    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAnrHandleInt::getChromaTFStrength(float *pPercent)
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ret = rk_aiq_uapi_anr_GetChromaTFStrength(mAlgoCtx, pPercent);
    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAnrHandleInt::setRawnrSFStrength(float fPercent)
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ret = rk_aiq_uapi_anr_SetRawnrSFStrength(mAlgoCtx, fPercent);
    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAnrHandleInt::getRawnrSFStrength(float *pPercent)
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ret = rk_aiq_uapi_anr_GetRawnrSFStrength(mAlgoCtx, pPercent);
    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAnrHandleInt::prepare()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = RkAiqAnrHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "anr handle prepare failed");

    RkAiqAlgoConfigAnrInt* anr_config_int = (RkAiqAlgoConfigAnrInt*)mConfig;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->prepare(mConfig);
    RKAIQCORE_CHECK_RET(ret, "anr algo prepare failed");

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAnrHandleInt::preProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPreAnrInt* anr_pre_int = (RkAiqAlgoPreAnrInt*)mPreInParam;
    RkAiqAlgoPreResAnrInt* anr_pre_res_int = (RkAiqAlgoPreResAnrInt*)mPreOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;
    RkAiqPreResComb* comb = &shared->preResComb;

    ret = RkAiqAnrHandle::preProcess();
    if (ret) {
        comb->anr_pre_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "anr handle preProcess failed");
    }

    comb->anr_pre_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->pre_process(mPreInParam, mPreOutParam);
    RKAIQCORE_CHECK_RET(ret, "anr algo pre_process failed");

    // set result to mAiqCore
    comb->anr_pre_res = (RkAiqAlgoPreResAnr*)anr_pre_res_int;

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAnrHandleInt::processing()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoProcAnrInt* anr_proc_int = (RkAiqAlgoProcAnrInt*)mProcInParam;
    RkAiqAlgoProcResAnrInt* anr_proc_res_int = (RkAiqAlgoProcResAnrInt*)mProcOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqProcResComb* comb = &shared->procResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;
    static int anr_proc_framecnt = 0;
    anr_proc_framecnt++;

    ret = RkAiqAnrHandle::processing();
    if (ret) {
        comb->anr_proc_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "anr handle processing failed");
    }
    comb->anr_proc_res = NULL;

    // TODO: fill procParam
    anr_proc_int->iso = shared->iso;

    anr_proc_int->hdr_mode = shared->working_mode;

    LOGD("%s:%d anr hdr_mode:%d  \n", __FUNCTION__, __LINE__, anr_proc_int->hdr_mode);


    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->processing(mProcInParam, mProcOutParam);
    RKAIQCORE_CHECK_RET(ret, "anr algo processing failed");

    comb->anr_proc_res = (RkAiqAlgoProcResAnr*)anr_proc_res_int;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAnrHandleInt::postProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPostAnrInt* anr_post_int = (RkAiqAlgoPostAnrInt*)mPostInParam;
    RkAiqAlgoPostResAnrInt* anr_post_res_int = (RkAiqAlgoPostResAnrInt*)mPostOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqPostResComb* comb = &shared->postResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAnrHandle::postProcess();
    if (ret) {
        comb->anr_post_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "anr handle postProcess failed");
        return ret;
    }

    comb->anr_post_res = NULL;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->post_process(mPostInParam, mPostOutParam);
    RKAIQCORE_CHECK_RET(ret, "anr algo post_process failed");
    // set result to mAiqCore
    comb->anr_post_res = (RkAiqAlgoPostResAnr*)anr_post_res_int ;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAsharpHandleInt::init()
{
    ENTER_ANALYZER_FUNCTION();

    RkAiqAsharpHandle::deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAsharpInt());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAsharpInt());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAsharpInt());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAsharpInt());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAsharpInt());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAsharpInt());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAsharpInt());

    EXIT_ANALYZER_FUNCTION();
}


XCamReturn
RkAiqAsharpHandleInt::updateConfig(bool needSync)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (needSync)
        mCfgMutex.lock();
    // if something changed
    if (updateAtt) {
        mCurAtt = mNewAtt;
        updateAtt = false;
        // TODO
        rk_aiq_uapi_asharp_SetAttrib(mAlgoCtx, &mCurAtt, false);
        sendSignal();
    }

    if(updateIQpara) {
        mCurIQPara = mNewIQPara;
        updateIQpara = false;
        // TODO
        rk_aiq_uapi_asharp_SetIQpara(mAlgoCtx, &mCurIQPara, false);
        sendSignal();
    }

    if (needSync)
        mCfgMutex.unlock();


    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAsharpHandleInt::setAttrib(rk_aiq_sharp_attrib_t *att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore

    // if something changed
    if (0 != memcmp(&mCurAtt, att, sizeof(rk_aiq_sharp_attrib_t))) {
        mNewAtt = *att;
        updateAtt = true;
        waitSignal();
    }

    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAsharpHandleInt::getAttrib(rk_aiq_sharp_attrib_t *att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_asharp_GetAttrib(mAlgoCtx, att);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAsharpHandleInt::setIQPara(rk_aiq_sharp_IQpara_t *para)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore

    // if something changed
    if (0 != memcmp(&mCurIQPara, para, sizeof(rk_aiq_sharp_IQpara_t))) {
        mNewIQPara = *para;
        updateIQpara = true;
        waitSignal();
    }

    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAsharpHandleInt::getIQPara(rk_aiq_sharp_IQpara_t *para)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_asharp_GetIQpara(mAlgoCtx, para);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}


XCamReturn
RkAiqAsharpHandleInt::setStrength(float fPercent)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_asharp_SetStrength(mAlgoCtx, fPercent);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}


XCamReturn
RkAiqAsharpHandleInt::getStrength(float *pPercent)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_asharp_GetStrength(mAlgoCtx, pPercent);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}


XCamReturn
RkAiqAsharpHandleInt::prepare()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = RkAiqAsharpHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "asharp handle prepare failed");

    RkAiqAlgoConfigAsharpInt* asharp_config_int = (RkAiqAlgoConfigAsharpInt*)mConfig;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->prepare(mConfig);
    RKAIQCORE_CHECK_RET(ret, "asharp algo prepare failed");

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAsharpHandleInt::preProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPreAsharpInt* asharp_pre_int = (RkAiqAlgoPreAsharpInt*)mPreInParam;
    RkAiqAlgoPreResAsharpInt* asharp_pre_res_int = (RkAiqAlgoPreResAsharpInt*)mPreOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;
    RkAiqPreResComb* comb = &shared->preResComb;

    ret = RkAiqAsharpHandle::preProcess();
    if (ret) {
        comb->asharp_pre_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "asharp handle preProcess failed");
    }

    comb->asharp_pre_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->pre_process(mPreInParam, mPreOutParam);
    RKAIQCORE_CHECK_RET(ret, "asharp algo pre_process failed");
    // set result to mAiqCore
    comb->asharp_pre_res = (RkAiqAlgoPreResAsharp*)asharp_pre_res_int;

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAsharpHandleInt::processing()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoProcAsharpInt* asharp_proc_int = (RkAiqAlgoProcAsharpInt*)mProcInParam;
    RkAiqAlgoProcResAsharpInt* asharp_proc_res_int = (RkAiqAlgoProcResAsharpInt*)mProcOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqProcResComb* comb = &shared->procResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;
    static int asharp_proc_framecnt = 0;
    asharp_proc_framecnt++;

    ret = RkAiqAsharpHandle::processing();
    if (ret) {
        comb->asharp_proc_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "asharp handle processing failed");
    }

    comb->asharp_proc_res = NULL;

    // TODO: fill procParam
    asharp_proc_int->iso = shared->iso;
    asharp_proc_int->hdr_mode = shared->working_mode;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->processing(mProcInParam, mProcOutParam);
    RKAIQCORE_CHECK_RET(ret, "asharp algo processing failed");

    comb->asharp_proc_res = (RkAiqAlgoProcResAsharp*)asharp_proc_res_int;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAsharpHandleInt::postProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPostAsharpInt* asharp_post_int = (RkAiqAlgoPostAsharpInt*)mPostInParam;
    RkAiqAlgoPostResAsharpInt* asharp_post_res_int = (RkAiqAlgoPostResAsharpInt*)mPostOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqPostResComb* comb = &shared->postResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAsharpHandle::postProcess();
    if (ret) {
        comb->asharp_post_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "asharp handle postProcess failed");
        return ret;
    }

    comb->asharp_post_res = NULL;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->post_process(mPostInParam, mPostOutParam);
    RKAIQCORE_CHECK_RET(ret, "asharp algo post_process failed");
    // set result to mAiqCore
    comb->asharp_post_res = (RkAiqAlgoPostResAsharp*)asharp_post_res_int ;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAdhazHandleInt::init()
{
    ENTER_ANALYZER_FUNCTION();

    RkAiqAdhazHandle::deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAdhazInt());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAdhazInt());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAdhazInt());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAdhazInt());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAdhazInt());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAdhazInt());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAdhazInt());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAdhazHandleInt::prepare()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = RkAiqAdhazHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "adhaz handle prepare failed");

    RkAiqAlgoConfigAdhazInt* adhaz_config_int = (RkAiqAlgoConfigAdhazInt*)mConfig;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;

    adhaz_config_int->calib = shared->calib;
    adhaz_config_int->rawHeight = shared->snsDes.isp_acq_height;
    adhaz_config_int->rawWidth = shared->snsDes.isp_acq_width;
    adhaz_config_int->working_mode = shared->working_mode;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->prepare(mConfig);
    RKAIQCORE_CHECK_RET(ret, "adhaz algo prepare failed");

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAdhazHandleInt::preProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPreAdhazInt* adhaz_pre_int = (RkAiqAlgoPreAdhazInt*)mPreInParam;
    RkAiqAlgoPreResAdhazInt* adhaz_pre_res_int = (RkAiqAlgoPreResAdhazInt*)mPreOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;
    RkAiqPreResComb* comb = &shared->preResComb;

    ret = RkAiqAdhazHandle::preProcess();
    if (ret) {
        comb->adhaz_pre_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "adhaz handle preProcess failed");
    }

    comb->adhaz_pre_res = NULL;

#ifdef RK_SIMULATOR_HW
    //nothing todo
#endif
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->pre_process(mPreInParam, mPreOutParam);
    RKAIQCORE_CHECK_RET(ret, "adhaz algo pre_process failed");

    // set result to mAiqCore
    comb->adhaz_pre_res = (RkAiqAlgoPreResAdhaz*)adhaz_pre_res_int;

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAdhazHandleInt::processing()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoProcAdhazInt* adhaz_proc_int = (RkAiqAlgoProcAdhazInt*)mProcInParam;
    RkAiqAlgoProcResAdhazInt* adhaz_proc_res_int = (RkAiqAlgoProcResAdhazInt*)mProcOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqProcResComb* comb = &shared->procResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    adhaz_proc_int->hdr_mode = shared->working_mode;

    ret = RkAiqAdhazHandle::processing();
    if (ret) {
        comb->adhaz_proc_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "adhaz handle processing failed");
    }

    adhaz_proc_int->pCalibDehaze = shared->calib;

    comb->adhaz_proc_res = NULL;

#ifdef RK_SIMULATOR_HW
    //nothing todo
#endif
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->processing(mProcInParam, mProcOutParam);
    RKAIQCORE_CHECK_RET(ret, "adhaz algo processing failed");

    comb->adhaz_proc_res = (RkAiqAlgoProcResAdhaz*)adhaz_proc_res_int;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAdhazHandleInt::postProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPostAdhazInt* adhaz_post_int = (RkAiqAlgoPostAdhazInt*)mPostInParam;
    RkAiqAlgoPostResAdhazInt* adhaz_post_res_int = (RkAiqAlgoPostResAdhazInt*)mPostOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqPostResComb* comb = &shared->postResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAdhazHandle::postProcess();
    if (ret) {
        comb->adhaz_post_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "adhaz handle postProcess failed");
        return ret;
    }

    comb->adhaz_post_res = NULL;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->post_process(mPostInParam, mPostOutParam);
    RKAIQCORE_CHECK_RET(ret, "adhaz algo post_process failed");
    // set result to mAiqCore
    comb->adhaz_post_res = (RkAiqAlgoPostResAdhaz*)adhaz_post_res_int ;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAdhazHandleInt::updateConfig(bool needSync)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (needSync)
        mCfgMutex.lock();
    // if something changed
    if (updateAtt) {
        mCurAtt = mNewAtt;
        updateAtt = false;
        // TODO
        rk_aiq_uapi_adehaze_SetAttrib(mAlgoCtx, mCurAtt, false);
        sendSignal();
    }

    if (needSync)
        mCfgMutex.unlock();


    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAdhazHandleInt::setSwAttrib(adehaze_sw_s att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore

    // if something changed
    if (0 != memcmp(&mCurAtt, &att, sizeof(adehaze_sw_s))) {
        mNewAtt = att;
        updateAtt = true;
        waitSignal();
    }

    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAdhazHandleInt::getSwAttrib(adehaze_sw_s *att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_adehaze_GetAttrib(mAlgoCtx, att);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAhdrHandleInt::init()
{
    ENTER_ANALYZER_FUNCTION();

    RkAiqAhdrHandle::deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAhdrInt());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAhdrInt());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAhdrInt());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAhdrInt());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAhdrInt());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAhdrInt());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAhdrInt());

    EXIT_ANALYZER_FUNCTION();
}
XCamReturn
RkAiqAhdrHandleInt::updateConfig(bool needSync)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (needSync)
        mCfgMutex.lock();
    // if something changed
    if (updateAtt) {
        mCurAtt = mNewAtt;
        updateAtt = false;
        rk_aiq_uapi_ahdr_SetAttrib(mAlgoCtx, mCurAtt, true);
        sendSignal();
    }
    if (needSync)
        mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAhdrHandleInt::setAttrib(ahdr_attrib_t att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore

    // if something changed
    if (0 != memcmp(&mCurAtt, &att, sizeof(ahdr_attrib_t))) {
        mNewAtt = att;
        updateAtt = true;
        waitSignal();
    }
    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}
XCamReturn
RkAiqAhdrHandleInt::getAttrib(ahdr_attrib_t* att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_ahdr_GetAttrib(mAlgoCtx, att);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAhdrHandleInt::prepare()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = RkAiqAhdrHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "ahdr handle prepare failed");

    RkAiqAlgoConfigAhdrInt* ahdr_config_int = (RkAiqAlgoConfigAhdrInt*)mConfig;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;

    //TODO
    ahdr_config_int->rawHeight = shared->snsDes.isp_acq_height;
    ahdr_config_int->rawWidth = shared->snsDes.isp_acq_width;
    ahdr_config_int->working_mode = shared->working_mode;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->prepare(mConfig);
    RKAIQCORE_CHECK_RET(ret, "ahdr algo prepare failed");

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAhdrHandleInt::preProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPreAhdrInt* ahdr_pre_int = (RkAiqAlgoPreAhdrInt*)mPreInParam;
    RkAiqAlgoPreResAhdrInt* ahdr_pre_res_int = (RkAiqAlgoPreResAhdrInt*)mPreOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;
    RkAiqPreResComb* comb = &shared->preResComb;

    ret = RkAiqAhdrHandle::preProcess();
    if (ret) {
        comb->ahdr_pre_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "ahdr handle preProcess failed");
    }

    if(!shared->ispStats.ahdr_stats_valid && !shared->init) {
        LOGD("no ahdr stats, ignore!");
        // TODO: keep last result ?
        //         comb->ahdr_proc_res = NULL;
        //
        return XCAM_RETURN_BYPASS;
    }

    comb->ahdr_pre_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->pre_process(mPreInParam, mPreOutParam);
    RKAIQCORE_CHECK_RET(ret, "ahdr algo pre_process failed");

    // set result to mAiqCore
    comb->ahdr_pre_res = (RkAiqAlgoPreResAhdr*)ahdr_pre_res_int;

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAhdrHandleInt::processing()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoProcAhdrInt* ahdr_proc_int = (RkAiqAlgoProcAhdrInt*)mProcInParam;
    RkAiqAlgoProcResAhdrInt* ahdr_proc_res_int = (RkAiqAlgoProcResAhdrInt*)mProcOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqProcResComb* comb = &shared->procResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;


    ret = RkAiqAhdrHandle::processing();
    if (ret) {
        comb->ahdr_proc_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "ahdr handle processing failed");
    }

    if(!shared->ispStats.ahdr_stats_valid && !shared->init) {
        LOGD("no ahdr stats, ignore!");
        // TODO: keep last result ?
        //         comb->ahdr_proc_res = NULL;
        //
        return XCAM_RETURN_BYPASS;
    } else {
        memcpy(&ahdr_proc_int->ispAhdrStats.tmo_stats, &ispStats->ahdr_stats.tmo_stats, sizeof(hdrtmo_stats_t));
        memcpy(ahdr_proc_int->ispAhdrStats.other_stats.tmo_luma,
               ispStats->aec_stats.ae_data.extra.rawae_big.channelg_xy, sizeof(ahdr_proc_int->ispAhdrStats.other_stats.tmo_luma));

        if(shared->working_mode == RK_AIQ_ISP_HDR_MODE_3_FRAME_HDR || shared->working_mode == RK_AIQ_ISP_HDR_MODE_3_LINE_HDR)
        {
            memcpy(ahdr_proc_int->ispAhdrStats.other_stats.short_luma,
                   ispStats->aec_stats.ae_data.chn[0].rawae_big.channelg_xy, sizeof(ahdr_proc_int->ispAhdrStats.other_stats.short_luma));
            memcpy(ahdr_proc_int->ispAhdrStats.other_stats.middle_luma,
                   ispStats->aec_stats.ae_data.chn[1].rawae_lite.channelg_xy, sizeof(ahdr_proc_int->ispAhdrStats.other_stats.middle_luma));
            memcpy(ahdr_proc_int->ispAhdrStats.other_stats.long_luma,
                   ispStats->aec_stats.ae_data.chn[2].rawae_big.channelg_xy, sizeof(ahdr_proc_int->ispAhdrStats.other_stats.long_luma));
        }
        else if(shared->working_mode == RK_AIQ_ISP_HDR_MODE_2_FRAME_HDR || shared->working_mode == RK_AIQ_ISP_HDR_MODE_2_LINE_HDR)
        {
            memcpy(ahdr_proc_int->ispAhdrStats.other_stats.short_luma,
                   ispStats->aec_stats.ae_data.chn[0].rawae_big.channelg_xy, sizeof(ahdr_proc_int->ispAhdrStats.other_stats.short_luma));
            memcpy(ahdr_proc_int->ispAhdrStats.other_stats.long_luma,
                   ispStats->aec_stats.ae_data.chn[1].rawae_big.channelg_xy, sizeof(ahdr_proc_int->ispAhdrStats.other_stats.long_luma));
        }
        else
            LOGD("Wrong working mode!!!");
    }

    comb->ahdr_proc_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->processing(mProcInParam, mProcOutParam);
    RKAIQCORE_CHECK_RET(ret, "ahdr algo processing failed");

    comb->ahdr_proc_res = (RkAiqAlgoProcResAhdr*)ahdr_proc_res_int;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAhdrHandleInt::postProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPostAhdrInt* ahdr_post_int = (RkAiqAlgoPostAhdrInt*)mPostInParam;
    RkAiqAlgoPostResAhdrInt* ahdr_post_res_int = (RkAiqAlgoPostResAhdrInt*)mPostOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqPostResComb* comb = &shared->postResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAhdrHandle::postProcess();
    if (ret) {
        comb->ahdr_post_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "ahdr handle postProcess failed");
        return ret;
    }

    if(!shared->ispStats.ahdr_stats_valid && !shared->init) {
        LOGD("no ahdr stats, ignore!");
        // TODO: keep last result ?
        //         comb->ahdr_proc_res = NULL;
        //
        return XCAM_RETURN_BYPASS;
    }

    comb->ahdr_post_res = NULL;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->post_process(mPostInParam, mPostOutParam);
    RKAIQCORE_CHECK_RET(ret, "ahdr algo post_process failed");
    // set result to mAiqCore
    comb->ahdr_post_res = (RkAiqAlgoPostResAhdr*)ahdr_post_res_int ;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAsdHandleInt::init()
{
    ENTER_ANALYZER_FUNCTION();

    RkAiqAsdHandle::deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAsdInt());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAsdInt());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAsdInt());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAsdInt());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAsdInt());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAsdInt());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAsdInt());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAsdHandleInt::updateConfig(bool needSync)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    if (needSync)
        mCfgMutex.lock();
    // if something changed
    if (updateAtt) {
        mCurAtt = mNewAtt;
        updateAtt = false;
        rk_aiq_uapi_asd_SetAttrib(mAlgoCtx, mCurAtt, false);
        sendSignal();
    }

    if (needSync)
        mCfgMutex.unlock();


    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAsdHandleInt::setAttrib(asd_attrib_t att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore

    // if something changed
    if (0 != memcmp(&mCurAtt, &att, sizeof(asd_attrib_t))) {
        mNewAtt = att;
        updateAtt = true;
        waitSignal();
    }

    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAsdHandleInt::getAttrib(asd_attrib_t *att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_asd_GetAttrib(mAlgoCtx, att);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAsdHandleInt::prepare()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = RkAiqAsdHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "asd handle prepare failed");

    RkAiqAlgoConfigAsdInt* asd_config_int = (RkAiqAlgoConfigAsdInt*)mConfig;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->prepare(mConfig);
    RKAIQCORE_CHECK_RET(ret, "asd algo prepare failed");

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAsdHandleInt::preProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPreAsdInt* asd_pre_int = (RkAiqAlgoPreAsdInt*)mPreInParam;
    RkAiqAlgoPreResAsdInt* asd_pre_res_int = (RkAiqAlgoPreResAsdInt*)mPreOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;
    RkAiqPreResComb* comb = &shared->preResComb;

    ret = RkAiqAsdHandle::preProcess();
    if (ret) {
        comb->asd_pre_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "asd handle preProcess failed");
    }

    comb->asd_pre_res = NULL;
    asd_pre_int->pre_params.cpsl_mode = shared->cpslCfg.mode;
    asd_pre_int->pre_params.cpsl_on = shared->cpslCfg.u.m.on;
    asd_pre_int->pre_params.cpsl_sensitivity = shared->cpslCfg.u.a.sensitivity;
    asd_pre_int->pre_params.cpsl_sw_interval = shared->cpslCfg.u.a.sw_interval;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->pre_process(mPreInParam, mPreOutParam);
    RKAIQCORE_CHECK_RET(ret, "asd algo pre_process failed");

    // set result to mAiqCore
    comb->asd_pre_res = (RkAiqAlgoPreResAsd*)asd_pre_res_int;

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAsdHandleInt::processing()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoProcAsdInt* asd_proc_int = (RkAiqAlgoProcAsdInt*)mProcInParam;
    RkAiqAlgoProcResAsdInt* asd_proc_res_int = (RkAiqAlgoProcResAsdInt*)mProcOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqProcResComb* comb = &shared->procResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAsdHandle::processing();
    if (ret) {
        comb->asd_proc_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "asd handle processing failed");
    }

    comb->asd_proc_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->processing(mProcInParam, mProcOutParam);
    RKAIQCORE_CHECK_RET(ret, "asd algo processing failed");

    comb->asd_proc_res = (RkAiqAlgoProcResAsd*)asd_proc_res_int;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAsdHandleInt::postProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPostAsdInt* asd_post_int = (RkAiqAlgoPostAsdInt*)mPostInParam;
    RkAiqAlgoPostResAsdInt* asd_post_res_int = (RkAiqAlgoPostResAsdInt*)mPostOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqPostResComb* comb = &shared->postResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAsdHandle::postProcess();
    if (ret) {
        comb->asd_post_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "asd handle postProcess failed");
        return ret;
    }

    comb->asd_post_res = NULL;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->post_process(mPostInParam, mPostOutParam);
    RKAIQCORE_CHECK_RET(ret, "asd algo post_process failed");
    // set result to mAiqCore
    comb->asd_post_res = (RkAiqAlgoPostResAsd*)asd_post_res_int ;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAcpHandleInt::init()
{
    ENTER_ANALYZER_FUNCTION();

    RkAiqAcpHandle::deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAcpInt());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAcpInt());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAcpInt());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAcpInt());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAcpInt());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAcpInt());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAcpInt());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAcpHandleInt::updateConfig(bool needSync)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (needSync)
        mCfgMutex.lock();
    // if something changed
    if (updateAtt) {
        mCurAtt = mNewAtt;
        updateAtt = false;
        rk_aiq_uapi_acp_SetAttrib(mAlgoCtx, mCurAtt, false);
        sendSignal();
    }
    if (needSync)
        mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAcpHandleInt::setAttrib(acp_attrib_t att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore
    if (0 != memcmp(&mCurAtt, &att, sizeof(acp_attrib_t))) {
        mNewAtt = att;
        updateAtt = true;
        waitSignal();
    }

    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAcpHandleInt::getAttrib(acp_attrib_t *att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_acp_GetAttrib(mAlgoCtx, att);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAcpHandleInt::prepare()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = RkAiqAcpHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "acp handle prepare failed");

    RkAiqAlgoConfigAcpInt* acp_config_int = (RkAiqAlgoConfigAcpInt*)mConfig;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->prepare(mConfig);
    RKAIQCORE_CHECK_RET(ret, "acp algo prepare failed");

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAcpHandleInt::preProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPreAcpInt* acp_pre_int = (RkAiqAlgoPreAcpInt*)mPreInParam;
    RkAiqAlgoPreResAcpInt* acp_pre_res_int = (RkAiqAlgoPreResAcpInt*)mPreOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;
    RkAiqPreResComb* comb = &shared->preResComb;

    ret = RkAiqAcpHandle::preProcess();
    if (ret) {
        comb->acp_pre_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "acp handle preProcess failed");
    }

    comb->acp_pre_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->pre_process(mPreInParam, mPreOutParam);
    RKAIQCORE_CHECK_RET(ret, "acp algo pre_process failed");

    // set result to mAiqCore
    comb->acp_pre_res = (RkAiqAlgoPreResAcp*)acp_pre_res_int;

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAcpHandleInt::processing()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoProcAcpInt* acp_proc_int = (RkAiqAlgoProcAcpInt*)mProcInParam;
    RkAiqAlgoProcResAcpInt* acp_proc_res_int = (RkAiqAlgoProcResAcpInt*)mProcOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqProcResComb* comb = &shared->procResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAcpHandle::processing();
    if (ret) {
        comb->acp_proc_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "acp handle processing failed");
    }

    comb->acp_proc_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->processing(mProcInParam, mProcOutParam);
    RKAIQCORE_CHECK_RET(ret, "acp algo processing failed");

    comb->acp_proc_res = (RkAiqAlgoProcResAcp*)acp_proc_res_int;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAcpHandleInt::postProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPostAcpInt* acp_post_int = (RkAiqAlgoPostAcpInt*)mPostInParam;
    RkAiqAlgoPostResAcpInt* acp_post_res_int = (RkAiqAlgoPostResAcpInt*)mPostOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqPostResComb* comb = &shared->postResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAcpHandle::postProcess();
    if (ret) {
        comb->acp_post_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "acp handle postProcess failed");
        return ret;
    }

    comb->acp_post_res = NULL;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->post_process(mPostInParam, mPostOutParam);
    RKAIQCORE_CHECK_RET(ret, "acp algo post_process failed");
    // set result to mAiqCore
    comb->acp_post_res = (RkAiqAlgoPostResAcp*)acp_post_res_int ;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqA3dlutHandleInt::init()
{
    ENTER_ANALYZER_FUNCTION();

    RkAiqA3dlutHandle::deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigA3dlutInt());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreA3dlutInt());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResA3dlutInt());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcA3dlutInt());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResA3dlutInt());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostA3dlutInt());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResA3dlutInt());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqA3dlutHandleInt::updateConfig(bool needSync)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (needSync)
        mCfgMutex.lock();
    // if something changed
    if (updateAtt) {
        mCurAtt = mNewAtt;
        updateAtt = false;
        // TODO
        rk_aiq_uapi_a3dlut_SetAttrib(mAlgoCtx, mCurAtt, false);
        sendSignal();
    }

    if (needSync)
        mCfgMutex.unlock();


    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqA3dlutHandleInt::setAttrib(rk_aiq_lut3d_attrib_t att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore

    // if something changed
    if (0 != memcmp(&mCurAtt, &att, sizeof(rk_aiq_lut3d_attrib_t))) {
        mNewAtt = att;
        updateAtt = true;
        waitSignal();
    }

    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqA3dlutHandleInt::getAttrib(rk_aiq_lut3d_attrib_t *att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_a3dlut_GetAttrib(mAlgoCtx, att);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqA3dlutHandleInt::query3dlutInfo(rk_aiq_lut3d_querry_info_t *lut3d_querry_info )
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_a3dlut_Query3dlutInfo(mAlgoCtx, lut3d_querry_info);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}


XCamReturn
RkAiqA3dlutHandleInt::prepare()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = RkAiqA3dlutHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "a3dlut handle prepare failed");

    RkAiqAlgoConfigA3dlutInt* a3dlut_config_int = (RkAiqAlgoConfigA3dlutInt*)mConfig;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->prepare(mConfig);
    RKAIQCORE_CHECK_RET(ret, "a3dlut algo prepare failed");

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqA3dlutHandleInt::preProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPreA3dlutInt* a3dlut_pre_int = (RkAiqAlgoPreA3dlutInt*)mPreInParam;
    RkAiqAlgoPreResA3dlutInt* a3dlut_pre_res_int = (RkAiqAlgoPreResA3dlutInt*)mPreOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;
    RkAiqPreResComb* comb = &shared->preResComb;

    ret = RkAiqA3dlutHandle::preProcess();
    if (ret) {
        comb->a3dlut_pre_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "a3dlut handle preProcess failed");
    }

    comb->a3dlut_pre_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->pre_process(mPreInParam, mPreOutParam);
    RKAIQCORE_CHECK_RET(ret, "a3dlut algo pre_process failed");

    // set result to mAiqCore
    comb->a3dlut_pre_res = (RkAiqAlgoPreResA3dlut*)a3dlut_pre_res_int;

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqA3dlutHandleInt::processing()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoProcA3dlutInt* a3dlut_proc_int = (RkAiqAlgoProcA3dlutInt*)mProcInParam;
    RkAiqAlgoProcResA3dlutInt* a3dlut_proc_res_int = (RkAiqAlgoProcResA3dlutInt*)mProcOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqProcResComb* comb = &shared->procResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqA3dlutHandle::processing();
    if (ret) {
        comb->a3dlut_proc_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "a3dlut handle processing failed");
    }

    comb->a3dlut_proc_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->processing(mProcInParam, mProcOutParam);
    RKAIQCORE_CHECK_RET(ret, "a3dlut algo processing failed");

    comb->a3dlut_proc_res = (RkAiqAlgoProcResA3dlut*)a3dlut_proc_res_int;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqA3dlutHandleInt::postProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPostA3dlutInt* a3dlut_post_int = (RkAiqAlgoPostA3dlutInt*)mPostInParam;
    RkAiqAlgoPostResA3dlutInt* a3dlut_post_res_int = (RkAiqAlgoPostResA3dlutInt*)mPostOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqPostResComb* comb = &shared->postResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqA3dlutHandle::postProcess();
    if (ret) {
        comb->a3dlut_post_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "a3dlut handle postProcess failed");
        return ret;
    }

    comb->a3dlut_post_res = NULL;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->post_process(mPostInParam, mPostOutParam);
    RKAIQCORE_CHECK_RET(ret, "a3dlut algo post_process failed");
    // set result to mAiqCore
    comb->a3dlut_post_res = (RkAiqAlgoPostResA3dlut*)a3dlut_post_res_int ;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAblcHandleInt::init()
{
    ENTER_ANALYZER_FUNCTION();

    RkAiqAblcHandle::deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAblcInt());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAblcInt());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAblcInt());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAblcInt());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAblcInt());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAblcInt());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAblcInt());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAblcHandleInt::updateConfig(bool needSync)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (needSync)
        mCfgMutex.lock();
    // if something changed
    if (updateAtt) {
        mCurAtt = mNewAtt;
        updateAtt = false;
        // TODO
        rk_aiq_uapi_ablc_SetAttrib(mAlgoCtx, &mCurAtt, false);
        sendSignal();
    }

    if (needSync)
        mCfgMutex.unlock();


    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAblcHandleInt::setAttrib(rk_aiq_blc_attrib_t *att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore

    // if something changed
    if (0 != memcmp(&mCurAtt, att, sizeof(rk_aiq_blc_attrib_t))) {
        mNewAtt = *att;
        updateAtt = true;
        waitSignal();
    }

    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAblcHandleInt::getAttrib(rk_aiq_blc_attrib_t *att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_ablc_GetAttrib(mAlgoCtx, att);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}


XCamReturn
RkAiqAblcHandleInt::prepare()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = RkAiqAblcHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "ablc handle prepare failed");

    RkAiqAlgoConfigAblcInt* ablc_config_int = (RkAiqAlgoConfigAblcInt*)mConfig;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->prepare(mConfig);
    RKAIQCORE_CHECK_RET(ret, "ablc algo prepare failed");

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAblcHandleInt::preProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPreAblcInt* ablc_pre_int = (RkAiqAlgoPreAblcInt*)mPreInParam;
    RkAiqAlgoPreResAblcInt* ablc_pre_res_int = (RkAiqAlgoPreResAblcInt*)mPreOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;
    RkAiqPreResComb* comb = &shared->preResComb;

    ret = RkAiqAblcHandle::preProcess();
    if (ret) {
        comb->ablc_pre_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "ablc handle preProcess failed");
    }

    comb->ablc_pre_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->pre_process(mPreInParam, mPreOutParam);
    RKAIQCORE_CHECK_RET(ret, "ablc algo pre_process failed");

    // set result to mAiqCore
    comb->ablc_pre_res = (RkAiqAlgoPreResAblc*)ablc_pre_res_int;

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAblcHandleInt::processing()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoProcAblcInt* ablc_proc_int = (RkAiqAlgoProcAblcInt*)mProcInParam;
    RkAiqAlgoProcResAblcInt* ablc_proc_res_int = (RkAiqAlgoProcResAblcInt*)mProcOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqProcResComb* comb = &shared->procResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAblcHandle::processing();
    if (ret) {
        comb->ablc_proc_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "ablc handle processing failed");
    }

    comb->ablc_proc_res = NULL;
    ablc_proc_int->iso = shared->iso;
    ablc_proc_int->hdr_mode = shared->working_mode;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->processing(mProcInParam, mProcOutParam);
    RKAIQCORE_CHECK_RET(ret, "ablc algo processing failed");

    comb->ablc_proc_res = (RkAiqAlgoProcResAblc*)ablc_proc_res_int;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAblcHandleInt::postProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPostAblcInt* ablc_post_int = (RkAiqAlgoPostAblcInt*)mPostInParam;
    RkAiqAlgoPostResAblcInt* ablc_post_res_int = (RkAiqAlgoPostResAblcInt*)mPostOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqPostResComb* comb = &shared->postResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAblcHandle::postProcess();
    if (ret) {
        comb->ablc_post_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "ablc handle postProcess failed");
        return ret;
    }

    comb->ablc_post_res = NULL;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->post_process(mPostInParam, mPostOutParam);
    RKAIQCORE_CHECK_RET(ret, "ablc algo post_process failed");
    // set result to mAiqCore
    comb->ablc_post_res = (RkAiqAlgoPostResAblc*)ablc_post_res_int ;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAccmHandleInt::init()
{
    ENTER_ANALYZER_FUNCTION();

    RkAiqAccmHandle::deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAccmInt());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAccmInt());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAccmInt());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAccmInt());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAccmInt());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAccmInt());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAccmInt());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAccmHandleInt::updateConfig(bool needSync)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (needSync)
        mCfgMutex.lock();
    // if something changed
    if (updateAtt) {
        mCurAtt = mNewAtt;
        updateAtt = false;
        // TODO
        rk_aiq_uapi_accm_SetAttrib(mAlgoCtx, mCurAtt, false);
        sendSignal();
    }

    if (needSync)
        mCfgMutex.unlock();


    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAccmHandleInt::setAttrib(rk_aiq_ccm_attrib_t att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore

    // if something changed
    if (0 != memcmp(&mCurAtt, &att, sizeof(rk_aiq_ccm_attrib_t))) {
        mNewAtt = att;
        updateAtt = true;
        waitSignal();
    }

    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAccmHandleInt::getAttrib(rk_aiq_ccm_attrib_t *att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_accm_GetAttrib(mAlgoCtx, att);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAccmHandleInt::queryCcmInfo(rk_aiq_ccm_querry_info_t *ccm_querry_info )
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_accm_QueryCcmInfo(mAlgoCtx, ccm_querry_info);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}


XCamReturn
RkAiqAccmHandleInt::prepare()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = RkAiqAccmHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "accm handle prepare failed");

    RkAiqAlgoConfigAccmInt* accm_config_int = (RkAiqAlgoConfigAccmInt*)mConfig;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->prepare(mConfig);
    RKAIQCORE_CHECK_RET(ret, "accm algo prepare failed");

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAccmHandleInt::preProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPreAccmInt* accm_pre_int = (RkAiqAlgoPreAccmInt*)mPreInParam;
    RkAiqAlgoPreResAccmInt* accm_pre_res_int = (RkAiqAlgoPreResAccmInt*)mPreOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;
    RkAiqPreResComb* comb = &shared->preResComb;

    ret = RkAiqAccmHandle::preProcess();
    if (ret) {
        comb->accm_pre_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "accm handle preProcess failed");
    }

    comb->accm_pre_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->pre_process(mPreInParam, mPreOutParam);
    RKAIQCORE_CHECK_RET(ret, "accm algo pre_process failed");

    // set result to mAiqCore
    comb->accm_pre_res = (RkAiqAlgoPreResAccm*)accm_pre_res_int;

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAccmHandleInt::processing()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoProcAccmInt* accm_proc_int = (RkAiqAlgoProcAccmInt*)mProcInParam;
    RkAiqAlgoProcResAccmInt* accm_proc_res_int = (RkAiqAlgoProcResAccmInt*)mProcOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqProcResComb* comb = &shared->procResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAccmHandle::processing();
    if (ret) {
        comb->accm_proc_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "accm handle processing failed");
    }

    comb->accm_proc_res = NULL;
    // TODO should check if the rk awb algo used
    RkAiqAlgoProcResAwb* awb_res =
        (RkAiqAlgoProcResAwb*)(shared->procResComb.awb_proc_res);
    RkAiqAlgoProcResAwbInt* awb_res_int =
        (RkAiqAlgoProcResAwbInt*)(shared->procResComb.awb_proc_res);

    if(awb_res ) {
        if(awb_res->awb_gain_algo.grgain < DIVMIN ||
                awb_res->awb_gain_algo.gbgain < DIVMIN ) {
            LOGE("get wrong awb gain from AWB module ,use default value ");
        } else {
            accm_proc_int->accm_sw_info.awbGain[0] =
                awb_res->awb_gain_algo.rgain / awb_res->awb_gain_algo.grgain;

            accm_proc_int->accm_sw_info.awbGain[1] = awb_res->awb_gain_algo.bgain / awb_res->awb_gain_algo.gbgain;
        }
        accm_proc_int->accm_sw_info.awbIIRDampCoef =  awb_res_int->awb_smooth_factor;
        accm_proc_int->accm_sw_info.varianceLuma = awb_res_int->varianceLuma;
        accm_proc_int->accm_sw_info.awbConverged = awb_res_int->awbConverged;
    } else {
        LOGE("fail to get awb gain form AWB module,use default value ");
    }
    // id != 0 means the thirdparty's algo
#if 0
    if (mDes->id != 0) {
        RkAiqAlgoPreResAeInt *ae_int = (RkAiqAlgoPreResAeInt*)shared->preResComb.ae_pre_res;
        if( ae_int) {
            if(shared->working_mode == RK_AIQ_WORKING_MODE_NORMAL) {
                accm_proc_int->accm_sw_info.sensorGain = ae_int->ae_pre_res_rk.LinearExp.exp_real_params.analog_gain
                        * ae_int->ae_pre_res_rk.LinearExp.exp_real_params.digital_gain
                        * ae_int->ae_pre_res_rk.LinearExp.exp_real_params.isp_dgain;
            } else if((rk_aiq_working_mode_t)shared->working_mode >= RK_AIQ_WORKING_MODE_ISP_HDR2
                      && (rk_aiq_working_mode_t)shared->working_mode < RK_AIQ_WORKING_MODE_ISP_HDR3)  {
                LOGD("%sensor gain choose from second hdr frame for accm");
                accm_proc_int->accm_sw_info.sensorGain = ae_int->ae_pre_res_rk.HdrExp[1].exp_real_params.analog_gain
                        * ae_int->ae_pre_res_rk.HdrExp[1].exp_real_params.digital_gain
                        * ae_int->ae_pre_res_rk.HdrExp[1].exp_real_params.isp_dgain;
            } else if((rk_aiq_working_mode_t)shared->working_mode >= RK_AIQ_WORKING_MODE_ISP_HDR2
                      && (rk_aiq_working_mode_t)shared->working_mode >= RK_AIQ_WORKING_MODE_ISP_HDR3)  {
                LOGD("sensor gain choose from third hdr frame for accm");
                accm_proc_int->accm_sw_info.sensorGain = ae_int->ae_pre_res_rk.HdrExp[2].exp_real_params.analog_gain
                        * ae_int->ae_pre_res_rk.HdrExp[2].exp_real_params.digital_gain
                        * ae_int->ae_pre_res_rk.HdrExp[2].exp_real_params.isp_dgain;
            } else {
                LOGE("working_mode (%d) is invaild ,fail to get sensor gain form AE module,use default value ",
                     shared->working_mode);
            }
        } else {
            LOGE("fail to get sensor gain form AE module,use default value ");
        }
    } else {
        RkAiqAlgoPreResAe *ae_int = (RkAiqAlgoPreResAe*)shared->preResComb.ae_pre_res;
        if( ae_int) {
            if(shared->working_mode == RK_AIQ_WORKING_MODE_NORMAL) {
                accm_proc_int->accm_sw_info.sensorGain = ae_int->ae_pre_res.LinearExp.exp_real_params.analog_gain
                        * ae_int->ae_pre_res.LinearExp.exp_real_params.digital_gain
                        * ae_int->ae_pre_res.LinearExp.exp_real_params.isp_dgain;
            } else if((rk_aiq_working_mode_t)shared->working_mode >= RK_AIQ_WORKING_MODE_ISP_HDR2
                      && (rk_aiq_working_mode_t)shared->working_mode < RK_AIQ_WORKING_MODE_ISP_HDR3)  {
                LOGD("%sensor gain choose from second hdr frame for accm");
                accm_proc_int->accm_sw_info.sensorGain = ae_int->ae_pre_res.HdrExp[1].exp_real_params.analog_gain
                        * ae_int->ae_pre_res.HdrExp[1].exp_real_params.digital_gain
                        * ae_int->ae_pre_res.HdrExp[1].exp_real_params.isp_dgain;
            } else if((rk_aiq_working_mode_t)shared->working_mode >= RK_AIQ_WORKING_MODE_ISP_HDR2
                      && (rk_aiq_working_mode_t)shared->working_mode >= RK_AIQ_WORKING_MODE_ISP_HDR3)  {
                LOGD("sensor gain choose from third hdr frame for accm");
                accm_proc_int->accm_sw_info.sensorGain = ae_int->ae_pre_res.HdrExp[2].exp_real_params.analog_gain
                        * ae_int->ae_pre_res.HdrExp[2].exp_real_params.digital_gain
                        * ae_int->ae_pre_res.HdrExp[2].exp_real_params.isp_dgain;
            } else {
                LOGE("working_mode (%d) is invaild ,fail to get sensor gain form AE module,use default value ",
                     shared->working_mode);
            }
        } else {
            LOGE("fail to get sensor gain form AE module,use default value ");
        }
    }
#else
    RKAiqAecExpInfo_t *pCurExp = &shared->curExp;
    if(pCurExp) {
        if((rk_aiq_working_mode_t)shared->working_mode == RK_AIQ_WORKING_MODE_NORMAL) {
            accm_proc_int->accm_sw_info.sensorGain = pCurExp->LinearExp.exp_real_params.analog_gain
                    * pCurExp->LinearExp.exp_real_params.digital_gain
                    * pCurExp->LinearExp.exp_real_params.isp_dgain;
        } else if((rk_aiq_working_mode_t)shared->working_mode >= RK_AIQ_WORKING_MODE_ISP_HDR2
                  && (rk_aiq_working_mode_t)shared->working_mode < RK_AIQ_WORKING_MODE_ISP_HDR3)  {
            LOGD("sensor gain choose from second hdr frame for accm");
            accm_proc_int->accm_sw_info.sensorGain = pCurExp->HdrExp[1].exp_real_params.analog_gain
                    * pCurExp->HdrExp[1].exp_real_params.digital_gain
                    * pCurExp->HdrExp[1].exp_real_params.isp_dgain;
        } else if((rk_aiq_working_mode_t)shared->working_mode >= RK_AIQ_WORKING_MODE_ISP_HDR2
                  && (rk_aiq_working_mode_t)shared->working_mode >= RK_AIQ_WORKING_MODE_ISP_HDR3)  {
            LOGD("sensor gain choose from third hdr frame for accm");
            accm_proc_int->accm_sw_info.sensorGain = pCurExp->HdrExp[2].exp_real_params.analog_gain
                    * pCurExp->HdrExp[2].exp_real_params.digital_gain
                    * pCurExp->HdrExp[2].exp_real_params.isp_dgain;
        } else {
            LOGE("working_mode (%d) is invaild ,fail to get sensor gain form AE module,use default value ",
                 shared->working_mode);
        }
    } else {
        LOGE("fail to get sensor gain form AE module,use default value ");
    }
#endif
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->processing(mProcInParam, mProcOutParam);
    RKAIQCORE_CHECK_RET(ret, "accm algo processing failed");

    comb->accm_proc_res = (RkAiqAlgoProcResAccm*)accm_proc_res_int;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAccmHandleInt::postProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPostAccmInt* accm_post_int = (RkAiqAlgoPostAccmInt*)mPostInParam;
    RkAiqAlgoPostResAccmInt* accm_post_res_int = (RkAiqAlgoPostResAccmInt*)mPostOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqPostResComb* comb = &shared->postResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAccmHandle::postProcess();
    if (ret) {
        comb->accm_post_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "accm handle postProcess failed");
        return ret;
    }

    comb->accm_post_res = NULL;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->post_process(mPostInParam, mPostOutParam);
    RKAIQCORE_CHECK_RET(ret, "accm algo post_process failed");
    // set result to mAiqCore
    comb->accm_post_res = (RkAiqAlgoPostResAccm*)accm_post_res_int ;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAcgcHandleInt::init()
{
    ENTER_ANALYZER_FUNCTION();

    RkAiqAcgcHandle::deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAcgcInt());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAcgcInt());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAcgcInt());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAcgcInt());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAcgcInt());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAcgcInt());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAcgcInt());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAcgcHandleInt::prepare()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = RkAiqAcgcHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "acgc handle prepare failed");

    RkAiqAlgoConfigAcgcInt* acgc_config_int = (RkAiqAlgoConfigAcgcInt*)mConfig;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->prepare(mConfig);
    RKAIQCORE_CHECK_RET(ret, "acgc algo prepare failed");

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAcgcHandleInt::preProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPreAcgcInt* acgc_pre_int = (RkAiqAlgoPreAcgcInt*)mPreInParam;
    RkAiqAlgoPreResAcgcInt* acgc_pre_res_int = (RkAiqAlgoPreResAcgcInt*)mPreOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;
    RkAiqPreResComb* comb = &shared->preResComb;

    ret = RkAiqAcgcHandle::preProcess();
    if (ret) {
        comb->acgc_pre_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "acgc handle preProcess failed");
    }

    comb->acgc_pre_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->pre_process(mPreInParam, mPreOutParam);
    RKAIQCORE_CHECK_RET(ret, "acgc algo pre_process failed");

    // set result to mAiqCore
    comb->acgc_pre_res = (RkAiqAlgoPreResAcgc*)acgc_pre_res_int;

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAcgcHandleInt::processing()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoProcAcgcInt* acgc_proc_int = (RkAiqAlgoProcAcgcInt*)mProcInParam;
    RkAiqAlgoProcResAcgcInt* acgc_proc_res_int = (RkAiqAlgoProcResAcgcInt*)mProcOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqProcResComb* comb = &shared->procResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAcgcHandle::processing();
    if (ret) {
        comb->acgc_proc_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "acgc handle processing failed");
    }

    comb->acgc_proc_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->processing(mProcInParam, mProcOutParam);
    RKAIQCORE_CHECK_RET(ret, "acgc algo processing failed");

    comb->acgc_proc_res = (RkAiqAlgoProcResAcgc*)acgc_proc_res_int;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAcgcHandleInt::postProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPostAcgcInt* acgc_post_int = (RkAiqAlgoPostAcgcInt*)mPostInParam;
    RkAiqAlgoPostResAcgcInt* acgc_post_res_int = (RkAiqAlgoPostResAcgcInt*)mPostOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqPostResComb* comb = &shared->postResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAcgcHandle::postProcess();
    if (ret) {
        comb->acgc_post_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "acgc handle postProcess failed");
        return ret;
    }

    comb->acgc_post_res = NULL;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->post_process(mPostInParam, mPostOutParam);
    RKAIQCORE_CHECK_RET(ret, "acgc algo post_process failed");
    // set result to mAiqCore
    comb->acgc_post_res = (RkAiqAlgoPostResAcgc*)acgc_post_res_int ;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAdebayerHandleInt::init()
{
    ENTER_ANALYZER_FUNCTION();

    RkAiqAdebayerHandle::deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAdebayerInt());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAdebayerInt());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAdebayerInt());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAdebayerInt());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAdebayerInt());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAdebayerInt());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAdebayerInt());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAdebayerHandleInt::updateConfig(bool needSync)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (needSync)
        mCfgMutex.lock();
    // if something changed
    if (updateAtt) {
        mCurAtt = mNewAtt;
        updateAtt = false;
        rk_aiq_uapi_adebayer_SetAttrib(mAlgoCtx, mCurAtt, false);
        sendSignal();
    }
    if (needSync)
        mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAdebayerHandleInt::setAttrib(adebayer_attrib_t att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore
    if (0 != memcmp(&mCurAtt, &att, sizeof(adebayer_attrib_t))) {
        mNewAtt = att;
        updateAtt = true;
        waitSignal();
    }

    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAdebayerHandleInt::getAttrib(adebayer_attrib_t *att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_adebayer_GetAttrib(mAlgoCtx, att);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAdebayerHandleInt::prepare()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = RkAiqAdebayerHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "adebayer handle prepare failed");

    RkAiqAlgoConfigAdebayerInt* adebayer_config_int = (RkAiqAlgoConfigAdebayerInt*)mConfig;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->prepare(mConfig);
    RKAIQCORE_CHECK_RET(ret, "adebayer algo prepare failed");

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAdebayerHandleInt::preProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPreAdebayerInt* adebayer_pre_int = (RkAiqAlgoPreAdebayerInt*)mPreInParam;
    RkAiqAlgoPreResAdebayerInt* adebayer_pre_res_int = (RkAiqAlgoPreResAdebayerInt*)mPreOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;
    RkAiqPreResComb* comb = &shared->preResComb;

    ret = RkAiqAdebayerHandle::preProcess();
    if (ret) {
        comb->adebayer_pre_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "adebayer handle preProcess failed");
    }

    comb->adebayer_pre_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->pre_process(mPreInParam, mPreOutParam);
    RKAIQCORE_CHECK_RET(ret, "adebayer algo pre_process failed");

    // set result to mAiqCore
    comb->adebayer_pre_res = (RkAiqAlgoPreResAdebayer*)adebayer_pre_res_int;

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAdebayerHandleInt::processing()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoProcAdebayerInt* adebayer_proc_int = (RkAiqAlgoProcAdebayerInt*)mProcInParam;
    RkAiqAlgoProcResAdebayerInt* adebayer_proc_res_int = (RkAiqAlgoProcResAdebayerInt*)mProcOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqProcResComb* comb = &shared->procResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAdebayerHandle::processing();
    if (ret) {
        comb->adebayer_proc_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "adebayer handle processing failed");
    }

    comb->adebayer_proc_res = NULL;
    // TODO: fill procParam
    adebayer_proc_int->hdr_mode = shared->working_mode;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->processing(mProcInParam, mProcOutParam);
    RKAIQCORE_CHECK_RET(ret, "adebayer algo processing failed");

    comb->adebayer_proc_res = (RkAiqAlgoProcResAdebayer*)adebayer_proc_res_int;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAdebayerHandleInt::postProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPostAdebayerInt* adebayer_post_int = (RkAiqAlgoPostAdebayerInt*)mPostInParam;
    RkAiqAlgoPostResAdebayerInt* adebayer_post_res_int = (RkAiqAlgoPostResAdebayerInt*)mPostOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqPostResComb* comb = &shared->postResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAdebayerHandle::postProcess();
    if (ret) {
        comb->adebayer_post_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "adebayer handle postProcess failed");
        return ret;
    }

    comb->adebayer_post_res = NULL;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->post_process(mPostInParam, mPostOutParam);
    RKAIQCORE_CHECK_RET(ret, "adebayer algo post_process failed");
    // set result to mAiqCore
    comb->adebayer_post_res = (RkAiqAlgoPostResAdebayer*)adebayer_post_res_int ;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAdpccHandleInt::init()
{
    ENTER_ANALYZER_FUNCTION();

    RkAiqAdpccHandle::deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAdpccInt());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAdpccInt());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAdpccInt());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAdpccInt());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAdpccInt());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAdpccInt());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAdpccInt());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAdpccHandleInt::updateConfig(bool needSync)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (needSync)
        mCfgMutex.lock();
    // if something changed
    if (updateAtt) {
        mCurAtt = mNewAtt;
        updateAtt = false;
        // TODO
        rk_aiq_uapi_adpcc_SetAttrib(mAlgoCtx, &mCurAtt, false);
        sendSignal();
    }

    if (needSync)
        mCfgMutex.unlock();


    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAdpccHandleInt::setAttrib(rk_aiq_dpcc_attrib_t *att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore

    // if something changed
    if (0 != memcmp(&mCurAtt, att, sizeof(rk_aiq_dpcc_attrib_t))) {
        mNewAtt = *att;
        updateAtt = true;
        waitSignal();
    }

    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAdpccHandleInt::getAttrib(rk_aiq_dpcc_attrib_t *att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_adpcc_GetAttrib(mAlgoCtx, att);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}



XCamReturn
RkAiqAdpccHandleInt::prepare()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = RkAiqAdpccHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "adpcc handle prepare failed");

    RkAiqAlgoConfigAdpccInt* adpcc_config_int = (RkAiqAlgoConfigAdpccInt*)mConfig;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->prepare(mConfig);
    RKAIQCORE_CHECK_RET(ret, "adpcc algo prepare failed");

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAdpccHandleInt::preProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPreAdpccInt* adpcc_pre_int = (RkAiqAlgoPreAdpccInt*)mPreInParam;
    RkAiqAlgoPreResAdpccInt* adpcc_pre_res_int = (RkAiqAlgoPreResAdpccInt*)mPreOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;
    RkAiqPreResComb* comb = &shared->preResComb;

    ret = RkAiqAdpccHandle::preProcess();
    if (ret) {
        comb->adpcc_pre_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "adpcc handle preProcess failed");
    }

    comb->adpcc_pre_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->pre_process(mPreInParam, mPreOutParam);
    RKAIQCORE_CHECK_RET(ret, "adpcc algo pre_process failed");

    // set result to mAiqCore
    comb->adpcc_pre_res = (RkAiqAlgoPreResAdpcc*)adpcc_pre_res_int;

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAdpccHandleInt::processing()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoProcAdpccInt* adpcc_proc_int = (RkAiqAlgoProcAdpccInt*)mProcInParam;
    RkAiqAlgoProcResAdpccInt* adpcc_proc_res_int = (RkAiqAlgoProcResAdpccInt*)mProcOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqProcResComb* comb = &shared->procResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAdpccHandle::processing();
    if (ret) {
        comb->adpcc_proc_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "adpcc handle processing failed");
    }

    comb->adpcc_proc_res = NULL;
    // TODO: fill procParam
    adpcc_proc_int->iso = shared->iso;
    adpcc_proc_int->hdr_mode = shared->working_mode;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->processing(mProcInParam, mProcOutParam);
    RKAIQCORE_CHECK_RET(ret, "adpcc algo processing failed");

    comb->adpcc_proc_res = (RkAiqAlgoProcResAdpcc*)adpcc_proc_res_int;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAdpccHandleInt::postProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPostAsdInt* asd_post_int = (RkAiqAlgoPostAsdInt*)mPostInParam;
    RkAiqAlgoPostResAsdInt* asd_post_res_int = (RkAiqAlgoPostResAsdInt*)mPostOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqPostResComb* comb = &shared->postResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAdpccHandle::postProcess();
    if (ret) {
        comb->asd_post_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "asd handle postProcess failed");
        return ret;
    }

    comb->asd_post_res = NULL;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->post_process(mPostInParam, mPostOutParam);
    RKAIQCORE_CHECK_RET(ret, "asd algo post_process failed");
    // set result to mAiqCore
    comb->asd_post_res = (RkAiqAlgoPostResAsd*)asd_post_res_int ;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAfecHandleInt::init()
{
    ENTER_ANALYZER_FUNCTION();

    RkAiqAfecHandle::deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAfecInt());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAfecInt());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAfecInt());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAfecInt());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAfecInt());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAfecInt());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAfecInt());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAfecHandleInt::prepare()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = RkAiqAfecHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "afec handle prepare failed");

    RkAiqAlgoConfigAfecInt* afec_config_int = (RkAiqAlgoConfigAfecInt*)mConfig;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;

    /* memcpy(&afec_config_int->afec_calib_cfg, &shared->calib->afec, sizeof(CalibDb_FEC_t)); */
    afec_config_int->resource_path = shared->resourcePath;
    afec_config_int->mem_ops_ptr = mAiqCore->mShareMemOps;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->prepare(mConfig);
    RKAIQCORE_CHECK_RET(ret, "afec algo prepare failed");

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAfecHandleInt::preProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPreAfecInt* afec_pre_int = (RkAiqAlgoPreAfecInt*)mPreInParam;
    RkAiqAlgoPreResAfecInt* afec_pre_res_int = (RkAiqAlgoPreResAfecInt*)mPreOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;
    RkAiqPreResComb* comb = &shared->preResComb;

    ret = RkAiqAfecHandle::preProcess();
    if (ret) {
        comb->afec_pre_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "afec handle preProcess failed");
    }

    comb->afec_pre_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->pre_process(mPreInParam, mPreOutParam);
    RKAIQCORE_CHECK_RET(ret, "afec algo pre_process failed");

    // set result to mAiqCore
    comb->afec_pre_res = (RkAiqAlgoPreResAfec*)afec_pre_res_int;

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAfecHandleInt::processing()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoProcAfecInt* afec_proc_int = (RkAiqAlgoProcAfecInt*)mProcInParam;
    RkAiqAlgoProcResAfecInt* afec_proc_res_int = (RkAiqAlgoProcResAfecInt*)mProcOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqProcResComb* comb = &shared->procResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAfecHandle::processing();
    if (ret) {
        comb->afec_proc_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "afec handle processing failed");
    }

    comb->afec_proc_res = NULL;
    //fill procParam
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->processing(mProcInParam, mProcOutParam);
    RKAIQCORE_CHECK_RET(ret, "afec algo processing failed");

    comb->afec_proc_res = (RkAiqAlgoProcResAfec*)afec_proc_res_int;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfecHandleInt::postProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPostAfecInt* afec_post_int = (RkAiqAlgoPostAfecInt*)mPostInParam;
    RkAiqAlgoPostResAfecInt* afec_post_res_int = (RkAiqAlgoPostResAfecInt*)mPostOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqPostResComb* comb = &shared->postResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAfecHandle::postProcess();
    if (ret) {
        comb->afec_post_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "afec handle postProcess failed");
        return ret;
    }

    comb->afec_post_res = NULL;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->post_process(mPostInParam, mPostOutParam);
    RKAIQCORE_CHECK_RET(ret, "afec algo post_process failed");
    // set result to mAiqCore
    comb->afec_post_res = (RkAiqAlgoPostResAfec*)afec_post_res_int ;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfecHandleInt::updateConfig(bool needSync)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (needSync)
        mCfgMutex.lock();
    // if something changed
    if (updateAtt) {
        mCurAtt = mNewAtt;
        updateAtt = false;
        // TODO
        rk_aiq_uapi_afec_SetAttrib(mAlgoCtx, mCurAtt, false);
        sendSignal();
    }

    if (needSync)
        mCfgMutex.unlock();


    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfecHandleInt::setAttrib(rk_aiq_fec_attrib_t att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore

    // if something changed
    if (0 != memcmp(&mCurAtt, &att, sizeof(rk_aiq_fec_attrib_t))) {
        mNewAtt = att;
        updateAtt = true;
        waitSignal();
    }

    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAfecHandleInt::getAttrib(rk_aiq_fec_attrib_t *att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_afec_GetAttrib(mAlgoCtx, att);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAgammaHandleInt::init()
{
    ENTER_ANALYZER_FUNCTION();

    RkAiqAgammaHandle::deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAgammaInt());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAgammaInt());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAgammaInt());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAgammaInt());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAgammaInt());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAgammaInt());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAgammaInt());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAgammaHandleInt::updateConfig(bool needSync)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (needSync)
        mCfgMutex.lock();
    // if something changed
    if (updateAtt) {
        mCurAtt = mNewAtt;
        updateAtt = false;
        // TODO
        rk_aiq_uapi_agamma_SetAttrib(mAlgoCtx, mCurAtt, false);
        waitSignal();
    }

    if (needSync)
        mCfgMutex.unlock();


    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAgammaHandleInt::setAttrib(rk_aiq_gamma_attrib_t att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore

    // if something changed
    if (0 != memcmp(&mCurAtt, &att, sizeof(rk_aiq_gamma_attrib_t))) {
        mNewAtt = att;
        updateAtt = true;
        sendSignal();
    }

    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAgammaHandleInt::getAttrib(rk_aiq_gamma_attrib_t *att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_agamma_GetAttrib(mAlgoCtx, att);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}


XCamReturn
RkAiqAgammaHandleInt::prepare()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = RkAiqAgammaHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "agamma handle prepare failed");

    RkAiqAlgoConfigAgammaInt* agamma_config_int = (RkAiqAlgoConfigAgammaInt*)mConfig;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;

    agamma_config_int->calib = shared ->calib;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->prepare(mConfig);
    RKAIQCORE_CHECK_RET(ret, "agamma algo prepare failed");

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAgammaHandleInt::preProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPreAgammaInt* agamma_pre_int = (RkAiqAlgoPreAgammaInt*)mPreInParam;
    RkAiqAlgoPreResAgammaInt* agamma_pre_res_int = (RkAiqAlgoPreResAgammaInt*)mPreOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;
    RkAiqPreResComb* comb = &shared->preResComb;

    ret = RkAiqAgammaHandle::preProcess();
    if (ret) {
        comb->agamma_pre_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "agamma handle preProcess failed");
    }

    comb->agamma_pre_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->pre_process(mPreInParam, mPreOutParam);
    RKAIQCORE_CHECK_RET(ret, "agamma algo pre_process failed");

    // set result to mAiqCore
    comb->agamma_pre_res = (RkAiqAlgoPreResAgamma*)agamma_pre_res_int;

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAgammaHandleInt::processing()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoProcAgammaInt* agamma_proc_int = (RkAiqAlgoProcAgammaInt*)mProcInParam;
    RkAiqAlgoProcResAgammaInt* agamma_proc_res_int = (RkAiqAlgoProcResAgammaInt*)mProcOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqProcResComb* comb = &shared->procResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAgammaHandle::processing();
    if (ret) {
        comb->agamma_proc_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "agamma handle processing failed");
    }

    comb->agamma_proc_res = NULL;
    agamma_proc_int->calib = shared->calib;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->processing(mProcInParam, mProcOutParam);
    RKAIQCORE_CHECK_RET(ret, "agamma algo processing failed");

    comb->agamma_proc_res = (RkAiqAlgoProcResAgamma*)agamma_proc_res_int;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAgammaHandleInt::postProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPostAgammaInt* agamma_post_int = (RkAiqAlgoPostAgammaInt*)mPostInParam;
    RkAiqAlgoPostResAgammaInt* agamma_post_res_int = (RkAiqAlgoPostResAgammaInt*)mPostOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqPostResComb* comb = &shared->postResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAgammaHandle::postProcess();
    if (ret) {
        comb->agamma_post_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "agamma handle postProcess failed");
        return ret;
    }

    comb->agamma_post_res = NULL;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->post_process(mPostInParam, mPostOutParam);
    RKAIQCORE_CHECK_RET(ret, "agamma algo post_process failed");
    // set result to mAiqCore
    comb->agamma_post_res = (RkAiqAlgoPostResAgamma*)agamma_post_res_int ;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAdegammaHandleInt::init()
{
    ENTER_ANALYZER_FUNCTION();

    RkAiqAdegammaHandle::deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAdegammaInt());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAdegammaInt());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAdegammaInt());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAdegammaInt());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAdegammaInt());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAdegammaInt());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAdegammaInt());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAdegammaHandleInt::updateConfig(bool needSync)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (needSync)
        mCfgMutex.lock();
    // if something changed
    if (updateAtt) {
        mCurAtt = mNewAtt;
        updateAtt = false;
        // TODO
        rk_aiq_uapi_adegamma_SetAttrib(mAlgoCtx, mCurAtt, false);
        waitSignal();
    }

    if (needSync)
        mCfgMutex.unlock();


    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAdegammaHandleInt::setAttrib(rk_aiq_degamma_attrib_t att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore

    // if something changed
    if (0 != memcmp(&mCurAtt, &att, sizeof(rk_aiq_degamma_attrib_t))) {
        mNewAtt = att;
        updateAtt = true;
        sendSignal();
    }

    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAdegammaHandleInt::getAttrib(rk_aiq_degamma_attrib_t *att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_adegamma_GetAttrib(mAlgoCtx, att);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}


XCamReturn
RkAiqAdegammaHandleInt::prepare()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = RkAiqAdegammaHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "adegamma handle prepare failed");

    RkAiqAlgoConfigAdegammaInt* adegamma_config_int = (RkAiqAlgoConfigAdegammaInt*)mConfig;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;

    adegamma_config_int->calib = shared ->calib;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->prepare(mConfig);
    RKAIQCORE_CHECK_RET(ret, "adegamma algo prepare failed");

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAdegammaHandleInt::preProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPreAdegammaInt* adegamma_pre_int = (RkAiqAlgoPreAdegammaInt*)mPreInParam;
    RkAiqAlgoPreResAdegammaInt* adegamma_pre_res_int = (RkAiqAlgoPreResAdegammaInt*)mPreOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;
    RkAiqPreResComb* comb = &shared->preResComb;

    ret = RkAiqAdegammaHandle::preProcess();
    if (ret) {
        comb->adegamma_pre_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "adegamma handle preProcess failed");
    }

    comb->adegamma_pre_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->pre_process(mPreInParam, mPreOutParam);
    RKAIQCORE_CHECK_RET(ret, "adegamma algo pre_process failed");

    // set result to mAiqCore
    comb->adegamma_pre_res = (RkAiqAlgoPreResAdegamma*)adegamma_pre_res_int;

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAdegammaHandleInt::processing()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoProcAdegammaInt* adegamma_proc_int = (RkAiqAlgoProcAdegammaInt*)mProcInParam;
    RkAiqAlgoProcResAdegammaInt* adegamma_proc_res_int = (RkAiqAlgoProcResAdegammaInt*)mProcOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqProcResComb* comb = &shared->procResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAdegammaHandle::processing();
    if (ret) {
        comb->adegamma_proc_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "adegamma handle processing failed");
    }

    comb->adegamma_proc_res = NULL;
    adegamma_proc_int->calib = shared->calib;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->processing(mProcInParam, mProcOutParam);
    RKAIQCORE_CHECK_RET(ret, "adegamma algo processing failed");

    comb->adegamma_proc_res = (RkAiqAlgoProcResAdegamma*)adegamma_proc_res_int;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAdegammaHandleInt::postProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPostAdegammaInt* adegamma_post_int = (RkAiqAlgoPostAdegammaInt*)mPostInParam;
    RkAiqAlgoPostResAdegammaInt* adegamma_post_res_int = (RkAiqAlgoPostResAdegammaInt*)mPostOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqPostResComb* comb = &shared->postResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAdegammaHandle::postProcess();
    if (ret) {
        comb->adegamma_post_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "adegamma handle postProcess failed");
        return ret;
    }

    comb->adegamma_post_res = NULL;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->post_process(mPostInParam, mPostOutParam);
    RKAIQCORE_CHECK_RET(ret, "agamma algo post_process failed");
    // set result to mAiqCore
    comb->adegamma_post_res = (RkAiqAlgoPostResAdegamma*)adegamma_post_res_int ;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAgicHandleInt::init()
{
    ENTER_ANALYZER_FUNCTION();

    RkAiqAgicHandle::deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAgicInt());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAgicInt());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAgicInt());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAgicInt());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAgicInt());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAgicInt());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAgicInt());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAgicHandleInt::updateConfig(bool needSync)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (needSync)
        mCfgMutex.lock();
    // if something changed
    if (updateAtt) {
        mCurAtt = mNewAtt;
        updateAtt = false;
        rk_aiq_uapi_agic_SetAttrib(mAlgoCtx, mCurAtt, false);
        sendSignal();
    }
    if (needSync)
        mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAgicHandleInt::setAttrib(agic_attrib_t att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore
    if (0 != memcmp(&mCurAtt, &att, sizeof(agic_attrib_t))) {
        mNewAtt = att;
        updateAtt = true;
        waitSignal();
    }

    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAgicHandleInt::getAttrib(agic_attrib_t *att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_agic_GetAttrib(mAlgoCtx, att);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAgicHandleInt::prepare()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = RkAiqAgicHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "agic handle prepare failed");

    RkAiqAlgoConfigAgicInt* agic_config_int = (RkAiqAlgoConfigAgicInt*)mConfig;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->prepare(mConfig);
    RKAIQCORE_CHECK_RET(ret, "agic algo prepare failed");

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAgicHandleInt::preProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPreAgicInt* agic_pre_int = (RkAiqAlgoPreAgicInt*)mPreInParam;
    RkAiqAlgoPreResAgicInt* agic_pre_res_int = (RkAiqAlgoPreResAgicInt*)mPreOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;
    RkAiqPreResComb* comb = &shared->preResComb;

    ret = RkAiqAgicHandle::preProcess();
    if (ret) {
        comb->agic_pre_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "agic handle preProcess failed");
    }

    comb->agic_pre_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->pre_process(mPreInParam, mPreOutParam);
    RKAIQCORE_CHECK_RET(ret, "agic algo pre_process failed");

    // set result to mAiqCore
    comb->agic_pre_res = (RkAiqAlgoPreResAgic*)agic_pre_res_int;

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAgicHandleInt::processing()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoProcAgicInt* agic_proc_int = (RkAiqAlgoProcAgicInt*)mProcInParam;
    RkAiqAlgoProcResAgicInt* agic_proc_res_int = (RkAiqAlgoProcResAgicInt*)mProcOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqProcResComb* comb = &shared->procResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAgicHandle::processing();
    if (ret) {
        comb->agic_proc_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "agic handle processing failed");
    }

    comb->agic_proc_res = NULL;
    agic_proc_int->hdr_mode = shared->working_mode;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->processing(mProcInParam, mProcOutParam);
    RKAIQCORE_CHECK_RET(ret, "agic algo processing failed");

    comb->agic_proc_res = (RkAiqAlgoProcResAgic*)agic_proc_res_int;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAgicHandleInt::postProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPostAgicInt* agic_post_int = (RkAiqAlgoPostAgicInt*)mPostInParam;
    RkAiqAlgoPostResAgicInt* agic_post_res_int = (RkAiqAlgoPostResAgicInt*)mPostOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqPostResComb* comb = &shared->postResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAgicHandle::postProcess();
    if (ret) {
        comb->agic_post_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "agic handle postProcess failed");
        return ret;
    }

    comb->agic_post_res = NULL;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->post_process(mPostInParam, mPostOutParam);
    RKAIQCORE_CHECK_RET(ret, "agic algo post_process failed");
    // set result to mAiqCore
    comb->agic_post_res = (RkAiqAlgoPostResAgic*)agic_post_res_int ;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAieHandleInt::updateConfig(bool needSync)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (needSync)
        mCfgMutex.lock();
    // if something changed
    if (updateAtt) {
        mCurAtt = mNewAtt;
        updateAtt = false;
        rk_aiq_uapi_aie_SetAttrib(mAlgoCtx, mCurAtt, false);
        sendSignal();
    }
    if (needSync)
        mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAieHandleInt::setAttrib(aie_attrib_t att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore
    if (0 != memcmp(&mCurAtt, &att, sizeof(aie_attrib_t))) {
        mNewAtt = att;
        updateAtt = true;
        waitSignal();
    }

    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAieHandleInt::getAttrib(aie_attrib_t *att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_aie_GetAttrib(mAlgoCtx, att);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAieHandleInt::init()
{
    ENTER_ANALYZER_FUNCTION();

    RkAiqAieHandle::deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAieInt());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAieInt());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAieInt());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAieInt());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAieInt());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAieInt());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAieInt());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAieHandleInt::prepare()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = RkAiqAieHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "aie handle prepare failed");

    RkAiqAlgoConfigAieInt* aie_config_int = (RkAiqAlgoConfigAieInt*)mConfig;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->prepare(mConfig);
    RKAIQCORE_CHECK_RET(ret, "aie algo prepare failed");

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAieHandleInt::preProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPreAieInt* aie_pre_int = (RkAiqAlgoPreAieInt*)mPreInParam;
    RkAiqAlgoPreResAieInt* aie_pre_res_int = (RkAiqAlgoPreResAieInt*)mPreOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;
    RkAiqPreResComb* comb = &shared->preResComb;

    ret = RkAiqAieHandle::preProcess();
    if (ret) {
        comb->aie_pre_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "aie handle preProcess failed");
    }

    comb->aie_pre_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->pre_process(mPreInParam, mPreOutParam);
    RKAIQCORE_CHECK_RET(ret, "aie algo pre_process failed");

    // set result to mAiqCore
    comb->aie_pre_res = (RkAiqAlgoPreResAie*)aie_pre_res_int;

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAieHandleInt::processing()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoProcAieInt* aie_proc_int = (RkAiqAlgoProcAieInt*)mProcInParam;
    RkAiqAlgoProcResAieInt* aie_proc_res_int = (RkAiqAlgoProcResAieInt*)mProcOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqProcResComb* comb = &shared->procResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAieHandle::processing();
    if (ret) {
        comb->aie_proc_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "aie handle processing failed");
    }

    comb->aie_proc_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->processing(mProcInParam, mProcOutParam);
    RKAIQCORE_CHECK_RET(ret, "aie algo processing failed");

    comb->aie_proc_res = (RkAiqAlgoProcResAie*)aie_proc_res_int;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAieHandleInt::postProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPostAieInt* aie_post_int = (RkAiqAlgoPostAieInt*)mPostInParam;
    RkAiqAlgoPostResAieInt* aie_post_res_int = (RkAiqAlgoPostResAieInt*)mPostOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqPostResComb* comb = &shared->postResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAieHandle::postProcess();
    if (ret) {
        comb->aie_post_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "aie handle postProcess failed");
        return ret;
    }

    comb->aie_post_res = NULL;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->post_process(mPostInParam, mPostOutParam);
    RKAIQCORE_CHECK_RET(ret, "aie algo post_process failed");
    // set result to mAiqCore
    comb->aie_post_res = (RkAiqAlgoPostResAie*)aie_post_res_int ;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAldchHandleInt::init()
{
    ENTER_ANALYZER_FUNCTION();

    RkAiqAldchHandle::deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAldchInt());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAldchInt());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAldchInt());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAldchInt());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAldchInt());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAldchInt());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAldchInt());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAldchHandleInt::prepare()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = RkAiqAldchHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "aldch handle prepare failed");

    RkAiqAlgoConfigAldchInt* aldch_config_int = (RkAiqAlgoConfigAldchInt*)mConfig;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;

    // memcpy(&aldch_config_int->aldch_calib_cfg, &shared->calib->aldch, sizeof(CalibDb_LDCH_t));
    aldch_config_int->resource_path = shared->resourcePath;
    aldch_config_int->mem_ops_ptr = mAiqCore->mShareMemOps;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->prepare(mConfig);
    RKAIQCORE_CHECK_RET(ret, "aldch algo prepare failed");

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAldchHandleInt::preProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPreAldchInt* aldch_pre_int = (RkAiqAlgoPreAldchInt*)mPreInParam;
    RkAiqAlgoPreResAldchInt* aldch_pre_res_int = (RkAiqAlgoPreResAldchInt*)mPreOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;
    RkAiqPreResComb* comb = &shared->preResComb;

    ret = RkAiqAldchHandle::preProcess();
    if (ret) {
        comb->aldch_pre_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "aldch handle preProcess failed");
    }

    comb->aldch_pre_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->pre_process(mPreInParam, mPreOutParam);
    RKAIQCORE_CHECK_RET(ret, "aldch algo pre_process failed");

    // set result to mAiqCore
    comb->aldch_pre_res = (RkAiqAlgoPreResAldch*)aldch_pre_res_int;

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAldchHandleInt::processing()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoProcAldchInt* aldch_proc_int = (RkAiqAlgoProcAldchInt*)mProcInParam;
    RkAiqAlgoProcResAldchInt* aldch_proc_res_int = (RkAiqAlgoProcResAldchInt*)mProcOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqProcResComb* comb = &shared->procResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAldchHandle::processing();
    if (ret) {
        comb->aldch_proc_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "aldch handle processing failed");
    }

    comb->aldch_proc_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->processing(mProcInParam, mProcOutParam);
    RKAIQCORE_CHECK_RET(ret, "aldch algo processing failed");

    comb->aldch_proc_res = (RkAiqAlgoProcResAldch*)aldch_proc_res_int;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAldchHandleInt::postProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPostAldchInt* aldch_post_int = (RkAiqAlgoPostAldchInt*)mPostInParam;
    RkAiqAlgoPostResAldchInt* aldch_post_res_int = (RkAiqAlgoPostResAldchInt*)mPostOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqPostResComb* comb = &shared->postResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAldchHandle::postProcess();
    if (ret) {
        comb->aldch_post_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "aldch handle postProcess failed");
        return ret;
    }

    comb->aldch_post_res = NULL;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->post_process(mPostInParam, mPostOutParam);
    RKAIQCORE_CHECK_RET(ret, "aldch algo post_process failed");
    // set result to mAiqCore
    comb->aldch_post_res = (RkAiqAlgoPostResAldch*)aldch_post_res_int ;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAldchHandleInt::updateConfig(bool needSync)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (needSync)
        mCfgMutex.lock();
    // if something changed
    if (updateAtt) {
        mCurAtt = mNewAtt;
        updateAtt = false;
        // TODO
        rk_aiq_uapi_aldch_SetAttrib(mAlgoCtx, mCurAtt, false);
        sendSignal();
    }

    if (needSync)
        mCfgMutex.unlock();


    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAldchHandleInt::setAttrib(rk_aiq_ldch_attrib_t att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be efldchtive later when updateConfig
    // called by RkAiqCore

    // if something changed
    if (0 != memcmp(&mCurAtt, &att, sizeof(rk_aiq_ldch_attrib_t))) {
        mNewAtt = att;
        updateAtt = true;
        waitSignal();
    }

    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAldchHandleInt::getAttrib(rk_aiq_ldch_attrib_t *att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_aldch_GetAttrib(mAlgoCtx, att);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAlscHandleInt::init()
{
    ENTER_ANALYZER_FUNCTION();

    RkAiqAlscHandle::deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAlscInt());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAlscInt());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAlscInt());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAlscInt());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAlscInt());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAlscInt());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAlscInt());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAlscHandleInt::updateConfig(bool needSync)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (needSync)
        mCfgMutex.lock();
    // if something changed
    if (updateAtt) {
        mCurAtt = mNewAtt;
        updateAtt = false;
        // TODO
        rk_aiq_uapi_alsc_SetAttrib(mAlgoCtx, mCurAtt, false);
        sendSignal();
    }

    if (needSync)
        mCfgMutex.unlock();


    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAlscHandleInt::setAttrib(rk_aiq_lsc_attrib_t att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore

    // if something changed
    if (0 != memcmp(&mCurAtt, &att, sizeof(rk_aiq_lsc_attrib_t))) {
        mNewAtt = att;
        updateAtt = true;
        waitSignal();
    }

    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAlscHandleInt::getAttrib(rk_aiq_lsc_attrib_t *att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_alsc_GetAttrib(mAlgoCtx, att);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAlscHandleInt::queryLscInfo(rk_aiq_lsc_querry_info_t *lsc_querry_info )
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_alsc_QueryLscInfo(mAlgoCtx, lsc_querry_info);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAlscHandleInt::prepare()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = RkAiqAlscHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "alsc handle prepare failed");

    RkAiqAlgoConfigAlscInt* alsc_config_int = (RkAiqAlgoConfigAlscInt*)mConfig;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->prepare(mConfig);
    RKAIQCORE_CHECK_RET(ret, "alsc algo prepare failed");

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAlscHandleInt::preProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPreAlscInt* alsc_pre_int = (RkAiqAlgoPreAlscInt*)mPreInParam;
    RkAiqAlgoPreResAlscInt* alsc_pre_res_int = (RkAiqAlgoPreResAlscInt*)mPreOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;
    RkAiqPreResComb* comb = &shared->preResComb;

    ret = RkAiqAlscHandle::preProcess();
    if (ret) {
        comb->alsc_pre_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "alsc handle preProcess failed");
    }

    comb->alsc_pre_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->pre_process(mPreInParam, mPreOutParam);
    RKAIQCORE_CHECK_RET(ret, "alsc algo pre_process failed");

    // set result to mAiqCore
    comb->alsc_pre_res = (RkAiqAlgoPreResAlsc*)alsc_pre_res_int;

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAlscHandleInt::processing()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoProcAlscInt* alsc_proc_int = (RkAiqAlgoProcAlscInt*)mProcInParam;
    RkAiqAlgoProcResAlscInt* alsc_proc_res_int = (RkAiqAlgoProcResAlscInt*)mProcOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqProcResComb* comb = &shared->procResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAlscHandle::processing();
    if (ret) {
        comb->alsc_proc_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "alsc handle processing failed");
    }

    comb->alsc_proc_res = NULL;

    // TODO should check if the rk awb algo used
    RkAiqAlgoProcResAwb* awb_res =
        (RkAiqAlgoProcResAwb*)(shared->procResComb.awb_proc_res);
    RkAiqAlgoProcResAwbInt* awb_res_int =
        (RkAiqAlgoProcResAwbInt*)(shared->procResComb.awb_proc_res);
    if(awb_res) {
        if(awb_res->awb_gain_algo.grgain < DIVMIN ||
                awb_res->awb_gain_algo.gbgain < DIVMIN ) {
            LOGE("get wrong awb gain from AWB module ,use default value ");
        } else {
            alsc_proc_int->alsc_sw_info.awbGain[0] =
                awb_res->awb_gain_algo.rgain / awb_res->awb_gain_algo.grgain;

            alsc_proc_int->alsc_sw_info.awbGain[1] =
                awb_res->awb_gain_algo.bgain / awb_res->awb_gain_algo.gbgain;
        }
        alsc_proc_int->alsc_sw_info.awbIIRDampCoef = awb_res_int->awb_smooth_factor;
        alsc_proc_int->alsc_sw_info.varianceLuma = awb_res_int->varianceLuma;
        alsc_proc_int->alsc_sw_info.awbConverged = awb_res_int->awbConverged;
    } else {
        LOGE("fail to get awb gain form AWB module,use default value ");
    }

#if 0
    // id != 0 means the thirdparty's algo
    if (mDes->id != 0) {
        RkAiqAlgoPreResAe *ae_int = (RkAiqAlgoPreResAe*)shared->preResComb.ae_pre_res;
        if( ae_int) {
            if((rk_aiq_working_mode_t)shared->working_mode == RK_AIQ_WORKING_MODE_NORMAL) {
                alsc_proc_int->alsc_sw_info.sensorGain = ae_int->ae_pre_res.LinearExp.exp_real_params.analog_gain
                        * ae_int->ae_pre_res.LinearExp.exp_real_params.digital_gain
                        * ae_int->ae_pre_res.LinearExp.exp_real_params.isp_dgain;
            } else if((rk_aiq_working_mode_t)shared->working_mode >= RK_AIQ_WORKING_MODE_ISP_HDR2
                      && (rk_aiq_working_mode_t)shared->working_mode < RK_AIQ_WORKING_MODE_ISP_HDR3)  {
                LOGD("sensor gain choose from second hdr frame for alsc");
                alsc_proc_int->alsc_sw_info.sensorGain = ae_int->ae_pre_res.HdrExp[1].exp_real_params.analog_gain
                        * ae_int->ae_pre_res.HdrExp[1].exp_real_params.digital_gain
                        * ae_int->ae_pre_res.HdrExp[1].exp_real_params.isp_dgain;
            } else if((rk_aiq_working_mode_t)shared->working_mode >= RK_AIQ_WORKING_MODE_ISP_HDR2
                      && (rk_aiq_working_mode_t)shared->working_mode >= RK_AIQ_WORKING_MODE_ISP_HDR3)  {
                LOGD("sensor gain choose from third hdr frame for alsc");
                alsc_proc_int->alsc_sw_info.sensorGain = ae_int->ae_pre_res.HdrExp[2].exp_real_params.analog_gain
                        * ae_int->ae_pre_res.HdrExp[2].exp_real_params.digital_gain
                        * ae_int->ae_pre_res.HdrExp[2].exp_real_params.isp_dgain;
            } else {
                LOGE("working_mode (%d) is invaild ,fail to get sensor gain form AE module,use default value ",
                     shared->working_mode);
            }
        } else {
            LOGE("fail to get sensor gain form AE module,use default value ");
        }
    } else {
        RkAiqAlgoPreResAeInt *ae_int = (RkAiqAlgoPreResAeInt*)shared->preResComb.ae_pre_res;
        if( ae_int) {
            if((rk_aiq_working_mode_t)shared->working_mode == RK_AIQ_WORKING_MODE_NORMAL) {
                alsc_proc_int->alsc_sw_info.sensorGain = ae_int->ae_pre_res_rk.LinearExp.exp_real_params.analog_gain
                        * ae_int->ae_pre_res_rk.LinearExp.exp_real_params.digital_gain
                        * ae_int->ae_pre_res_rk.LinearExp.exp_real_params.isp_dgain;
            } else if((rk_aiq_working_mode_t)shared->working_mode >= RK_AIQ_WORKING_MODE_ISP_HDR2
                      && (rk_aiq_working_mode_t)shared->working_mode < RK_AIQ_WORKING_MODE_ISP_HDR3)  {
                LOGD("sensor gain choose from second hdr frame for alsc");
                alsc_proc_int->alsc_sw_info.sensorGain = ae_int->ae_pre_res_rk.HdrExp[1].exp_real_params.analog_gain
                        * ae_int->ae_pre_res_rk.HdrExp[1].exp_real_params.digital_gain
                        * ae_int->ae_pre_res_rk.HdrExp[1].exp_real_params.isp_dgain;
            } else if((rk_aiq_working_mode_t)shared->working_mode >= RK_AIQ_WORKING_MODE_ISP_HDR2
                      && (rk_aiq_working_mode_t)shared->working_mode >= RK_AIQ_WORKING_MODE_ISP_HDR3)  {
                LOGD("sensor gain choose from third hdr frame for alsc");
                alsc_proc_int->alsc_sw_info.sensorGain = ae_int->ae_pre_res_rk.HdrExp[2].exp_real_params.analog_gain
                        * ae_int->ae_pre_res_rk.HdrExp[2].exp_real_params.digital_gain
                        * ae_int->ae_pre_res_rk.HdrExp[2].exp_real_params.isp_dgain;
            } else {
                LOGE("working_mode (%d) is invaild ,fail to get sensor gain form AE module,use default value ",
                     shared->working_mode);
            }
        } else {
            LOGE("fail to get sensor gain form AE module,use default value ");
        }
    }
#else
    RKAiqAecExpInfo_t *pCurExp = &shared->curExp;
    int cur_mode = RK_AIQ_HDR_GET_WORKING_MODE(shared->working_mode);
    if(pCurExp) {
        switch (cur_mode)
        {
        case RK_AIQ_WORKING_MODE_NORMAL:
        {
            alsc_proc_int->alsc_sw_info.sensorGain
                = pCurExp->LinearExp.exp_real_params.analog_gain
                  * pCurExp->LinearExp.exp_real_params.digital_gain
                  * pCurExp->LinearExp.exp_real_params.isp_dgain;
        }
        break;

        case RK_AIQ_WORKING_MODE_ISP_HDR2:
        {
            LOGD("sensor gain choose from second hdr frame for alsc");
            alsc_proc_int->alsc_sw_info.sensorGain
                = pCurExp->HdrExp[1].exp_real_params.analog_gain
                  * pCurExp->HdrExp[1].exp_real_params.digital_gain
                  * pCurExp->HdrExp[1].exp_real_params.isp_dgain;
        }
        break;

        case RK_AIQ_WORKING_MODE_ISP_HDR3:
        {
            LOGD("sensor gain choose from third hdr frame for alsc");
            alsc_proc_int->alsc_sw_info.sensorGain
                = pCurExp->HdrExp[2].exp_real_params.analog_gain
                  * pCurExp->HdrExp[2].exp_real_params.digital_gain
                  * pCurExp->HdrExp[2].exp_real_params.isp_dgain;
        }
        break;

        default:
        {
            LOGE("working_mode (%d) is invaild, fail to get sensor gain form AE module, use default value ", shared->working_mode);
        }
        break;
        }
    } else {
        LOGE("fail to get sensor gain form AE module,use default value ");
    }
#endif

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->processing(mProcInParam, mProcOutParam);
    RKAIQCORE_CHECK_RET(ret, "alsc algo processing failed");

    comb->alsc_proc_res = (RkAiqAlgoProcResAlsc*)alsc_proc_res_int;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAlscHandleInt::postProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPostAlscInt* alsc_post_int = (RkAiqAlgoPostAlscInt*)mPostInParam;
    RkAiqAlgoPostResAlscInt* alsc_post_res_int = (RkAiqAlgoPostResAlscInt*)mPostOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqPostResComb* comb = &shared->postResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAlscHandle::postProcess();
    if (ret) {
        comb->alsc_post_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "alsc handle postProcess failed");
        return ret;
    }

    comb->alsc_post_res = NULL;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->post_process(mPostInParam, mPostOutParam);
    RKAIQCORE_CHECK_RET(ret, "alsc algo post_process failed");
    // set result to mAiqCore
    comb->alsc_post_res = (RkAiqAlgoPostResAlsc*)alsc_post_res_int ;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAorbHandleInt::init()
{
    ENTER_ANALYZER_FUNCTION();

    RkAiqAorbHandle::deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAorbInt());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAorbInt());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAorbInt());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAorbInt());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAorbInt());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAorbInt());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAorbInt());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAorbHandleInt::prepare()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = RkAiqAorbHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "aorb handle prepare failed");

    RkAiqAlgoConfigAorbInt* aorb_config_int = (RkAiqAlgoConfigAorbInt*)mConfig;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;

    memcpy(&aorb_config_int->orb_calib_cfg, &shared->calib->orb, sizeof(CalibDb_ORB_t));

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->prepare(mConfig);
    RKAIQCORE_CHECK_RET(ret, "aorb algo prepare failed");

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAorbHandleInt::preProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPreAorbInt* aorb_pre_int = (RkAiqAlgoPreAorbInt*)mPreInParam;
    RkAiqAlgoPreResAorbInt* aorb_pre_res_int = (RkAiqAlgoPreResAorbInt*)mPreOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;
    RkAiqPreResComb* comb = &shared->preResComb;

    ret = RkAiqAorbHandle::preProcess();
    if (ret) {
        comb->aorb_pre_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "aorb handle preProcess failed");
    }

    comb->aorb_pre_res = NULL;

    aorb_pre_int->orb_stats = &ispStats->orb_stats;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->pre_process(mPreInParam, mPreOutParam);
    RKAIQCORE_CHECK_RET(ret, "aorb algo pre_process failed");

    // set result to mAiqCore
    comb->aorb_pre_res = (RkAiqAlgoPreResAorb*)aorb_pre_res_int;

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAorbHandleInt::processing()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoProcAorbInt* aorb_proc_int = (RkAiqAlgoProcAorbInt*)mProcInParam;
    RkAiqAlgoProcResAorbInt* aorb_proc_res_int = (RkAiqAlgoProcResAorbInt*)mProcOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqProcResComb* comb = &shared->procResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAorbHandle::processing();
    if (ret) {
        comb->aorb_proc_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "aorb handle processing failed");
    }

    comb->aorb_proc_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->processing(mProcInParam, mProcOutParam);
    RKAIQCORE_CHECK_RET(ret, "aorb algo processing failed");

    comb->aorb_proc_res = (RkAiqAlgoProcResAorb*)aorb_proc_res_int;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAorbHandleInt::postProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPostAorbInt* aorb_post_int = (RkAiqAlgoPostAorbInt*)mPostInParam;
    RkAiqAlgoPostResAorbInt* aorb_post_res_int = (RkAiqAlgoPostResAorbInt*)mPostOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqPostResComb* comb = &shared->postResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAorbHandle::postProcess();
    if (ret) {
        comb->aorb_post_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "aorb handle postProcess failed");
        return ret;
    }

    comb->aorb_post_res = NULL;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->post_process(mPostInParam, mPostOutParam);
    RKAIQCORE_CHECK_RET(ret, "aorb algo post_process failed");
    // set result to mAiqCore
    comb->aorb_post_res = (RkAiqAlgoPostResAorb*)aorb_post_res_int ;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAr2yHandleInt::init()
{
    ENTER_ANALYZER_FUNCTION();

    RkAiqAr2yHandle::deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAr2yInt());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAr2yInt());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAr2yInt());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAr2yInt());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAr2yInt());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAr2yInt());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAr2yInt());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAr2yHandleInt::prepare()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = RkAiqAr2yHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "ar2y handle prepare failed");

    RkAiqAlgoConfigAr2yInt* ar2y_config_int = (RkAiqAlgoConfigAr2yInt*)mConfig;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->prepare(mConfig);
    RKAIQCORE_CHECK_RET(ret, "ar2y algo prepare failed");

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAr2yHandleInt::preProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPreAr2yInt* ar2y_pre_int = (RkAiqAlgoPreAr2yInt*)mPreInParam;
    RkAiqAlgoPreResAr2yInt* ar2y_pre_res_int = (RkAiqAlgoPreResAr2yInt*)mPreOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;
    RkAiqPreResComb* comb = &shared->preResComb;

    ret = RkAiqAr2yHandle::preProcess();
    if (ret) {
        comb->ar2y_pre_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "ar2y handle preProcess failed");
    }

    comb->ar2y_pre_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->pre_process(mPreInParam, mPreOutParam);
    RKAIQCORE_CHECK_RET(ret, "ar2y algo pre_process failed");

    // set result to mAiqCore
    comb->ar2y_pre_res = (RkAiqAlgoPreResAr2y*)ar2y_pre_res_int;

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAr2yHandleInt::processing()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoProcAr2yInt* ar2y_proc_int = (RkAiqAlgoProcAr2yInt*)mProcInParam;
    RkAiqAlgoProcResAr2yInt* ar2y_proc_res_int = (RkAiqAlgoProcResAr2yInt*)mProcOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqProcResComb* comb = &shared->procResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAr2yHandle::processing();
    if (ret) {
        comb->ar2y_proc_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "ar2y handle processing failed");
    }

    comb->ar2y_proc_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->processing(mProcInParam, mProcOutParam);
    RKAIQCORE_CHECK_RET(ret, "ar2y algo processing failed");

    comb->ar2y_proc_res = (RkAiqAlgoProcResAr2y*)ar2y_proc_res_int;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAr2yHandleInt::postProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPostAr2yInt* ar2y_post_int = (RkAiqAlgoPostAr2yInt*)mPostInParam;
    RkAiqAlgoPostResAr2yInt* ar2y_post_res_int = (RkAiqAlgoPostResAr2yInt*)mPostOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqPostResComb* comb = &shared->postResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAr2yHandle::postProcess();
    if (ret) {
        comb->ar2y_post_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "ar2y handle postProcess failed");
        return ret;
    }

    comb->ar2y_post_res = NULL;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->post_process(mPostInParam, mPostOutParam);
    RKAIQCORE_CHECK_RET(ret, "ar2y algo post_process failed");
    // set result to mAiqCore
    comb->ar2y_post_res = (RkAiqAlgoPostResAr2y*)ar2y_post_res_int ;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

void
RkAiqAwdrHandleInt::init()
{
    ENTER_ANALYZER_FUNCTION();

    RkAiqAwdrHandle::deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAwdrInt());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAwdrInt());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAwdrInt());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAwdrInt());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAwdrInt());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAwdrInt());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAwdrInt());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqAwdrHandleInt::updateConfig(bool needSync)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (needSync)
        mCfgMutex.lock();
    // if something changed
    if (updateAtt) {
        mCurAtt = mNewAtt;
        updateAtt = false;
        rk_aiq_uapi_awdr_SetAttrib(mAlgoCtx, mCurAtt, true);
        sendSignal();
    }
    if (needSync)
        mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAwdrHandleInt::setAttrib(awdr_attrib_t att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    //TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore

    // if something changed
    if (0 != memcmp(&mCurAtt, &att, sizeof(awdr_attrib_t))) {
        mNewAtt = att;
        updateAtt = true;
        waitSignal();
    }
    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}
XCamReturn
RkAiqAwdrHandleInt::getAttrib(awdr_attrib_t* att)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_awdr_GetAttrib(mAlgoCtx, att);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAwdrHandleInt::prepare()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = RkAiqAwdrHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "awdr handle prepare failed");

    RkAiqAlgoConfigAwdrInt* awdr_config_int = (RkAiqAlgoConfigAwdrInt*)mConfig;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;

    awdr_config_int->working_mode = shared->working_mode;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->prepare(mConfig);
    RKAIQCORE_CHECK_RET(ret, "awdr algo prepare failed");

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAwdrHandleInt::preProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPreAwdrInt* awdr_pre_int = (RkAiqAlgoPreAwdrInt*)mPreInParam;
    RkAiqAlgoPreResAwdrInt* awdr_pre_res_int = (RkAiqAlgoPreResAwdrInt*)mPreOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqIspStats* ispStats = &shared->ispStats;
    RkAiqPreResComb* comb = &shared->preResComb;

    ret = RkAiqAwdrHandle::preProcess();
    if (ret) {
        comb->awdr_pre_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "awdr handle preProcess failed");
    }

    comb->awdr_pre_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->pre_process(mPreInParam, mPreOutParam);
    RKAIQCORE_CHECK_RET(ret, "awdr algo pre_process failed");

    // set result to mAiqCore
    comb->awdr_pre_res = (RkAiqAlgoPreResAwdr*)awdr_pre_res_int;

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqAwdrHandleInt::processing()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoProcAwdrInt* awdr_proc_int = (RkAiqAlgoProcAwdrInt*)mProcInParam;
    RkAiqAlgoProcResAwdrInt* awdr_proc_res_int = (RkAiqAlgoProcResAwdrInt*)mProcOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqProcResComb* comb = &shared->procResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAwdrHandle::processing();
    if (ret) {
        comb->awdr_proc_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "awdr handle processing failed");
    }

    comb->awdr_proc_res = NULL;

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->processing(mProcInParam, mProcOutParam);
    RKAIQCORE_CHECK_RET(ret, "awdr algo processing failed");

    comb->awdr_proc_res = (RkAiqAlgoProcResAwdr*)awdr_proc_res_int;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn
RkAiqAwdrHandleInt::postProcess()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPostAwdrInt* awdr_post_int = (RkAiqAlgoPostAwdrInt*)mPostInParam;
    RkAiqAlgoPostResAwdrInt* awdr_post_res_int = (RkAiqAlgoPostResAwdrInt*)mPostOutParam;
    RkAiqCore::RkAiqAlgosShared_t* shared = &mAiqCore->mAlogsSharedParams;
    RkAiqPostResComb* comb = &shared->postResComb;
    RkAiqIspStats* ispStats = &shared->ispStats;

    ret = RkAiqAwdrHandle::postProcess();
    if (ret) {
        comb->awdr_post_res = NULL;
        RKAIQCORE_CHECK_RET(ret, "awdr handle postProcess failed");
        return ret;
    }

    comb->awdr_post_res = NULL;
    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret = des->post_process(mPostInParam, mPostOutParam);
    RKAIQCORE_CHECK_RET(ret, "awdr algo post_process failed");
    // set result to mAiqCore
    comb->awdr_post_res = (RkAiqAlgoPostResAwdr*)awdr_post_res_int ;

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

}; //namespace RkCam
