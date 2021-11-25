/*
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

#ifndef _CAM_HW_BASE_H_
#define _CAM_HW_BASE_H_

#include <map>
#include <string>
#include "xcam_thread.h"
#include "ICamHw.h"
#include "v4l2_device.h"
#include "fake_v4l2_device.h"
#include "poll_thread.h"
#ifndef RK_SIMULATOR_HW
#include "FlashLight.h"
#endif

using namespace XCam;

namespace RkCam {

class CamHwBase
    : public ICamHw
    , public PollCallback {
public:
    explicit CamHwBase();
    virtual ~CamHwBase();
    // from ICamHw
    virtual XCamReturn init(const char* sns_ent_name);
    virtual XCamReturn deInit();
    virtual void setCalib(const CamCalibDbContext_t* calib) {
        mCalibDb = calib;
    }
    virtual XCamReturn prepare(uint32_t width, uint32_t height, int mode, int t_dealy, int g_delay);
    virtual XCamReturn start();
    virtual XCamReturn stop();
    virtual XCamReturn pause();
    virtual XCamReturn resume();
    virtual XCamReturn swWorkingModeDyn(int mode) {
        return XCAM_RETURN_NO_ERROR;
    };
    virtual void keepHwStAtStop(bool ks) {
        mKpHwSt = ks;
    };
    virtual XCamReturn getSensorModeData(const char* sns_ent_name,
                                         rk_aiq_exposure_sensor_descriptor& sns_des);
    virtual XCamReturn setIspMeasParams(SmartPtr<RkAiqIspMeasParamsProxy>& ispMeasParams);
    virtual XCamReturn setIspOtherParams(SmartPtr<RkAiqIspOtherParamsProxy>& ispOtherParams);
    virtual XCamReturn setExposureParams(SmartPtr<RkAiqExpParamsProxy>& expPar);
    virtual XCamReturn setIrisParams(SmartPtr<RkAiqIrisParamsProxy>& irisPar, CalibDb_IrisType_t irisType);
    virtual XCamReturn setFocusParams(SmartPtr<RkAiqFocusParamsProxy>& focus_params);
    virtual XCamReturn setCpslParams(SmartPtr<RkAiqCpslParamsProxy>& cpsl_params);
    virtual XCamReturn setIsppMeasParams(SmartPtr<RkAiqIsppMeasParamsProxy>& isppMeasParams);
    virtual XCamReturn setIsppOtherParams(SmartPtr<RkAiqIsppOtherParamsProxy>& isppOtherParams);
    virtual XCamReturn setIsppStatsListener(IsppStatsListener* isppStatsListener);
    virtual XCamReturn setIspLumaListener(IspLumaListener* lumaListener);
    virtual XCamReturn setIspStatsListener(IspStatsListener* statsListener);
    virtual XCamReturn setEvtsListener(IspEvtsListener* evtListener);
    virtual XCamReturn setIspTxBufListener(IspTxBufListener* txBufListener);
    virtual XCamReturn setHdrProcessCount(rk_aiq_luma_params_t luma_params) {
        return XCAM_RETURN_ERROR_FAILED;
    };
    // from PollCallback
    virtual XCamReturn poll_buffer_ready (SmartPtr<VideoBuffer> &buf, int type);
    virtual XCamReturn poll_buffer_failed (int64_t timestamp, const char *msg);
    virtual XCamReturn poll_event_ready (uint32_t sequence, int type);
    virtual XCamReturn poll_event_failed (int64_t timestamp, const char *msg);
    virtual XCamReturn notify_capture_raw() {
        return  XCAM_RETURN_ERROR_FAILED;
    }
    virtual XCamReturn capture_raw_ctl(capture_raw_t type, int count = 0,
                                       const char* capture_dir = nullptr, \
                                       char* output_dir = nullptr) {
        return  XCAM_RETURN_ERROR_FAILED;
    }
    virtual XCamReturn enqueueRawBuffer(void *vbuf, bool sync) {
        return  XCAM_RETURN_ERROR_FAILED;
    }
    virtual XCamReturn enqueueRawFile(const char *path) {
        return  XCAM_RETURN_ERROR_FAILED;
    }
    virtual XCamReturn registRawdataCb(void (*callback)(void *)) {
        return  XCAM_RETURN_ERROR_FAILED;
    }
    virtual XCamReturn rawdataPrepare(rk_aiq_raw_prop_t prop) {
        return  XCAM_RETURN_ERROR_FAILED;
    }
    virtual XCamReturn setSensorFlip(bool mirror, bool flip, int skip_frm_cnt) {
        return  XCAM_RETURN_ERROR_FAILED;
    }
    virtual XCamReturn getSensorFlip(bool& mirror, bool& flip) {
        return  XCAM_RETURN_ERROR_FAILED;
    }
    virtual XCamReturn getZoomPosition(int& position) {
        return  XCAM_RETURN_ERROR_FAILED;
    }
    virtual XCamReturn setLensVcmCfg(rk_aiq_lens_vcmcfg& lens_cfg) {
        return  XCAM_RETURN_ERROR_FAILED;
    }
    virtual XCamReturn getLensVcmCfg(rk_aiq_lens_vcmcfg& lens_cfg) {
        return  XCAM_RETURN_ERROR_FAILED;
    }
    virtual XCamReturn FocusCorrection() {
        return  XCAM_RETURN_ERROR_FAILED;
    }
    virtual XCamReturn ZoomCorrection() {
        return  XCAM_RETURN_ERROR_FAILED;
    }
    virtual void getShareMemOps(isp_drv_share_mem_ops_t** mem_ops) {};
    virtual XCamReturn getEffectiveIspParams(SmartPtr<RkAiqIspMeasParamsProxy>& ispParams, int frame_id) {
        return  XCAM_RETURN_ERROR_FAILED;
    };
    XCamReturn setIspParamsSync(int frameId) {
        return  XCAM_RETURN_ERROR_FAILED;
    };
    XCamReturn setIsppParamsSync(int frameId) {
        return  XCAM_RETURN_ERROR_FAILED;
    };
    uint64_t getIspModuleEnState() {
        return true;
    };
    virtual XCamReturn getEffectiveExpParams(uint32_t id, SmartPtr<RkAiqExpParamsProxy>& ExpParams) {
        return  XCAM_RETURN_ERROR_FAILED;
    };
protected:
    SmartPtr<V4l2Device> mIsppStatsDev;
    SmartPtr<V4l2Device> mIsppParamsDev;
    SmartPtr<V4l2Device> mIspLumaDev;
    SmartPtr<V4l2Device> mIspStatsDev;
    SmartPtr<V4l2Device> mIspParamsDev;
    SmartPtr<V4l2SubDevice> mIspCoreDev;
    SmartPtr<V4l2SubDevice> mSensorDev;
    SmartPtr<V4l2SubDevice> mLensDev;
    SmartPtr<V4l2SubDevice> mIrcutDev;
    SmartPtr<V4l2Device> mIspSpDev;
#ifndef RK_SIMULATOR_HW
    SmartPtr<FlashLightHw> mFlashLight;
    SmartPtr<FlashLightHw> mFlashLightIr;
#endif
    SmartPtr<PollThread> mPollthread;
    SmartPtr<PollThread> mPollLumathread;
    SmartPtr<PollThread> mPollIsppthread;
    IsppStatsListener* mIsppStatsListener;
    IspLumaListener* mIspLumaListener;
    IspStatsListener* mIspStatsLintener;
    IspEvtsListener* mIspEvtsListener;
    IspTxBufListener* mIspTxBufLintener;
    int mWorkingMode;
    const CamCalibDbContext_t* mCalibDb;
    bool mKpHwSt;
private:
    XCAM_DEAD_COPY (CamHwBase);
};

}; //namespace RkCam

#endif
