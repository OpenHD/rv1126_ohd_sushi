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

#ifndef _RK_AIQ_HANDLE_H_
#define _RK_AIQ_HANDLE_H_

#include "rk_aiq_types.h"
#include "rk_aiq_algo_types.h"

namespace RkCam {

typedef enum custom_algo_running_mode_e {
    CUSTOM_ALGO_RUNNING_MODE_SINGLE,
    CUSTOM_ALGO_RUNNING_MODE_WITH_RKAE,
} custom_algo_runnig_mode_t;

class RkAiqCore;
class RkAiqHandle {
public:
    explicit RkAiqHandle(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore);
    virtual ~RkAiqHandle();
    void setEnable(bool enable) {
        mEnable = enable;
    };
    void setReConfig(bool reconfig) {
        mReConfig = reconfig;
    };
    bool getEnable() {
        return mEnable;
    };
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    RkAiqAlgoContext* getAlgoCtx() {
        return mAlgoCtx;
    }
    int getAlgoId() {
        return mDes->id;
    }
    virtual XCamReturn updateConfig(bool needSync) {
        return XCAM_RETURN_NO_ERROR;
    };
protected:
    virtual void init() = 0;
    virtual void deInit();
    enum {
        RKAIQ_CONFIG_COM_PREPARE,
        RKAIQ_CONFIG_COM_PRE,
        RKAIQ_CONFIG_COM_PROC,
        RKAIQ_CONFIG_COM_POST,
    };
    virtual XCamReturn configInparamsCom(RkAiqAlgoCom* com, int type);
    RkAiqAlgoCom*     mConfig;
    RkAiqAlgoCom*     mPreInParam;
    RkAiqAlgoResCom*  mPreOutParam;
    RkAiqAlgoCom*     mProcInParam;
    RkAiqAlgoResCom*  mProcOutParam;
    RkAiqAlgoCom*     mPostInParam;
    RkAiqAlgoResCom*  mPostOutParam;
    RkAiqAlgoDesComm* mDes;
    RkAiqAlgoContext* mAlgoCtx;
    RkAiqCore*        mAiqCore;
    bool              mEnable;
    bool              mReConfig;
};

#define RKAIQHANDLE(algo) \
    class RkAiq##algo##Handle: virtual public RkAiqHandle { \
    public: \
        explicit RkAiq##algo##Handle(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore) \
                    : RkAiqHandle(des, aiqCore) {}; \
        virtual ~RkAiq##algo##Handle() { deInit(); }; \
        virtual XCamReturn prepare(); \
        virtual XCamReturn preProcess(); \
        virtual XCamReturn processing(); \
        virtual XCamReturn postProcess(); \
    protected: \
        virtual void init(); \
        virtual void deInit() { RkAiqHandle::deInit(); }; \
    }

// define
RKAIQHANDLE(Ae);
RKAIQHANDLE(Afd);
RKAIQHANDLE(Awb);
RKAIQHANDLE(Af);
RKAIQHANDLE(Ahdr);
RKAIQHANDLE(Anr);
RKAIQHANDLE(Alsc);
RKAIQHANDLE(Asharp);
RKAIQHANDLE(Adhaz);
RKAIQHANDLE(Asd);
RKAIQHANDLE(Acp);
RKAIQHANDLE(A3dlut);
RKAIQHANDLE(Ablc);
RKAIQHANDLE(Accm);
RKAIQHANDLE(Acgc);
RKAIQHANDLE(Adebayer);
RKAIQHANDLE(Adpcc);
RKAIQHANDLE(Afec);
RKAIQHANDLE(Agamma);
RKAIQHANDLE(Adegamma);
RKAIQHANDLE(Agic);
RKAIQHANDLE(Aie);
RKAIQHANDLE(Aldch);
RKAIQHANDLE(Ar2y);
RKAIQHANDLE(Awdr);
RKAIQHANDLE(Aorb);

}; //namespace RkCam

#endif
