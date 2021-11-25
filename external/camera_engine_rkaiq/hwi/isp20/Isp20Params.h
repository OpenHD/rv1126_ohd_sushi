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

#ifndef _CAM_HW_ISP20_PARAMS_H_
#define _CAM_HW_ISP20_PARAMS_H_

#include "rk_aiq_pool.h"
#include "rkisp2-config.h"
#include "rkispp-config.h"

namespace RkCam {

#define ISP20PARAM_SUBM (0x2)

typedef struct AntiTmoFlicker_s {
    int preFrameNum;
    bool FirstChange;
    int FirstChangeNum;
    bool FirstChangeDone;
    int FirstChangeDoneNum;
} AntiTmoFlicker_t;

class Isp20Params {
public:
    explicit Isp20Params() : _last_pp_module_init_ens(0)
        , _force_isp_module_ens(0)
        , _force_ispp_module_ens(0)
        , _force_module_flags(0)
    {   AntiTmoFlicker.preFrameNum = 0;
        AntiTmoFlicker.FirstChange = false;
        AntiTmoFlicker.FirstChangeNum = 0;
        AntiTmoFlicker.FirstChangeDone = false;
        AntiTmoFlicker.FirstChangeDoneNum = 0;
    };
    virtual ~Isp20Params() {};

    virtual XCamReturn checkIsp20Params(struct isp2x_isp_params_cfg& isp_cfg);
    virtual XCamReturn convertAiqMeasResultsToIsp20Params(struct isp2x_isp_params_cfg& isp_cfg,
            SmartPtr<RkAiqIspMeasParamsProxy> aiq_results,
            SmartPtr<RkAiqIspOtherParamsProxy> aiq_other_results,
            SmartPtr<RkAiqIspMeasParamsProxy>& last_aiq_results);
    virtual XCamReturn convertAiqOtherResultsToIsp20Params(struct isp2x_isp_params_cfg& isp_cfg,
            SmartPtr<RkAiqIspOtherParamsProxy> aiq_results,
            SmartPtr<RkAiqIspOtherParamsProxy>& last_aiq_results);
    virtual XCamReturn convertAiqMeasResultsToIsp20PpParams(struct rkispp_params_cfg& pp_cfg,
            SmartPtr<RkAiqIsppMeasParamsProxy> aiq_results,
            SmartPtr<RkAiqIsppMeasParamsProxy> &last_aiq_results);
    virtual XCamReturn convertAiqOtherResultsToIsp20PpParams(struct rkispp_params_cfg& pp_cfg,
            SmartPtr<RkAiqIsppOtherParamsProxy> aiq_results,
            SmartPtr<RkAiqIsppOtherParamsProxy> &last_aiq_results);
    void set_working_mode(int mode);
    void setModuleStatus(rk_aiq_module_id_t mId, bool en);
    void getModuleStatus(rk_aiq_module_id_t mId, bool& en);
    void hdrtmoGetLumaInfo(rk_aiq_luma_params_t * Next, rk_aiq_luma_params_t *Cur,
                           s32 frameNum, s32 PixelNumBlock, float blc, float *luma);
    void hdrtmoGetAeInfo(RKAiqAecExpInfo_t* Next, RKAiqAecExpInfo_t* Cur, s32 frameNum, float* expo);
    s32 hdrtmoPredictK(float* luma, float* expo, s32 frameNum, PredictKPara_t *TmoPara);
    bool hdrtmoSceneStable(sint32_t frameId, int IIRMAX, int IIR, int SetWeight, s32 frameNum, float *LumaDeviation, float StableThr);
    void forceOverwriteAiqIsppCfg(struct rkispp_params_cfg& pp_cfg,
                                  SmartPtr<RkAiqIsppMeasParamsProxy> aiq_meas_results,
                                  SmartPtr<RkAiqIsppOtherParamsProxy> aiq_other_results);
    void forceOverwriteAiqIspCfg(struct isp2x_isp_params_cfg& isp_cfg,
                                 SmartPtr<RkAiqIspMeasParamsProxy> aiq_results,
                                 SmartPtr<RkAiqIspOtherParamsProxy> aiq_other_results);
private:
    XCAM_DEAD_COPY(Isp20Params);
    void convertAiqAeToIsp20Params(struct isp2x_isp_params_cfg& isp_cfg,
                                   const rk_aiq_isp_aec_meas_t& aec_meas);
    void convertAiqHistToIsp20Params(struct isp2x_isp_params_cfg& isp_cfg,
                                     const rk_aiq_isp_hist_meas_t& hist_meas);
    void convertAiqAwbToIsp20Params(struct isp2x_isp_params_cfg& isp_cfg,
                                    const rk_aiq_awb_stat_cfg_v200_t& awb_meas,
                                    bool awb_cfg_udpate);
    void convertAiqAwbGainToIsp20Params(struct isp2x_isp_params_cfg& isp_cfg,
                                        const rk_aiq_wb_gain_t& awb_gain, const rk_aiq_isp_blc_t &blc,
                                        bool awb_gain_update);
    void convertAiqMergeToIsp20Params(struct isp2x_isp_params_cfg& isp_cfg,
                                      const rk_aiq_isp_hdr_t& ahdr_data);
    void convertAiqTmoToIsp20Params(struct isp2x_isp_params_cfg& isp_cfg,
                                    const rk_aiq_isp_hdr_t& ahdr_data);
    void convertAiqAdehazeToIsp20Params(struct isp2x_isp_params_cfg& isp_cfg,
                                        const rk_aiq_dehaze_cfg_t& dhaze                     );
    void convertAiqAgammaToIsp20Params(struct isp2x_isp_params_cfg& isp_cfg,
                                       const AgammaProcRes_t& gamma_out_cfg);
    void convertAiqAdegammaToIsp20Params(struct isp2x_isp_params_cfg& isp_cfg,
                                         const AdegammaProcRes_t& degamma_cfg);
    void convertAiqAdemosaicToIsp20Params(struct isp2x_isp_params_cfg& isp_cfg,
                                          SmartPtr<RkAiqIspOtherParamsProxy> aiq_results);
    void convertAiqLscToIsp20Params(struct isp2x_isp_params_cfg& isp_cfg,
                                    const rk_aiq_lsc_cfg_t& lsc);
    void convertAiqBlcToIsp20Params(struct isp2x_isp_params_cfg& isp_cfg,
                                    SmartPtr<RkAiqIspOtherParamsProxy> aiq_results);
    void convertAiqDpccToIsp20Params(struct isp2x_isp_params_cfg& isp_cfg,
                                     SmartPtr<RkAiqIspMeasParamsProxy> aiq_results);
    void convertAiqCcmToIsp20Params(struct isp2x_isp_params_cfg& isp_cfg,
                                    const rk_aiq_ccm_cfg_t& ccm);
    void convertAiqA3dlutToIsp20Params(struct isp2x_isp_params_cfg& isp_cfg,
                                       const rk_aiq_lut3d_cfg_t& lut3d_cfg);
    void convertAiqCpToIsp20Params(struct isp2x_isp_params_cfg& isp_cfg,
                                   const rk_aiq_acp_params_t& lut3d_cfg);
    void convertAiqWdrToIsp20Params(struct isp2x_isp_params_cfg& isp_cfg,
                                    const rk_aiq_isp_wdr_t& wdr_cfg);
    void convertAiqIeToIsp20Params(struct isp2x_isp_params_cfg& isp_cfg,
                                   const rk_aiq_isp_ie_t& ie_cfg);
    void convertAiqRawnrToIsp20Params(struct isp2x_isp_params_cfg& isp_cfg,
                                      rk_aiq_isp_rawnr_t& rawnr);
    void convertAiqTnrToIsp20Params(struct rkispp_params_cfg& pp_cfg,
                                    rk_aiq_isp_tnr_t& tnr);
    void convertAiqUvnrToIsp20Params(struct rkispp_params_cfg& pp_cfg,
                                     rk_aiq_isp_uvnr_t& uvnr);
    void convertAiqYnrToIsp20Params(struct rkispp_params_cfg& pp_cfg,
                                    rk_aiq_isp_ynr_t& ynr);
    void convertAiqSharpenToIsp20Params(struct rkispp_params_cfg& pp_cfg,
                                        rk_aiq_isp_sharpen_t& sharp, rk_aiq_isp_edgeflt_t& edgeflt);
    void convertAiqAfToIsp20Params(struct isp2x_isp_params_cfg& isp_cfg,
                                   const rk_aiq_isp_af_meas_t& af_data, bool af_cfg_udpate);
    void convertAiqGainToIsp20Params(struct isp2x_isp_params_cfg& isp_cfg,
                                     rk_aiq_isp_gain_t& gain);
    void convertAiqAldchToIsp20Params(struct isp2x_isp_params_cfg& isp_cfg,
                                      const rk_aiq_isp_ldch_t& ldch_cfg);
    void convertAiqFecToIsp20Params(struct rkispp_params_cfg& pp_cfg,
                                    rk_aiq_isp_fec_t& fec);
    void convertAiqGicToIsp20Params(struct isp2x_isp_params_cfg& isp_cfg,
                                    const rk_aiq_isp_gic_t& gic_cfg);
    void convertAiqOrbToIsp20Params(struct rkispp_params_cfg& pp_cfg,
                                    rk_aiq_isp_orb_t& orb);
    bool getModuleForceFlag(int module_id);
    void setModuleForceFlagInverse(int module_id);
    bool getModuleForceEn(int module_id);
    void updateIspModuleForceEns(u64 module_ens);
    void updateIsppModuleForceEns(u32 module_ens);
    uint32_t _last_pp_module_init_ens;
    u64 _force_isp_module_ens;
    u32 _force_ispp_module_ens;
    u64 _force_module_flags;
    int _working_mode;
    AntiTmoFlicker_t AntiTmoFlicker;
    Mutex _mutex;
};
};
#endif
