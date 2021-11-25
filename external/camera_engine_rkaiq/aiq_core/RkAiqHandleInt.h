/*
 * RkAiqHandleInt.h
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

#ifndef _RK_AIQ_HANDLE_INT_H_
#define _RK_AIQ_HANDLE_INT_H_

#include "RkAiqHandle.h"
#include "rk_aiq_algo_types_int.h"
#include "ae/rk_aiq_uapi_ae_int.h"
#include "awb/rk_aiq_uapi_awb_int.h"
#include "adebayer/rk_aiq_uapi_adebayer_int.h"
#include "ahdr/rk_aiq_uapi_ahdr_int.h"
#include "awdr/rk_aiq_uapi_awdr_int.h"
#include "alsc/rk_aiq_uapi_alsc_int.h"
#include "accm/rk_aiq_uapi_accm_int.h"
#include "a3dlut/rk_aiq_uapi_a3dlut_int.h"
#include "xcam_mutex.h"
#include "adehaze/rk_aiq_uapi_adehaze_int.h"
#include "agamma/rk_aiq_uapi_agamma_int.h"
#include "adegamma/rk_aiq_uapi_adegamma_int.h"
#include "ablc/rk_aiq_uapi_ablc_int.h"
#include "adpcc/rk_aiq_uapi_adpcc_int.h"
#include "anr/rk_aiq_uapi_anr_int.h"
#include "asharp/rk_aiq_uapi_asharp_int.h"
#include "agic/rk_aiq_uapi_agic_int.h"
#include "afec/rk_aiq_uapi_afec_int.h"
#include "af/rk_aiq_uapi_af_int.h"
#include "asd/rk_aiq_uapi_asd_int.h"
#include "aldch/rk_aiq_uapi_aldch_int.h"
#include "acp/rk_aiq_uapi_acp_int.h"
#include "aie/rk_aiq_uapi_aie_int.h"

namespace RkCam {

class RkAiqCore;

class RkAiqHandleIntCom : virtual public RkAiqHandle
{
public:
    explicit RkAiqHandleIntCom(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore)
        , updateAtt(false)
        , mUpdateCond(false) {};
    virtual ~RkAiqHandleIntCom() {};
protected:
    XCamReturn configInparamsCom(RkAiqAlgoCom* com, int type);
    void waitSignal();
    void sendSignal();
    XCam::Mutex mCfgMutex;
    bool updateAtt;
    XCam::Cond mUpdateCond;
};

#define RKAIQHANDLEINT(algo) \
    class RkAiq##algo##HandleInt: \
            virtual public RkAiq##algo##Handle, \
            virtual public RkAiqHandleIntCom { \
    public: \
        explicit RkAiq##algo##HandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore) \
                    : RkAiqHandle(des, aiqCore) \
                    , RkAiq##algo##Handle(des, aiqCore) \
                    , RkAiqHandleIntCom(des, aiqCore) {}; \
        virtual ~RkAiq##algo##HandleInt() { RkAiq##algo##Handle::deInit(); }; \
        virtual XCamReturn prepare(); \
        virtual XCamReturn preProcess(); \
        virtual XCamReturn processing(); \
        virtual XCamReturn postProcess(); \
    protected: \
        virtual void init(); \
        virtual void deInit() { RkAiq##algo##Handle::deInit(); }; \
    }

// define
//RKAIQHANDLEINT(Ae);
//RKAIQHANDLEINT(Awb);
//RKAIQHANDLEINT(Af);
//RKAIQHANDLEINT(Ahdr);
//RKAIQHANDLEINT(Anr);
//RKAIQHANDLEINT(Alsc);
//RKAIQHANDLEINT(Asharp);
//RKAIQHANDLEINT(Adhaz);
//RKAIQHANDLEINT(Asd);
//RKAIQHANDLEINT(Acp);
//RKAIQHANDLEINT(A3dlut);
//RKAIQHANDLEINT(Ablc);
//RKAIQHANDLEINT(Accm);
RKAIQHANDLEINT(Acgc);
//RKAIQHANDLEINT(Adebayer);
//RKAIQHANDLEINT(Adpcc);
// RKAIQHANDLEINT(Afec);
//RKAIQHANDLEINT(Agamma);
//RKAIQHANDLEINT(Agic);
//RKAIQHANDLEINT(Aie);
// RKAIQHANDLEINT(Aldch);
RKAIQHANDLEINT(Ar2y);
//RKAIQHANDLEINT(Awdr);
RKAIQHANDLEINT(Aorb);
// ae
class RkAiqAeHandleInt:
    virtual public RkAiqAeHandle,
    virtual public RkAiqHandleIntCom {
public:
    explicit RkAiqAeHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore)
        , RkAiqAeHandle(des, aiqCore)
        , RkAiqHandleIntCom(des, aiqCore) {};
    virtual ~RkAiqAeHandleInt() {
        RkAiqAeHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    // TODO add algo specific methods, this is a sample
    XCamReturn setExpSwAttr(Uapi_ExpSwAttr_t ExpSwAttr);
    XCamReturn getExpSwAttr(Uapi_ExpSwAttr_t* pExpSwAttr);
    XCamReturn setLinExpAttr(Uapi_LinExpAttr_t LinExpAttr);
    XCamReturn getLinExpAttr(Uapi_LinExpAttr_t* pLinExpAttr);
    XCamReturn setHdrExpAttr(Uapi_HdrExpAttr_t HdrExpAttr);
    XCamReturn getHdrExpAttr (Uapi_HdrExpAttr_t* pHdrExpAttr);
    XCamReturn setLinAeDayRouteAttr(Uapi_LinAeRouteAttr_t LinAeRouteAttr);
    XCamReturn getLinAeDayRouteAttr(Uapi_LinAeRouteAttr_t* pLinAeRouteAttr);
    XCamReturn setHdrAeDayRouteAttr(Uapi_HdrAeRouteAttr_t HdrAeRouteAttr);
    XCamReturn getHdrAeDayRouteAttr(Uapi_HdrAeRouteAttr_t* pHdrAeRouteAttr);
    XCamReturn setLinAeNightRouteAttr(Uapi_LinAeRouteAttr_t LinAeRouteAttr);
    XCamReturn getLinAeNightRouteAttr(Uapi_LinAeRouteAttr_t* pLinAeRouteAttr);
    XCamReturn setHdrAeNightRouteAttr(Uapi_HdrAeRouteAttr_t HdrAeRouteAttr);
    XCamReturn getHdrAeNightRouteAttr(Uapi_HdrAeRouteAttr_t* pHdrAeRouteAttr);
    XCamReturn queryExpInfo(Uapi_ExpQueryInfo_t* pExpQueryInfo);
    XCamReturn setExpWinAttr(Uapi_ExpWin_t ExpWinAttr);
    XCamReturn getExpWinAttr(Uapi_ExpWin_t* pExpWinAttr);

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqAeHandle::deInit();
    };
private:
    Uapi_ExpSwAttr_t  mCurExpSwAttr;
    Uapi_ExpSwAttr_t  mNewExpSwAttr;
    Uapi_LinExpAttr_t mCurLinExpAttr;
    Uapi_LinExpAttr_t mNewLinExpAttr;
    Uapi_HdrExpAttr_t mCurHdrExpAttr;
    Uapi_HdrExpAttr_t mNewHdrExpAttr;
    Uapi_LinAeRouteAttr_t mCurLinAeDayRouteAttr;
    Uapi_LinAeRouteAttr_t mNewLinAeDayRouteAttr;
    Uapi_HdrAeRouteAttr_t mCurHdrAeDayRouteAttr;
    Uapi_HdrAeRouteAttr_t mNewHdrAeDayRouteAttr;
    Uapi_LinAeRouteAttr_t mCurLinAeNightRouteAttr;
    Uapi_LinAeRouteAttr_t mNewLinAeNightRouteAttr;
    Uapi_HdrAeRouteAttr_t mCurHdrAeNightRouteAttr;
    Uapi_HdrAeRouteAttr_t mNewHdrAeNightRouteAttr;
    Uapi_ExpWin_t         mCurExpWinAttr;
    Uapi_ExpWin_t         mNewExpWinAttr;
    bool updateExpSwAttr = false;
    bool updateExpHwAttr = false;
    bool updateLinExpAttr = false;
    bool updateHdrExpAttr = false;
    bool updateLinAeDayRouteAttr = false;
    bool updateHdrAeDayRouteAttr = false;
    bool updateLinAeNightRouteAttr = false;
    bool updateHdrAeNightRouteAttr = false;
    bool updateExpWinAttr = false;
};
// afd
class RkAiqAfdHandleInt:
    virtual public RkAiqAfdHandle,
    virtual public RkAiqHandleIntCom {
public:
    explicit RkAiqAfdHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore)
        , RkAiqAfdHandle(des, aiqCore)
        , RkAiqHandleIntCom(des, aiqCore) {
    };
    virtual ~RkAiqAfdHandleInt() {
        RkAiqAfdHandle::deInit();
    };
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    // TODO add algo specific methords, this is a sample

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqAfdHandle::deInit();
    };
private:
    // TODO
};


// awb
class RkAiqAwbHandleInt:
    virtual public RkAiqAwbHandle,
    virtual public RkAiqHandleIntCom {
public:
    explicit RkAiqAwbHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore)
        , RkAiqAwbHandle(des, aiqCore)
        , RkAiqHandleIntCom(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_wb_attrib_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_wb_attrib_t));
    };
    virtual ~RkAiqAwbHandleInt() {
        RkAiqAwbHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_wb_attrib_t att);
    XCamReturn getAttrib(rk_aiq_wb_attrib_t *att);
    XCamReturn getCct(rk_aiq_wb_cct_t *cct);
    XCamReturn queryWBInfo(rk_aiq_wb_querry_info_t *wb_querry_info );
    XCamReturn lock();
    XCamReturn unlock();

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqAwbHandle::deInit();
    };
private:
    // TODO
    rk_aiq_wb_attrib_t mCurAtt;
    rk_aiq_wb_attrib_t mNewAtt;
};

// af
class RkAiqAfHandleInt:
    virtual public RkAiqAfHandle,
    virtual public RkAiqHandleIntCom {
public:
    explicit RkAiqAfHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore)
        , RkAiqAfHandle(des, aiqCore)
        , RkAiqHandleIntCom(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_af_attrib_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_af_attrib_t));
    };
    virtual ~RkAiqAfHandleInt() {
        RkAiqAfHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_af_attrib_t *att);
    XCamReturn getAttrib(rk_aiq_af_attrib_t *att);
    XCamReturn lock();
    XCamReturn unlock();
    XCamReturn Oneshot();
    XCamReturn ManualTriger();
    XCamReturn Tracking();
    XCamReturn setZoomIndex(int index);
    XCamReturn getZoomIndex(int *index);
    XCamReturn endZoomChg();
    XCamReturn startZoomCalib();
    XCamReturn resetZoom();
    XCamReturn GetSearchPath(rk_aiq_af_sec_path_t* path);
    XCamReturn GetSearchResult(rk_aiq_af_result_t* result);
    XCamReturn GetFocusRange(rk_aiq_af_focusrange* range);

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqAfHandle::deInit();
    };
private:
    bool getValueFromFile(const char* path, int *pos);

    rk_aiq_af_attrib_t mCurAtt;
    rk_aiq_af_attrib_t mNewAtt;
    bool isUpdateAttDone;
    bool isUpdateZoomPosDone;
    int mLastZoomIndex;
};

class RkAiqAdebayerHandleInt:
    virtual public RkAiqAdebayerHandle,
    virtual public RkAiqHandleIntCom {
public:
    explicit RkAiqAdebayerHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore)
        , RkAiqAdebayerHandle(des, aiqCore)
        , RkAiqHandleIntCom(des, aiqCore) {};
    virtual ~RkAiqAdebayerHandleInt() {
        RkAiqAdebayerHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(adebayer_attrib_t att);
    XCamReturn getAttrib(adebayer_attrib_t *att);
protected:
    virtual void init();
    virtual void deInit() {
        RkAiqAdebayerHandle::deInit();
    };
private:
    adebayer_attrib_t mCurAtt;
    adebayer_attrib_t mNewAtt;
};

// ahdr
class RkAiqAhdrHandleInt:
    virtual public RkAiqAhdrHandle,
    virtual public RkAiqHandleIntCom {
public:
    explicit RkAiqAhdrHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore)
        , RkAiqAhdrHandle(des, aiqCore)
        , RkAiqHandleIntCom(des, aiqCore) {}
    virtual ~RkAiqAhdrHandleInt() {
        RkAiqAhdrHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    XCamReturn setAttrib(ahdr_attrib_t att);
    XCamReturn getAttrib(ahdr_attrib_t* att);
protected:
    virtual void init();
    virtual void deInit() {
        RkAiqAhdrHandle::deInit();
    };
private:
    ahdr_attrib_t mCurAtt;
    ahdr_attrib_t mNewAtt;
};

// awdr
class RkAiqAwdrHandleInt:
    virtual public RkAiqAwdrHandle,
    virtual public RkAiqHandleIntCom {
public:
    explicit RkAiqAwdrHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore)
        , RkAiqAwdrHandle(des, aiqCore)
        , RkAiqHandleIntCom(des, aiqCore) {}
    virtual ~RkAiqAwdrHandleInt() {
        RkAiqAwdrHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    XCamReturn setAttrib(awdr_attrib_t att);
    XCamReturn getAttrib(awdr_attrib_t* att);
protected:
    virtual void init();
    virtual void deInit() {
        RkAiqAwdrHandle::deInit();
    };
private:
    awdr_attrib_t mCurAtt;
    awdr_attrib_t mNewAtt;
};

class RkAiqAgicHandleInt:
    virtual public RkAiqAgicHandle,
    virtual public RkAiqHandleIntCom {
public:
    explicit RkAiqAgicHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore)
        , RkAiqAgicHandle(des, aiqCore)
        , RkAiqHandleIntCom(des, aiqCore) {};
    virtual ~RkAiqAgicHandleInt() {
        RkAiqAgicHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    XCamReturn setAttrib(agic_attrib_t att);
    XCamReturn getAttrib(agic_attrib_t *att);
protected:
    virtual void init();
    virtual void deInit() {
        RkAiqAgicHandle::deInit();
    };
private:
    agic_attrib_t mCurAtt;
    agic_attrib_t mNewAtt;
};

// adehaze
class RkAiqAdhazHandleInt:
    virtual public RkAiqAdhazHandle,
    virtual public RkAiqHandleIntCom {
public:
    explicit RkAiqAdhazHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore)
        , RkAiqAdhazHandle(des, aiqCore)
        , RkAiqHandleIntCom(des, aiqCore) {};
    virtual ~RkAiqAdhazHandleInt() {
        RkAiqAdhazHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    // TODO add algo specific methords, this is a sample
    XCamReturn setSwAttrib(adehaze_sw_s att);
    XCamReturn getSwAttrib(adehaze_sw_s *att);

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqAdhazHandle::deInit();
    };
private:
    // TODO
    adehaze_sw_t mCurAtt;
    adehaze_sw_t mNewAtt;
};


// agamma
class RkAiqAgammaHandleInt:
    virtual public RkAiqAgammaHandle,
    virtual public RkAiqHandleIntCom {
public:
    explicit RkAiqAgammaHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore)
        , RkAiqAgammaHandle(des, aiqCore)
        , RkAiqHandleIntCom(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_gamma_attrib_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_gamma_attrib_t));
    };
    virtual ~RkAiqAgammaHandleInt() {
        RkAiqAgammaHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_gamma_attrib_t att);
    XCamReturn getAttrib(rk_aiq_gamma_attrib_t *att);
    //XCamReturn queryLscInfo(rk_aiq_lsc_querry_info_t *lsc_querry_info );

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqAgammaHandle::deInit();
    };
private:
    // TODO
    rk_aiq_gamma_attrib_t mCurAtt;
    rk_aiq_gamma_attrib_t mNewAtt;
};

// adegamma
class RkAiqAdegammaHandleInt:
    virtual public RkAiqAdegammaHandle,
    virtual public RkAiqHandleIntCom {
public:
    explicit RkAiqAdegammaHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore)
        , RkAiqAdegammaHandle(des, aiqCore)
        , RkAiqHandleIntCom(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_degamma_attrib_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_degamma_attrib_t));
    };
    virtual ~RkAiqAdegammaHandleInt() {
        RkAiqAdegammaHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_degamma_attrib_t att);
    XCamReturn getAttrib(rk_aiq_degamma_attrib_t *att);
    //XCamReturn queryLscInfo(rk_aiq_lsc_querry_info_t *lsc_querry_info );

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqAdegammaHandle::deInit();
    };
private:
    // TODO
    rk_aiq_degamma_attrib_t mCurAtt;
    rk_aiq_degamma_attrib_t mNewAtt;
};

// alsc
class RkAiqAlscHandleInt:
    virtual public RkAiqAlscHandle,
    virtual public RkAiqHandleIntCom {
public:
    explicit RkAiqAlscHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore)
        , RkAiqAlscHandle(des, aiqCore)
        , RkAiqHandleIntCom(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_lsc_attrib_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_lsc_attrib_t));
    };
    virtual ~RkAiqAlscHandleInt() {
        RkAiqAlscHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_lsc_attrib_t att);
    XCamReturn getAttrib(rk_aiq_lsc_attrib_t *att);
    XCamReturn queryLscInfo(rk_aiq_lsc_querry_info_t *lsc_querry_info );

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqAlscHandle::deInit();
    };
private:
    // TODO
    rk_aiq_lsc_attrib_t mCurAtt;
    rk_aiq_lsc_attrib_t mNewAtt;
};

// accm
class RkAiqAccmHandleInt:
    virtual public RkAiqAccmHandle,
    virtual public RkAiqHandleIntCom {
public:
    explicit RkAiqAccmHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore)
        , RkAiqAccmHandle(des, aiqCore)
        , RkAiqHandleIntCom(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_ccm_attrib_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_ccm_attrib_t));
    };
    virtual ~RkAiqAccmHandleInt() {
        RkAiqAccmHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_ccm_attrib_t att);
    XCamReturn getAttrib(rk_aiq_ccm_attrib_t *att);
    XCamReturn queryCcmInfo(rk_aiq_ccm_querry_info_t *ccm_querry_info );

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqAccmHandle::deInit();
    };
private:
    // TODO
    rk_aiq_ccm_attrib_t mCurAtt;
    rk_aiq_ccm_attrib_t mNewAtt;
};

// a3dlut
class RkAiqA3dlutHandleInt:
    virtual public RkAiqA3dlutHandle,
    virtual public RkAiqHandleIntCom {
public:
    explicit RkAiqA3dlutHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore)
        , RkAiqA3dlutHandle(des, aiqCore)
        , RkAiqHandleIntCom(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_lut3d_attrib_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_lut3d_attrib_t));
    };
    virtual ~RkAiqA3dlutHandleInt() {
        RkAiqA3dlutHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_lut3d_attrib_t att);
    XCamReturn getAttrib(rk_aiq_lut3d_attrib_t *att);
    XCamReturn query3dlutInfo(rk_aiq_lut3d_querry_info_t *lut3d_querry_info );

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqA3dlutHandle::deInit();
    };
private:
    // TODO
    rk_aiq_lut3d_attrib_t mCurAtt;
    rk_aiq_lut3d_attrib_t mNewAtt;
};




class RkAiqAblcHandleInt:
    virtual public RkAiqAblcHandle,
    virtual public RkAiqHandleIntCom {
public:
    explicit RkAiqAblcHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore)
        , RkAiqAblcHandle(des, aiqCore)
        , RkAiqHandleIntCom(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_blc_attrib_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_blc_attrib_t));
    };
    virtual ~RkAiqAblcHandleInt() {
        RkAiqAblcHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_blc_attrib_t *att);
    XCamReturn getAttrib(rk_aiq_blc_attrib_t *att);

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqAblcHandle::deInit();
    };
private:
    // TODO
    rk_aiq_blc_attrib_t mCurAtt;
    rk_aiq_blc_attrib_t mNewAtt;
};




// adpcc
class RkAiqAdpccHandleInt:
    virtual public RkAiqAdpccHandle,
    virtual public RkAiqHandleIntCom {
public:
    explicit RkAiqAdpccHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore)
        , RkAiqAdpccHandle(des, aiqCore)
        , RkAiqHandleIntCom(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_dpcc_attrib_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_dpcc_attrib_t));
    };
    virtual ~RkAiqAdpccHandleInt() {
        RkAiqAdpccHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_dpcc_attrib_t *att);
    XCamReturn getAttrib(rk_aiq_dpcc_attrib_t *att);

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqAdpccHandle::deInit();
    };
private:
    // TODO
    rk_aiq_dpcc_attrib_t mCurAtt;
    rk_aiq_dpcc_attrib_t mNewAtt;
};


// anr
class RkAiqAnrHandleInt:
    virtual public RkAiqAnrHandle,
    virtual public RkAiqHandleIntCom {
public:
    explicit RkAiqAnrHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore)
        , RkAiqAnrHandle(des, aiqCore)
        , RkAiqHandleIntCom(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_nr_attrib_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_nr_attrib_t));
    };
    virtual ~RkAiqAnrHandleInt() {
        RkAiqAnrHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_nr_attrib_t *att);
    XCamReturn getAttrib(rk_aiq_nr_attrib_t *att);
    XCamReturn setLumaSFStrength(float fPercent);
    XCamReturn setLumaTFStrength(float fPercent);
    XCamReturn getLumaSFStrength(float *pPercent);
    XCamReturn getLumaTFStrength(float *pPercent);
    XCamReturn setChromaSFStrength(float fPercent);
    XCamReturn setChromaTFStrength(float fPercent);
    XCamReturn getChromaSFStrength(float *pPercent);
    XCamReturn getChromaTFStrength(float *pPercent);
    XCamReturn setRawnrSFStrength(float fPercent);
    XCamReturn getRawnrSFStrength(float *pPercent);
    XCamReturn setIQPara(rk_aiq_nr_IQPara_t *pPara);
    XCamReturn getIQPara(rk_aiq_nr_IQPara_t *pPara);
protected:
    virtual void init();
    virtual void deInit() {
        RkAiqAnrHandle::deInit();
    };
private:
    // TODO
    rk_aiq_nr_attrib_t mCurAtt;
    rk_aiq_nr_attrib_t mNewAtt;
    rk_aiq_nr_IQPara_t mCurIQpara;
    rk_aiq_nr_IQPara_t mNewIQpara;
    bool UpdateIQpara = false;
};


// anr
class RkAiqAsharpHandleInt:
    virtual public RkAiqAsharpHandle,
    virtual public RkAiqHandleIntCom {
public:
    explicit RkAiqAsharpHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore)
        , RkAiqAsharpHandle(des, aiqCore)
        , RkAiqHandleIntCom(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_sharp_attrib_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_sharp_attrib_t));
    };
    virtual ~RkAiqAsharpHandleInt() {
        RkAiqAsharpHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_sharp_attrib_t *att);
    XCamReturn getAttrib(rk_aiq_sharp_attrib_t *att);
    XCamReturn setStrength(float fPercent);
    XCamReturn getStrength(float *pPercent);
    XCamReturn setIQPara(rk_aiq_sharp_IQpara_t *para);
    XCamReturn getIQPara(rk_aiq_sharp_IQpara_t *para);

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqAsharpHandle::deInit();
    };
private:
    // TODO
    rk_aiq_sharp_attrib_t mCurAtt;
    rk_aiq_sharp_attrib_t mNewAtt;
    rk_aiq_sharp_IQpara_t mCurIQPara;
    rk_aiq_sharp_IQpara_t mNewIQPara;
    bool updateIQpara = false;

};

// afec
class RkAiqAfecHandleInt:
    virtual public RkAiqAfecHandle,
    virtual public RkAiqHandleIntCom {
public:
    explicit RkAiqAfecHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore)
        , RkAiqAfecHandle(des, aiqCore)
        , RkAiqHandleIntCom(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_fec_attrib_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_fec_attrib_t));
        mCurAtt.en = 0xff;
    };
    virtual ~RkAiqAfecHandleInt() {
        RkAiqAfecHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();

    XCamReturn setAttrib(rk_aiq_fec_attrib_t att);
    XCamReturn getAttrib(rk_aiq_fec_attrib_t *att);

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqAfecHandle::deInit();
    };
private:
    rk_aiq_fec_attrib_t mCurAtt;
    rk_aiq_fec_attrib_t mNewAtt;
};

class RkAiqAsdHandleInt:
    virtual public RkAiqAsdHandle,
    virtual public RkAiqHandleIntCom {
public:
    explicit RkAiqAsdHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore)
        , RkAiqAsdHandle(des, aiqCore)
        , RkAiqHandleIntCom(des, aiqCore) {};
    virtual ~RkAiqAsdHandleInt() {
        RkAiqAsdHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(asd_attrib_t att);
    XCamReturn getAttrib(asd_attrib_t *att);
protected:
    virtual void init();
    virtual void deInit() {
        RkAiqAsdHandle::deInit();
    };
private:
    asd_attrib_t mCurAtt;
    asd_attrib_t mNewAtt;
};

// aldch
class RkAiqAldchHandleInt:
    virtual public RkAiqAldchHandle,
    virtual public RkAiqHandleIntCom {
public:
    explicit RkAiqAldchHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore)
        , RkAiqAldchHandle(des, aiqCore)
        , RkAiqHandleIntCom(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_ldch_attrib_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_ldch_attrib_t));
    };
    virtual ~RkAiqAldchHandleInt() {
        RkAiqAldchHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();

    XCamReturn setAttrib(rk_aiq_ldch_attrib_t att);
    XCamReturn getAttrib(rk_aiq_ldch_attrib_t *att);

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqAldchHandle::deInit();
    };
private:
    rk_aiq_ldch_attrib_t mCurAtt;
    rk_aiq_ldch_attrib_t mNewAtt;
};

class RkAiqAcpHandleInt:
    virtual public RkAiqAcpHandle,
    virtual public RkAiqHandleIntCom {
public:
    explicit RkAiqAcpHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore)
        , RkAiqAcpHandle(des, aiqCore)
        , RkAiqHandleIntCom(des, aiqCore) {};
    virtual ~RkAiqAcpHandleInt() {
        RkAiqAcpHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    XCamReturn setAttrib(acp_attrib_t att);
    XCamReturn getAttrib(acp_attrib_t *att);
protected:
    virtual void init();
    virtual void deInit() {
        RkAiqAcpHandle::deInit();
    };
private:
    acp_attrib_t mCurAtt;
    acp_attrib_t mNewAtt;
};

class RkAiqAieHandleInt:
    virtual public RkAiqAieHandle,
    virtual public RkAiqHandleIntCom {
public:
    explicit RkAiqAieHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore)
        , RkAiqAieHandle(des, aiqCore)
        , RkAiqHandleIntCom(des, aiqCore) {};
    virtual ~RkAiqAieHandleInt() {
        RkAiqAieHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    XCamReturn setAttrib(aie_attrib_t att);
    XCamReturn getAttrib(aie_attrib_t *att);
protected:
    virtual void init();
    virtual void deInit() {
        RkAiqAieHandle::deInit();
    };
private:
    aie_attrib_t mCurAtt;
    aie_attrib_t mNewAtt;
};

}; //namespace RkCam

#endif
