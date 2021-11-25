/*
 * rkisp_aiq_core.h
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
#include "v4l2_buffer_proxy.h"
#include "acp/rk_aiq_algo_acp_itf.h"
#include "ae/rk_aiq_algo_ae_itf.h"
#include "awb/rk_aiq_algo_awb_itf.h"
#include "af/rk_aiq_algo_af_itf.h"
#include "anr/rk_aiq_algo_anr_itf.h"
#include "asd/rk_aiq_algo_asd_itf.h"
#include "ahdr/rk_aiq_algo_ahdr_itf.h"
#include "asharp/rk_aiq_algo_asharp_itf.h"
#include "adehaze/rk_aiq_algo_adhaz_itf.h"
#include "ablc/rk_aiq_algo_ablc_itf.h"
#include "adpcc/rk_aiq_algo_adpcc_itf.h"
#include "alsc/rk_aiq_algo_alsc_itf.h"
#include "agic/rk_aiq_algo_agic_itf.h"
#include "adebayer/rk_aiq_algo_adebayer_itf.h"
#include "accm/rk_aiq_algo_accm_itf.h"
#include "agamma/rk_aiq_algo_agamma_itf.h"
#include "adegamma/rk_aiq_algo_adegamma_itf.h"
#include "awdr/rk_aiq_algo_awdr_itf.h"
#include "a3dlut/rk_aiq_algo_a3dlut_itf.h"
#include "aldch/rk_aiq_algo_aldch_itf.h"
#include "ar2y/rk_aiq_algo_ar2y_itf.h"
#include "aie/rk_aiq_algo_aie_itf.h"
#include "aorb/rk_aiq_algo_aorb_itf.h"
#include "afec/rk_aiq_algo_afec_itf.h"
#include "acgc/rk_aiq_algo_acgc_itf.h"
#include "afd/rk_aiq_algo_afd_itf.h"
#ifdef RK_SIMULATOR_HW
#include "simulator/isp20_hw_simulator.h"
#else
#include "isp20/Isp20Evts.h"
#include "isp20/Isp20StatsBuffer.h"
#include "isp20/rkisp2-config.h"
#include "isp20/rkispp-config.h"
#endif
#include <fcntl.h>
#include <unistd.h>

namespace RkCam {
#define EPSINON 0.0000001

/*
 * notice that the order should be the same as enum RkAiqAlgoType_t
 * which defined in rk_aiq/algos/rk_aiq_algo_des.h
 */
static RkAiqAlgoDesComm* g_default_3a_des[] = {
    (RkAiqAlgoDesComm*)&g_RkIspAlgoDescAe,
    (RkAiqAlgoDesComm*)&g_RkIspAlgoDescAwb,
    (RkAiqAlgoDesComm*)&g_RkIspAlgoDescAf,
    (RkAiqAlgoDesComm*)&g_RkIspAlgoDescAblc,
    (RkAiqAlgoDesComm*)&g_RkIspAlgoDescAdpcc,
    (RkAiqAlgoDesComm*)&g_RkIspAlgoDescAhdr,
    (RkAiqAlgoDesComm*)&g_RkIspAlgoDescAnr,
    (RkAiqAlgoDesComm*)&g_RkIspAlgoDescAlsc,
    (RkAiqAlgoDesComm*)&g_RkIspAlgoDescAgic,
    (RkAiqAlgoDesComm*)&g_RkIspAlgoDescAdebayer,
    (RkAiqAlgoDesComm*)&g_RkIspAlgoDescAccm,
    (RkAiqAlgoDesComm*)&g_RkIspAlgoDescAgamma,
    (RkAiqAlgoDesComm*)&g_RkIspAlgoDescAwdr,
    (RkAiqAlgoDesComm*)&g_RkIspAlgoDescAdhaz,
    (RkAiqAlgoDesComm*)&g_RkIspAlgoDescA3dlut,
    (RkAiqAlgoDesComm*)&g_RkIspAlgoDescAldch,
    (RkAiqAlgoDesComm*)&g_RkIspAlgoDescAr2y,
    (RkAiqAlgoDesComm*)&g_RkIspAlgoDescAcp,
    (RkAiqAlgoDesComm*)&g_RkIspAlgoDescAie,
    (RkAiqAlgoDesComm*)&g_RkIspAlgoDescAsharp,
    (RkAiqAlgoDesComm*)&g_RkIspAlgoDescAorb,
    (RkAiqAlgoDesComm*)&g_RkIspAlgoDescAfec,
    (RkAiqAlgoDesComm*)&g_RkIspAlgoDescAcgc,
    (RkAiqAlgoDesComm*)&g_RkIspAlgoDescAsd,
    (RkAiqAlgoDesComm*)&g_RkIspAlgoDescAdegamma,
    (RkAiqAlgoDesComm*)&g_RkIspAlgoDescAfd,
};

bool
RkAiqCoreThread::loop()
{
    ENTER_ANALYZER_FUNCTION();

    const static int32_t timeout = -1;
    SmartPtr<VideoBuffer> stats = mStatsQueue.pop (timeout);

    if (!stats.ptr()) {
        LOGW_ANALYZER("RkAiqCoreThread got empty stats, stop thread");
        return false;
    }

    XCamReturn ret = mRkAiqCore->analyze (stats);


    if (ret == XCAM_RETURN_NO_ERROR || ret == XCAM_RETURN_BYPASS)
        return true;

    LOGE_ANALYZER("RkAiqCoreThread failed to analyze 3a stats");

    EXIT_ANALYZER_FUNCTION();

    return false;
}

bool
RkAiqCoreEvtsThread::loop()
{
    ENTER_ANALYZER_FUNCTION();

    const static int32_t timeout = -1;

    SmartPtr<ispHwEvt_t> evts = mEvtsQueue.pop (timeout);
    if (!evts.ptr()) {
        LOGW_ANALYZER("RkAiqCoreEvtsThread got empty stats, stop thread");
        return false;
    }

    XCamReturn ret = mRkAiqCore->events_analyze (evts);
    if (ret == XCAM_RETURN_NO_ERROR || ret == XCAM_RETURN_BYPASS)
        return true;

    LOGE_ANALYZER("RkAiqCoreEvtsThread failed to analyze events");

    EXIT_ANALYZER_FUNCTION();

    return false;
}

bool
RkAiqCoreTxBufAnalyzerThread::loop()
{
    ENTER_ANALYZER_FUNCTION();

    const static int32_t timeout = -1;

    SmartPtr<RkAiqTxBufInfo> txBuf = mTxBufQueue.pop (timeout);
    if (!txBuf.ptr()) {
        LOGW_ANALYZER("RkAiqCoreEvtsThread got empty stats, stop thread");
        return false;
    }

    XCamReturn ret = mRkAiqCore->txBufAnalyze(txBuf);
    if (ret == XCAM_RETURN_NO_ERROR || ret == XCAM_RETURN_BYPASS)
        return true;

    LOGE_ANALYZER("RkAiqCoreEvtsThread failed to analyze events");

    EXIT_ANALYZER_FUNCTION();

    return false;
}

// notice that some pool shared items may be cached by other
// modules(e.g. CamHwIsp20), so here should consider the cached number
uint16_t RkAiqCore::DEFAULT_POOL_SIZE = 15;

RkAiqCore::RkAiqCore()
    : mRkAiqCoreTh(new RkAiqCoreThread(this))
    , mRkAiqCorePpTh(new RkAiqCoreThread(this))
    , mRkAiqCoreEvtsTh(new RkAiqCoreEvtsThread(this))
    , mRkAiqCoreTxBufAnalyzerTh(new RkAiqCoreTxBufAnalyzerThread(this))
    , mState(RK_AIQ_CORE_STATE_INVALID)
    , mCb(NULL)
    , mAiqParamsPool(new RkAiqFullParamsPool("RkAiqFullParams", 4))
    , mAiqExpParamsPool(new RkAiqExpParamsPool("RkAiqExpParams", 4))
    , mAiqIrisParamsPool(new RkAiqIrisParamsPool("RkAiqIrisParams", 4))
    , mAiqIspMeasParamsPool(new RkAiqIspMeasParamsPool("RkAiqIspMeasParams", RkAiqCore::DEFAULT_POOL_SIZE))
    , mAiqIspOtherParamsPool(new RkAiqIspOtherParamsPool("RkAiqIspOtherParams", RkAiqCore::DEFAULT_POOL_SIZE))
    , mAiqIsppMeasParamsPool(new RkAiqIsppMeasParamsPool("RkAiqIsppMeasParams", RkAiqCore::DEFAULT_POOL_SIZE))
    , mAiqIsppOtherParamsPool(new RkAiqIsppOtherParamsPool("RkAiqIsppOtherParams", RkAiqCore::DEFAULT_POOL_SIZE))
    , mAiqFocusParamsPool(new RkAiqFocusParamsPool("RkAiqFocusParams", 4))
    , mAiqCpslParamsPool(new RkAiqCpslParamsPool("RkAiqCpslParamsPool", 4))
    , mAiqStatsPool(new RkAiqStatsPool("RkAiqStatsPool", 4))
{
    ENTER_ANALYZER_FUNCTION();
    mAlogsSharedParams.reset();
    mCurAhdrAlgoHdl = NULL;
    mCurAnrAlgoHdl = NULL;
    mCurAdhazAlgoHdl = NULL;
    mCurAsdAlgoHdl = NULL;
    mCurAcpAlgoHdl = NULL;
    mCurAsharpAlgoHdl = NULL;
    mCurA3dlutAlgoHdl = NULL;
    mCurAblcAlgoHdl = NULL;
    mCurAccmAlgoHdl = NULL;
    mCurAcgcAlgoHdl = NULL;
    mCurAdebayerAlgoHdl = NULL;
    mCurAdpccAlgoHdl = NULL;
    mCurAfecAlgoHdl = NULL;
    mCurAgammaAlgoHdl = NULL;
    mCurAdegammaAlgoHdl = NULL;
    mCurAgicAlgoHdl = NULL;
    mCurAieAlgoHdl = NULL;
    mCurAldchAlgoHdl = NULL;
    mCurAlscAlgoHdl = NULL;
    mCurAorbAlgoHdl = NULL;
    mCurAr2yAlgoHdl = NULL;
    mCurAwdrAlgoHdl = NULL;
    mCurAeAlgoHdl = NULL;
    mCurCustomAeAlgoHdl = NULL;
    mCurCustomAwbAlgoHdl = NULL;
    mCurCustomAfAlgoHdl = NULL;
    mCurAwbAlgoHdl = NULL;
    mCurAfAlgoHdl = NULL;
    xcam_mem_clear(mHwInfo);
    mCurCpslOn = false;
    mStrthLed = 0.0f;
    mStrthIr = 0.0f;
    mGrayMode = RK_AIQ_GRAY_MODE_CPSL;
    firstStatsReceived = false;
    mSafeEnableAlgo = true;

    SmartPtr<RkAiqFullParams> fullParam = new RkAiqFullParams();
    mAiqCurParams = new RkAiqFullParamsProxy(fullParam );

    EXIT_ANALYZER_FUNCTION();
}

RkAiqCore::~RkAiqCore()
{
    ENTER_ANALYZER_FUNCTION();
    if (mAlogsSharedParams.resourcePath) {
        xcam_free((void*)(mAlogsSharedParams.resourcePath));
        mAlogsSharedParams.resourcePath = NULL;
    }
    EXIT_ANALYZER_FUNCTION();
}

void RkAiqCore::initCpsl()
{
    queryCpsLtCap(mCpslCap);

    rk_aiq_cpsl_cfg_t* cfg = &mAlogsSharedParams.cpslCfg;
    const CamCalibDbContext_t* aiqCalib = mAlogsSharedParams.calib;
    // TODO: something from calib
    if (mCpslCap.modes_num > 0 && aiqCalib->cpsl.support_en) {
        if (aiqCalib->cpsl.mode == 0) {
            cfg->mode = RK_AIQ_OP_MODE_AUTO;
        } else if (aiqCalib->cpsl.mode == 1) {
            cfg->mode = RK_AIQ_OP_MODE_MANUAL;
        } else {
            cfg->mode = RK_AIQ_OP_MODE_INVALID;
        }

        if (aiqCalib->cpsl.lght_src == 0) {
            cfg->lght_src = RK_AIQ_CPSLS_LED;
        } else if (aiqCalib->cpsl.lght_src == 1) {
            cfg->lght_src = RK_AIQ_CPSLS_IR;
        } else if (aiqCalib->cpsl.lght_src == 2) {
            cfg->lght_src = RK_AIQ_CPSLS_MIX;
        } else {
            cfg->lght_src = RK_AIQ_CPSLS_INVALID;
        }
        cfg->gray_on = aiqCalib->cpsl.gray;
        if (cfg->mode == RK_AIQ_OP_MODE_AUTO) {
            cfg->u.a.sensitivity = aiqCalib->cpsl.ajust_sens;
            cfg->u.a.sw_interval = aiqCalib->cpsl.sw_interval;
            LOGI_ANALYZER("mode sensitivity %f, interval time %d s\n",
                          cfg->u.a.sensitivity, cfg->u.a.sw_interval);
        } else {
            cfg->u.m.on = aiqCalib->cpsl.cpsl_on;
            cfg->u.m.strength_ir = aiqCalib->cpsl.strength;
            cfg->u.m.strength_led = aiqCalib->cpsl.strength;
            LOGI_ANALYZER("on %d, strength_led %f, strength_ir %f \n",
                          cfg->u.m.on, cfg->u.m.strength_led, cfg->u.m.strength_ir);
        }
    } else {
        cfg->mode = RK_AIQ_OP_MODE_INVALID;
        LOGI_ANALYZER("not support light compensation \n");
    }
}

XCamReturn
RkAiqCore::init(const char* sns_ent_name, const CamCalibDbContext_t* aiqCalib)
{
    ENTER_ANALYZER_FUNCTION();

    if (mState != RK_AIQ_CORE_STATE_INVALID) {
        LOGE_ANALYZER("wrong state %d\n", mState);
        return XCAM_RETURN_ERROR_ANALYZER;
    }

    mAlogsSharedParams.calib = aiqCalib;

    addDefaultAlgos();
    initCpsl();

    mState = RK_AIQ_CORE_STATE_INITED;
    return XCAM_RETURN_NO_ERROR;

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn
RkAiqCore::deInit()
{
    ENTER_ANALYZER_FUNCTION();

    if (mState == RK_AIQ_CORE_STATE_STARTED || mState == RK_AIQ_CORE_STATE_RUNNING) {
        LOGE_ANALYZER("wrong state %d\n", mState);
        return XCAM_RETURN_ERROR_ANALYZER;
    }

    mState = RK_AIQ_CORE_STATE_INVALID;

    EXIT_ANALYZER_FUNCTION();

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqCore::start()
{
    ENTER_ANALYZER_FUNCTION();

    if ((mState != RK_AIQ_CORE_STATE_PREPARED) &&
            (mState != RK_AIQ_CORE_STATE_STOPED)) {
        LOGE_ANALYZER("wrong state %d\n", mState);
        return XCAM_RETURN_ERROR_ANALYZER;
    }

    mRkAiqCoreTh->triger_start();
    mRkAiqCoreTh->start();
    mRkAiqCorePpTh->triger_start();
    mRkAiqCorePpTh->start();
    mRkAiqCoreEvtsTh->triger_start();
    mRkAiqCoreEvtsTh->start();
    if (mRkAiqCoreEvtsTh.ptr()) {
        mRkAiqCoreTxBufAnalyzerTh->triger_start();
        mRkAiqCoreTxBufAnalyzerTh->start();
    }

    mState = RK_AIQ_CORE_STATE_STARTED;

    EXIT_ANALYZER_FUNCTION();

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqCore::stop()
{
    ENTER_ANALYZER_FUNCTION();

    if (mState != RK_AIQ_CORE_STATE_STARTED && mState != RK_AIQ_CORE_STATE_RUNNING) {
        LOGW_ANALYZER("in state %d\n", mState);
        return XCAM_RETURN_NO_ERROR;
    }

    mRkAiqCoreTh->triger_stop();
    mRkAiqCoreTh->stop();
    mRkAiqCorePpTh->triger_stop();
    mRkAiqCorePpTh->stop();
    mRkAiqCoreEvtsTh->triger_stop();
    mRkAiqCoreEvtsTh->stop();
    if (mRkAiqCoreEvtsTh.ptr()) {
        mRkAiqCoreTxBufAnalyzerTh->triger_stop();
        mRkAiqCoreTxBufAnalyzerTh->stop();
    }
    mAiqStatsCachedList.clear();
    mAiqStatsOutMap.clear();
    mState = RK_AIQ_CORE_STATE_STOPED;
    firstStatsReceived = false;
    mIspStatsCond.broadcast ();
    EXIT_ANALYZER_FUNCTION();

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqCore::prepare(const rk_aiq_exposure_sensor_descriptor* sensor_des,
                   int mode)
{
    ENTER_ANALYZER_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    // check state
    if ((mState == RK_AIQ_CORE_STATE_STARTED) ||
            (mState == RK_AIQ_CORE_STATE_INVALID) ||
            (mState == RK_AIQ_CORE_STATE_RUNNING)) {
        LOGW_ANALYZER("in state %d\n", mState);
        return XCAM_RETURN_NO_ERROR;
    }

    if (mAlogsSharedParams.working_mode != mode)
        mAlogsSharedParams.conf_type = RK_AIQ_ALGO_CONFTYPE_INIT;

    mAlogsSharedParams.snsDes = *sensor_des;
    mAlogsSharedParams.working_mode = mode;

    if ((mAlogsSharedParams.snsDes.sensor_pixelformat == V4L2_PIX_FMT_GREY) ||
            (mAlogsSharedParams.snsDes.sensor_pixelformat == V4L2_PIX_FMT_Y10) ||
            (mAlogsSharedParams.snsDes.sensor_pixelformat == V4L2_PIX_FMT_Y12)) {
        mAlogsSharedParams.is_bw_sensor = true;
        mGrayMode = RK_AIQ_GRAY_MODE_ON;
        mAlogsSharedParams.gray_mode = true;
    } else {
        mAlogsSharedParams.is_bw_sensor = false;
        if (mAlogsSharedParams.calib->colorAsGrey.enable) {
            mAlogsSharedParams.gray_mode = true;
            mGrayMode = RK_AIQ_GRAY_MODE_ON;
        }
    }

#define PREPARE_ALGO(at) \
    LOGD_ANALYZER("%s handle prepare start ....", #at); \
    if (mCur##at##AlgoHdl.ptr() && mCur##at##AlgoHdl->getEnable()) { \
        /* update user initial params */ \
        ret = mCur##at##AlgoHdl->updateConfig(true); \
        RKAIQCORE_CHECK_BYPASS(ret, "%s update initial user params failed", #at); \
        mCur##at##AlgoHdl->setReConfig(mState == RK_AIQ_CORE_STATE_STOPED); \
        ret = mCur##at##AlgoHdl->prepare(); \
        RKAIQCORE_CHECK_BYPASS(ret, "%s prepare failed", #at); \
    } \
    LOGD_ANALYZER("%s handle prepare end ....", #at);

#define PREPARE_CUSTOM_ALGO(at) \
    LOGD_ANALYZER("%s handle custom prepare start ....", #at); \
    if (mCurCustom##at##AlgoHdl.ptr() && mCurCustom##at##AlgoHdl->getEnable()) { \
        /* update user initial params */ \
        ret = mCurCustom##at##AlgoHdl->updateConfig(true); \
        RKAIQCORE_CHECK_BYPASS(ret, "%s custom update initial user params failed", #at); \
        mCurCustom##at##AlgoHdl->setReConfig(mState == RK_AIQ_CORE_STATE_STOPED); \
        ret = mCurCustom##at##AlgoHdl->prepare(); \
        RKAIQCORE_CHECK_BYPASS(ret, "%s custom prepare failed", #at); \
    } \
    LOGD_ANALYZER("%s handle custom prepare end ....", #at);

    PREPARE_ALGO(Ae);
    PREPARE_CUSTOM_ALGO(Ae);
    PREPARE_ALGO(Awb);
    PREPARE_CUSTOM_ALGO(Awb);
    PREPARE_ALGO(Af);
    PREPARE_CUSTOM_ALGO(Af);
    PREPARE_ALGO(Ahdr);
    PREPARE_ALGO(Anr);
    PREPARE_ALGO(Adhaz);
    PREPARE_ALGO(Acp);
    PREPARE_ALGO(Asharp);
    PREPARE_ALGO(A3dlut);
    PREPARE_ALGO(Ablc);
    PREPARE_ALGO(Accm);
    PREPARE_ALGO(Acgc);
    PREPARE_ALGO(Adebayer);
    PREPARE_ALGO(Adpcc);
    PREPARE_ALGO(Afec);
    PREPARE_ALGO(Agamma);
    PREPARE_ALGO(Adegamma);
    PREPARE_ALGO(Agic);
    PREPARE_ALGO(Aie);
    PREPARE_ALGO(Aldch);
    PREPARE_ALGO(Alsc);
    PREPARE_ALGO(Aorb);
    PREPARE_ALGO(Ar2y);
    PREPARE_ALGO(Awdr);
    PREPARE_ALGO(Asd);
    PREPARE_ALGO(Afd);

    mAlogsSharedParams.init = true;
    analyzeInternal(RK_AIQ_CORE_ANALYZE_ALL);
    analyzeInternalPp();
    mAlogsSharedParams.init = false;

    if (mAiqCurParams->data()->mIspMeasParams.ptr()) {
        mAiqCurParams->data()->mIspMeasParams->data()->frame_id = 0;
    }

    if (mAiqCurParams->data()->mIspOtherParams.ptr()) {
        mAiqCurParams->data()->mIspOtherParams->data()->frame_id = 0;
    }

    if (mAiqCurParams->data()->mIsppMeasParams.ptr()) {
        mAiqCurParams->data()->mIsppMeasParams->data()->frame_id = 0;
    }

    if (mAiqCurParams->data()->mIsppOtherParams.ptr()) {
        mAiqCurParams->data()->mIsppOtherParams->data()->frame_id = 0;
    }

    mState = RK_AIQ_CORE_STATE_PREPARED;

    EXIT_ANALYZER_FUNCTION();

    return XCAM_RETURN_NO_ERROR;
}

SmartPtr<RkAiqFullParamsProxy>
RkAiqCore::analyzeInternal(enum rk_aiq_core_analyze_type_e type)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    if (mAlogsSharedParams.init) {
        // run algos without stats to generate
        // initial params
        mAlogsSharedParams.ispStats.aec_stats_valid = false;
        mAlogsSharedParams.ispStats.awb_stats_valid = false;
        mAlogsSharedParams.ispStats.awb_cfg_effect_valid = false;
        mAlogsSharedParams.ispStats.af_stats_valid = false;
        mAlogsSharedParams.ispStats.ahdr_stats_valid = false;
    }

    SmartPtr<RkAiqFullParamsProxy> aiqParamProxy = mAiqParamsPool->get_item();

    if (!aiqParamProxy.ptr()) {
        LOGE_ANALYZER("no free aiq params buffer!");
        return NULL;
    }

    RkAiqFullParams* aiqParams = aiqParamProxy->data().ptr();
    aiqParams->reset();

    if (type == RK_AIQ_CORE_ANALYZE_ALL || \
            type == RK_AIQ_CORE_ANALYZE_MEAS) {
        if (mAiqIspMeasParamsPool->has_free_items()) {
            aiqParams->mIspMeasParams = mAiqIspMeasParamsPool->get_item();
            aiqParams->mIspMeasParams->data()->update_mask = 0;
        } else {
            LOGE_ANALYZER("no free isp params buffer!");
            return NULL;
        }

        if (mAiqExpParamsPool->has_free_items()) {
            aiqParams->mExposureParams = mAiqExpParamsPool->get_item();
        } else {
            LOGE_ANALYZER("no free exposure params buffer!");
            return NULL;
        }

        if (mAiqIrisParamsPool->has_free_items()) {
            aiqParams->mIrisParams = mAiqIrisParamsPool->get_item();
        } else {
            LOGE_ANALYZER("no free iris params buffer!");
            return NULL;
        }

        if (mAiqFocusParamsPool->has_free_items()) {
            aiqParams->mFocusParams = mAiqFocusParamsPool->get_item();
        } else {
            LOGE_ANALYZER("no free focus params buffer!");
            return NULL;
        }
    }

#ifndef RK_SIMULATOR_HW
    if (type == RK_AIQ_CORE_ANALYZE_ALL || \
            type == RK_AIQ_CORE_ANALYZE_OTHER) {
        if (mAiqIspOtherParamsPool->has_free_items()) {
            aiqParams->mIspOtherParams = mAiqIspOtherParamsPool->get_item();
            aiqParams->mIspOtherParams->data()->update_mask = 0;
        } else {
            LOGE_ANALYZER("no free isp other params buffer!");
            return NULL;
        }

        if (mAiqIsppOtherParamsPool->has_free_items()) {
            aiqParams->mIsppOtherParams = mAiqIsppOtherParamsPool->get_item();
            aiqParams->mIsppOtherParams->data()->update_mask = 0;
        } else {
            LOGE_ANALYZER("no free ispp other params buffer!");
            return NULL;
        }
    }
#endif

#if 0
    // for test
    int fd = open("/tmp/cpsl", O_RDWR);
    if (fd != -1) {
        char c;
        read(fd, &c, 1);
        int enable = atoi(&c);

        rk_aiq_cpsl_cfg_t cfg;

        cfg.mode = (RKAiqOPMode_t)enable;
        cfg.lght_src = RK_AIQ_CPSLS_LED;
        if (cfg.mode == RK_AIQ_OP_MODE_AUTO) {
            cfg.u.a.sensitivity = 100;
            cfg.u.a.sw_interval = 60;
            cfg.gray_on = false;
            LOGI_ANALYZER("mode sensitivity %f, interval time %d s\n",
                          cfg.u.a.sensitivity, cfg.u.a.sw_interval);
        } else {
            cfg.gray_on = true;
            cfg.u.m.on = true;
            cfg.u.m.strength_ir = 100;
            cfg.u.m.strength_led = 100;
            LOGI_ANALYZER("on %d, strength_led %f, strength_ir %f\n",
                          cfg.u.m.on, cfg.u.m.strength_led, cfg.u.m.strength_ir);
        }
        close(fd);
        setCpsLtCfg(cfg);
    }
#endif
    ret = preProcess(type);
    RKAIQCORE_CHECK_RET_NULL(ret, "preprocess failed");
    genCpslResult(aiqParams);

    ret = processing(type);
    RKAIQCORE_CHECK_RET_NULL(ret, "processing failed");

    ret = postProcess(type);
    RKAIQCORE_CHECK_RET_NULL(ret, "post process failed");

    ret = genIspResult(aiqParams, type);
    EXIT_ANALYZER_FUNCTION();

    return aiqParamProxy;
}

SmartPtr<RkAiqFullParamsProxy>
RkAiqCore::analyzeInternalPp()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    if (mAlogsSharedParams.init) {
        // run algos without stats to generate
        // initial params
        mAlogsSharedParams.ispStats.orb_stats_valid = false;
    }

    SmartPtr<RkAiqFullParamsProxy> aiqParamProxy = mAiqParamsPool->get_item();

    if (!aiqParamProxy.ptr()) {
        LOGE_ANALYZER("no free aiq params buffer!");
        return NULL;
    }

    RkAiqFullParams* aiqParams = aiqParamProxy->data().ptr();
    aiqParams->reset();

    if (mAiqIsppMeasParamsPool->has_free_items()) {
        aiqParams->mIsppMeasParams = mAiqIsppMeasParamsPool->get_item();
        if (!aiqParams->mIsppMeasParams.ptr()) {
            LOGE_ANALYZER("no free ispp params buffer!");
            return NULL;
        }
        aiqParams->mIsppMeasParams->data()->update_mask = 0;
    } else {
        LOGE_ANALYZER("no free ispp params buffer!");
        return NULL;
    }

    ret = preProcessPp();
    RKAIQCORE_CHECK_RET_NULL(ret, "preprocessPp failed");

    ret = processingPp();
    RKAIQCORE_CHECK_RET_NULL(ret, "processingPp failed");

    ret = postProcessPp();
    RKAIQCORE_CHECK_RET_NULL(ret, "post processPp failed");

    genIspAorbResult(aiqParams);

    if (!mAiqCurParams->data()->mIsppMeasParams.ptr()) {
        mAiqCurParams->data()->mIsppMeasParams = aiqParams->mIsppMeasParams;
    } else {
        if (aiqParams->mIsppMeasParams->data()->update_mask & RKAIQ_ISPP_ORB_ID) {
            mAiqCurParams->data()->mIsppMeasParams->data()->update_mask |= RKAIQ_ISPP_ORB_ID;
            mAiqCurParams->data()->mIsppMeasParams->data()->orb =
                aiqParams->mIsppMeasParams->data()->orb;
        }
    }

    EXIT_ANALYZER_FUNCTION();

    return aiqParamProxy;
}

XCamReturn
RkAiqCore::genIspResult(RkAiqFullParams *aiqParams, enum rk_aiq_core_analyze_type_e type)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    if (type == RK_AIQ_CORE_ANALYZE_ALL || \
            type == RK_AIQ_CORE_ANALYZE_MEAS) {
        genIspAeResult(aiqParams);
        genIspAwbResult(aiqParams);
        genIspAfResult(aiqParams);
        genIspAhdrResult(aiqParams);
        genIspAccmResult(aiqParams);
        genIspAdpccResult(aiqParams);
        genIspAlscResult(aiqParams);
#ifdef RK_SIMULATOR_HW
        genIspAnrResult(aiqParams);
        genIspAdhazResult(aiqParams);
        genIspAsdResult(aiqParams);
        genIspAcpResult(aiqParams);
        genIspAieResult(aiqParams);
        genIspAsharpResult(aiqParams);
        genIspA3dlutResult(aiqParams);
        genIspAblcResult(aiqParams);
        genIspAcgcResult(aiqParams);
        genIspAdebayerResult(aiqParams);
        genIspAfecResult(aiqParams);
        //genIspAorbResult(aiqParams);
        genIspAgammaResult(aiqParams);
        genIspAdegammaResult(aiqParams);
        genIspAgicResult(aiqParams);
        genIspAldchResult(aiqParams);
        genIspAr2yResult(aiqParams);
        genIspAwdrResult(aiqParams);
#endif
        mAiqCurParams->data()->mIspMeasParams = aiqParams->mIspMeasParams;
        // mAiqCurParams->data()->mIsppMeasParams = aiqParams->mIsppMeasParams;
        mAiqCurParams->data()->mExposureParams = aiqParams->mExposureParams;
        mAiqCurParams->data()->mIrisParams = aiqParams->mIrisParams;
        mAiqCurParams->data()->mFocusParams = aiqParams->mFocusParams;
        mAiqCurParams->data()->mCpslParams = aiqParams->mCpslParams;
    }

#ifndef RK_SIMULATOR_HW
    if (type == RK_AIQ_CORE_ANALYZE_ALL || \
            type == RK_AIQ_CORE_ANALYZE_OTHER) {
        genIspAnrResult(aiqParams);
        genIspAdhazResult(aiqParams);
        genIspAsdResult(aiqParams);
        genIspAcpResult(aiqParams);
        genIspAieResult(aiqParams);
        genIspAsharpResult(aiqParams);
        genIspA3dlutResult(aiqParams);
        genIspAblcResult(aiqParams);
        genIspAcgcResult(aiqParams);
        genIspAdebayerResult(aiqParams);
        genIspAfecResult(aiqParams);
        genIspAgammaResult(aiqParams);
        genIspAdegammaResult(aiqParams);
        genIspAgicResult(aiqParams);
        genIspAldchResult(aiqParams);
        genIspAr2yResult(aiqParams);
        genIspAwdrResult(aiqParams);

        mAiqCurParams->data()->mIspOtherParams = aiqParams->mIspOtherParams;
        mAiqCurParams->data()->mIsppOtherParams = aiqParams->mIsppOtherParams;
    }
#endif

    return ret;
}

XCamReturn
RkAiqCore::genIspAeResult(RkAiqFullParams* params)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoProcResAe* ae_proc =
        mAlogsSharedParams.procResComb.ae_proc_res;
    RkAiqAlgoPostResAe* ae_post =
        mAlogsSharedParams.postResComb.ae_post_res;

    SmartPtr<rk_aiq_isp_meas_params_t> isp_param =
        params->mIspMeasParams->data();
    SmartPtr<rk_aiq_exposure_params_wrapper_t> exp_param =
        params->mExposureParams->data();
    SmartPtr<rk_aiq_iris_params_wrapper_t> iris_param =
        params->mIrisParams->data();

    if (!ae_proc) {
        LOGD_ANALYZER("no ae_proc result");
        return XCAM_RETURN_NO_ERROR;
    }

    if (!ae_post) {
        LOGD_ANALYZER("no ae_post result");
        return XCAM_RETURN_NO_ERROR;
    }

    // TODO: gen ae common result
    SmartPtr<RkAiqHandle>* handle = getCurAlgoTypeHandle(RK_AIQ_ALGO_TYPE_AE);
    int algo_id = (*handle)->getAlgoId();
    // gen common result

    exp_param->aecExpInfo.LinearExp = ae_proc->new_ae_exp.LinearExp;
    memcpy(exp_param->aecExpInfo.HdrExp, ae_proc->new_ae_exp.HdrExp, sizeof(ae_proc->new_ae_exp.HdrExp));
    exp_param->aecExpInfo.frame_length_lines = ae_proc->new_ae_exp.frame_length_lines;
    exp_param->aecExpInfo.line_length_pixels = ae_proc->new_ae_exp.line_length_pixels;
    exp_param->aecExpInfo.pixel_clock_freq_mhz = ae_proc->new_ae_exp.pixel_clock_freq_mhz;
    exp_param->aecExpInfo.Iris.PIris = ae_proc->new_ae_exp.Iris.PIris;

    iris_param->PIris.step = ae_proc->new_ae_exp.Iris.PIris.step;
    iris_param->PIris.update = ae_proc->new_ae_exp.Iris.PIris.update;

    isp_param->aec_meas = ae_proc->ae_meas;
    isp_param->update_mask |= RKAIQ_ISP_AEC_ID;

    isp_param->hist_meas = ae_proc->hist_meas;
    isp_param->update_mask |= RKAIQ_ISP_HIST_ID;


    // gen rk ae result
    if (algo_id == 0) {
        RkAiqAlgoProcResAeInt* ae_proc_rk = (RkAiqAlgoProcResAeInt*)ae_proc;
        memcpy(exp_param->exp_tbl, ae_proc_rk->ae_proc_res_rk.exp_set_tbl, sizeof(exp_param->exp_tbl));
        exp_param->exp_tbl_size = ae_proc_rk->ae_proc_res_rk.exp_set_cnt;
        exp_param->algo_id = algo_id;

        mAlogsSharedParams.tx_buf.IsAeConverged = ae_proc_rk->ae_proc_res_rk.IsConverged;
        mAlogsSharedParams.tx_buf.envChange = ae_proc_rk->ae_proc_res_rk.envChange;

        RkAiqAlgoPostResAeInt* ae_post_rk = (RkAiqAlgoPostResAeInt*)ae_post;
        iris_param->DCIris.update = ae_post_rk->ae_post_res_rk.DCIris.update;
        iris_param->DCIris.pwmDuty = ae_post_rk->ae_post_res_rk.DCIris.pwmDuty;
    } else {
        RkAiqAlgoProcResAe* ae_customer = (RkAiqAlgoProcResAe*)ae_proc;
        memcpy(exp_param->exp_tbl, ae_customer->ae_proc_res.exp_set_tbl, sizeof(exp_param->exp_tbl));
        exp_param->exp_tbl_size = ae_customer->ae_proc_res.exp_set_cnt;
        exp_param->algo_id = 0;
    }

    EXIT_ANALYZER_FUNCTION();

    return ret;
}

XCamReturn
RkAiqCore::genIspAfdResult(RkAiqFullParams* params)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoProcResAfd* afd_proc =
        mAlogsSharedParams.procResComb.afd_proc_res;
    SmartPtr<rk_aiq_ispp_meas_params_t> ispp_param =
        params->mIsppMeasParams->data();

    if (!afd_proc) {
        LOGD_ANALYZER("no afd result");
        return XCAM_RETURN_NO_ERROR;
    }

    // TODO: gen afd common result

    SmartPtr<RkAiqHandle>* handle = getCurAlgoTypeHandle(RK_AIQ_ALGO_TYPE_AFD);
    int algo_id = (*handle)->getAlgoId();

    // gen rk afd result
    if (algo_id == 0) {
        //TODO
    }

    EXIT_ANALYZER_FUNCTION();

    return ret;
}

XCamReturn
RkAiqCore::genIspAwbResult(RkAiqFullParams* params)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoProcResAwb* awb_com =
        mAlogsSharedParams.procResComb.awb_proc_res;
    SmartPtr<rk_aiq_isp_meas_params_t> isp_param =
        params->mIspMeasParams->data();

    if (!awb_com) {
        LOGD_ANALYZER("no awb result");
        return XCAM_RETURN_NO_ERROR;
    }

    // TODO: gen awb common result
    RkAiqAlgoProcResAwb* awb_rk = (RkAiqAlgoProcResAwb*)awb_com;
    isp_param->awb_gain_update = awb_rk->awb_gain_update;
    isp_param->awb_gain = awb_rk->awb_gain_algo;
    isp_param->update_mask |= RKAIQ_ISP_AWB_GAIN_ID;

    isp_param->awb_cfg_update = awb_rk->awb_cfg_update;
    isp_param->awb_cfg_v200 = awb_rk->awb_hw0_para;
    isp_param->awb_cfg_v201 = awb_rk->awb_hw1_para;
    isp_param->update_mask |= RKAIQ_ISP_AWB_ID;

    SmartPtr<RkAiqHandle>* handle = getCurAlgoTypeHandle(RK_AIQ_ALGO_TYPE_AWB);
    int algo_id = (*handle)->getAlgoId();

    // gen rk awb result
    if (algo_id == 0) {
        RkAiqAlgoProcResAwbInt* awb_rk_int = (RkAiqAlgoProcResAwbInt*)awb_com;
    }

    EXIT_ANALYZER_FUNCTION();

    return ret;
}

XCamReturn
RkAiqCore::genIspAfResult(RkAiqFullParams* params)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoProcResAf* af_com =
        mAlogsSharedParams.procResComb.af_proc_res;
    SmartPtr<rk_aiq_isp_meas_params_t> isp_param =
        params->mIspMeasParams->data();
    SmartPtr<rk_aiq_focus_params_t> focus_param =
        params->mFocusParams->data();

    isp_param->af_cfg_update = false;
    memset(focus_param.ptr(), 0, sizeof(rk_aiq_focus_params_t));
    if (!af_com) {
        LOGD_ANALYZER("no af result");
        return XCAM_RETURN_NO_ERROR;
    }

    // TODO: gen af common result

    SmartPtr<RkAiqHandle>* handle = getCurAlgoTypeHandle(RK_AIQ_ALGO_TYPE_AF);
    int algo_id = (*handle)->getAlgoId();

    // gen rk af result
    if (algo_id == 0) {
        RkAiqAlgoProcResAfInt* af_rk = (RkAiqAlgoProcResAfInt*)af_com;

        isp_param->af_meas = af_rk->af_proc_res_com.af_isp_param;
        isp_param->af_cfg_update = af_rk->af_proc_res_com.af_cfg_update;
        isp_param->update_mask |= RKAIQ_ISP_AF_ID;

        focus_param->zoomfocus_modifypos = af_rk->af_proc_res_com.af_focus_param.zoomfocus_modifypos;
        focus_param->focus_correction = af_rk->af_proc_res_com.af_focus_param.focus_correction;
        focus_param->zoom_correction = af_rk->af_proc_res_com.af_focus_param.zoom_correction;
        focus_param->lens_pos_valid = af_rk->af_proc_res_com.af_focus_param.lens_pos_valid;
        focus_param->zoom_pos_valid = af_rk->af_proc_res_com.af_focus_param.zoom_pos_valid;
        focus_param->send_zoom_reback = af_rk->af_proc_res_com.af_focus_param.send_zoom_reback;
        focus_param->send_focus_reback = af_rk->af_proc_res_com.af_focus_param.send_focus_reback;
        focus_param->end_zoom_chg = af_rk->af_proc_res_com.af_focus_param.end_zoom_chg;
        focus_param->focus_noreback = af_rk->af_proc_res_com.af_focus_param.focus_noreback;
        focus_param->use_manual = af_rk->af_proc_res_com.af_focus_param.use_manual;
        focus_param->auto_focpos = af_rk->af_proc_res_com.af_focus_param.auto_focpos;
        focus_param->auto_zoompos = af_rk->af_proc_res_com.af_focus_param.auto_zoompos;
        focus_param->manual_focpos = af_rk->af_proc_res_com.af_focus_param.manual_focpos;
        focus_param->manual_zoompos = af_rk->af_proc_res_com.af_focus_param.manual_zoompos;
        focus_param->next_pos_num = af_rk->af_proc_res_com.af_focus_param.next_pos_num;
        for (int i = 0; i < af_rk->af_proc_res_com.af_focus_param.next_pos_num; i++) {
            focus_param->next_lens_pos[i] = af_rk->af_proc_res_com.af_focus_param.next_lens_pos[i];
            focus_param->next_zoom_pos[i] = af_rk->af_proc_res_com.af_focus_param.next_zoom_pos[i];
        }

        {
            SmartPtr<RkAiqHandle>* ae_handle = getCurAlgoTypeHandle(RK_AIQ_ALGO_TYPE_AE);
            int algo_id = (*ae_handle)->getAlgoId();

            if (ae_handle) {
                if ((algo_id == 0) && (af_rk->af_proc_res_com.lockae_en)) {
                    RkAiqAeHandleInt *ae_algo = dynamic_cast<RkAiqAeHandleInt*>(ae_handle->ptr());
                    Uapi_ExpSwAttr_t expSwAttr;

                    ae_algo->getExpSwAttr(&expSwAttr);
                    if (expSwAttr.enable != !af_rk->af_proc_res_com.lockae) {
                        expSwAttr.enable = !af_rk->af_proc_res_com.lockae;
                        ae_algo->setExpSwAttr(expSwAttr);
                    }
                }
            }
        }
    }

    EXIT_ANALYZER_FUNCTION();

    return ret;
}

XCamReturn
RkAiqCore::genIspAhdrResult(RkAiqFullParams* params)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoProcResAhdr* ahdr_com =
        mAlogsSharedParams.procResComb.ahdr_proc_res;
    SmartPtr<rk_aiq_isp_meas_params_t> isp_param =
        params->mIspMeasParams->data();

    if (!ahdr_com) {
        LOGD_ANALYZER("no ahdr result");
        return XCAM_RETURN_NO_ERROR;
    }

    // TODO: gen ahdr common result

    SmartPtr<RkAiqHandle>* handle = getCurAlgoTypeHandle(RK_AIQ_ALGO_TYPE_AHDR);
    int algo_id = (*handle)->getAlgoId();

    // gen rk ahdr result
    if (algo_id == 0) {
        RkAiqAlgoProcResAhdrInt* ahdr_rk = (RkAiqAlgoProcResAhdrInt*)ahdr_com;

        isp_param->ahdr_proc_res.MgeProcRes.sw_hdrmge_mode =
            ahdr_rk->AhdrProcRes.MgeProcRes.sw_hdrmge_mode;
        isp_param->ahdr_proc_res.MgeProcRes.sw_hdrmge_lm_dif_0p9 =
            ahdr_rk->AhdrProcRes.MgeProcRes.sw_hdrmge_lm_dif_0p9;
        isp_param->ahdr_proc_res.MgeProcRes.sw_hdrmge_ms_dif_0p8 =
            ahdr_rk->AhdrProcRes.MgeProcRes.sw_hdrmge_ms_dif_0p8;
        isp_param->ahdr_proc_res.MgeProcRes.sw_hdrmge_lm_dif_0p15 =
            ahdr_rk->AhdrProcRes.MgeProcRes.sw_hdrmge_lm_dif_0p15;
        isp_param->ahdr_proc_res.MgeProcRes.sw_hdrmge_ms_dif_0p15 =
            ahdr_rk->AhdrProcRes.MgeProcRes.sw_hdrmge_ms_dif_0p15;
        isp_param->ahdr_proc_res.MgeProcRes.sw_hdrmge_gain0 =
            ahdr_rk->AhdrProcRes.MgeProcRes.sw_hdrmge_gain0;
        isp_param->ahdr_proc_res.MgeProcRes.sw_hdrmge_gain0_inv =
            ahdr_rk->AhdrProcRes.MgeProcRes.sw_hdrmge_gain0_inv;
        isp_param->ahdr_proc_res.MgeProcRes.sw_hdrmge_gain1 =
            ahdr_rk->AhdrProcRes.MgeProcRes.sw_hdrmge_gain1;
        isp_param->ahdr_proc_res.MgeProcRes.sw_hdrmge_gain1_inv =
            ahdr_rk->AhdrProcRes.MgeProcRes.sw_hdrmge_gain1_inv;
        isp_param->ahdr_proc_res.MgeProcRes.sw_hdrmge_gain2 =
            ahdr_rk->AhdrProcRes.MgeProcRes.sw_hdrmge_gain2;
        for(int i = 0; i < 17; i++)
        {
            isp_param->ahdr_proc_res.MgeProcRes.sw_hdrmge_e_y[i] =
                ahdr_rk->AhdrProcRes.MgeProcRes.sw_hdrmge_e_y[i];
            isp_param->ahdr_proc_res.MgeProcRes.sw_hdrmge_l1_y[i] =
                ahdr_rk->AhdrProcRes.MgeProcRes.sw_hdrmge_l1_y[i];
            isp_param->ahdr_proc_res.MgeProcRes.sw_hdrmge_l0_y[i] =
                ahdr_rk->AhdrProcRes.MgeProcRes.sw_hdrmge_l0_y[i];
        }
        isp_param->update_mask |= RKAIQ_ISP_AHDRMGE_ID;

        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_lgmax =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_lgmax;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_lgscl =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_lgscl;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_lgscl_inv =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_lgscl_inv;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_clipratio0 =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_clipratio0;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_clipratio1 =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_clipratio1;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_clipgap0 =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_clipgap0;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_clipgap1 =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_clipgap1;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_ratiol =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_ratiol;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_hist_min =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_hist_min;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_hist_low =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_hist_low;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_hist_high =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_hist_high;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_hist_0p3 =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_hist_0p3;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_hist_shift =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_hist_shift;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_palpha_0p18 =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_palpha_0p18;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_palpha_lw0p5 =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_palpha_lw0p5;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_palpha_lwscl =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_palpha_lwscl;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_maxpalpha =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_maxpalpha;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_maxgain =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_maxgain;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_cfg_alpha =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_cfg_alpha;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_set_gainoff =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_gainoff;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_set_lgmin =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_lgmin;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_set_lgmax =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_lgmax;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_set_lgmean =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_lgmean;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_set_weightkey =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_weightkey;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_set_lgrange0 =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_lgrange0;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_set_lgrange1 =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_lgrange1;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_set_lgavgmax =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_lgavgmax;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_set_palpha =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_palpha;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_gain_ld_off1 =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_gain_ld_off1;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_gain_ld_off2 =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_gain_ld_off2;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_cnt_vsize =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_cnt_vsize;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_big_en =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_big_en;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_nobig_en =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_nobig_en;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_newhist_en =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_newhist_en;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_cnt_mode =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_cnt_mode;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_expl_lgratio =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_expl_lgratio;
        isp_param->ahdr_proc_res.TmoProcRes.sw_hdrtmo_lgscl_ratio =
            ahdr_rk->AhdrProcRes.TmoProcRes.sw_hdrtmo_lgscl_ratio;

        isp_param->ahdr_proc_res.LongFrameMode =
            ahdr_rk->AhdrProcRes.LongFrameMode;

        isp_param->ahdr_proc_res.isHdrGlobalTmo =
            ahdr_rk->AhdrProcRes.isHdrGlobalTmo;

        isp_param->ahdr_proc_res.bTmoEn =
            ahdr_rk->AhdrProcRes.bTmoEn;

        isp_param->ahdr_proc_res.isLinearTmo =
            ahdr_rk->AhdrProcRes.isLinearTmo;

        isp_param->ahdr_proc_res.TmoFlicker.GlobalTmoStrengthDown =
            ahdr_rk->AhdrProcRes.TmoFlicker.GlobalTmoStrengthDown;
        isp_param->ahdr_proc_res.TmoFlicker.GlobalTmoStrength =
            ahdr_rk->AhdrProcRes.TmoFlicker.GlobalTmoStrength;
        isp_param->ahdr_proc_res.TmoFlicker.iir =
            ahdr_rk->AhdrProcRes.TmoFlicker.iir;
        isp_param->ahdr_proc_res.TmoFlicker.iirmax =
            ahdr_rk->AhdrProcRes.TmoFlicker.iirmax;
        isp_param->ahdr_proc_res.TmoFlicker.height =
            ahdr_rk->AhdrProcRes.TmoFlicker.height;
        isp_param->ahdr_proc_res.TmoFlicker.width =
            ahdr_rk->AhdrProcRes.TmoFlicker.width;

        isp_param->ahdr_proc_res.TmoFlicker.PredictK.correction_factor =
            ahdr_rk->AhdrProcRes.TmoFlicker.PredictK.correction_factor;
        isp_param->ahdr_proc_res.TmoFlicker.PredictK.correction_offset =
            ahdr_rk->AhdrProcRes.TmoFlicker.PredictK.correction_offset;
        isp_param->ahdr_proc_res.TmoFlicker.PredictK.Hdr3xLongPercent =
            ahdr_rk->AhdrProcRes.TmoFlicker.PredictK.Hdr3xLongPercent;
        isp_param->ahdr_proc_res.TmoFlicker.PredictK.UseLongUpTh =
            ahdr_rk->AhdrProcRes.TmoFlicker.PredictK.UseLongUpTh;
        isp_param->ahdr_proc_res.TmoFlicker.PredictK.UseLongLowTh =
            ahdr_rk->AhdrProcRes.TmoFlicker.PredictK.UseLongLowTh;
        for(int i = 0; i < 3; i++)
            isp_param->ahdr_proc_res.TmoFlicker.LumaDeviation[i] =
                ahdr_rk->AhdrProcRes.TmoFlicker.LumaDeviation[i];
        isp_param->ahdr_proc_res.TmoFlicker.StableThr =
            ahdr_rk->AhdrProcRes.TmoFlicker.StableThr;
        isp_param->update_mask |= RKAIQ_ISP_AHDRTMO_ID;
    }

    EXIT_ANALYZER_FUNCTION();

    return ret;
}

XCamReturn
RkAiqCore::genIspAnrResult(RkAiqFullParams* params)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoProcResAnr* anr_com =
        mAlogsSharedParams.procResComb.anr_proc_res;

    if (!anr_com) {
        LOGD_ANALYZER("no anr result");
        return XCAM_RETURN_NO_ERROR;
    }

    // TODO: gen anr common result

    SmartPtr<RkAiqHandle>* handle = getCurAlgoTypeHandle(RK_AIQ_ALGO_TYPE_ANR);
    int algo_id = (*handle)->getAlgoId();

    // gen rk anr result
    if (algo_id == 0) {
        RkAiqAlgoProcResAnrInt* anr_rk = (RkAiqAlgoProcResAnrInt*)anr_com;

#ifdef RK_SIMULATOR_HW
        SmartPtr<rk_aiq_isp_meas_params_t> isp_param =
            params->mIspMeasParams->data();
        LOGD_ANR("oyyf: %s:%d output isp param start\n", __FUNCTION__, __LINE__);
        memcpy(&isp_param->rkaiq_anr_proc_res.stBayernrParamSelect,
               &anr_rk->stAnrProcResult.stBayernrParamSelect,
               sizeof(RKAnr_Bayernr_Params_Select_t));
        memcpy(&isp_param->rkaiq_anr_proc_res.stUvnrParamSelect,
               &anr_rk->stAnrProcResult.stUvnrParamSelect,
               sizeof(RKAnr_Uvnr_Params_Select_t));

        memcpy(&isp_param->rkaiq_anr_proc_res.stMfnrParamSelect,
               &anr_rk->stAnrProcResult.stMfnrParamSelect,
               sizeof(RKAnr_Mfnr_Params_Select_t));

        memcpy(&isp_param->rkaiq_anr_proc_res.stYnrParamSelect,
               &anr_rk->stAnrProcResult.stYnrParamSelect,
               sizeof(RKAnr_Ynr_Params_Select_t));

        LOGD_ANR("oyyf: %s:%d output isp param end \n", __FUNCTION__, __LINE__);
#else
        SmartPtr<rk_aiq_isp_other_params_t> isp_other_param =
            params->mIspOtherParams->data();
        SmartPtr<rk_aiq_ispp_other_params_t> ispp_other_param =
            params->mIsppOtherParams->data();

        LOGD_ANR("oyyf: %s:%d output isp param start\n", __FUNCTION__, __LINE__);
        isp_other_param->update_mask |= RKAIQ_ISP_RAWNR_ID;
        memcpy(&isp_other_param->rawnr,
               &anr_rk->stAnrProcResult.stBayernrFix,
               sizeof(rk_aiq_isp_rawnr_t));

        ispp_other_param->update_mask |= RKAIQ_ISPP_NR_ID;
        memcpy(&ispp_other_param->uvnr,
               &anr_rk->stAnrProcResult.stUvnrFix,
               sizeof(RKAnr_Uvnr_Fix_t));

        memcpy(&ispp_other_param->ynr,
               &anr_rk->stAnrProcResult.stYnrFix,
               sizeof(RKAnr_Ynr_Fix_t));

        ispp_other_param->update_mask |= RKAIQ_ISPP_TNR_ID;
        memcpy(&ispp_other_param->tnr,
               &anr_rk->stAnrProcResult.stMfnrFix,
               sizeof(RKAnr_Mfnr_Fix_t));

        isp_other_param->update_mask |= RKAIQ_ISP_GAIN_ID;
        memcpy(&isp_other_param->gain_config,
               &anr_rk->stAnrProcResult.stGainFix,
               sizeof(rk_aiq_isp_gain_t));

        isp_other_param->motion_param = anr_rk->stAnrProcResult.stMotionParam;

        LOGD_ANR("oyyf: %s:%d output isp param end \n", __FUNCTION__, __LINE__);

#endif
    }

    EXIT_ANALYZER_FUNCTION();

    return ret;
}

XCamReturn
RkAiqCore::genIspAdhazResult(RkAiqFullParams* params)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoProcResAdhaz* adhaz_com =
        mAlogsSharedParams.procResComb.adhaz_proc_res;
#ifdef RK_SIMULATOR_HW
    SmartPtr<rk_aiq_isp_meas_params_t> isp_param =
        params->mIspMeasParams->data();
#else
    SmartPtr<rk_aiq_isp_other_params_t> isp_param =
        params->mIspOtherParams->data();
#endif

    if (!adhaz_com) {
        LOGD_ANALYZER("no adhaz result");
        return XCAM_RETURN_NO_ERROR;
    }

    // TODO: gen adhaz common result
    RkAiqAlgoProcResAdhaz* adhaz_rk = (RkAiqAlgoProcResAdhaz*)adhaz_com;

    isp_param->adhaz_config = adhaz_rk->adhaz_config;
    isp_param->update_mask |= RKAIQ_ISP_DEHAZE_ID;

    SmartPtr<RkAiqHandle>* handle = getCurAlgoTypeHandle(RK_AIQ_ALGO_TYPE_ADHAZ);
    int algo_id = (*handle)->getAlgoId();

    // gen rk adhaz result
    if (algo_id == 0) {
        RkAiqAlgoProcResAdhazInt* adhaz_rk = (RkAiqAlgoProcResAdhazInt*)adhaz_com;
    }

    EXIT_ANALYZER_FUNCTION();

    return ret;
}

XCamReturn
RkAiqCore::genIspAsdResult(RkAiqFullParams* params)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoProcResAsd* asd_com =
        mAlogsSharedParams.procResComb.asd_proc_res;
#ifdef RK_SIMULATOR_HW
    SmartPtr<rk_aiq_isp_meas_params_t> isp_param =
        params->mIspMeasParams->data();
#else
    SmartPtr<rk_aiq_isp_other_params_t> isp_param =
        params->mIspOtherParams->data();
#endif

    if (!asd_com) {
        LOGD_ANALYZER("no asd result");
        return XCAM_RETURN_NO_ERROR;
    }

    // TODO: gen asd common result

    SmartPtr<RkAiqHandle>* handle = getCurAlgoTypeHandle(RK_AIQ_ALGO_TYPE_ASD);
    int algo_id = (*handle)->getAlgoId();

    // gen rk asd result
    if (algo_id == 0) {
        RkAiqAlgoProcResAsdInt* asd_rk = (RkAiqAlgoProcResAsdInt*)asd_com;

#if 0 // flash test
        RkAiqAlgoPreResAsdInt* asd_pre_rk = (RkAiqAlgoPreResAsdInt*)mAlogsSharedParams.preResComb.asd_pre_res;
        if (asd_pre_rk->asd_result.fl_on) {
            fl_param->flash_mode = RK_AIQ_FLASH_MODE_TORCH;
            fl_param->power[0] = 1000;
            fl_param->strobe = true;
        } else {
            fl_param->flash_mode = RK_AIQ_FLASH_MODE_OFF;
            fl_param->power[0] = 0;
            fl_param->strobe = false;
        }
#endif
    }

    EXIT_ANALYZER_FUNCTION();

    return ret;
}

XCamReturn
RkAiqCore::genIspAcpResult(RkAiqFullParams* params)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoProcResAcp* acp_com =
        mAlogsSharedParams.procResComb.acp_proc_res;
#ifdef RK_SIMULATOR_HW
    SmartPtr<rk_aiq_isp_meas_params_t> isp_param =
        params->mIspMeasParams->data();
#else
    SmartPtr<rk_aiq_isp_other_params_t> isp_param =
        params->mIspOtherParams->data();
#endif

    if (!acp_com) {
        LOGD_ANALYZER("no acp result");
        return XCAM_RETURN_NO_ERROR;
    }

    // TODO: gen acp common result
    rk_aiq_acp_params_t* isp_cp = &isp_param->cp;

    *isp_cp = acp_com->acp_res;
    isp_param->update_mask |= RKAIQ_ISP_CP_ID;

    SmartPtr<RkAiqHandle>* handle = getCurAlgoTypeHandle(RK_AIQ_ALGO_TYPE_ACP);
    int algo_id = (*handle)->getAlgoId();

    // gen rk acp result
    if (algo_id == 0) {
        RkAiqAlgoProcResAcpInt* acp_rk = (RkAiqAlgoProcResAcpInt*)acp_com;
#ifdef RK_SIMULATOR_HW
#else
#endif
    }

    EXIT_ANALYZER_FUNCTION();

    return ret;
}

XCamReturn
RkAiqCore::genIspAieResult(RkAiqFullParams* params)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoProcResAie* aie_com =
        mAlogsSharedParams.procResComb.aie_proc_res;

#ifdef RK_SIMULATOR_HW
    SmartPtr<rk_aiq_isp_meas_params_t> isp_param =
        params->mIspMeasParams->data();
#else
    SmartPtr<rk_aiq_isp_other_params_t> isp_param =
        params->mIspOtherParams->data();
#endif

    if (!aie_com) {
        LOGD_ANALYZER("no aie result");
        return XCAM_RETURN_NO_ERROR;
    }

    // TODO: gen aie common result
    rk_aiq_isp_ie_t* isp_ie = &isp_param->ie;
    isp_ie->base = aie_com->params;
    isp_param->update_mask |= RKAIQ_ISP_IE_ID;

    SmartPtr<RkAiqHandle>* handle = getCurAlgoTypeHandle(RK_AIQ_ALGO_TYPE_AIE);
    int algo_id = (*handle)->getAlgoId();

    // gen rk aie result
    if (algo_id == 0) {
        RkAiqAlgoProcResAieInt* aie_rk = (RkAiqAlgoProcResAieInt*)aie_com;

        isp_ie->extra = aie_rk->params;
#ifdef RK_SIMULATOR_HW
#else
#endif
    }

    EXIT_ANALYZER_FUNCTION();

    return ret;
}

XCamReturn
RkAiqCore::genIspAsharpResult(RkAiqFullParams* params)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoProcResAsharp* asharp_com =
        mAlogsSharedParams.procResComb.asharp_proc_res;

    if (!asharp_com) {
        LOGD_ANALYZER("no asharp result");
        return XCAM_RETURN_NO_ERROR;
    }

    // TODO: gen asharp common result

    SmartPtr<RkAiqHandle>* handle = getCurAlgoTypeHandle(RK_AIQ_ALGO_TYPE_ASHARP);
    int algo_id = (*handle)->getAlgoId();

    // gen rk asharp result
    if (algo_id == 0) {
        RkAiqAlgoProcResAsharpInt* asharp_rk = (RkAiqAlgoProcResAsharpInt*)asharp_com;

#ifdef RK_SIMULATOR_HW
        SmartPtr<rk_aiq_isp_meas_params_t> isp_meas_param =
            params->mIspMeasParams->data();

        LOGD_ASHARP("oyyf: %s:%d output isp param start\n", __FUNCTION__, __LINE__);
        memcpy(&isp_meas_param->rkaiq_asharp_proc_res.stSharpParamSelect.rk_sharpen_params_selected_V1,
               &asharp_rk->stAsharpProcResult.stSharpParamSelect.rk_sharpen_params_selected_V1,
               sizeof(RKAsharp_Sharp_HW_Params_Select_t));

        memcpy(&isp_meas_param->rkaiq_asharp_proc_res.stSharpParamSelect.rk_sharpen_params_selected_V2,
               &asharp_rk->stAsharpProcResult.stSharpParamSelect.rk_sharpen_params_selected_V2,
               sizeof(RKAsharp_Sharp_HW_V2_Params_Select_t));

        memcpy(&isp_meas_param->rkaiq_asharp_proc_res.stSharpParamSelect.rk_sharpen_params_selected_V3,
               &asharp_rk->stAsharpProcResult.stSharpParamSelect.rk_sharpen_params_selected_V3,
               sizeof(RKAsharp_Sharp_HW_V3_Params_Select_t));

        memcpy(&isp_meas_param->rkaiq_asharp_proc_res.stEdgefilterParamSelect,
               &asharp_rk->stAsharpProcResult.stEdgefilterParamSelect,
               sizeof(RKAsharp_EdgeFilter_Params_Select_t));

        LOGD_ASHARP("oyyf: %s:%d output isp param end \n", __FUNCTION__, __LINE__);
#else
        SmartPtr<rk_aiq_ispp_other_params_t> ispp_other_param =
            params->mIsppOtherParams->data();

        LOGD_ASHARP("oyyf: %s:%d output isp param start\n", __FUNCTION__, __LINE__);
        ispp_other_param->update_mask |= RKAIQ_ISPP_SHARP_ID;
        memcpy(&ispp_other_param->sharpen,
               &asharp_rk->stAsharpProcResult.stSharpFix,
               sizeof(rk_aiq_isp_sharpen_t));

        memcpy(&ispp_other_param->edgeflt,
               &asharp_rk->stAsharpProcResult.stEdgefltFix,
               sizeof(rk_aiq_isp_edgeflt_t));

        LOGD_ASHARP("oyyf: %s:%d output isp param end \n", __FUNCTION__, __LINE__);
#endif
    }

    EXIT_ANALYZER_FUNCTION();

    return ret;
}

XCamReturn
RkAiqCore::genIspA3dlutResult(RkAiqFullParams* params)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoProcResA3dlut* a3dlut_com =
        mAlogsSharedParams.procResComb.a3dlut_proc_res;
#ifdef RK_SIMULATOR_HW
    SmartPtr<rk_aiq_isp_meas_params_t> isp_param =
        params->mIspMeasParams->data();
#else
    SmartPtr<rk_aiq_isp_other_params_t> isp_param =
        params->mIspOtherParams->data();
#endif

    if (!a3dlut_com) {
        LOGD_ANALYZER("no a3dlut result");
        return XCAM_RETURN_NO_ERROR;
    }

    // TODO: gen a3dlut common result
    RkAiqAlgoProcResA3dlut* a3dlut_rk = (RkAiqAlgoProcResA3dlut*)a3dlut_com;
    isp_param->lut3d = a3dlut_rk->lut3d_hw_conf;
    isp_param->update_mask |= RKAIQ_ISP_LUT3D_ID;

    SmartPtr<RkAiqHandle>* handle = getCurAlgoTypeHandle(RK_AIQ_ALGO_TYPE_A3DLUT);
    int algo_id = (*handle)->getAlgoId();

    // gen rk a3dlut result
    if (algo_id == 0) {
        RkAiqAlgoProcResA3dlutInt* a3dlut_rk_int = (RkAiqAlgoProcResA3dlutInt*)a3dlut_com;

#ifdef RK_SIMULATOR_HW
#else
#endif
    }

    EXIT_ANALYZER_FUNCTION();

    return ret;
}

XCamReturn
RkAiqCore::genIspAblcResult(RkAiqFullParams* params)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoProcResAblc* ablc_com =
        mAlogsSharedParams.procResComb.ablc_proc_res;
#ifdef RK_SIMULATOR_HW
    SmartPtr<rk_aiq_isp_meas_params_t> isp_param =
        params->mIspMeasParams->data();
#else
    SmartPtr<rk_aiq_isp_other_params_t> isp_param =
        params->mIspOtherParams->data();
#endif

    if (!ablc_com) {
        LOGD_ANALYZER("no ablc result");
        return XCAM_RETURN_NO_ERROR;
    }

    // TODO: gen ablc common result

    SmartPtr<RkAiqHandle>* handle = getCurAlgoTypeHandle(RK_AIQ_ALGO_TYPE_ABLC);
    int algo_id = (*handle)->getAlgoId();

    // gen rk ablc result
    if (algo_id == 0) {
        RkAiqAlgoProcResAblcInt* ablc_rk = (RkAiqAlgoProcResAblcInt*)ablc_com;

        memcpy(&isp_param->blc, &ablc_rk->ablc_proc_res,
               sizeof(rk_aiq_isp_blc_t));
        isp_param->update_mask |= RKAIQ_ISP_BLC_ID;
    }

    EXIT_ANALYZER_FUNCTION();

    return ret;
}

XCamReturn
RkAiqCore::genIspAccmResult(RkAiqFullParams* params)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoProcResAccm* accm_com =
        mAlogsSharedParams.procResComb.accm_proc_res;
    SmartPtr<rk_aiq_isp_meas_params_t> isp_param =
        params->mIspMeasParams->data();

    if (!accm_com) {
        LOGD_ANALYZER("no accm result");
        return XCAM_RETURN_NO_ERROR;
    }

    // TODO: gen accm common result
    RkAiqAlgoProcResAccm* accm_rk = (RkAiqAlgoProcResAccm*)accm_com;
    isp_param->ccm = accm_rk->accm_hw_conf;
    isp_param->update_mask |= RKAIQ_ISP_CCM_ID;

    SmartPtr<RkAiqHandle>* handle = getCurAlgoTypeHandle(RK_AIQ_ALGO_TYPE_ACCM);
    int algo_id = (*handle)->getAlgoId();

    // gen rk accm result
    if (algo_id == 0) {
        RkAiqAlgoProcResAccmInt* accm_rk_int = (RkAiqAlgoProcResAccmInt*)accm_com;
    }

    EXIT_ANALYZER_FUNCTION();

    return ret;
}

XCamReturn
RkAiqCore::genIspAcgcResult(RkAiqFullParams* params)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoProcResAcgc* acgc_com =
        mAlogsSharedParams.procResComb.acgc_proc_res;
#ifdef RK_SIMULATOR_HW
    SmartPtr<rk_aiq_isp_meas_params_t> isp_param =
        params->mIspMeasParams->data();
#else
    SmartPtr<rk_aiq_isp_other_params_t> isp_param =
        params->mIspOtherParams->data();
#endif

    if (!acgc_com) {
        LOGD_ANALYZER("no acgc result");
        return XCAM_RETURN_NO_ERROR;
    }

    // TODO: gen acgc common result

    SmartPtr<RkAiqHandle>* handle = getCurAlgoTypeHandle(RK_AIQ_ALGO_TYPE_ACGC);
    int algo_id = (*handle)->getAlgoId();

    // gen rk acgc result
    if (algo_id == 0) {
        RkAiqAlgoProcResAcgcInt* acgc_rk = (RkAiqAlgoProcResAcgcInt*)acgc_com;

#ifdef RK_SIMULATOR_HW
#else
#endif
    }

    EXIT_ANALYZER_FUNCTION();

    return ret;
}

XCamReturn
RkAiqCore::genIspAdebayerResult(RkAiqFullParams* params)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoProcResAdebayer* adebayer_com =
        mAlogsSharedParams.procResComb.adebayer_proc_res;
#ifdef RK_SIMULATOR_HW
    SmartPtr<rk_aiq_isp_meas_params_t> isp_param =
        params->mIspMeasParams->data();
#else
    SmartPtr<rk_aiq_isp_other_params_t> isp_param =
        params->mIspOtherParams->data();
#endif

    if (!adebayer_com) {
        LOGD_ANALYZER("no adebayer result");
        return XCAM_RETURN_NO_ERROR;
    }

    // TODO: gen adebayer common result

    SmartPtr<RkAiqHandle>* handle = getCurAlgoTypeHandle(RK_AIQ_ALGO_TYPE_ADEBAYER);
    int algo_id = (*handle)->getAlgoId();

    // gen rk adebayer result
    if (algo_id == 0) {
        RkAiqAlgoProcResAdebayerInt* adebayer_rk = (RkAiqAlgoProcResAdebayerInt*)adebayer_com;
        memcpy(&isp_param->demosaic, &adebayer_rk->debayerRes.config, sizeof(AdebayerConfig_t));
        isp_param->update_mask |= RKAIQ_ISP_DEBAYER_ID;
    }

    EXIT_ANALYZER_FUNCTION();

    return ret;
}

XCamReturn
RkAiqCore::genIspAdpccResult(RkAiqFullParams* params)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoProcResAdpcc* adpcc_com =
        mAlogsSharedParams.procResComb.adpcc_proc_res;
    SmartPtr<rk_aiq_isp_meas_params_t> isp_param =
        params->mIspMeasParams->data();
    /* TODO: xuhf */
    SmartPtr<rk_aiq_exposure_params_wrapper_t> exp_param =
        params->mExposureParams->data();

    if (!adpcc_com) {
        LOGD_ANALYZER("no adpcc result");
        return XCAM_RETURN_NO_ERROR;
    }

    // TODO: gen adpcc common result

    SmartPtr<RkAiqHandle>* handle = getCurAlgoTypeHandle(RK_AIQ_ALGO_TYPE_ADPCC);
    int algo_id = (*handle)->getAlgoId();

    // gen rk adpcc result
    exp_param->SensorDpccInfo.enable = adpcc_com->SenDpccRes.enable;
    exp_param->SensorDpccInfo.cur_single_dpcc = adpcc_com->SenDpccRes.cur_single_dpcc;
    exp_param->SensorDpccInfo.cur_multiple_dpcc = adpcc_com->SenDpccRes.cur_multiple_dpcc;
    exp_param->SensorDpccInfo.total_dpcc = adpcc_com->SenDpccRes.total_dpcc;

    if (algo_id == 0) {
        RkAiqAlgoProcResAdpccInt* adpcc_rk = (RkAiqAlgoProcResAdpccInt*)adpcc_com;

        LOGD_ADPCC("oyyf: %s:%d output dpcc param start\n", __FUNCTION__, __LINE__);
        memcpy(&isp_param->dpcc,
               &adpcc_rk->stAdpccProcResult,
               sizeof(rk_aiq_isp_dpcc_t));
        isp_param->update_mask |= RKAIQ_ISP_DPCC_ID;
        LOGD_ADPCC("oyyf: %s:%d output dpcc param end\n", __FUNCTION__, __LINE__);
    }

    EXIT_ANALYZER_FUNCTION();

    return ret;
}

XCamReturn
RkAiqCore::genIspAfecResult(RkAiqFullParams* params)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoProcResAfec* afec_com =
        mAlogsSharedParams.procResComb.afec_proc_res;
#ifdef RK_SIMULATOR_HW
    SmartPtr<rk_aiq_ispp_meas_params_t> ispp_param =
        params->mIsppMeasParams->data();
#else
    SmartPtr<rk_aiq_ispp_other_params_t> ispp_param =
        params->mIsppOtherParams->data();
#endif

    if (!afec_com) {
        LOGD_ANALYZER("no afec result");
        return XCAM_RETURN_NO_ERROR;
    }

    // TODO: gen afec common result
    SmartPtr<RkAiqHandle>* handle = getCurAlgoTypeHandle(RK_AIQ_ALGO_TYPE_AFEC);
    int algo_id = (*handle)->getAlgoId();
    // gen rk afec result
    if (algo_id == 0) {
        RkAiqAlgoProcResAfecInt* afec_rk = (RkAiqAlgoProcResAfecInt*)afec_com;

        if (afec_rk->afec_result.update) {
            ispp_param->update_mask |= RKAIQ_ISPP_FEC_ID;
            ispp_param->fec.fec_en = afec_rk->afec_result.sw_fec_en;
            if (ispp_param->fec.fec_en) {
                ispp_param->fec.crop_en = afec_rk->afec_result.crop_en;
                ispp_param->fec.crop_width = afec_rk->afec_result.crop_width;
                ispp_param->fec.crop_height = afec_rk->afec_result.crop_height;
                ispp_param->fec.mesh_density = afec_rk->afec_result.mesh_density;
                ispp_param->fec.mesh_size = afec_rk->afec_result.mesh_size;
                ispp_param->fec.mesh_buf_fd = afec_rk->afec_result.mesh_buf_fd;
                //memcpy(ispp_param->fec.sw_mesh_xi, afec_rk->afec_result.meshxi, sizeof(ispp_param->fec.sw_mesh_xi));
                //memcpy(ispp_param->fec.sw_mesh_xf, afec_rk->afec_result.meshxf, sizeof(ispp_param->fec.sw_mesh_xf));
                //memcpy(ispp_param->fec.sw_mesh_yi, afec_rk->afec_result.meshyi, sizeof(ispp_param->fec.sw_mesh_yi));
                //memcpy(ispp_param->fec.sw_mesh_yf, afec_rk->afec_result.meshyf, sizeof(ispp_param->fec.sw_mesh_yf));
            }
            LOGD_ANALYZER("afec update %d", afec_rk->afec_result.update);
        } else
            ispp_param->update_mask &= ~RKAIQ_ISPP_FEC_ID;
    }

    EXIT_ANALYZER_FUNCTION();

    return ret;
}

XCamReturn
RkAiqCore::genIspAgammaResult(RkAiqFullParams* params)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoProcResAgamma* agamma_com =
        mAlogsSharedParams.procResComb.agamma_proc_res;
#ifdef RK_SIMULATOR_HW
    SmartPtr<rk_aiq_isp_meas_params_t> isp_param =
        params->mIspMeasParams->data();
#else
    SmartPtr<rk_aiq_isp_other_params_t> isp_param =
        params->mIspOtherParams->data();
#endif

    if (!agamma_com) {
        LOGD_ANALYZER("no agamma result");
        return XCAM_RETURN_NO_ERROR;
    }

    // TODO: gen agamma common result
    RkAiqAlgoProcResAgamma* agamma_rk = (RkAiqAlgoProcResAgamma*)agamma_com;

    isp_param->agamma = agamma_rk->agamma_proc_res;
    isp_param->update_mask |= RKAIQ_ISP_GAMMA_ID;

    SmartPtr<RkAiqHandle>* handle = getCurAlgoTypeHandle(RK_AIQ_ALGO_TYPE_AGAMMA);
    int algo_id = (*handle)->getAlgoId();

    // gen rk agamma result
    if (algo_id == 0) {
        RkAiqAlgoProcResAgammaInt* agamma_rk = (RkAiqAlgoProcResAgammaInt*)agamma_com;
    }

    EXIT_ANALYZER_FUNCTION();

    return ret;
}

XCamReturn
RkAiqCore::genIspAdegammaResult(RkAiqFullParams* params)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoProcResAdegamma* adegamma_com =
        mAlogsSharedParams.procResComb.adegamma_proc_res;
#ifdef RK_SIMULATOR_HW
    SmartPtr<rk_aiq_isp_meas_params_t> isp_param =
        params->mIspMeasParams->data();
#else
    SmartPtr<rk_aiq_isp_other_params_t> isp_param =
        params->mIspOtherParams->data();
#endif

    if (!adegamma_com) {
        LOGD_ANALYZER("no adegamma result");
        return XCAM_RETURN_NO_ERROR;
    }

    // TODO: gen agamma common result
    RkAiqAlgoProcResAdegamma* adegamma_rk = (RkAiqAlgoProcResAdegamma*)adegamma_com;

    isp_param->adegamma = adegamma_rk->adegamma_proc_res;
    isp_param->update_mask |= RKAIQ_ISP_DEGAMMA_ID;

    SmartPtr<RkAiqHandle>* handle = getCurAlgoTypeHandle(RK_AIQ_ALGO_TYPE_ADEGAMMA);
    int algo_id = (*handle)->getAlgoId();

    // gen rk adegamma result
    if (algo_id == 0) {
        RkAiqAlgoProcResAdegammaInt* adegamma_rk = (RkAiqAlgoProcResAdegammaInt*)adegamma_com;
    }

    EXIT_ANALYZER_FUNCTION();

    return ret;
}


XCamReturn
RkAiqCore::genIspAgicResult(RkAiqFullParams* params)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoProcResAgic* agic_com =
        mAlogsSharedParams.procResComb.agic_proc_res;
#ifdef RK_SIMULATOR_HW
    SmartPtr<rk_aiq_isp_meas_params_t> isp_param =
        params->mIspMeasParams->data();
#else
    SmartPtr<rk_aiq_isp_other_params_t> isp_param =
        params->mIspOtherParams->data();
#endif

    if (!agic_com) {
        LOGD_ANALYZER("no agic result");
        return XCAM_RETURN_NO_ERROR;
    }

    // TODO: gen agic common result

    SmartPtr<RkAiqHandle>* handle = getCurAlgoTypeHandle(RK_AIQ_ALGO_TYPE_AGIC);
    int algo_id = (*handle)->getAlgoId();

    // gen rk agic result
    if (algo_id == 0) {
        RkAiqAlgoProcResAgicInt* agic_rk = (RkAiqAlgoProcResAgicInt*)agic_com;
        memcpy(&isp_param->gic, &agic_rk->gicRes.config, sizeof(AgicConfig_t));
        isp_param->update_mask |= RKAIQ_ISP_GIC_ID;
    }

    EXIT_ANALYZER_FUNCTION();

    return ret;
}

XCamReturn
RkAiqCore::genIspAldchResult(RkAiqFullParams* params)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoProcResAldch* aldch_com =
        mAlogsSharedParams.procResComb.aldch_proc_res;
#ifdef RK_SIMULATOR_HW
    SmartPtr<rk_aiq_isp_meas_params_t> isp_param =
        params->mIspMeasParams->data();
#else
    SmartPtr<rk_aiq_isp_other_params_t> isp_param =
        params->mIspOtherParams->data();
#endif

    if (!aldch_com) {
        LOGD_ANALYZER("no aldch result");
        return XCAM_RETURN_NO_ERROR;
    }

    RkAiqAlgoProcResAldchInt* aldch_rk = (RkAiqAlgoProcResAldchInt*)aldch_com;
    if (aldch_rk->ldch_result.update) {
        isp_param->update_mask |= RKAIQ_ISP_LDCH_ID;
        isp_param->ldch.ldch_en = aldch_rk->ldch_result.sw_ldch_en;
        if (isp_param->ldch.ldch_en) {
            isp_param->ldch.lut_h_size = aldch_rk->ldch_result.lut_h_size;
            isp_param->ldch.lut_v_size = aldch_rk->ldch_result.lut_v_size;
            isp_param->ldch.lut_size = aldch_rk->ldch_result.lut_map_size;
            isp_param->ldch.lut_mem_fd = aldch_rk->ldch_result.lut_mapxy_buf_fd;
        }
    } else {
        isp_param->update_mask &= ~RKAIQ_ISP_LDCH_ID;
    }

    SmartPtr<RkAiqHandle>* handle = getCurAlgoTypeHandle(RK_AIQ_ALGO_TYPE_ALDCH);
    int algo_id = (*handle)->getAlgoId();

    // gen rk aldch result
    if (algo_id == 0) {
        RkAiqAlgoProcResAldchInt* aldch_rk = (RkAiqAlgoProcResAldchInt*)aldch_com;
    }

    EXIT_ANALYZER_FUNCTION();

    return ret;
}

XCamReturn
RkAiqCore::genIspAlscResult(RkAiqFullParams* params)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoProcResAlsc* alsc_com =
        mAlogsSharedParams.procResComb.alsc_proc_res;
#ifdef RK_SIMULATOR_HW
    SmartPtr<rk_aiq_isp_meas_params_t> isp_param =
        params->mIspMeasParams->data();
#else
    SmartPtr<rk_aiq_isp_meas_params_t> isp_param =
        params->mIspMeasParams->data();
#endif

    if (!alsc_com) {
        LOGD_ANALYZER("no alsc result");
        return XCAM_RETURN_NO_ERROR;
    }
    // TODO: gen alsc common result
    RkAiqAlgoProcResAlsc* alsc_rk = (RkAiqAlgoProcResAlsc*)alsc_com;
    isp_param->lsc = alsc_rk->alsc_hw_conf;
    if(mAlogsSharedParams.sns_mirror) {
        for(int i = 0; i < LSC_DATA_TBL_V_SIZE; i++) {
            for(int j = 0; j < LSC_DATA_TBL_H_SIZE; j++) {
                SWAP(unsigned short, isp_param->lsc.r_data_tbl[i * LSC_DATA_TBL_H_SIZE + j], isp_param->lsc.r_data_tbl[i * LSC_DATA_TBL_H_SIZE + (LSC_DATA_TBL_H_SIZE - 1 - j)]);
                SWAP(unsigned short, isp_param->lsc.gr_data_tbl[i * LSC_DATA_TBL_H_SIZE + j], isp_param->lsc.gr_data_tbl[i * LSC_DATA_TBL_H_SIZE + (LSC_DATA_TBL_H_SIZE - 1 - j)]);
                SWAP(unsigned short, isp_param->lsc.gb_data_tbl[i * LSC_DATA_TBL_H_SIZE + j], isp_param->lsc.gb_data_tbl[i * LSC_DATA_TBL_H_SIZE + (LSC_DATA_TBL_H_SIZE - 1 - j)]);
                SWAP(unsigned short, isp_param->lsc.b_data_tbl[i * LSC_DATA_TBL_H_SIZE + j], isp_param->lsc.b_data_tbl[i * LSC_DATA_TBL_H_SIZE + (LSC_DATA_TBL_H_SIZE - 1 - j)]);
            }
        }
    }
    if(mAlogsSharedParams.sns_flip) {
        for(int i = 0; i < LSC_DATA_TBL_V_SIZE; i++) {
            for(int j = 0; j < LSC_DATA_TBL_H_SIZE; j++) {
                SWAP(unsigned short, isp_param->lsc.r_data_tbl[i * LSC_DATA_TBL_H_SIZE + j], isp_param->lsc.r_data_tbl[(LSC_DATA_TBL_V_SIZE - 1 - i)*LSC_DATA_TBL_H_SIZE + j]);
                SWAP(unsigned short, isp_param->lsc.gr_data_tbl[i * LSC_DATA_TBL_H_SIZE + j], isp_param->lsc.gr_data_tbl[(LSC_DATA_TBL_V_SIZE - 1 - i)*LSC_DATA_TBL_H_SIZE + j]);
                SWAP(unsigned short, isp_param->lsc.gb_data_tbl[i * LSC_DATA_TBL_H_SIZE + j], isp_param->lsc.gb_data_tbl[(LSC_DATA_TBL_V_SIZE - 1 - i)*LSC_DATA_TBL_H_SIZE + j]);
                SWAP(unsigned short, isp_param->lsc.b_data_tbl[i * LSC_DATA_TBL_H_SIZE + j], isp_param->lsc.b_data_tbl[(LSC_DATA_TBL_V_SIZE - 1 - i)*LSC_DATA_TBL_H_SIZE + j]);
            }
        }
    }
    isp_param->update_mask |= RKAIQ_ISP_LSC_ID;

    SmartPtr<RkAiqHandle>* handle = getCurAlgoTypeHandle(RK_AIQ_ALGO_TYPE_ALSC);
    int algo_id = (*handle)->getAlgoId();

    // gen rk alsc result
    if (algo_id == 0) {
        RkAiqAlgoProcResAlscInt* alsc_rk_int = (RkAiqAlgoProcResAlscInt*)alsc_com;
    }

    EXIT_ANALYZER_FUNCTION();

    return ret;
}

XCamReturn
RkAiqCore::genIspAorbResult(RkAiqFullParams* params)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoProcResAorb* aorb_com =
        mAlogsSharedParams.procResComb.aorb_proc_res;
    SmartPtr<rk_aiq_ispp_meas_params_t> ispp_param =
        params->mIsppMeasParams->data();

    if (!aorb_com) {
        LOGD_ANALYZER("no aorb result");
        return XCAM_RETURN_NO_ERROR;
    }

    // TODO: gen aorb common result

    SmartPtr<RkAiqHandle>* handle = getCurAlgoTypeHandle(RK_AIQ_ALGO_TYPE_AORB);
    int algo_id = (*handle)->getAlgoId();

    // gen rk aorb result
    if (algo_id == 0) {
        RkAiqAlgoProcResAorbInt* aorb_rk = (RkAiqAlgoProcResAorbInt*)aorb_com;

        if (aorb_rk->aorb_meas.update) {
            ispp_param->update_mask |= RKAIQ_ISPP_ORB_ID;
            ispp_param->orb.orb_en = aorb_rk->aorb_meas.orb_en;
            if (ispp_param->orb.orb_en) {
                ispp_param->orb.limit_value = aorb_rk->aorb_meas.limit_value;
                ispp_param->orb.max_feature = aorb_rk->aorb_meas.max_feature;
            }
        } else {
            ispp_param->update_mask &= ~RKAIQ_ISPP_ORB_ID;
        }
    }

    EXIT_ANALYZER_FUNCTION();

    return ret;
}

XCamReturn
RkAiqCore::genIspAr2yResult(RkAiqFullParams* params)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoProcResAr2y* ar2y_com =
        mAlogsSharedParams.procResComb.ar2y_proc_res;
#ifdef RK_SIMULATOR_HW
    SmartPtr<rk_aiq_isp_meas_params_t> isp_param =
        params->mIspOtherParams->data();
#else
    SmartPtr<rk_aiq_isp_other_params_t> isp_param =
        params->mIspOtherParams->data();
#endif

    if (!ar2y_com) {
        LOGD_ANALYZER("no ar2y result");
        return XCAM_RETURN_NO_ERROR;
    }

    // TODO: gen ar2y common result

    SmartPtr<RkAiqHandle>* handle = getCurAlgoTypeHandle(RK_AIQ_ALGO_TYPE_AR2Y);
    int algo_id = (*handle)->getAlgoId();

    // gen rk ar2y result
    if (algo_id == 0) {
        RkAiqAlgoProcResAr2yInt* ar2y_rk = (RkAiqAlgoProcResAr2yInt*)ar2y_com;

#ifdef RK_SIMULATOR_HW
#else
#endif
    }

    EXIT_ANALYZER_FUNCTION();

    return ret;
}

XCamReturn
RkAiqCore::genIspAwdrResult(RkAiqFullParams* params)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqAlgoProcResAwdr* awdr_com =
        mAlogsSharedParams.procResComb.awdr_proc_res;
#ifdef RK_SIMULATOR_HW
    SmartPtr<rk_aiq_isp_meas_params_t> isp_param =
        params->mIspMeasParams->data();
#else
    SmartPtr<rk_aiq_isp_other_params_t> isp_other_param =
        params->mIspOtherParams->data();
#endif

    if (!awdr_com) {
        LOGD_ANALYZER("no awdr result");
        return XCAM_RETURN_NO_ERROR;
    }

    // TODO: gen awdr common result
    RkAiqAlgoProcResAwdr* awdr_rk = (RkAiqAlgoProcResAwdr*)awdr_com;

    isp_other_param->wdr = awdr_rk->AwdrProcRes;
    isp_other_param->update_mask |= RKAIQ_ISP_WDR_ID;

    SmartPtr<RkAiqHandle>* handle = getCurAlgoTypeHandle(RK_AIQ_ALGO_TYPE_AWDR);
    int algo_id = (*handle)->getAlgoId();

    // gen rk awdr result
    if (algo_id == 0) {
        RkAiqAlgoProcResAwdrInt* awdr_rk = (RkAiqAlgoProcResAwdrInt*)awdr_com;

        isp_other_param->wdr.enable = awdr_rk->AwdrProcRes.enable;
        isp_other_param->wdr.mode = awdr_rk->AwdrProcRes.mode;
        for(int i = 0; i < WDR_PROC_SIZE; i++)
            isp_other_param->wdr.c_wdr[i] = awdr_rk->AwdrProcRes.c_wdr[i];
    }

    EXIT_ANALYZER_FUNCTION();

    return ret;
}

XCamReturn
RkAiqCore::pushStats(SmartPtr<VideoBuffer> &buffer)
{
    ENTER_ANALYZER_FUNCTION();

    XCAM_ASSERT(buffer.ptr());
    if (buffer->get_video_info().format == V4L2_META_FMT_RK_ISP1_STAT_3A)
        mRkAiqCoreTh->push_stats(buffer);
    else
        mRkAiqCorePpTh->push_stats(buffer);

    EXIT_ANALYZER_FUNCTION();

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqCore::pushTxBuf(SmartPtr<VideoBuffer> &buffer, SmartPtr<RkAiqExpParamsProxy>& expParams)
{
    ENTER_ANALYZER_FUNCTION();

    XCAM_ASSERT(buffer.ptr());
    SmartPtr<RkAiqTxBufInfo> txBufInfo = new RkAiqTxBufInfo();
    if (expParams.ptr())
        txBufInfo->expParams = expParams;
    txBufInfo->txBuf = buffer;
    mRkAiqCoreTxBufAnalyzerTh->push_buf(txBufInfo);

    EXIT_ANALYZER_FUNCTION();

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqCore::pushEvts(SmartPtr<ispHwEvt_t> &evts)
{
    ENTER_ANALYZER_FUNCTION();

    XCAM_ASSERT(evts.ptr());

    if (evts->evt_code == V4L2_EVENT_FRAME_SYNC)
        mRkAiqCoreEvtsTh->push_evts(evts);

    EXIT_ANALYZER_FUNCTION();

    return XCAM_RETURN_NO_ERROR;
}

RkAiqHandle*
RkAiqCore::getAiqAlgoHandle(const int algo_type)
{
    SmartPtr<RkAiqHandle>* handlePtr = getCurAlgoTypeHandle(algo_type);

    return (*handlePtr).ptr();
}

SmartPtr<RkAiqHandle>*
RkAiqCore::getCurAlgoTypeHandle(int algo_type)
{
    SmartPtr<RkAiqHandle>* hdlArray[] = {
        (&mCurAeAlgoHdl),
        (&mCurAwbAlgoHdl),
        (&mCurAfAlgoHdl),
        (&mCurAblcAlgoHdl),
        (&mCurAdpccAlgoHdl),
        (&mCurAhdrAlgoHdl),
        (&mCurAnrAlgoHdl),
        (&mCurAlscAlgoHdl),
        (&mCurAgicAlgoHdl),
        (&mCurAdebayerAlgoHdl),
        (&mCurAccmAlgoHdl),
        (&mCurAgammaAlgoHdl),
        (&mCurAwdrAlgoHdl),
        (&mCurAdhazAlgoHdl),
        (&mCurA3dlutAlgoHdl),
        (&mCurAldchAlgoHdl),
        (&mCurAr2yAlgoHdl),
        (&mCurAcpAlgoHdl),
        (&mCurAieAlgoHdl),
        (&mCurAsharpAlgoHdl),
        (&mCurAorbAlgoHdl),
        (&mCurAfecAlgoHdl),
        (&mCurAcgcAlgoHdl),
        (&mCurAsdAlgoHdl),
        (&mCurAdegammaAlgoHdl),
        (&mCurAfdAlgoHdl),
    };

    return hdlArray[algo_type];
}

std::map<int, SmartPtr<RkAiqHandle>>*
                                  RkAiqCore::getAlgoTypeHandleMap(int algo_type)
{
    std::map<int, SmartPtr<RkAiqHandle>>* algo_map_array[] = {
        &mAeAlgoHandleMap,
        &mAwbAlgoHandleMap,
        &mAfAlgoHandleMap,
        &mAblcAlgoHandleMap,
        &mAdpccAlgoHandleMap,
        &mAhdrAlgoHandleMap,
        &mAnrAlgoHandleMap,
        &mAlscAlgoHandleMap,
        &mAgicAlgoHandleMap,
        &mAdebayerAlgoHandleMap,
        &mAccmAlgoHandleMap,
        &mAgammaAlgoHandleMap,
        &mAwdrAlgoHandleMap,
        &mAdhazAlgoHandleMap,
        &mA3dlutAlgoHandleMap,
        &mAldchAlgoHandleMap,
        &mAr2yAlgoHandleMap,
        &mAcpAlgoHandleMap,
        &mAieAlgoHandleMap,
        &mAsharpAlgoHandleMap,
        &mAorbAlgoHandleMap,
        &mAfecAlgoHandleMap,
        &mAcgcAlgoHandleMap,
        &mAsdAlgoHandleMap,
        &mAdegammaAlgoHandleMap,
        &mAfdAlgoHandleMap,
    };

    return algo_map_array[algo_type];
}

void
RkAiqCore::addDefaultAlgos()
{
#define ADD_ALGO_HANDLE(lc, BC) \
    mAlogsSharedParams.ctxCfigs[RK_AIQ_ALGO_TYPE_##BC].calib = \
        const_cast<CamCalibDbContext_t*>(mAlogsSharedParams.calib); \
    m##lc##AlgoHandleMap[0] = \
        new RkAiq##lc##HandleInt(g_default_3a_des[RK_AIQ_ALGO_TYPE_##BC], this); \

    ADD_ALGO_HANDLE(Ae, AE);
    ADD_ALGO_HANDLE(Awb, AWB);
    ADD_ALGO_HANDLE(Af, AF);
    ADD_ALGO_HANDLE(Ahdr, AHDR);
    ADD_ALGO_HANDLE(Anr, ANR);
    ADD_ALGO_HANDLE(Adhaz, ADHAZ);
    ADD_ALGO_HANDLE(Asd, ASD);
    ADD_ALGO_HANDLE(Acp, ACP);
    ADD_ALGO_HANDLE(Asharp, ASHARP);
    ADD_ALGO_HANDLE(A3dlut, A3DLUT);
    ADD_ALGO_HANDLE(Ablc, ABLC);
    ADD_ALGO_HANDLE(Accm, ACCM);
    ADD_ALGO_HANDLE(Acgc, ACGC);
    ADD_ALGO_HANDLE(Adebayer, ADEBAYER);
    ADD_ALGO_HANDLE(Adpcc, ADPCC);
    ADD_ALGO_HANDLE(Afec, AFEC);
    ADD_ALGO_HANDLE(Agamma, AGAMMA);
    ADD_ALGO_HANDLE(Adegamma, ADEGAMMA);
    ADD_ALGO_HANDLE(Agic, AGIC);
    ADD_ALGO_HANDLE(Aie, AIE);
    ADD_ALGO_HANDLE(Aldch, ALDCH);
    ADD_ALGO_HANDLE(Alsc, ALSC);
    ADD_ALGO_HANDLE(Aorb, AORB);
    ADD_ALGO_HANDLE(Ar2y, AR2Y);
    ADD_ALGO_HANDLE(Awdr, AWDR);
    ADD_ALGO_HANDLE(Afd, AFD);

#ifdef RK_SIMULATOR_HW
    for (int i = 0; i < RK_AIQ_ALGO_TYPE_MAX; i++)
        enableAlgo(i, 0, true);
#else
    /*
     * enable the modules that has been verified to work properly on the board
     * TODO: enable all modules after validation in isp
     */
#if 0
    for (int i = 0; i < RK_AIQ_ALGO_TYPE_MAX; i++)
        enableAlgo(i, 0, true);
#else
    enableAlgo(RK_AIQ_ALGO_TYPE_AE, 0, true);
    enableAlgo(RK_AIQ_ALGO_TYPE_AHDR, 0, true);
    enableAlgo(RK_AIQ_ALGO_TYPE_AWB, 0, true);
    enableAlgo(RK_AIQ_ALGO_TYPE_AGAMMA, 0, true);
    enableAlgo(RK_AIQ_ALGO_TYPE_ADEGAMMA, 0, true);
    enableAlgo(RK_AIQ_ALGO_TYPE_ABLC, 0, true);
    enableAlgo(RK_AIQ_ALGO_TYPE_ACCM, 0, true);
    enableAlgo(RK_AIQ_ALGO_TYPE_ALSC, 0, true);
    enableAlgo(RK_AIQ_ALGO_TYPE_ADPCC, 0, true);
    enableAlgo(RK_AIQ_ALGO_TYPE_ANR, 0, true);
    enableAlgo(RK_AIQ_ALGO_TYPE_AF, 0, true);
    enableAlgo(RK_AIQ_ALGO_TYPE_ASHARP, 0, true);
    enableAlgo(RK_AIQ_ALGO_TYPE_ADHAZ, 0, true);
    enableAlgo(RK_AIQ_ALGO_TYPE_A3DLUT, 0, true);
    enableAlgo(RK_AIQ_ALGO_TYPE_ALDCH, 0, true);
    enableAlgo(RK_AIQ_ALGO_TYPE_AFEC, 0, true);
    enableAlgo(RK_AIQ_ALGO_TYPE_AGIC, 0, true);
    enableAlgo(RK_AIQ_ALGO_TYPE_ADEBAYER, 0, true);
    enableAlgo(RK_AIQ_ALGO_TYPE_AORB, 0, true);
    enableAlgo(RK_AIQ_ALGO_TYPE_ASD, 0, true);
    enableAlgo(RK_AIQ_ALGO_TYPE_AIE, 0, true);
    enableAlgo(RK_AIQ_ALGO_TYPE_ACP, 0, true);
    enableAlgo(RK_AIQ_ALGO_TYPE_AWDR, 0, true);
    enableAlgo(RK_AIQ_ALGO_TYPE_AFD, 0, true);
#endif
#endif
}

SmartPtr<RkAiqHandle>
RkAiqCore::newAlgoHandle(RkAiqAlgoDesComm* algo)
{
#define NEW_ALGO_HANDLE(lc, BC) \
    if (algo->type == RK_AIQ_ALGO_TYPE_##BC) \
        return new RkAiq##lc##Handle(algo, this);

    NEW_ALGO_HANDLE(Ae, AE);
    NEW_ALGO_HANDLE(Awb, AWB);
    NEW_ALGO_HANDLE(Af, AF);
    NEW_ALGO_HANDLE(Ahdr, AHDR);
    NEW_ALGO_HANDLE(Anr, ANR);
    NEW_ALGO_HANDLE(Adhaz, ADHAZ);
    NEW_ALGO_HANDLE(Asd, ASD);
    NEW_ALGO_HANDLE(Acp, ACP);
    NEW_ALGO_HANDLE(Asharp, ASHARP);
    NEW_ALGO_HANDLE(A3dlut, A3DLUT);
    NEW_ALGO_HANDLE(Ablc, ABLC);
    NEW_ALGO_HANDLE(Accm, ACCM);
    NEW_ALGO_HANDLE(Acgc, ACGC);
    NEW_ALGO_HANDLE(Adebayer, ADEBAYER);
    NEW_ALGO_HANDLE(Adpcc, ADPCC);
    NEW_ALGO_HANDLE(Afec, AFEC);
    NEW_ALGO_HANDLE(Agamma, AGAMMA);
    NEW_ALGO_HANDLE(Adegamma, ADEGAMMA);
    NEW_ALGO_HANDLE(Agic, AGIC);
    NEW_ALGO_HANDLE(Aie, AIE);
    NEW_ALGO_HANDLE(Aldch, ALDCH);
    NEW_ALGO_HANDLE(Alsc, ALSC);
    NEW_ALGO_HANDLE(Aorb, AORB);
    NEW_ALGO_HANDLE(Ar2y, AR2Y);
    NEW_ALGO_HANDLE(Awdr, AWDR);

    return NULL;
}

XCamReturn
RkAiqCore::addAlgo(RkAiqAlgoDesComm& algo)
{
    ENTER_ANALYZER_FUNCTION();

    std::map<int, SmartPtr<RkAiqHandle>>* algo_map = getAlgoTypeHandleMap(algo.type);

    // TODO, check if exist befor insert ?
    std::map<int, SmartPtr<RkAiqHandle>>::reverse_iterator rit = algo_map->rbegin();

    algo.id = rit->first + 1;

    // add to map
    SmartPtr<RkAiqHandle> new_hdl = newAlgoHandle(&algo);
    new_hdl->setEnable(false);
    (*algo_map)[algo.id] = new_hdl;

    EXIT_ANALYZER_FUNCTION();

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqCore::enableAlgo(int algoType, int id, bool enable)
{
    ENTER_ANALYZER_FUNCTION();
    // get current algotype handle, get id
    SmartPtr<RkAiqHandle>* cur_algo_hdl = getCurAlgoTypeHandle(algoType);
    std::map<int, SmartPtr<RkAiqHandle>>* algo_map = getAlgoTypeHandleMap(algoType);
    std::map<int, SmartPtr<RkAiqHandle>>::iterator it = algo_map->find(id);
    bool switch_algo = false;

    if (it == algo_map->end()) {
        LOGE_ANALYZER("can't find type id <%d, %d> algo", algoType, id);
        return XCAM_RETURN_ERROR_FAILED;
    }

    if (!cur_algo_hdl) {
        LOGE_ANALYZER("can't find current type %d algo", algoType);
        return XCAM_RETURN_ERROR_FAILED;
    }

    it->second->setEnable(enable);
    /* WARNING:
     * Be careful when use SmartPtr<RkAiqxxxHandle> = SmartPtr<RkAiqHandle>
     * if RkAiqxxxHandle is derived from multiple RkAiqHandle,
     * the ptr of RkAiqxxxHandle and RkAiqHandle IS NOT the same
     * (RkAiqHandle ptr = RkAiqxxxHandle ptr + offset), but seams like
     * SmartPtr do not deal with this correctly.
     */
    if (enable) {
        SmartLock locker (mApiMutex);
        while (mSafeEnableAlgo != true)
            mApiMutexCond.wait(mApiMutex);
        // replace current algo
        if (id == 0 ||
                mAlogsSharedParams.mCustomAlgoRunningMode == CUSTOM_ALGO_RUNNING_MODE_SINGLE) {
            if ((*cur_algo_hdl).ptr() &&
                    (*cur_algo_hdl).ptr() != it->second.ptr()) {
                (*cur_algo_hdl)->setEnable(false);
                switch_algo = true;
            }
            *cur_algo_hdl = it->second;
            if (switch_algo && (mState >= RK_AIQ_CORE_STATE_PREPARED))
                (*cur_algo_hdl)->prepare();
        } else {
            if (algoType == RK_AIQ_ALGO_TYPE_AE)
                mCurCustomAeAlgoHdl = it->second;
            else if (algoType == RK_AIQ_ALGO_TYPE_AWB)
                mCurCustomAwbAlgoHdl = it->second;
            else if (algoType == RK_AIQ_ALGO_TYPE_AF)
                mCurCustomAfAlgoHdl = it->second;
            else
                LOGE_ANALYZER("only support ae/awb/af custom algos !");
            if (mState >= RK_AIQ_CORE_STATE_PREPARED)
                it->second->prepare();
        }
    }

    EXIT_ANALYZER_FUNCTION();

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqCore::rmAlgo(int algoType, int id)
{
    ENTER_ANALYZER_FUNCTION();

    // can't remove default algos
    if (id == 0)
        return XCAM_RETURN_NO_ERROR;

    SmartPtr<RkAiqHandle>* cur_algo_hdl = getCurAlgoTypeHandle(algoType);
    std::map<int, SmartPtr<RkAiqHandle>>* algo_map = getAlgoTypeHandleMap(algoType);
    std::map<int, SmartPtr<RkAiqHandle>>::iterator it = algo_map->find(id);

    if (it == algo_map->end()) {
        LOGE_ANALYZER("can't find type id <%d, %d> algo", algoType, id);
        return XCAM_RETURN_ERROR_FAILED;
    }

    if (!cur_algo_hdl) {
        LOGE_ANALYZER("can't find current type %d algo", algoType);
        return XCAM_RETURN_ERROR_FAILED;
    }

    // if it's the current algo handle, clear it
    // if not single mode, cur_algo_hdr is always the rk algo
    if (mAlogsSharedParams.mCustomAlgoRunningMode == CUSTOM_ALGO_RUNNING_MODE_SINGLE) {
        if ((*cur_algo_hdl).ptr() == it->second.ptr())
            (*cur_algo_hdl).release();
    } else {
        if (algoType == RK_AIQ_ALGO_TYPE_AE)
            mCurCustomAeAlgoHdl.release();
        else if (algoType == RK_AIQ_ALGO_TYPE_AWB)
            mCurCustomAwbAlgoHdl.release();
        else if (algoType == RK_AIQ_ALGO_TYPE_AF)
            mCurCustomAfAlgoHdl.release();
        else
            LOGE_ANALYZER("only support ae/awb/af custom algos !");
    }
    algo_map->erase(it);

    EXIT_ANALYZER_FUNCTION();

    return XCAM_RETURN_NO_ERROR;
}

bool
RkAiqCore::getAxlibStatus(int algoType, int id)
{
    std::map<int, SmartPtr<RkAiqHandle>>* algo_map = getAlgoTypeHandleMap(algoType);
    std::map<int, SmartPtr<RkAiqHandle>>::iterator it = algo_map->find(id);

    if (it == algo_map->end()) {
        LOGE_ANALYZER("can't find type id <%d, %d> algo", algoType, id);
        return false;
    }

    LOGD_ANALYZER("algo type id <%d,%d> status %s", algoType, id,
                  it->second->getEnable() ? "enable" : "disable");

    return it->second->getEnable();
}

RkAiqAlgoContext*
RkAiqCore::getEnabledAxlibCtx(const int algo_type)
{
    if (algo_type <= RK_AIQ_ALGO_TYPE_NONE ||
            algo_type >= RK_AIQ_ALGO_TYPE_MAX)
        return NULL;

    if (mAlogsSharedParams.mCustomAlgoRunningMode == CUSTOM_ALGO_RUNNING_MODE_SINGLE) {
        SmartPtr<RkAiqHandle>* algo_handle = getCurAlgoTypeHandle(algo_type);

        if ((*algo_handle).ptr())
            return (*algo_handle)->getAlgoCtx();
        else
            return NULL;
    } else {
        std::map<int, SmartPtr<RkAiqHandle>>* algo_map = getAlgoTypeHandleMap(algo_type);
        std::map<int, SmartPtr<RkAiqHandle>>::reverse_iterator rit = algo_map->rbegin();
        if (rit !=  algo_map->rend() && rit->second->getEnable())
            return rit->second->getAlgoCtx();
        else
            return NULL;
    }
}

RkAiqAlgoContext*
RkAiqCore::getAxlibCtx(const int algo_type, const int lib_id)
{
    if (algo_type <= RK_AIQ_ALGO_TYPE_NONE ||
            algo_type >= RK_AIQ_ALGO_TYPE_MAX)
        return NULL;

    std::map<int, SmartPtr<RkAiqHandle>>* algo_map = getAlgoTypeHandleMap(algo_type);

    std::map<int, SmartPtr<RkAiqHandle>>::iterator it = algo_map->find(lib_id);

    if (it != algo_map->end()) {
        return it->second->getAlgoCtx();
    }

    EXIT_ANALYZER_FUNCTION();

    return NULL;

}

void
RkAiqCore::cacheIspStatsToList()
{
    SmartLock locker (ispStatsListMutex);
    SmartPtr<RkAiqStatsProxy> stats = NULL;
    if (mAiqStatsPool->has_free_items()) {
        stats = mAiqStatsPool->get_item();
    } else {
        if(mAiqStatsCachedList.empty()) {
            LOGW_ANALYZER("no free or cached stats, user may hold all stats buf !");
            return ;
        }
        stats = mAiqStatsCachedList.front();
        mAiqStatsCachedList.pop_front();
    }

    stats->data()->aec_stats = mAlogsSharedParams.ispStats.aec_stats;
    stats->data()->awb_stats_v200 = mAlogsSharedParams.ispStats.awb_stats;
    stats->data()->af_stats = mAlogsSharedParams.ispStats.af_stats;
    stats->data()->frame_id = mAlogsSharedParams.frameId;

    mAiqStatsCachedList.push_back(stats);
    mIspStatsCond.broadcast ();
}

XCamReturn RkAiqCore::get3AStatsFromCachedList(rk_aiq_isp_stats_t **stats, int timeout_ms)
{
    SmartLock locker (ispStatsListMutex);
    int code = 0;
    while (mState != RK_AIQ_CORE_STATE_STOPED &&
            mAiqStatsCachedList.empty() &&
            code == 0) {
        if (timeout_ms < 0)
            code = mIspStatsCond.wait(ispStatsListMutex);
        else
            code = mIspStatsCond.timedwait(ispStatsListMutex, timeout_ms * 1000);
    }

    if (mState == RK_AIQ_CORE_STATE_STOPED) {
        *stats = NULL;
        return XCAM_RETURN_NO_ERROR;
    }

    if (mAiqStatsCachedList.empty()) {
        if (code == ETIMEDOUT) {
            *stats = NULL;
            return XCAM_RETURN_ERROR_TIMEOUT;
        } else {
            *stats = NULL;
            return XCAM_RETURN_ERROR_FAILED;
        }
    }
    SmartPtr<RkAiqStatsProxy> stats_proxy = mAiqStatsCachedList.front();
    mAiqStatsCachedList.pop_front();
    *stats = stats_proxy->data().ptr();
    mAiqStatsOutMap[*stats] = stats_proxy;
    stats_proxy.release();

    return XCAM_RETURN_NO_ERROR;
}

void RkAiqCore::release3AStatsRef(rk_aiq_isp_stats_t *stats)
{
    SmartLock locker (ispStatsListMutex);

    std::map<rk_aiq_isp_stats_t*, SmartPtr<RkAiqStatsProxy>>::iterator it;
    it = mAiqStatsOutMap.find(stats);
    if (it != mAiqStatsOutMap.end()) {
        mAiqStatsOutMap.erase(it);
    }
}

XCamReturn RkAiqCore::get3AStatsFromCachedList(rk_aiq_isp_stats_t &stats)
{
    SmartLock locker (ispStatsListMutex);
    if(!mAiqStatsCachedList.empty()) {
        SmartPtr<RkAiqStatsProxy> stats_proxy = mAiqStatsCachedList.front();
        mAiqStatsCachedList.pop_front();
        stats = *(stats_proxy->data().ptr());
        stats_proxy.release();
        return XCAM_RETURN_NO_ERROR;
    } else {
        return XCAM_RETURN_ERROR_FAILED;
    }
}

XCamReturn
RkAiqCore::convertIspstatsToAlgo(const SmartPtr<VideoBuffer> &buffer)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
#ifndef RK_SIMULATOR_HW
    const SmartPtr<Isp20StatsBuffer> buf =
        buffer.dynamic_cast_ptr<Isp20StatsBuffer>();
    struct rkisp_isp2x_stat_buffer *stats;

    SmartPtr<RkAiqExpParamsProxy> expParams = nullptr;
    SmartPtr<RkAiqIspMeasParamsProxy> ispParams = nullptr;
    SmartPtr<RkAiqAfInfoProxy> afParams = buf->get_af_params();
    SmartPtr<RkAiqIrisParamsProxy> irisParams = buf->get_iris_params();


    stats = (struct rkisp_isp2x_stat_buffer *)(buf->get_v4l2_userptr());
    if(stats == NULL) {
        LOGE("fail to get stats ,ignore\n");
        return XCAM_RETURN_BYPASS;
    }
    LOGD_ANALYZER("stats frame_id(%d), meas_type; 0x%x, buf sequence(%d)",
                  stats->frame_id, stats->meas_type, buf->get_sequence());

#if 1
    if (buf->getEffectiveExpParams(stats->frame_id, expParams) < 0)
        LOGE("fail to get expParams");
    if (buf->getEffectiveIspParams(stats->frame_id, ispParams) < 0)
        LOGE("fail to get ispParams");
#endif

    if(ispParams.ptr() == NULL) {
        LOGE("fail to get ispParams ,ignore\n");
        return XCAM_RETURN_BYPASS;
    }

    mAlogsSharedParams.frameId = stats->frame_id;
    mAlogsSharedParams.ispStats.frame_id = stats->frame_id;
    if(expParams.ptr() != NULL)
        mAlogsSharedParams.curExp = expParams->data()->aecExpInfo;

    //awb2.0

    mAlogsSharedParams.ispStats.awb_cfg_effect_v200 = ispParams->data()->awb_cfg_v200;
    mAlogsSharedParams.ispStats.awb_cfg_effect_valid = true;

    for(int i = 0; i < mAlogsSharedParams.ispStats.awb_cfg_effect_v200.lightNum; i++) {
        mAlogsSharedParams.ispStats.awb_stats.light[i].xYType[RK_AIQ_AWB_XY_TYPE_NORMAL_V200].Rvalue =
            stats->params.rawawb.ro_rawawb_sum_r_nor[i];
        mAlogsSharedParams.ispStats.awb_stats.light[i].xYType[RK_AIQ_AWB_XY_TYPE_NORMAL_V200].Gvalue =
            stats->params.rawawb.ro_rawawb_sum_g_nor[i];
        mAlogsSharedParams.ispStats.awb_stats.light[i].xYType[RK_AIQ_AWB_XY_TYPE_NORMAL_V200].Bvalue =
            stats->params.rawawb.ro_rawawb_sum_b_nor[i];
        mAlogsSharedParams.ispStats.awb_stats.light[i].xYType[RK_AIQ_AWB_XY_TYPE_NORMAL_V200].WpNo =
            stats->params.rawawb.ro_rawawb_wp_num_nor[i];
        mAlogsSharedParams.ispStats.awb_stats.light[i].xYType[RK_AIQ_AWB_XY_TYPE_BIG_V200].Rvalue =
            stats->params.rawawb.ro_rawawb_sum_r_big[i];
        mAlogsSharedParams.ispStats.awb_stats.light[i].xYType[RK_AIQ_AWB_XY_TYPE_BIG_V200].Gvalue =
            stats->params.rawawb.ro_rawawb_sum_g_big[i];
        mAlogsSharedParams.ispStats.awb_stats.light[i].xYType[RK_AIQ_AWB_XY_TYPE_BIG_V200].Bvalue =
            stats->params.rawawb.ro_rawawb_sum_b_big[i];
        mAlogsSharedParams.ispStats.awb_stats.light[i].xYType[RK_AIQ_AWB_XY_TYPE_BIG_V200].WpNo =
            stats->params.rawawb.ro_rawawb_wp_num_big[i];
        mAlogsSharedParams.ispStats.awb_stats.light[i].xYType[RK_AIQ_AWB_XY_TYPE_SMALL_V200].Rvalue =
            stats->params.rawawb.ro_rawawb_sum_r_sma[i];
        mAlogsSharedParams.ispStats.awb_stats.light[i].xYType[RK_AIQ_AWB_XY_TYPE_SMALL_V200].Gvalue =
            stats->params.rawawb.ro_rawawb_sum_g_sma[i];
        mAlogsSharedParams.ispStats.awb_stats.light[i].xYType[RK_AIQ_AWB_XY_TYPE_SMALL_V200].Bvalue =
            stats->params.rawawb.ro_rawawb_sum_b_sma[i];
        mAlogsSharedParams.ispStats.awb_stats.light[i].xYType[RK_AIQ_AWB_XY_TYPE_SMALL_V200].WpNo =
            stats->params.rawawb.ro_rawawb_wp_num_sma[i];
    }
    for(int i = 0; i < mAlogsSharedParams.ispStats.awb_cfg_effect_v200.lightNum; i++) {
        mAlogsSharedParams.ispStats.awb_stats.multiwindowLightResult[i].xYType[RK_AIQ_AWB_XY_TYPE_NORMAL_V200].Rvalue =
            stats->params.rawawb.ro_sum_r_nor_multiwindow[i];
        mAlogsSharedParams.ispStats.awb_stats.multiwindowLightResult[i].xYType[RK_AIQ_AWB_XY_TYPE_NORMAL_V200].Gvalue =
            stats->params.rawawb.ro_sum_g_nor_multiwindow[i];
        mAlogsSharedParams.ispStats.awb_stats.multiwindowLightResult[i].xYType[RK_AIQ_AWB_XY_TYPE_NORMAL_V200].Bvalue =
            stats->params.rawawb.ro_sum_b_nor_multiwindow[i];
        mAlogsSharedParams.ispStats.awb_stats.multiwindowLightResult[i].xYType[RK_AIQ_AWB_XY_TYPE_NORMAL_V200].WpNo =
            stats->params.rawawb.ro_wp_nm_nor_multiwindow[i];
        mAlogsSharedParams.ispStats.awb_stats.multiwindowLightResult[i].xYType[RK_AIQ_AWB_XY_TYPE_BIG_V200].Rvalue =
            stats->params.rawawb.ro_sum_r_big_multiwindow[i];
        mAlogsSharedParams.ispStats.awb_stats.multiwindowLightResult[i].xYType[RK_AIQ_AWB_XY_TYPE_BIG_V200].Gvalue =
            stats->params.rawawb.ro_sum_g_big_multiwindow[i];
        mAlogsSharedParams.ispStats.awb_stats.multiwindowLightResult[i].xYType[RK_AIQ_AWB_XY_TYPE_BIG_V200].Bvalue =
            stats->params.rawawb.ro_sum_b_big_multiwindow[i];
        mAlogsSharedParams.ispStats.awb_stats.multiwindowLightResult[i].xYType[RK_AIQ_AWB_XY_TYPE_BIG_V200].WpNo =
            stats->params.rawawb.ro_wp_nm_big_multiwindow[i];
        mAlogsSharedParams.ispStats.awb_stats.multiwindowLightResult[i].xYType[RK_AIQ_AWB_XY_TYPE_SMALL_V200].Rvalue =
            stats->params.rawawb.ro_sum_r_sma_multiwindow[i];
        mAlogsSharedParams.ispStats.awb_stats.multiwindowLightResult[i].xYType[RK_AIQ_AWB_XY_TYPE_SMALL_V200].Gvalue =
            stats->params.rawawb.ro_sum_g_sma_multiwindow[i];
        mAlogsSharedParams.ispStats.awb_stats.multiwindowLightResult[i].xYType[RK_AIQ_AWB_XY_TYPE_SMALL_V200].Bvalue =
            stats->params.rawawb.ro_sum_b_sma_multiwindow[i];
        mAlogsSharedParams.ispStats.awb_stats.multiwindowLightResult[i].xYType[RK_AIQ_AWB_XY_TYPE_SMALL_V200].WpNo =
            stats->params.rawawb.ro_wp_nm_sma_multiwindow[i];
    }
    for(int i = 0; i < RK_AIQ_AWB_STAT_WP_RANGE_NUM_V200; i++) {
        mAlogsSharedParams.ispStats.awb_stats.excWpRangeResult[i].Rvalue = stats->params.rawawb.ro_sum_r_exc[i];
        mAlogsSharedParams.ispStats.awb_stats.excWpRangeResult[i].Gvalue = stats->params.rawawb.ro_sum_g_exc[i];
        mAlogsSharedParams.ispStats.awb_stats.excWpRangeResult[i].Bvalue = stats->params.rawawb.ro_sum_b_exc[i];
        mAlogsSharedParams.ispStats.awb_stats.excWpRangeResult[i].WpNo =    stats->params.rawawb.ro_wp_nm_exc[i];

    }
    for(int i = 0; i < RK_AIQ_AWB_GRID_NUM_TOTAL; i++) {
        mAlogsSharedParams.ispStats.awb_stats.blockResult[i].Rvalue = stats->params.rawawb.ramdata[i].r;
        mAlogsSharedParams.ispStats.awb_stats.blockResult[i].Gvalue = stats->params.rawawb.ramdata[i].g;
        mAlogsSharedParams.ispStats.awb_stats.blockResult[i].Bvalue = stats->params.rawawb.ramdata[i].b;
        mAlogsSharedParams.ispStats.awb_stats.blockResult[i].isWP[2] = stats->params.rawawb.ramdata[i].wp & 0x1;
        mAlogsSharedParams.ispStats.awb_stats.blockResult[i].isWP[1] = (stats->params.rawawb.ramdata[i].wp >> 1) & 0x1;
        mAlogsSharedParams.ispStats.awb_stats.blockResult[i].isWP[0] = (stats->params.rawawb.ramdata[i].wp >> 2) & 0x1;
    }
    //mAlogsSharedParams.ispStats.awb_stats_valid = ISP2X_STAT_RAWAWB(stats->meas_type)? true:false;
    mAlogsSharedParams.ispStats.awb_stats_valid = stats->meas_type >> 5 & 1;

    //ahdr
    mAlogsSharedParams.ispStats.ahdr_stats_valid = stats->meas_type >> 16 & 1;
    mAlogsSharedParams.ispStats.ahdr_stats.tmo_stats.ro_hdrtmo_lglow = stats->params.hdrtmo.lglow;
    mAlogsSharedParams.ispStats.ahdr_stats.tmo_stats.ro_hdrtmo_lgmin = stats->params.hdrtmo.lgmin;
    mAlogsSharedParams.ispStats.ahdr_stats.tmo_stats.ro_hdrtmo_lghigh = stats->params.hdrtmo.lghigh;
    mAlogsSharedParams.ispStats.ahdr_stats.tmo_stats.ro_hdrtmo_lgmax = stats->params.hdrtmo.lgmax;
    mAlogsSharedParams.ispStats.ahdr_stats.tmo_stats.ro_hdrtmo_weightkey = stats->params.hdrtmo.weightkey;
    mAlogsSharedParams.ispStats.ahdr_stats.tmo_stats.ro_hdrtmo_lgmean = stats->params.hdrtmo.lgmean;
    mAlogsSharedParams.ispStats.ahdr_stats.tmo_stats.ro_hdrtmo_lgrange0 = stats->params.hdrtmo.lgrange0;
    mAlogsSharedParams.ispStats.ahdr_stats.tmo_stats.ro_hdrtmo_lgrange1 = stats->params.hdrtmo.lgrange1;
    mAlogsSharedParams.ispStats.ahdr_stats.tmo_stats.ro_hdrtmo_palpha = stats->params.hdrtmo.palpha;
    mAlogsSharedParams.ispStats.ahdr_stats.tmo_stats.ro_hdrtmo_lgavgmax = stats->params.hdrtmo.lgavgmax;
    mAlogsSharedParams.ispStats.ahdr_stats.tmo_stats.ro_hdrtmo_linecnt = stats->params.hdrtmo.linecnt;
    for(int i = 0; i < 32; i++)
        mAlogsSharedParams.ispStats.ahdr_stats.tmo_stats.ro_array_min_max[i] = stats->params.hdrtmo.min_max[i];

    //ae
    mAlogsSharedParams.ispStats.aec_stats_valid = (stats->meas_type >> 11) & (0x01) ? true : false;

    /*rawae stats*/
    uint8_t AeSwapMode, AeSelMode;
    AeSwapMode = ispParams->data()->aec_meas.rawae0.rawae_sel;
    AeSelMode = ispParams->data()->aec_meas.rawae3.rawae_sel;

    switch(AeSwapMode) {
    case AEC_RAWSWAP_MODE_S_LITE:
        for(int i = 0; i < ISP2X_RAWAEBIG_MEAN_NUM; i++) {
            if(i < ISP2X_RAWAELITE_MEAN_NUM) {
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[0].rawae_lite.channelr_xy[i] = stats->params.rawae0.data[i].channelr_xy;
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[0].rawae_lite.channelg_xy[i] = stats->params.rawae0.data[i].channelg_xy;
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[0].rawae_lite.channelb_xy[i] = stats->params.rawae0.data[i].channelb_xy;
            }
            mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[1].rawae_big.channelr_xy[i] = stats->params.rawae1.data[i].channelr_xy;
            mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[1].rawae_big.channelg_xy[i] = stats->params.rawae1.data[i].channelg_xy;
            mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[1].rawae_big.channelb_xy[i] = stats->params.rawae1.data[i].channelb_xy;
            mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[2].rawae_big.channelr_xy[i] = stats->params.rawae2.data[i].channelr_xy;
            mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[2].rawae_big.channelg_xy[i] = stats->params.rawae2.data[i].channelg_xy;
            mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[2].rawae_big.channelb_xy[i] = stats->params.rawae2.data[i].channelb_xy;
            if(i < ISP2X_RAWAEBIG_SUBWIN_NUM) {
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[1].rawae_big.wndx_sumr[i] = stats->params.rawae1.sumr[i];
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[1].rawae_big.wndx_sumg[i] = stats->params.rawae1.sumg[i];
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[1].rawae_big.wndx_sumb[i] = stats->params.rawae1.sumb[i];
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[2].rawae_big.wndx_sumr[i] = stats->params.rawae2.sumr[i];
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[2].rawae_big.wndx_sumg[i] = stats->params.rawae2.sumg[i];
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[2].rawae_big.wndx_sumb[i] = stats->params.rawae2.sumb[i];
            }
        }
        memcpy(mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[0].rawhist_lite.bins, stats->params.rawhist0.hist_bin, ISP2X_HIST_BIN_N_MAX * sizeof(u32));
        memcpy(mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[1].rawhist_big.bins, stats->params.rawhist1.hist_bin, ISP2X_HIST_BIN_N_MAX * sizeof(u32));
        memcpy(mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[2].rawhist_big.bins, stats->params.rawhist2.hist_bin, ISP2X_HIST_BIN_N_MAX * sizeof(u32));
        break;

    case AEC_RAWSWAP_MODE_M_LITE:
        for(int i = 0; i < ISP2X_RAWAEBIG_MEAN_NUM; i++) {
            if(i < ISP2X_RAWAELITE_MEAN_NUM) {
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[1].rawae_lite.channelr_xy[i] = stats->params.rawae0.data[i].channelr_xy;
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[1].rawae_lite.channelg_xy[i] = stats->params.rawae0.data[i].channelg_xy;
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[1].rawae_lite.channelb_xy[i] = stats->params.rawae0.data[i].channelb_xy;
            }
            mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[0].rawae_big.channelr_xy[i] = stats->params.rawae1.data[i].channelr_xy;
            mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[0].rawae_big.channelg_xy[i] = stats->params.rawae1.data[i].channelg_xy;
            mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[0].rawae_big.channelb_xy[i] = stats->params.rawae1.data[i].channelb_xy;
            mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[2].rawae_big.channelr_xy[i] = stats->params.rawae2.data[i].channelr_xy;
            mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[2].rawae_big.channelg_xy[i] = stats->params.rawae2.data[i].channelg_xy;
            mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[2].rawae_big.channelb_xy[i] = stats->params.rawae2.data[i].channelb_xy;

            if(i < ISP2X_RAWAEBIG_SUBWIN_NUM) {
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[0].rawae_big.wndx_sumr[i] = stats->params.rawae1.sumr[i];
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[0].rawae_big.wndx_sumg[i] = stats->params.rawae1.sumg[i];
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[0].rawae_big.wndx_sumb[i] = stats->params.rawae1.sumb[i];
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[2].rawae_big.wndx_sumr[i] = stats->params.rawae2.sumr[i];
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[2].rawae_big.wndx_sumg[i] = stats->params.rawae2.sumg[i];
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[2].rawae_big.wndx_sumb[i] = stats->params.rawae2.sumb[i];
            }
        }
        memcpy(mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[0].rawhist_big.bins, stats->params.rawhist1.hist_bin, ISP2X_HIST_BIN_N_MAX * sizeof(u32));
        memcpy(mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[1].rawhist_lite.bins, stats->params.rawhist0.hist_bin, ISP2X_HIST_BIN_N_MAX * sizeof(u32));
        memcpy(mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[2].rawhist_big.bins, stats->params.rawhist2.hist_bin, ISP2X_HIST_BIN_N_MAX * sizeof(u32));
        break;

    case AEC_RAWSWAP_MODE_L_LITE:
        for(int i = 0; i < ISP2X_RAWAEBIG_MEAN_NUM; i++) {
            if(i < ISP2X_RAWAELITE_MEAN_NUM) {
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[2].rawae_lite.channelr_xy[i] = stats->params.rawae0.data[i].channelr_xy;
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[2].rawae_lite.channelg_xy[i] = stats->params.rawae0.data[i].channelg_xy;
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[2].rawae_lite.channelb_xy[i] = stats->params.rawae0.data[i].channelb_xy;
            }
            mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[0].rawae_big.channelr_xy[i] = stats->params.rawae2.data[i].channelr_xy;
            mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[0].rawae_big.channelg_xy[i] = stats->params.rawae2.data[i].channelg_xy;
            mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[0].rawae_big.channelb_xy[i] = stats->params.rawae2.data[i].channelb_xy;
            mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[1].rawae_big.channelr_xy[i] = stats->params.rawae1.data[i].channelr_xy;
            mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[1].rawae_big.channelg_xy[i] = stats->params.rawae1.data[i].channelg_xy;
            mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[1].rawae_big.channelb_xy[i] = stats->params.rawae1.data[i].channelb_xy;

            if(i < ISP2X_RAWAEBIG_SUBWIN_NUM) {
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[0].rawae_big.wndx_sumr[i] = stats->params.rawae2.sumr[i];
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[0].rawae_big.wndx_sumg[i] = stats->params.rawae2.sumg[i];
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[0].rawae_big.wndx_sumb[i] = stats->params.rawae2.sumb[i];
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[1].rawae_big.wndx_sumr[i] = stats->params.rawae1.sumr[i];
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[1].rawae_big.wndx_sumg[i] = stats->params.rawae1.sumg[i];
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[1].rawae_big.wndx_sumb[i] = stats->params.rawae1.sumb[i];
            }
        }
        memcpy(mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[0].rawhist_big.bins, stats->params.rawhist2.hist_bin, ISP2X_HIST_BIN_N_MAX * sizeof(u32));
        memcpy(mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[1].rawhist_big.bins, stats->params.rawhist1.hist_bin, ISP2X_HIST_BIN_N_MAX * sizeof(u32));
        memcpy(mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[2].rawhist_lite.bins, stats->params.rawhist0.hist_bin, ISP2X_HIST_BIN_N_MAX * sizeof(u32));
        break;

    default:
        LOGE("wrong AeSwapMode=%d\n", AeSwapMode);
        return XCAM_RETURN_ERROR_PARAM;
        break;
    }

    switch(AeSelMode) {
    case AEC_RAWSEL_MODE_CHN_0:
        for(int i = 0; i < ISP2X_RAWAEBIG_MEAN_NUM; i++) {

            mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[0].rawae_big.channelr_xy[i] = stats->params.rawae3.data[i].channelr_xy;
            mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[0].rawae_big.channelg_xy[i] = stats->params.rawae3.data[i].channelg_xy;
            mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[0].rawae_big.channelb_xy[i] = stats->params.rawae3.data[i].channelb_xy;

            if(i < ISP2X_RAWAEBIG_SUBWIN_NUM) {
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[0].rawae_big.wndx_sumr[i] = stats->params.rawae3.sumr[i];
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[0].rawae_big.wndx_sumg[i] = stats->params.rawae3.sumg[i];
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[0].rawae_big.wndx_sumb[i] = stats->params.rawae3.sumb[i];
            }
        }
        memcpy(mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[0].rawhist_big.bins, stats->params.rawhist3.hist_bin, ISP2X_HIST_BIN_N_MAX * sizeof(u32));
        break;

    case AEC_RAWSEL_MODE_CHN_1:
        for(int i = 0; i < ISP2X_RAWAEBIG_MEAN_NUM; i++) {

            mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[1].rawae_big.channelr_xy[i] = stats->params.rawae3.data[i].channelr_xy;
            mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[1].rawae_big.channelg_xy[i] = stats->params.rawae3.data[i].channelg_xy;
            mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[1].rawae_big.channelb_xy[i] = stats->params.rawae3.data[i].channelb_xy;

            if(i < ISP2X_RAWAEBIG_SUBWIN_NUM) {
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[1].rawae_big.wndx_sumr[i] = stats->params.rawae3.sumr[i];
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[1].rawae_big.wndx_sumg[i] = stats->params.rawae3.sumg[i];
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[1].rawae_big.wndx_sumb[i] = stats->params.rawae3.sumb[i];
            }
        }
        memcpy(mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[1].rawhist_big.bins, stats->params.rawhist3.hist_bin, ISP2X_HIST_BIN_N_MAX * sizeof(u32));
        break;

    case AEC_RAWSEL_MODE_CHN_2:
        for(int i = 0; i < ISP2X_RAWAEBIG_MEAN_NUM; i++) {

            mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[2].rawae_big.channelr_xy[i] = stats->params.rawae3.data[i].channelr_xy;
            mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[2].rawae_big.channelg_xy[i] = stats->params.rawae3.data[i].channelg_xy;
            mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[2].rawae_big.channelb_xy[i] = stats->params.rawae3.data[i].channelb_xy;

            if(i < ISP2X_RAWAEBIG_SUBWIN_NUM) {
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[2].rawae_big.wndx_sumr[i] = stats->params.rawae3.sumr[i];
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[2].rawae_big.wndx_sumg[i] = stats->params.rawae3.sumg[i];
                mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[2].rawae_big.wndx_sumb[i] = stats->params.rawae3.sumb[i];
            }
        }
        memcpy(mAlogsSharedParams.ispStats.aec_stats.ae_data.chn[2].rawhist_big.bins, stats->params.rawhist3.hist_bin, ISP2X_HIST_BIN_N_MAX * sizeof(u32));
        break;

    case AEC_RAWSEL_MODE_TMO:
        for(int i = 0; i < ISP2X_RAWAEBIG_MEAN_NUM; i++) {

            mAlogsSharedParams.ispStats.aec_stats.ae_data.extra.rawae_big.channelr_xy[i] = stats->params.rawae3.data[i].channelr_xy;
            mAlogsSharedParams.ispStats.aec_stats.ae_data.extra.rawae_big.channelg_xy[i] = stats->params.rawae3.data[i].channelg_xy;
            mAlogsSharedParams.ispStats.aec_stats.ae_data.extra.rawae_big.channelb_xy[i] = stats->params.rawae3.data[i].channelb_xy;

            if(i < ISP2X_RAWAEBIG_SUBWIN_NUM) {
                mAlogsSharedParams.ispStats.aec_stats.ae_data.extra.rawae_big.wndx_sumr[i] = stats->params.rawae3.sumr[i];
                mAlogsSharedParams.ispStats.aec_stats.ae_data.extra.rawae_big.wndx_sumg[i] = stats->params.rawae3.sumg[i];
                mAlogsSharedParams.ispStats.aec_stats.ae_data.extra.rawae_big.wndx_sumb[i] = stats->params.rawae3.sumb[i];
            }
        }
        memcpy(mAlogsSharedParams.ispStats.aec_stats.ae_data.extra.rawhist_big.bins, stats->params.rawhist3.hist_bin, ISP2X_HIST_BIN_N_MAX * sizeof(u32));
        break;

    default:
        LOGE("wrong AeSelMode=%d\n", AeSelMode);
        return XCAM_RETURN_ERROR_PARAM;
    }

    for(int i = 0; i < ISP2X_YUVAE_MEAN_NUM; i++) {
        mAlogsSharedParams.ispStats.aec_stats.ae_data.yuvae.mean[i] = stats->params.yuvae.mean[i];
        if(i < ISP2X_YUVAE_SUBWIN_NUM)
            mAlogsSharedParams.ispStats.aec_stats.ae_data.yuvae.ro_yuvae_sumy[i] = stats->params.yuvae.ro_yuvae_sumy[i];
    }

    memcpy(mAlogsSharedParams.ispStats.aec_stats.ae_data.sihist.bins, stats->params.sihst.win_stat[0].hist_bins, SIHIST_BIN_N_MAX * sizeof(u32));

    if (expParams.ptr()) {

        mAlogsSharedParams.ispStats.aec_stats.ae_exp = expParams->data()->aecExpInfo;
        /*
         * printf("%s: L: [0x%x-0x%x], M: [0x%x-0x%x], S: [0x%x-0x%x]\n",
         *        __func__,
         *        expParams->data()->aecExpInfo.HdrExp[2].exp_sensor_params.coarse_integration_time,
         *        expParams->data()->aecExpInfo.HdrExp[2].exp_sensor_params.analog_gain_code_global,
         *        expParams->data()->aecExpInfo.HdrExp[1].exp_sensor_params.coarse_integration_time,
         *        expParams->data()->aecExpInfo.HdrExp[1].exp_sensor_params.analog_gain_code_global,
         *        expParams->data()->aecExpInfo.HdrExp[0].exp_sensor_params.coarse_integration_time,
         *        expParams->data()->aecExpInfo.HdrExp[0].exp_sensor_params.analog_gain_code_global);
         */
    }

    if (irisParams.ptr()) {

        float sof_time = (float)irisParams->data()->sofTime / 1000000000.0f;
        float start_time = (float)irisParams->data()->PIris.StartTim.tv_sec + (float)irisParams->data()->PIris.StartTim.tv_usec / 1000000.0f;
        float end_time = (float)irisParams->data()->PIris.EndTim.tv_sec + (float)irisParams->data()->PIris.EndTim.tv_usec / 1000000.0f;
        float frm_intval = 1 / (mAlogsSharedParams.ispStats.aec_stats.ae_exp.pixel_clock_freq_mhz * 1000000.0f /
                                (float)mAlogsSharedParams.ispStats.aec_stats.ae_exp.line_length_pixels / (float)mAlogsSharedParams.ispStats.aec_stats.ae_exp.frame_length_lines);

        /*printf("%s: step=%d,last-step=%d,start-tim=%f,end-tim=%f,sof_tim=%f\n",
            __func__,
            mAlogsSharedParams.ispStats.aec_stats.ae_exp.Iris.PIris.step,
            irisParams->data()->PIris.laststep,start_time,end_time,sof_time);
        */

        if(sof_time < end_time + frm_intval)
            mAlogsSharedParams.ispStats.aec_stats.ae_exp.Iris.PIris.step = irisParams->data()->PIris.laststep;
        else
            mAlogsSharedParams.ispStats.aec_stats.ae_exp.Iris.PIris.step = irisParams->data()->PIris.step;
    }

    //af
    {
        mAlogsSharedParams.ispStats.af_stats_valid =
            (stats->meas_type >> 6) & (0x01) ? true : false;
        mAlogsSharedParams.ispStats.af_stats.roia_luminance =
            stats->params.rawaf.afm_lum[0];
        mAlogsSharedParams.ispStats.af_stats.roib_sharpness =
            stats->params.rawaf.afm_sum[1];
        mAlogsSharedParams.ispStats.af_stats.roib_luminance =
            stats->params.rawaf.afm_lum[1];
        memcpy(mAlogsSharedParams.ispStats.af_stats.global_sharpness,
               stats->params.rawaf.ramdata, ISP2X_RAWAF_SUMDATA_NUM * sizeof(u32));

        mAlogsSharedParams.ispStats.af_stats.roia_sharpness = 0LL;
        for (int i = 0; i < ISP2X_RAWAF_SUMDATA_NUM; i++)
            mAlogsSharedParams.ispStats.af_stats.roia_sharpness += stats->params.rawaf.ramdata[i];

        if(afParams.ptr()) {
            mAlogsSharedParams.ispStats.af_stats.focusCode = afParams->data()->focusCode;
            mAlogsSharedParams.ispStats.af_stats.zoomCode = afParams->data()->zoomCode;
            mAlogsSharedParams.ispStats.af_stats.focus_endtim = afParams->data()->focusEndTim;
            mAlogsSharedParams.ispStats.af_stats.focus_starttim = afParams->data()->focusStartTim;
            mAlogsSharedParams.ispStats.af_stats.zoom_endtim = afParams->data()->zoomEndTim;
            mAlogsSharedParams.ispStats.af_stats.zoom_starttim = afParams->data()->zoomStartTim;
            mAlogsSharedParams.ispStats.af_stats.sof_tim = afParams->data()->sofTime;
            mAlogsSharedParams.ispStats.af_stats.lowpass_id = afParams->data()->lowPassId;
            mAlogsSharedParams.ispStats.af_stats.focusCorrection = afParams->data()->focusCorrection;
            mAlogsSharedParams.ispStats.af_stats.zoomCorrection = afParams->data()->zoomCorrection;
            memcpy(mAlogsSharedParams.ispStats.af_stats.lowpass_fv4_4,
                   afParams->data()->lowPassFv4_4, ISP2X_RAWAF_SUMDATA_NUM * sizeof(u32));
            memcpy(mAlogsSharedParams.ispStats.af_stats.lowpass_fv8_8,
                   afParams->data()->lowPassFv8_8, ISP2X_RAWAF_SUMDATA_NUM * sizeof(u32));
            memcpy(mAlogsSharedParams.ispStats.af_stats.lowpass_highlht,
                   afParams->data()->lowPassHighLht, ISP2X_RAWAF_SUMDATA_NUM * sizeof(u32));
            memcpy(mAlogsSharedParams.ispStats.af_stats.lowpass_highlht2,
                   afParams->data()->lowPassHighLht2, ISP2X_RAWAF_SUMDATA_NUM * sizeof(u32));
        }
    }
#endif
    return ret;
}

XCamReturn
RkAiqCore::analyze(const SmartPtr<VideoBuffer> &buffer)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    SmartPtr<RkAiqFullParamsProxy> fullParam;
    SmartPtr<RkAiqFullParamsProxy> fullPparam;
    bool has_orb_stats = false;
    bool has_3a_stats = false;

    {
        SmartLock locker (mApiMutex);
        mSafeEnableAlgo = false;
    }

    if (!firstStatsReceived) {
        firstStatsReceived = true;
        mState = RK_AIQ_CORE_STATE_RUNNING;
    }

#ifdef RK_SIMULATOR_HW
    has_orb_stats = true;
    has_3a_stats = true;
#else
    if (buffer->get_video_info().format == V4L2_META_FMT_RK_ISP1_STAT_3A) {
        has_3a_stats = true;
    } else {
        has_orb_stats = true;
    }
#endif

#ifdef RK_SIMULATOR_HW
    const SmartPtr<V4l2BufferProxy> buf =
        buffer.dynamic_cast_ptr<V4l2BufferProxy>();
    //SmartPtr<RkAiqIspMeasParamsProxy> ispParams = buf->get_isp_params();

    rk_sim_isp_v200_stats_t* stats =
        (rk_sim_isp_v200_stats_t*)(buf->get_v4l2_userptr());
    // copy directly for simulator
    mAlogsSharedParams.ispStats.awb_stats = stats->awb;
    mAlogsSharedParams.ispStats.awb_stats_valid = stats->valid_awb;
    //mAlogsSharedParams.ispStats.awb_cfg_effect_v200 = ispParams->data()->awb_cfg_v200;
    mAlogsSharedParams.ispStats.awb_cfg_effect_valid = true;
    mAlogsSharedParams.ispStats.awb_stats_v201 = stats->awb_v201;
    //mAlogsSharedParams.ispStats.awb_cfg_effect_v201 = ispParams->data()->awb_cfg_v201;

    mAlogsSharedParams.ispStats.aec_stats = stats->ae;
    mAlogsSharedParams.ispStats.aec_stats_valid = stats->valid_ae;

    mAlogsSharedParams.ispStats.af_stats = stats->af;
    mAlogsSharedParams.ispStats.af_stats_valid = stats->valid_af;


#if 1
    LOGD_ANR("oyyf: %s:%d input stats param start\n", __FUNCTION__, __LINE__);
    mAlogsSharedParams.iso = stats->iso;
    LOGD_ANR("oyyf: %s:%d input stats param end\n", __FUNCTION__, __LINE__);
#endif

    //Ahdr
    mAlogsSharedParams.ispStats.ahdr_stats_valid = true;
    mAlogsSharedParams.ispStats.ahdr_stats = stats->ahdr;

    mAlogsSharedParams.ispStats.orb_stats = stats->orb;
    mAlogsSharedParams.ispStats.orb_stats_valid = stats->valid_orb;
#else
    if (has_3a_stats) {
        alogsSharedParamsMutex.lock();
        convertIspstatsToAlgo(buffer);
        cacheIspStatsToList();
    } else if (has_orb_stats) {
        const SmartPtr<V4l2BufferProxy> buf =
            buffer.dynamic_cast_ptr<V4l2BufferProxy>();
        struct rkispp_stats_buffer *ppstats =
            (struct rkispp_stats_buffer *)(buf->get_v4l2_userptr());

        mAlogsSharedParams.ispStats.orb_stats_valid =
            (ppstats->meas_type >> 4) & (0x01) ? true : false;
        mAlogsSharedParams.ispStats.orb_stats.num_points =
            ppstats->total_num;
        for (u32 i = 0; i < ppstats->total_num; i++) {
            mAlogsSharedParams.ispStats.orb_stats.points[i].x =
                ppstats->data[i].x;
            mAlogsSharedParams.ispStats.orb_stats.points[i].y =
                ppstats->data[i].y;
            memcpy(mAlogsSharedParams.ispStats.orb_stats.points[i].brief,
                   ppstats->data[i].brief, 15);
        }
    } else {
        LOGW_ANALYZER("no orb or 3a stats !", __FUNCTION__, __LINE__);
    }
#endif

    if (has_3a_stats) {
        fullParam = analyzeInternal(RK_AIQ_CORE_ANALYZE_MEAS);
        alogsSharedParamsMutex.unlock();
    }
    if (has_orb_stats)
        fullPparam = analyzeInternalPp();
    {
        SmartLock locker (mApiMutex);
        mSafeEnableAlgo = true;
        mApiMutexCond.broadcast ();
    }

#ifdef RK_SIMULATOR_HW
    // merge results for simulator
    fullParam->data()->mIsppMeasParams->data()->orb =
        fullPparam->data()->mIsppMeasParams->data()->orb;
#endif

    if (fullParam.ptr() && mCb) {
        fullParam->data()->mIspMeasParams->data()->frame_id = buffer->get_sequence() + 1;
        /*
         * if (fullParam->data()->mIsppMeasParams.ptr())
         *     fullParam->data()->mIsppMeasParams->data()->frame_id = buffer->get_sequence() + 1;
         */
        mCb->rkAiqCalcDone(fullParam);
    } else if (fullPparam.ptr() && mCb) {
        fullPparam->data()->mIsppMeasParams->data()->frame_id = buffer->get_sequence() + 1;
        mCb->rkAiqCalcDone(fullPparam);
    }

    return ret;
}

XCamReturn
RkAiqCore::events_analyze(const SmartPtr<ispHwEvt_t> &evts)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
#ifndef RK_SIMULATOR_HW
    SmartPtr<RkAiqFullParamsProxy> fullParam;
    SmartPtr<RkAiqExpParamsProxy> preExpParams = nullptr;
    SmartPtr<RkAiqExpParamsProxy> curExpParams = nullptr;
    const SmartPtr<Isp20Evt> isp20Evts =
        evts.dynamic_cast_ptr<Isp20Evt>();
    uint32_t frameId = isp20Evts->sequence < 0 ? 0 : isp20Evts->sequence;

    if (isp20Evts->getEffectiveExpParams(frameId, curExpParams) < 0)
        goto out;

    if (frameId > 0) {
        if (isp20Evts->getEffectiveExpParams(frameId - 1, preExpParams) < 0)
            goto out;
    } else {
        if (isp20Evts->getEffectiveExpParams(frameId, preExpParams) < 0)
            goto out;
    }

    LOGD("id(%d), linear mode exp(%d-%d)", frameId,
         curExpParams->data()->aecExpInfo.LinearExp.exp_sensor_params.analog_gain_code_global,
         curExpParams->data()->aecExpInfo.LinearExp.exp_sensor_params.coarse_integration_time);

    alogsSharedParamsMutex.lock();
    mAlogsSharedParams.frameId = frameId;
    mAlogsSharedParams.preExp = preExpParams->data()->aecExpInfo;
    mAlogsSharedParams.curExp = curExpParams->data()->aecExpInfo;
    fullParam = analyzeInternal(RK_AIQ_CORE_ANALYZE_OTHER);
    alogsSharedParamsMutex.unlock();
    if (fullParam.ptr() && mCb) {
        fullParam->data()->mIspOtherParams->data()->frame_id = frameId;
        fullParam->data()->mIsppOtherParams->data()->frame_id = frameId;
        mCb->rkAiqCalcDone(fullParam);
    }

    LOGD_ANALYZER("%s, analyze sequence(%d)", __func__, isp20Evts->sequence);
#endif

out:
    return ret;
}

XCamReturn
RkAiqCore::txBufAnalyze(const SmartPtr<RkAiqTxBufInfo> &buffer)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    const SmartPtr<V4l2BufferProxy> txBufProxy =
        buffer->txBuf.dynamic_cast_ptr<V4l2BufferProxy>();

    int32_t working_mode = mAlogsSharedParams.working_mode;
    RKAiqAecExpInfo_t *aecExpInfo = &buffer->expParams->data()->aecExpInfo;

    mAlogsSharedParams.tx_buf.width = txBufProxy->get_video_info().width;
    mAlogsSharedParams.tx_buf.height = txBufProxy->get_video_info().height;
    mAlogsSharedParams.tx_buf.bpp = txBufProxy->get_video_info().color_bits;
    mAlogsSharedParams.tx_buf.stridePerLine = txBufProxy->get_video_info().strides[0];
    mAlogsSharedParams.tx_buf.id = txBufProxy->get_sequence();
    mAlogsSharedParams.tx_buf.data_addr = (void*)txBufProxy->get_v4l2_userptr();
    mAlogsSharedParams.tx_buf.aecExpInfo = &buffer->expParams->data()->aecExpInfo;

    LOGD_ANALYZER("%s, id(%d), working_mode(%d), bpp(%d), strides(%d), wxh(%dx%x), addr(0x%x)",
                  __func__, txBufProxy->get_sequence(), working_mode,
                  mAlogsSharedParams.tx_buf.bpp, mAlogsSharedParams.tx_buf.stridePerLine,
                  mAlogsSharedParams.tx_buf.width, mAlogsSharedParams.tx_buf.height,
                  txBufProxy->get_v4l2_userptr());

    analyzeInternal(RK_AIQ_CORE_ANALYZE_AFD);

    return ret;
}

#define PREPROCESS_ALGO(at) \
    if (mCur##at##AlgoHdl.ptr() && mCur##at##AlgoHdl->getEnable()) { \
        /* TODO, should be called before all algos preProcess ? */ \
        ret = mCur##at##AlgoHdl->updateConfig(true); \
        RKAIQCORE_CHECK_BYPASS(ret, "%s updateConfig failed", #at); \
        ret = mCur##at##AlgoHdl->preProcess(); \
        RKAIQCORE_CHECK_BYPASS(ret, "%s preProcess failed", #at); \
    } \

#define PREPROCESS_CUSTOM_ALGO(at) \
    if (mCurCustom##at##AlgoHdl.ptr() && mCurCustom##at##AlgoHdl->getEnable()) { \
        /* TODO, should be called before all algos preProcess ? */ \
        ret = mCurCustom##at##AlgoHdl->updateConfig(true); \
        RKAIQCORE_CHECK_BYPASS(ret, "%s updateConfig failed", #at); \
        ret = mCurCustom##at##AlgoHdl->preProcess(); \
        RKAIQCORE_CHECK_BYPASS(ret, "%s preProcess failed", #at); \
    } \

XCamReturn
RkAiqCore::preProcessPp()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    // TODO: may adjust the preprocess order

    PREPROCESS_ALGO(Aorb);

    EXIT_ANALYZER_FUNCTION();

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqCore::preProcess(enum rk_aiq_core_analyze_type_e type)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    // TODO: may adjust the preprocess order
    if (type == RK_AIQ_CORE_ANALYZE_ALL || \
            type == RK_AIQ_CORE_ANALYZE_MEAS) {
        PREPROCESS_ALGO(Ae);
        PREPROCESS_CUSTOM_ALGO(Ae);
        PREPROCESS_ALGO(Awb);
        PREPROCESS_CUSTOM_ALGO(Awb);
        PREPROCESS_ALGO(Af);
        PREPROCESS_CUSTOM_ALGO(Af);
        PREPROCESS_ALGO(Accm);
        PREPROCESS_ALGO(Ahdr);
        PREPROCESS_ALGO(Adpcc);
        PREPROCESS_ALGO(Alsc);
#ifdef RK_SIMULATOR_HW
        PREPROCESS_ALGO(Anr);
        PREPROCESS_ALGO(Adhaz);
        PREPROCESS_ALGO(Acp);
        PREPROCESS_ALGO(Asharp);
        PREPROCESS_ALGO(A3dlut);
        PREPROCESS_ALGO(Ablc);
        PREPROCESS_ALGO(Acgc);
        PREPROCESS_ALGO(Adebayer);
        PREPROCESS_ALGO(Afec);
        PREPROCESS_ALGO(Agamma);
        PREPROCESS_ALGO(Adegamma);
        PREPROCESS_ALGO(Agic);
        PREPROCESS_ALGO(Aie);
        PREPROCESS_ALGO(Aldch);
        PREPROCESS_ALGO(Ar2y);
        PREPROCESS_ALGO(Awdr);
        PREPROCESS_ALGO(Asd);
#endif
    }

#ifndef RK_SIMULATOR_HW
    if (type == RK_AIQ_CORE_ANALYZE_ALL || \
            type == RK_AIQ_CORE_ANALYZE_OTHER) {
        PREPROCESS_ALGO(Anr);
        PREPROCESS_ALGO(Adhaz);
        PREPROCESS_ALGO(Acp);
        PREPROCESS_ALGO(Asharp);
        PREPROCESS_ALGO(A3dlut);
        PREPROCESS_ALGO(Ablc);
        PREPROCESS_ALGO(Acgc);
        PREPROCESS_ALGO(Adebayer);
        PREPROCESS_ALGO(Afec);
        PREPROCESS_ALGO(Agamma);
        PREPROCESS_ALGO(Adegamma);
        PREPROCESS_ALGO(Agic);
        PREPROCESS_ALGO(Aie);
        PREPROCESS_ALGO(Aldch);
        PREPROCESS_ALGO(Ar2y);
        PREPROCESS_ALGO(Awdr);
        PREPROCESS_ALGO(Asd);
    }
#endif

#ifndef RK_SIMULATOR_HW
    if (type == RK_AIQ_CORE_ANALYZE_AFD) {
        PREPROCESS_ALGO(Afd);
    }
#endif

    EXIT_ANALYZER_FUNCTION();

    return XCAM_RETURN_NO_ERROR;
}

#define PROCESSING_ALGO(at) \
    if (mCur##at##AlgoHdl.ptr() && mCur##at##AlgoHdl->getEnable()) \
        ret = mCur##at##AlgoHdl->processing(); \
    RKAIQCORE_CHECK_BYPASS(ret, "%s processing failed", #at);

#define PROCESSING_CUSTOM_ALGO(at) \
    if (mCurCustom##at##AlgoHdl.ptr() && mCurCustom##at##AlgoHdl->getEnable()) \
        ret = mCurCustom##at##AlgoHdl->processing(); \
    RKAIQCORE_CHECK_BYPASS(ret, "%s processing failed", #at);

XCamReturn
RkAiqCore::processingPp()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    PROCESSING_ALGO(Aorb);

    EXIT_ANALYZER_FUNCTION();

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqCore::processing(enum rk_aiq_core_analyze_type_e type)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    // TODO: may adjust the processing order

    if (type == RK_AIQ_CORE_ANALYZE_ALL || \
            type == RK_AIQ_CORE_ANALYZE_MEAS) {
        PROCESSING_ALGO(Ae);
        PROCESSING_CUSTOM_ALGO(Ae);
        PROCESSING_ALGO(Awb);
        PROCESSING_CUSTOM_ALGO(Awb);
        PROCESSING_ALGO(Af);
        PROCESSING_CUSTOM_ALGO(Af);
        PROCESSING_ALGO(Ahdr);
        PROCESSING_ALGO(Accm);
        PROCESSING_ALGO(Adpcc);
        PROCESSING_ALGO(Alsc);
#ifdef RK_SIMULATOR_HW
        PROCESSING_ALGO(Anr);
        PROCESSING_ALGO(Adhaz);
        PROCESSING_ALGO(Acp);
        PROCESSING_ALGO(Asharp);
        PROCESSING_ALGO(A3dlut);
        PROCESSING_ALGO(Ablc);
        PROCESSING_ALGO(Acgc);
        PROCESSING_ALGO(Adebayer);
        PROCESSING_ALGO(Afec);
        PROCESSING_ALGO(Agamma);
        PROCESSING_ALGO(Adegamma);
        PROCESSING_ALGO(Agic);
        PROCESSING_ALGO(Aie);
        PROCESSING_ALGO(Aldch);
        PROCESSING_ALGO(Ar2y);
        PROCESSING_ALGO(Awdr);
        PROCESSING_ALGO(Asd);
#endif
    }

#ifndef RK_SIMULATOR_HW
    if (type == RK_AIQ_CORE_ANALYZE_ALL || \
            type == RK_AIQ_CORE_ANALYZE_OTHER) {
        PROCESSING_ALGO(Anr);
        PROCESSING_ALGO(Adhaz);
        PROCESSING_ALGO(Acp);
        PROCESSING_ALGO(Asharp);
        PROCESSING_ALGO(A3dlut);
        PROCESSING_ALGO(Ablc);
        PROCESSING_ALGO(Acgc);
        PROCESSING_ALGO(Adebayer);
        PROCESSING_ALGO(Afec);
        PROCESSING_ALGO(Agamma);
        PROCESSING_ALGO(Adegamma);
        PROCESSING_ALGO(Agic);
        PROCESSING_ALGO(Aie);
        PROCESSING_ALGO(Aldch);
        PROCESSING_ALGO(Ar2y);
        PROCESSING_ALGO(Awdr);
        PROCESSING_ALGO(Asd);
    }
#endif

#ifndef RK_SIMULATOR_HW
    if (type == RK_AIQ_CORE_ANALYZE_AFD) {
        PROCESSING_ALGO(Afd);
    }
#endif

    EXIT_ANALYZER_FUNCTION();

    return XCAM_RETURN_NO_ERROR;
}

#define POSTPROCESS_ALGO(at) \
    if (mCur##at##AlgoHdl.ptr() && mCur##at##AlgoHdl->getEnable()) \
        ret = mCur##at##AlgoHdl->postProcess(); \
    RKAIQCORE_CHECK_BYPASS(ret, "%s postProcess failed", #at);

#define POSTPROCESS_CUSTOM_ALGO(at) \
    if (mCurCustom##at##AlgoHdl.ptr() && mCurCustom##at##AlgoHdl->getEnable()) \
        ret = mCurCustom##at##AlgoHdl->postProcess(); \
    RKAIQCORE_CHECK_BYPASS(ret, "%s postProcess failed", #at);

XCamReturn
RkAiqCore::postProcessPp()
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    POSTPROCESS_ALGO(Aorb);

    EXIT_ANALYZER_FUNCTION();

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqCore::postProcess(enum rk_aiq_core_analyze_type_e type)
{
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    if (type == RK_AIQ_CORE_ANALYZE_ALL || \
            type == RK_AIQ_CORE_ANALYZE_MEAS) {
        POSTPROCESS_ALGO(Ae);
        POSTPROCESS_CUSTOM_ALGO(Ae);
        POSTPROCESS_ALGO(Awb);
        POSTPROCESS_CUSTOM_ALGO(Awb);
        POSTPROCESS_ALGO(Af);
        POSTPROCESS_CUSTOM_ALGO(Af);
        POSTPROCESS_ALGO(Ahdr);
        POSTPROCESS_ALGO(Accm);
        POSTPROCESS_ALGO(Adpcc);
        POSTPROCESS_ALGO(Alsc);
#ifdef RK_SIMULATOR_HW
        POSTPROCESS_ALGO(Anr);
        POSTPROCESS_ALGO(Adhaz);
        POSTPROCESS_ALGO(Acp);
        POSTPROCESS_ALGO(Asharp);
        POSTPROCESS_ALGO(A3dlut);
        POSTPROCESS_ALGO(Ablc);
        POSTPROCESS_ALGO(Acgc);
        POSTPROCESS_ALGO(Adebayer);
        POSTPROCESS_ALGO(Afec);
        POSTPROCESS_ALGO(Agamma);
        POSTPROCESS_ALGO(Adegamma);
        POSTPROCESS_ALGO(Agic);
        POSTPROCESS_ALGO(Aie);
        POSTPROCESS_ALGO(Aldch);
        POSTPROCESS_ALGO(Ar2y);
        POSTPROCESS_ALGO(Awdr);
        POSTPROCESS_ALGO(Asd);
#endif
    }

#ifndef RK_SIMULATOR_HW
    if (type == RK_AIQ_CORE_ANALYZE_ALL || \
            type == RK_AIQ_CORE_ANALYZE_OTHER) {
        POSTPROCESS_ALGO(Anr);
        POSTPROCESS_ALGO(Adhaz);
        POSTPROCESS_ALGO(Acp);
        POSTPROCESS_ALGO(Asharp);
        POSTPROCESS_ALGO(A3dlut);
        POSTPROCESS_ALGO(Ablc);
        POSTPROCESS_ALGO(Acgc);
        POSTPROCESS_ALGO(Adebayer);
        POSTPROCESS_ALGO(Afec);
        POSTPROCESS_ALGO(Agamma);
        POSTPROCESS_ALGO(Adegamma);
        POSTPROCESS_ALGO(Agic);
        POSTPROCESS_ALGO(Aie);
        POSTPROCESS_ALGO(Aldch);
        POSTPROCESS_ALGO(Ar2y);
        POSTPROCESS_ALGO(Awdr);
        POSTPROCESS_ALGO(Asd);
    }
#endif

#ifndef RK_SIMULATOR_HW
    if (type == RK_AIQ_CORE_ANALYZE_AFD) {
        POSTPROCESS_ALGO(Afd);
    }
#endif
    // TODO: may adjust the postProcess order
    EXIT_ANALYZER_FUNCTION();

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqCore::setHwInfos(struct RkAiqHwInfo &hw_info)
{
    ENTER_ANALYZER_FUNCTION();
    mHwInfo = hw_info;
    EXIT_ANALYZER_FUNCTION();

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqCore::setCpsLtCfg(rk_aiq_cpsl_cfg_t &cfg)
{
    ENTER_ANALYZER_FUNCTION();
    if (mState < RK_AIQ_CORE_STATE_INITED) {
        LOGE_ANALYZER("should call afer init");
        return XCAM_RETURN_ERROR_FAILED;
    }

    if (mCpslCap.modes_num == 0)
        return XCAM_RETURN_ERROR_PARAM;

    int i = 0;
    for (; i < mCpslCap.modes_num; i++) {
        if (mCpslCap.supported_modes[i] == cfg.mode)
            break;
    }

    if (i == mCpslCap.modes_num) {
        return XCAM_RETURN_ERROR_PARAM;
    }

    if (cfg.mode == RK_AIQ_OP_MODE_AUTO) {
        mAlogsSharedParams.cpslCfg.u.a = cfg.u.a;
    } else if (cfg.mode == RK_AIQ_OP_MODE_MANUAL) {
        mAlogsSharedParams.cpslCfg.u.m = cfg.u.m;
    } else {
        return XCAM_RETURN_ERROR_PARAM;
    }

    mAlogsSharedParams.cpslCfg.mode = cfg.mode;

    for (i = 0; i < mCpslCap.lght_src_num; i++) {
        if (mCpslCap.supported_lght_src[i] == cfg.lght_src)
            break;
    }

    if (i == mCpslCap.lght_src_num) {
        return XCAM_RETURN_ERROR_PARAM;
    }

    mAlogsSharedParams.cpslCfg = cfg;
    LOGD("set cpsl: mode %d", cfg.mode);
    EXIT_ANALYZER_FUNCTION();

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqCore::getCpsLtInfo(rk_aiq_cpsl_info_t &info)
{
    ENTER_ANALYZER_FUNCTION();
    if (mState < RK_AIQ_CORE_STATE_INITED) {
        LOGE_ANALYZER("should call afer init");
        return XCAM_RETURN_ERROR_FAILED;
    }

    info.mode = mAlogsSharedParams.cpslCfg.mode;
    if (info.mode == RK_AIQ_OP_MODE_MANUAL) {
        info.on = mAlogsSharedParams.cpslCfg.u.m.on;
        info.strength_led = mAlogsSharedParams.cpslCfg.u.m.strength_led;
        info.strength_ir = mAlogsSharedParams.cpslCfg.u.m.strength_ir;
    } else {
        info.on = mCurCpslOn;
        info.gray = mAlogsSharedParams.gray_mode;
    }

    info.lght_src = mAlogsSharedParams.cpslCfg.lght_src;
    EXIT_ANALYZER_FUNCTION();

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqCore::queryCpsLtCap(rk_aiq_cpsl_cap_t &cap)
{
    ENTER_ANALYZER_FUNCTION();
    if (mHwInfo.fl_supported || mHwInfo.irc_supported) {
        cap.supported_modes[0] = RK_AIQ_OP_MODE_AUTO;
        cap.supported_modes[1] = RK_AIQ_OP_MODE_MANUAL;
        cap.modes_num = 2;
    } else {
        cap.modes_num = 0;
    }

    cap.lght_src_num = 0;
    if (mHwInfo.fl_supported) {
        cap.supported_lght_src[0] = RK_AIQ_CPSLS_LED;
        cap.lght_src_num++;
    }

    if (mHwInfo.irc_supported) {
        cap.supported_lght_src[cap.lght_src_num] = RK_AIQ_CPSLS_IR;
        cap.lght_src_num++;
    }

    if (cap.lght_src_num > 1) {
        cap.supported_lght_src[cap.lght_src_num] = RK_AIQ_CPSLS_MIX;
        cap.lght_src_num++;
    }

    cap.strength_led.min = 0;
    cap.strength_led.max = 100;
    if (!mHwInfo.fl_strth_adj)
        cap.strength_led.step = 100;
    else
        cap.strength_led.step = 1;

    cap.strength_ir.min = 0;
    cap.strength_ir.max = 100;
    if (!mHwInfo.fl_ir_strth_adj)
        cap.strength_ir.step = 100;
    else
        cap.strength_ir.step = 1;

    cap.sensitivity.min = 0;
    cap.sensitivity.max = 100;
    cap.sensitivity.step = 1;

    LOGI("cpsl cap: light_src_num %d, led_step %f, ir_step %f",
         cap.lght_src_num, cap.strength_led.step, cap.strength_ir.step);

    EXIT_ANALYZER_FUNCTION();

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqCore::genCpslResult(RkAiqFullParams* params)
{
    rk_aiq_cpsl_cfg_t* cpsl_cfg = &mAlogsSharedParams.cpslCfg;

    if (cpsl_cfg->mode == RK_AIQ_OP_MODE_INVALID)
        return XCAM_RETURN_NO_ERROR;

    if (mAiqCpslParamsPool->has_free_items()) {
        params->mCpslParams = mAiqCpslParamsPool->get_item();
    } else {
        LOGW_ANALYZER("no free cpsl params buffer!");
        return XCAM_RETURN_NO_ERROR;
    }

    RKAiqCpslInfoWrapper_t* cpsl_param = params->mCpslParams->data().ptr();
    xcam_mem_clear(*cpsl_param);

    LOGD_ANALYZER("cpsl mode %d, light src %d", cpsl_cfg->mode, cpsl_cfg->lght_src);
    bool cpsl_on = false;
    bool need_update = false;

    if (mCurOpMode != cpsl_cfg->mode) {
        mCurOpMode = cpsl_cfg->mode;
        need_update = true;
    }

    if (cpsl_cfg->mode == RK_AIQ_OP_MODE_MANUAL) {
        if ((mCurCpslOn != cpsl_cfg->u.m.on) ||
                (fabs(mStrthLed - cpsl_cfg->u.m.strength_led) > EPSINON) ||
                (fabs(mStrthIr - cpsl_cfg->u.m.strength_ir) > EPSINON)) {
            need_update = true;
            cpsl_on = cpsl_cfg->u.m.on;
            cpsl_param->fl.power[0] = cpsl_cfg->u.m.strength_led / 100.0f;
            cpsl_param->fl_ir.power[0] = cpsl_cfg->u.m.strength_ir / 100.0f;
        }
    } else {
        RkAiqAlgoPreResAsdInt* asd_pre_rk = (RkAiqAlgoPreResAsdInt*)mAlogsSharedParams.preResComb.asd_pre_res;
        if (asd_pre_rk) {
            asd_preprocess_result_t* asd_result = &asd_pre_rk->asd_result;
            if (mCurCpslOn != asd_result->cpsl_on) {
                need_update = true;
                cpsl_on = asd_result->cpsl_on;
            }
        }
        cpsl_param->fl.power[0] = 1.0f;
        cpsl_param->fl_ir.power[0] = 1.0f;
    }

    // need to init cpsl status, cause the cpsl driver state
    // may be not correct
    if (mState == RK_AIQ_CORE_STATE_INITED)
        need_update = true;

    if (need_update) {
        if (cpsl_cfg->lght_src & RK_AIQ_CPSLS_LED) {
            cpsl_param->update_fl = true;
            if (cpsl_on)
                cpsl_param->fl.flash_mode = RK_AIQ_FLASH_MODE_TORCH;
            else
                cpsl_param->fl.flash_mode = RK_AIQ_FLASH_MODE_OFF;
            if (cpsl_on ) {
                cpsl_param->fl.strobe = true;
                mAlogsSharedParams.fill_light_on = true;
            } else {
                cpsl_param->fl.strobe = false;
                mAlogsSharedParams.fill_light_on = false;
            }
            LOGD_ANALYZER("cpsl fl mode %d, strength %f, strobe %d",
                          cpsl_param->fl.flash_mode, cpsl_param->fl.power[0],
                          cpsl_param->fl.strobe);
        }

        if (cpsl_cfg->lght_src & RK_AIQ_CPSLS_IR) {
            cpsl_param->update_ir = true;
            if (cpsl_on) {
                cpsl_param->ir.irc_on = true;
                cpsl_param->fl_ir.flash_mode = RK_AIQ_FLASH_MODE_TORCH;
                cpsl_param->fl_ir.strobe = true;
                mAlogsSharedParams.fill_light_on = true;
            } else {
                cpsl_param->ir.irc_on = false;
                cpsl_param->fl_ir.flash_mode = RK_AIQ_FLASH_MODE_OFF;
                cpsl_param->fl_ir.strobe = false;
                mAlogsSharedParams.fill_light_on = false;
            }
            LOGD_ANALYZER("cpsl irc on %d, fl_ir: mode %d, strength %f, strobe %d",
                          cpsl_param->ir.irc_on, cpsl_param->fl_ir.flash_mode, cpsl_param->fl_ir.power[0],
                          cpsl_param->fl_ir.strobe);
        }

        if (mGrayMode == RK_AIQ_GRAY_MODE_CPSL) {
            if (mAlogsSharedParams.fill_light_on && cpsl_cfg->gray_on) {
                mAlogsSharedParams.gray_mode = true;
            } else
                mAlogsSharedParams.gray_mode = false;

        } else {
            /* no mutex lock protection for gray_mode with setGrayMode,
             * so need set again here
             */
            if (mGrayMode == RK_AIQ_GRAY_MODE_OFF)
                mAlogsSharedParams.gray_mode = false;
            else if (mGrayMode == RK_AIQ_GRAY_MODE_ON)
                mAlogsSharedParams.gray_mode = true;
        }
        mCurCpslOn = cpsl_on;
        mStrthLed = cpsl_cfg->u.m.strength_led;
        mStrthIr = cpsl_cfg->u.m.strength_ir;
    } else {
        cpsl_param->update_ir = false;
        cpsl_param->update_fl = false;
    }

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
RkAiqCore::setGrayMode(rk_aiq_gray_mode_t mode)
{
    LOGD_ANALYZER("%s: gray mode %d", __FUNCTION__, mode);

    if (mAlogsSharedParams.is_bw_sensor) {
        LOGE_ANALYZER("%s: not support for black&white sensor", __FUNCTION__);
        return XCAM_RETURN_ERROR_PARAM;
    }
    if (mAlogsSharedParams.calib->colorAsGrey.enable) {
        LOGE_ANALYZER("%s: not support,since color_as_grey is enabled in xml", __FUNCTION__);
        return XCAM_RETURN_ERROR_PARAM;
    }

    mGrayMode = mode;
    if (mode == RK_AIQ_GRAY_MODE_OFF)
        mAlogsSharedParams.gray_mode = false;
    else if (mode == RK_AIQ_GRAY_MODE_ON)
        mAlogsSharedParams.gray_mode = true;
    else if (mode == RK_AIQ_GRAY_MODE_CPSL)
        ; // do nothing
    else
        LOGE_ANALYZER("%s: gray mode %d error", __FUNCTION__, mode);

    return XCAM_RETURN_NO_ERROR;
}

rk_aiq_gray_mode_t
RkAiqCore::getGrayMode()
{
    LOGD_ANALYZER("%s: gray mode %d", __FUNCTION__, mGrayMode);
    return mGrayMode;
}

void
RkAiqCore::setSensorFlip(bool mirror, bool flip)
{
    mAlogsSharedParams.sns_mirror = mirror;
    mAlogsSharedParams.sns_flip = flip;
}

XCamReturn
RkAiqCore::setCalib(const CamCalibDbContext_t* aiqCalib)
{
    ENTER_ANALYZER_FUNCTION();

    if (mState != RK_AIQ_CORE_STATE_STOPED) {
        LOGE_ANALYZER("wrong state %d\n", mState);
        return XCAM_RETURN_ERROR_ANALYZER;
    }

    mAlogsSharedParams.calib = aiqCalib;
    mAlogsSharedParams.conf_type = RK_AIQ_ALGO_CONFTYPE_UPDATECALIB;

    EXIT_ANALYZER_FUNCTION();

    return XCAM_RETURN_NO_ERROR;
}


} //namespace RkCam
