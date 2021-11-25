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

#ifndef _ICAMHW_H_
#define _ICAMHW_H_

#include "video_buffer.h"
#include "smartptr.h"
#include "rk_aiq_types.h"
#include "rk_aiq_pool.h"

namespace RkCam {

using namespace XCam;

typedef struct ispHwEvt_s {
    virtual ~ispHwEvt_s() {};
    int evt_code;
    union {

    } msg;
} ispHwEvt_t;

class IsppStatsListener {
public:
    IsppStatsListener() {};
    virtual ~IsppStatsListener() {};
    virtual XCamReturn isppStatsCb(SmartPtr<VideoBuffer>& isppStats) = 0;
private:
    XCAM_DEAD_COPY (IsppStatsListener);
};

class IspLumaListener {
public:
    IspLumaListener() {};
    virtual ~IspLumaListener() {};
    virtual XCamReturn ispLumaCb(SmartPtr<VideoBuffer>& ispLuma) = 0;
private:
    XCAM_DEAD_COPY (IspLumaListener);
};

class IspStatsListener {
public:
    IspStatsListener() {};
    virtual ~IspStatsListener() {};
    virtual XCamReturn ispStatsCb(SmartPtr<VideoBuffer>& ispStats) = 0;
private:
    XCAM_DEAD_COPY (IspStatsListener);
};

class IspEvtsListener {
public:
    IspEvtsListener() {};
    virtual ~IspEvtsListener() {};
    virtual XCamReturn ispEvtsCb(SmartPtr<ispHwEvt_t> evt) = 0;
private:
    XCAM_DEAD_COPY (IspEvtsListener);
};

class IspTxBufListener {
public:
    IspTxBufListener () {};
    virtual ~IspTxBufListener () {};
    virtual XCamReturn ispTxBufCb(SmartPtr<VideoBuffer>& ispTxBuf) = 0;
private:
    XCAM_DEAD_COPY (IspTxBufListener);
};

class ICamHw {
public:
    ICamHw() {};
    virtual ~ICamHw() {};
    virtual XCamReturn init(const char* sns_ent_name) = 0;
    virtual XCamReturn deInit() = 0;
    virtual void setCalib(const CamCalibDbContext_t* calib) = 0;
    virtual XCamReturn prepare(uint32_t width, uint32_t height, int mode, int t_delay, int g_delay) = 0;
    virtual XCamReturn start() = 0;
    virtual XCamReturn stop() = 0;
    virtual XCamReturn pause() = 0;
    virtual XCamReturn resume() = 0;
    virtual void keepHwStAtStop(bool ks) = 0;
    virtual XCamReturn swWorkingModeDyn(int mode) = 0;
    virtual XCamReturn getSensorModeData(const char* sns_ent_name,
                                         rk_aiq_exposure_sensor_descriptor& sns_des) = 0;
    virtual XCamReturn setIspMeasParams(SmartPtr<RkAiqIspMeasParamsProxy>& ispMeasParams) = 0;
    virtual XCamReturn setIspOtherParams(SmartPtr<RkAiqIspOtherParamsProxy>& ispOtherParams) = 0;
    virtual XCamReturn setHdrProcessCount(rk_aiq_luma_params_t luma_params) = 0;
    virtual XCamReturn setExposureParams(SmartPtr<RkAiqExpParamsProxy>& expPar) = 0;
    virtual XCamReturn setIrisParams(SmartPtr<RkAiqIrisParamsProxy>& irisPar, CalibDb_IrisType_t irisType) = 0;
    virtual XCamReturn setFocusParams(SmartPtr<RkAiqFocusParamsProxy>& focus_params) = 0;
    virtual XCamReturn setCpslParams(SmartPtr<RkAiqCpslParamsProxy>& cpsl_params) = 0;
    virtual XCamReturn setIsppMeasParams(SmartPtr<RkAiqIsppMeasParamsProxy>& isppParams) = 0;
    virtual XCamReturn setIsppOtherParams(SmartPtr<RkAiqIsppOtherParamsProxy>& isppOtherParams) = 0;
    virtual XCamReturn setIsppStatsListener(IsppStatsListener* isppStatsListener) = 0;
    virtual XCamReturn setIspLumaListener(IspLumaListener* lumaListener) = 0;
    virtual XCamReturn setIspStatsListener(IspStatsListener* statsListener) = 0;
    virtual XCamReturn setEvtsListener(IspEvtsListener* evtListener) = 0;
    virtual XCamReturn setIspTxBufListener(IspTxBufListener* txBufListener) = 0;
    virtual XCamReturn setModuleCtl(rk_aiq_module_id_t mId, bool mod_en) = 0;
    virtual XCamReturn getModuleCtl(rk_aiq_module_id_t mId, bool& mod_en) = 0;
    virtual XCamReturn notify_capture_raw() = 0;
    virtual XCamReturn capture_raw_ctl(capture_raw_t type, int count = 0,
                                       const char* capture_dir = nullptr,
                                       char* output_dir = nullptr) = 0;
    virtual XCamReturn enqueueRawBuffer(void *vbuf, bool sync) = 0;
    virtual XCamReturn enqueueRawFile(const char *path) = 0;
    virtual XCamReturn registRawdataCb(void (*callback)(void *)) = 0;
    virtual XCamReturn rawdataPrepare(rk_aiq_raw_prop_t prop) = 0;
    virtual XCamReturn setSensorFlip(bool mirror, bool flip, int skip_frm_cnt) = 0;
    virtual XCamReturn getSensorFlip(bool& mirror, bool& flip) = 0;
    virtual XCamReturn getSensorCrop(rk_aiq_rect_t& rect) = 0;
    virtual XCamReturn setSensorCrop(rk_aiq_rect_t& rect) = 0;
    virtual XCamReturn getZoomPosition(int& position) = 0;
    virtual XCamReturn getLensVcmCfg(rk_aiq_lens_vcmcfg& lens_cfg) = 0;
    virtual XCamReturn setLensVcmCfg(rk_aiq_lens_vcmcfg& lens_cfg) = 0;
    virtual XCamReturn FocusCorrection() = 0;
    virtual XCamReturn ZoomCorrection() = 0;
    virtual void getShareMemOps(isp_drv_share_mem_ops_t** mem_ops) = 0;
    virtual XCamReturn getEffectiveIspParams(SmartPtr<RkAiqIspMeasParamsProxy>& ispParams, int frame_id) = 0;
    virtual XCamReturn setIspParamsSync(int frameId) = 0;
    virtual XCamReturn setIsppParamsSync(int frameId) = 0;
    virtual uint64_t getIspModuleEnState() = 0;
    virtual XCamReturn getEffectiveExpParams(uint32_t id, SmartPtr<RkAiqExpParamsProxy>& ExpParams) = 0;
private:
    XCAM_DEAD_COPY (ICamHw);
};

}; //namespace RkCam

#endif
