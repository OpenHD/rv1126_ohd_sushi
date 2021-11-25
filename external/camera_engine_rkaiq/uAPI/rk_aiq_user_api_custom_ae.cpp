/*
 *  Copyright (c) 2021 Rockchip Corporation
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

#include "rk_aiq_user_api_custom_ae.h"

#ifdef RK_SIMULATOR_HW
#define CHECK_USER_API_ENABLE
#endif

RKAIQ_BEGIN_DECLARE

#define RKISP_ALGO_AE_DEMO_VERSION     "v0.0.1"
#define RKISP_ALGO_AE_DEMO_VENDOR      "Rockchip"
#define RKISP_ALGO_AE_DEMO_DESCRIPTION "Rockchip Custom Ae"

typedef struct rk_aiq_rkAe_config_s {
    int Working_mode;//values look up in rk_aiq_working_mode_t definiton
    int RawWidth;
    int RawHeight;
    CalibDb_Sensor_Para_t stSensorInfo;
    CalibDb_Dcg_t         stDcgInfo;
    RkAiqAecHwConfig_t    aeHwConfig;
    rk_aiq_sensor_nr_switch_t stNRswitch;
    float        LinePeriodsPerField;
    float        PixelClockFreqMHZ;
    float        PixelPeriodsPerLine;
} rk_aiq_rkAe_config_t;

/* instance was created by AIQ framework when rk_aiq_uapi_sysctl_regLib called */
typedef struct _RkAiqAlgoContext {
    rk_aiq_customeAe_cbs_t cbs; // set by register
    rk_aiq_sys_ctx_t* aiq_ctx;  // set by register
    rk_aiq_rkAe_config_t rkCfg; // ae config of AIQ framework
    rk_aiq_customeAe_results_t customRes; // result of pfn_ae_run
    CamCalibDbContext_t* pcalib;
    bool cutomAeInit;
    bool updateCalib;
} RkAiqAlgoContext;

#if 1
/******************************************************************************
 * AeReg2RealConv()
 *****************************************************************************/
static XCamReturn AeReg2RealConv
(
    rk_aiq_rkAe_config_t* pConfig,
    int sensorGain,
    int sensorInttime,
    int sensorDcgmode,
    float& realGain,
    float& realInttime
) {

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    //gain
    if(pConfig->stSensorInfo.GainRange.GainMode == RKAIQ_EXPGAIN_MODE_LINEAR) {
        float gainRange[] = {1, 2, 128, 0, 1, 128, 255,
                             2, 4, 64, -248, 1, 376, 504,
                             4, 8, 32, -756, 1, 884, 1012,
                             8, 16, 16, -1784, 1, 1912, 2040
                            };

        float* pgainrange = NULL;
        uint32_t size = 0;

        if (pConfig->stSensorInfo.GainRange.array_size <= 0) {
            pgainrange = gainRange;
            size = sizeof(gainRange) / sizeof(float);
        } else {
            pgainrange = pConfig->stSensorInfo.GainRange.pGainRange;
            size = pConfig->stSensorInfo.GainRange.array_size;
        }

        int *revert_gain_array = (int *)malloc((size / 7 * 2) * sizeof(int));
        if(revert_gain_array == NULL) {
            LOGE_AEC("%s: malloc fail", __func__);
            return  XCAM_RETURN_ERROR_MEM;
        }

        for(uint32_t i = 0; i < (size / 7); i++) {
            revert_gain_array[i * 2 + 0] = (int)(pgainrange[i * 7 + 2] * pow(pgainrange[i * 7 + 0], pgainrange[i * 7 + 4]) - pgainrange[i * 7 + 3] + 0.5f);
            revert_gain_array[i * 2 + 1] = (int)(pgainrange[i * 7 + 2] * pow(pgainrange[i * 7 + 1], pgainrange[i * 7 + 4]) - pgainrange[i * 7 + 3] + 0.5f);
        }

        // AG = (C1 * (analog gain^M0) - C0) + 0.5f
        float C1 = 0.0f, C0 = 0.0f, M0 = 0.0f, minReg = 0.0f, maxReg = 0.0f, minrealGain = 0.0f, maxrealGain = 0.0f;
        float ag = sensorGain;
        uint32_t i = 0;
        for(i = 0; i < (size / 7); i++) {
            if (ag >= revert_gain_array[i * 2 + 0] && ag <= revert_gain_array[i * 2 + 1]) {
                C1 = pgainrange[i * 7 + 2];
                C0 = pgainrange[i * 7 + 3];
                M0 = pgainrange[i * 7 + 4];
                minReg = pgainrange[i * 7 + 5];
                maxReg = pgainrange[i * 7 + 6];
#if 0
                LOGE("gain(%f) c1:%f c0:%f m0:%f min:%f max:%f", ag, C1, C0, M0, minReg, maxReg);
#endif
                break;
            }
        }

        if(i > (size / 7)) {
            LOGE_AEC_SUBM(0xff, "GAIN OUT OF RANGE: lasttime-gain: %d-%d", sensorInttime, sensorGain);
            C1 = 16;
            C0 = 0;
            M0 = 1;
            minReg = 16;
            maxReg = 255;
        }

        realGain = pow(10, log10(((float)sensorGain + C0) / C1) / M0);
        minrealGain = pow(10, log10(((float)minReg + C0) / C1) / M0);
        maxrealGain = pow(10, log10(((float)maxReg + C0) / C1) / M0);

        if (realGain < minrealGain)
            realGain = minrealGain;
        if (realGain > maxrealGain)
            realGain = maxrealGain;

        if(revert_gain_array != NULL) {
            free(revert_gain_array);
            revert_gain_array = NULL;
        }
    } else if(pConfig->stSensorInfo.GainRange.GainMode == RKAIQ_EXPGAIN_MODE_NONLINEAR_DB) {
        realGain = pow(10, (float)sensorGain * 3 / (10.0f * 20.0f));
    }

    float dcg_ratio = (sensorDcgmode >= 1 ? pConfig->stDcgInfo.Normal.dcg_ratio : 1.0f);

    realGain *= dcg_ratio;

    //time
    float timeC0 = pConfig->stSensorInfo.TimeFactor[0];
    float timeC1 = pConfig->stSensorInfo.TimeFactor[1];
    float timeC2 = pConfig->stSensorInfo.TimeFactor[2];
    float timeC3 = pConfig->stSensorInfo.TimeFactor[3];

    realInttime = ((sensorInttime - timeC0 * pConfig->LinePeriodsPerField - timeC1) / timeC2 /*- timeC3*/) *
                  pConfig->PixelPeriodsPerLine / (pConfig->PixelClockFreqMHZ * 1000000);

    return (ret);
}

/******************************************************************************
 * AeReal2RegConv()
 *****************************************************************************/
static XCamReturn AeReal2RegConv
(
    rk_aiq_rkAe_config_t* pConfig,
    float SplitIntegrationTime,
    float SplitGain,
    unsigned int *regIntegrationTime,
    unsigned int *regGain,
    int *pDcgMode
) {
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    float C1 = 0.0f, C0 = 0.0f, M0 = 0.0f, minReg = 0.0f, maxReg = 0.0f, ag = 0.0f;
    int i;

    float SplitGainIn = 0.0f, SplitIntegrationTimeIn = 0.0f;

    SplitGainIn = SplitGain;
    SplitIntegrationTimeIn = SplitIntegrationTime;

    //gain convertion
    ag = SplitGainIn / (pDcgMode[AEC_NORMAL_FRAME] >= 1 ? pConfig->stDcgInfo.Normal.dcg_ratio : 1.0f);

    if(pConfig->stSensorInfo.GainRange.GainMode == RKAIQ_EXPGAIN_MODE_LINEAR) {
        for (i = 0; i < pConfig->stSensorInfo.GainRange.array_size; i += 7) {
            if (ag >= pConfig->stSensorInfo.GainRange.pGainRange[i] && ag <= pConfig->stSensorInfo.GainRange.pGainRange[i + 1]) {
                C1 = pConfig->stSensorInfo.GainRange.pGainRange[i + 2];
                C0 = pConfig->stSensorInfo.GainRange.pGainRange[i + 3];
                M0 = pConfig->stSensorInfo.GainRange.pGainRange[i + 4];
                minReg = pConfig->stSensorInfo.GainRange.pGainRange[i + 5];
                maxReg = pConfig->stSensorInfo.GainRange.pGainRange[i + 6];
                break;
            }
        }

        if (C1 == 0.0f) {
            LOGE_AEC_SUBM(0xff, "GAIN OUT OF RANGE: lasttime-gain: %f-%f", SplitIntegrationTimeIn, SplitGainIn);
            C1 = 16;
            C0 = 0;
            M0 = 1;
            minReg = 16;
            maxReg = 255;
        }


        LOGV_AEC_SUBM(0xff, "ag: %2.2f, C1: %2.2f  C0: %2.2f M0: %2.2f, minReg: %2.2f maxReg: %2.2f",
                      ag, C1, C0, M0, minReg, maxReg);

        *regGain = (int)(C1 * pow(ag, M0) - C0 + 0.5f);
        if (*regGain < minReg)
            *regGain = minReg;
        if (*regGain > maxReg)
            *regGain = maxReg;

    } else if(pConfig->stSensorInfo.GainRange.GainMode == RKAIQ_EXPGAIN_MODE_NONLINEAR_DB) {
        *regGain = (int)(20.0f * log10(ag) * 10.0f / 3.0f + 0.5f);
    }

    //time convertion
    float timeC0 = pConfig->stSensorInfo.TimeFactor[0];
    float timeC1 = pConfig->stSensorInfo.TimeFactor[1];
    float timeC2 = pConfig->stSensorInfo.TimeFactor[2];
    float timeC3 = pConfig->stSensorInfo.TimeFactor[3];
    LOGV_AEC_SUBM(0xff, "time coefficient: %f-%f-%f-%f", timeC0, timeC1, timeC2, timeC3);

    float pclk = pConfig->PixelClockFreqMHZ;
    float hts = pConfig->PixelPeriodsPerLine;
    float vts = pConfig->LinePeriodsPerField;

    *regIntegrationTime = (int)(timeC0 * vts + timeC1 + timeC2 * ((SplitIntegrationTimeIn * pclk * 1000000 / hts) + timeC3));


    int Index = (*regIntegrationTime - pConfig->stSensorInfo.CISTimeRegOdevity.fCoeff[1]) / (pConfig->stSensorInfo.CISTimeRegOdevity.fCoeff[0]);
    *regIntegrationTime = pConfig->stSensorInfo.CISTimeRegOdevity.fCoeff[0] * Index + pConfig->stSensorInfo.CISTimeRegOdevity.fCoeff[1];
    *regIntegrationTime = MAX(*regIntegrationTime, pConfig->stSensorInfo.CISTimeRegMin);

    return (ret);
}

/******************************************************************************
 * AeDcgConv
 *****************************************************************************/
static XCamReturn AeDcgConv
(
    rk_aiq_rkAe_config_t*  pConfig,
    float                  Gain,
    int*                   pDcgMode
) {
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    LOG1_AEC_SUBM(0xff, "%s:(enter)\n", __FUNCTION__);

    //pointer check
    if (pConfig == NULL) {
        LOGE_AEC_SUBM(0xff, "%s: pConfig NULL pointer! \n", __FUNCTION__);
        return (XCAM_RETURN_ERROR_FAILED);
    }

    if(!pConfig->stDcgInfo.Normal.support_en) {
        *pDcgMode = -1;
        return XCAM_RETURN_NO_ERROR;
    }

    if(pConfig->stDcgInfo.Normal.dcg_optype <= RK_AIQ_OP_MODE_AUTO) {

        if(Gain >= pConfig->stDcgInfo.Normal.lcg2hcg_gain_th) {
            *pDcgMode = 1;
        } else if(Gain < pConfig->stDcgInfo.Normal.hcg2lcg_gain_th) {
            *pDcgMode = 0;
        }

        LOG1_AEC_SUBM(0xff, "gain=%f,dcg_mode=[%d]", Gain, *pDcgMode);
    }

    LOG1_AEC_SUBM(0xff, "%s: (exit)\n", __FUNCTION__);
    return (ret);
}

#endif
static XCamReturn AeDemoCreateCtx(RkAiqAlgoContext **context, const AlgoCtxInstanceCfg* cfg)
{
    LOGD_AEC_SUBM(0xff, "%s ENTER", __func__);

    RESULT ret = RK_AIQ_RET_SUCCESS;
    RkAiqAlgoContext *ctx = new RkAiqAlgoContext();
    if (ctx == NULL) {
        printf( "%s: create ae context fail!\n", __FUNCTION__);
        return XCAM_RETURN_ERROR_MEM;
    }
    memset(ctx, 0, sizeof(*ctx));

    *context = ctx;
    LOGD_AEC_SUBM(0xff, "%s EXIT", __func__);

    return XCAM_RETURN_NO_ERROR;
}

static XCamReturn AeDemoDestroyCtx(RkAiqAlgoContext *context)
{
    LOGD_AEC_SUBM(0xff, "%s ENTER", __func__);

    if(context == NULL)
        return XCAM_RETURN_NO_ERROR;

    if (context->cbs.pfn_ae_exit) {
        context->cbs.pfn_ae_exit(context->aiq_ctx);
        context->cutomAeInit = false;
    }
    delete context;
    context = NULL;

    LOGD_AEC_SUBM(0xff, "%s EXIT", __func__);
    return XCAM_RETURN_NO_ERROR;
}

static XCamReturn initAecHwConfig(rk_aiq_rkAe_config_t* pConfig)
{
    LOGD_AEC_SUBM(0xff, "%s ENTER", __func__);
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    //set rawae_sel (only adapt to Nomal Mode)
    pConfig->aeHwConfig.ae_meas.rawae0.rawae_sel = 2;
    pConfig->aeHwConfig.ae_meas.rawae1.rawae_sel = 2;
    pConfig->aeHwConfig.ae_meas.rawae2.rawae_sel = 2;
    pConfig->aeHwConfig.ae_meas.rawae3.rawae_sel = 3;

    /*****rawae0, LITE mode****/
    pConfig->aeHwConfig.ae_meas.rawae0.wnd_num = 1;
    pConfig->aeHwConfig.ae_meas.rawae0.win.h_offs = 0;
    pConfig->aeHwConfig.ae_meas.rawae0.win.v_offs = 0;
    pConfig->aeHwConfig.ae_meas.rawae0.win.h_size = pConfig->RawWidth;
    pConfig->aeHwConfig.ae_meas.rawae0.win.v_size = pConfig->RawHeight;

    /*****rawae1-3, BIG mode****/
    pConfig->aeHwConfig.ae_meas.rawae1.wnd_num = 2;
    pConfig->aeHwConfig.ae_meas.rawae1.win.h_offs = 0;
    pConfig->aeHwConfig.ae_meas.rawae1.win.v_offs = 0;
    pConfig->aeHwConfig.ae_meas.rawae1.win.h_size = pConfig->RawWidth;
    pConfig->aeHwConfig.ae_meas.rawae1.win.v_size = pConfig->RawHeight;
    pConfig->aeHwConfig.ae_meas.rawae1.subwin[0].h_offs = 2;
    pConfig->aeHwConfig.ae_meas.rawae1.subwin[0].v_offs = 2;
    pConfig->aeHwConfig.ae_meas.rawae1.subwin[0].h_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.rawae1.subwin[0].v_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.rawae1.subwin[1].h_offs = 150;
    pConfig->aeHwConfig.ae_meas.rawae1.subwin[1].v_offs = 2;
    pConfig->aeHwConfig.ae_meas.rawae1.subwin[1].h_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.rawae1.subwin[1].v_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.rawae1.subwin[2].h_offs = 2;
    pConfig->aeHwConfig.ae_meas.rawae1.subwin[2].v_offs = 150;
    pConfig->aeHwConfig.ae_meas.rawae1.subwin[2].h_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.rawae1.subwin[2].v_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.rawae1.subwin[3].h_offs = 150;
    pConfig->aeHwConfig.ae_meas.rawae1.subwin[3].v_offs = 150;
    pConfig->aeHwConfig.ae_meas.rawae1.subwin[3].h_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.rawae1.subwin[3].v_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.rawae1.subwin_en[0] = 1;
    pConfig->aeHwConfig.ae_meas.rawae1.subwin_en[1] = 1;
    pConfig->aeHwConfig.ae_meas.rawae1.subwin_en[2] = 1;
    pConfig->aeHwConfig.ae_meas.rawae1.subwin_en[3] = 1;

    pConfig->aeHwConfig.ae_meas.rawae2.wnd_num = 2;
    pConfig->aeHwConfig.ae_meas.rawae2.win.h_offs = 0;
    pConfig->aeHwConfig.ae_meas.rawae2.win.v_offs = 0;
    pConfig->aeHwConfig.ae_meas.rawae2.win.h_size = pConfig->RawWidth;
    pConfig->aeHwConfig.ae_meas.rawae2.win.v_size = pConfig->RawHeight;
    pConfig->aeHwConfig.ae_meas.rawae2.subwin[0].h_offs = 2;
    pConfig->aeHwConfig.ae_meas.rawae2.subwin[0].v_offs = 2;
    pConfig->aeHwConfig.ae_meas.rawae2.subwin[0].h_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.rawae2.subwin[0].v_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.rawae2.subwin[1].h_offs = 150;
    pConfig->aeHwConfig.ae_meas.rawae2.subwin[1].v_offs = 2;
    pConfig->aeHwConfig.ae_meas.rawae2.subwin[1].h_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.rawae2.subwin[1].v_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.rawae2.subwin[2].h_offs = 2;
    pConfig->aeHwConfig.ae_meas.rawae2.subwin[2].v_offs = 150;
    pConfig->aeHwConfig.ae_meas.rawae2.subwin[2].h_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.rawae2.subwin[2].v_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.rawae2.subwin[3].h_offs = 150;
    pConfig->aeHwConfig.ae_meas.rawae2.subwin[3].v_offs = 150;
    pConfig->aeHwConfig.ae_meas.rawae2.subwin[3].h_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.rawae2.subwin[3].v_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.rawae2.subwin_en[0] = 1;
    pConfig->aeHwConfig.ae_meas.rawae2.subwin_en[1] = 1;
    pConfig->aeHwConfig.ae_meas.rawae2.subwin_en[2] = 1;
    pConfig->aeHwConfig.ae_meas.rawae2.subwin_en[3] = 1;

    pConfig->aeHwConfig.ae_meas.rawae3.wnd_num = 2;
    pConfig->aeHwConfig.ae_meas.rawae3.win.h_offs = 0;
    pConfig->aeHwConfig.ae_meas.rawae3.win.v_offs = 0;
    pConfig->aeHwConfig.ae_meas.rawae3.win.h_size = pConfig->RawWidth;
    pConfig->aeHwConfig.ae_meas.rawae3.win.v_size = pConfig->RawHeight;
    pConfig->aeHwConfig.ae_meas.rawae3.subwin[0].h_offs = 2;
    pConfig->aeHwConfig.ae_meas.rawae3.subwin[0].v_offs = 2;
    pConfig->aeHwConfig.ae_meas.rawae3.subwin[0].h_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.rawae3.subwin[0].v_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.rawae3.subwin[1].h_offs = 150;
    pConfig->aeHwConfig.ae_meas.rawae3.subwin[1].v_offs = 2;
    pConfig->aeHwConfig.ae_meas.rawae3.subwin[1].h_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.rawae3.subwin[1].v_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.rawae3.subwin[2].h_offs = 2;
    pConfig->aeHwConfig.ae_meas.rawae3.subwin[2].v_offs = 150;
    pConfig->aeHwConfig.ae_meas.rawae3.subwin[2].h_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.rawae3.subwin[2].v_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.rawae3.subwin[3].h_offs = 150;
    pConfig->aeHwConfig.ae_meas.rawae3.subwin[3].v_offs = 150;
    pConfig->aeHwConfig.ae_meas.rawae3.subwin[3].h_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.rawae3.subwin[3].v_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.rawae3.subwin_en[0] = 1;
    pConfig->aeHwConfig.ae_meas.rawae3.subwin_en[1] = 1;
    pConfig->aeHwConfig.ae_meas.rawae3.subwin_en[2] = 1;
    pConfig->aeHwConfig.ae_meas.rawae3.subwin_en[3] = 1;

    /****rawhist0, LITE mode****/
    pConfig->aeHwConfig.hist_meas.rawhist0.data_sel = 0;
    pConfig->aeHwConfig.hist_meas.rawhist0.waterline = 0;
    pConfig->aeHwConfig.hist_meas.rawhist0.mode = 5;
    pConfig->aeHwConfig.hist_meas.rawhist0.stepsize = 0;
    pConfig->aeHwConfig.hist_meas.rawhist0.win.h_offs = 0;
    pConfig->aeHwConfig.hist_meas.rawhist0.win.v_offs = 0;
    pConfig->aeHwConfig.hist_meas.rawhist0.win.h_size = pConfig->RawWidth;
    pConfig->aeHwConfig.hist_meas.rawhist0.win.v_size = pConfig->RawHeight;
    memset(pConfig->aeHwConfig.hist_meas.rawhist0.weight, 0x20, RAWHISTBIG_WIN_NUM * sizeof(unsigned char));
    pConfig->aeHwConfig.hist_meas.rawhist0.rcc = 0x4d;
    pConfig->aeHwConfig.hist_meas.rawhist0.gcc = 0x4b;
    pConfig->aeHwConfig.hist_meas.rawhist0.bcc = 0x1d;
    pConfig->aeHwConfig.hist_meas.rawhist0.off = 0x00;

    /****rawhist1-3, BIG mode****/
    pConfig->aeHwConfig.hist_meas.rawhist1.data_sel = 0;
    pConfig->aeHwConfig.hist_meas.rawhist1.waterline = 0;
    pConfig->aeHwConfig.hist_meas.rawhist1.mode = 5;
    pConfig->aeHwConfig.hist_meas.rawhist1.wnd_num = 2;
    pConfig->aeHwConfig.hist_meas.rawhist1.stepsize = 0;
    pConfig->aeHwConfig.hist_meas.rawhist1.win.h_offs = 0;
    pConfig->aeHwConfig.hist_meas.rawhist1.win.v_offs = 0;
    pConfig->aeHwConfig.hist_meas.rawhist1.win.h_size = pConfig->RawWidth;
    pConfig->aeHwConfig.hist_meas.rawhist1.win.v_size = pConfig->RawHeight;
    memset(pConfig->aeHwConfig.hist_meas.rawhist1.weight, 0x20, RAWHISTBIG_WIN_NUM * sizeof(unsigned char));
    pConfig->aeHwConfig.hist_meas.rawhist1.rcc = 0x4d;
    pConfig->aeHwConfig.hist_meas.rawhist1.gcc = 0x4b;
    pConfig->aeHwConfig.hist_meas.rawhist1.bcc = 0x1d;
    pConfig->aeHwConfig.hist_meas.rawhist1.off = 0x00;

    pConfig->aeHwConfig.hist_meas.rawhist2.data_sel = 0;
    pConfig->aeHwConfig.hist_meas.rawhist2.waterline = 0;
    pConfig->aeHwConfig.hist_meas.rawhist2.mode = 5;
    pConfig->aeHwConfig.hist_meas.rawhist2.wnd_num = 2;
    pConfig->aeHwConfig.hist_meas.rawhist2.stepsize = 0;
    pConfig->aeHwConfig.hist_meas.rawhist2.win.h_offs = 0;
    pConfig->aeHwConfig.hist_meas.rawhist2.win.v_offs = 0;
    pConfig->aeHwConfig.hist_meas.rawhist2.win.h_size = pConfig->RawWidth;
    pConfig->aeHwConfig.hist_meas.rawhist2.win.v_size = pConfig->RawHeight;
    memset(pConfig->aeHwConfig.hist_meas.rawhist2.weight, 0x20, RAWHISTBIG_WIN_NUM * sizeof(unsigned char));
    pConfig->aeHwConfig.hist_meas.rawhist2.rcc = 0x4d;
    pConfig->aeHwConfig.hist_meas.rawhist2.gcc = 0x4b;
    pConfig->aeHwConfig.hist_meas.rawhist2.bcc = 0x1d;
    pConfig->aeHwConfig.hist_meas.rawhist2.off = 0x00;

    pConfig->aeHwConfig.hist_meas.rawhist3.data_sel = 0;
    pConfig->aeHwConfig.hist_meas.rawhist3.waterline = 0;
    pConfig->aeHwConfig.hist_meas.rawhist3.mode = 5;
    pConfig->aeHwConfig.hist_meas.rawhist3.wnd_num = 2;
    pConfig->aeHwConfig.hist_meas.rawhist3.stepsize = 0;
    pConfig->aeHwConfig.hist_meas.rawhist3.win.h_offs = 0;
    pConfig->aeHwConfig.hist_meas.rawhist3.win.v_offs = 0;
    pConfig->aeHwConfig.hist_meas.rawhist3.win.h_size = pConfig->RawWidth;
    pConfig->aeHwConfig.hist_meas.rawhist3.win.v_size = pConfig->RawHeight;
    memset(pConfig->aeHwConfig.hist_meas.rawhist3.weight, 0x20, RAWHISTBIG_WIN_NUM * sizeof(unsigned char));
    pConfig->aeHwConfig.hist_meas.rawhist3.rcc = 0x4d;
    pConfig->aeHwConfig.hist_meas.rawhist3.gcc = 0x4b;
    pConfig->aeHwConfig.hist_meas.rawhist3.bcc = 0x1d;
    pConfig->aeHwConfig.hist_meas.rawhist3.off = 0x00;

    /***** yuvae ****/
    pConfig->aeHwConfig.ae_meas.yuvae.ysel = 1; //1:Y full range 0:Y limited range
    pConfig->aeHwConfig.ae_meas.yuvae.wnd_num = 1; //0:1x1, 1:15x15
    pConfig->aeHwConfig.ae_meas.yuvae.win.h_offs = 0;
    pConfig->aeHwConfig.ae_meas.yuvae.win.v_offs = 0;
    pConfig->aeHwConfig.ae_meas.yuvae.win.h_size = pConfig->RawWidth;
    pConfig->aeHwConfig.ae_meas.yuvae.win.v_size = pConfig->RawHeight;
    pConfig->aeHwConfig.ae_meas.yuvae.subwin[0].h_offs = 2;
    pConfig->aeHwConfig.ae_meas.yuvae.subwin[0].v_offs = 2;
    pConfig->aeHwConfig.ae_meas.yuvae.subwin[0].h_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.yuvae.subwin[0].v_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.yuvae.subwin[1].h_offs = 150;
    pConfig->aeHwConfig.ae_meas.yuvae.subwin[1].v_offs = 2;
    pConfig->aeHwConfig.ae_meas.yuvae.subwin[1].h_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.yuvae.subwin[1].v_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.yuvae.subwin[2].h_offs = 2;
    pConfig->aeHwConfig.ae_meas.yuvae.subwin[2].v_offs = 150;
    pConfig->aeHwConfig.ae_meas.yuvae.subwin[2].h_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.yuvae.subwin[2].v_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.yuvae.subwin[3].h_offs = 150;
    pConfig->aeHwConfig.ae_meas.yuvae.subwin[3].v_offs = 150;
    pConfig->aeHwConfig.ae_meas.yuvae.subwin[3].h_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.yuvae.subwin[3].v_size = 100;  // must even number
    pConfig->aeHwConfig.ae_meas.yuvae.subwin_en[0] = 1;
    pConfig->aeHwConfig.ae_meas.yuvae.subwin_en[1] = 1;
    pConfig->aeHwConfig.ae_meas.yuvae.subwin_en[2] = 1;
    pConfig->aeHwConfig.ae_meas.yuvae.subwin_en[3] = 1;

    /****sihist****/
    pConfig->aeHwConfig.hist_meas.sihist.wnd_num = 3;
    pConfig->aeHwConfig.hist_meas.sihist.win_cfg[0].data_sel = 0;
    pConfig->aeHwConfig.hist_meas.sihist.win_cfg[0].waterline = 0;
    pConfig->aeHwConfig.hist_meas.sihist.win_cfg[0].auto_stop = 0;
    pConfig->aeHwConfig.hist_meas.sihist.win_cfg[0].mode = 5;
    pConfig->aeHwConfig.hist_meas.sihist.win_cfg[0].stepsize = 3;
    pConfig->aeHwConfig.hist_meas.sihist.win_cfg[0].win.h_offs = 0;
    pConfig->aeHwConfig.hist_meas.sihist.win_cfg[0].win.v_offs = 0;
    pConfig->aeHwConfig.hist_meas.sihist.win_cfg[0].win.h_size = pConfig->RawWidth;
    pConfig->aeHwConfig.hist_meas.sihist.win_cfg[0].win.v_size = pConfig->RawHeight;
    memset(pConfig->aeHwConfig.hist_meas.sihist.hist_weight, 0x20, SIHIST_WIN_NUM * sizeof(unsigned char));

    LOGD_AEC_SUBM(0xff, "%s EXIT", __func__);
    return ret;
}

static XCamReturn updateAecHwConfig(RkAiqAlgoProcResAe* rkAeProcRes, rk_aiq_rkAe_config_t* rkAe)
{
    LOGD_AEC_SUBM(0xff, "%s ENTER", __func__);
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rkAeProcRes->ae_meas = rkAe->aeHwConfig.ae_meas;
    rkAeProcRes->hist_meas = rkAe->aeHwConfig.hist_meas;

    LOGD_AEC_SUBM(0xff, "%s EXIT", __func__);
    return ret;
}

// call after initAecHwConfig
static void initCustomAeRes(rk_aiq_customeAe_results_t* customAe, rk_aiq_rkAe_config_t* pConfig)
{
    customAe->IsConverged = false;
    customAe->exp_real_params.integration_time = 0.01f;
    customAe->exp_real_params.analog_gain = 1.0f;
    customAe->exp_real_params.digital_gain = 1.0f;
    customAe->exp_real_params.isp_dgain = 1.0f;

    customAe->exp_real_params.iso = customAe->exp_real_params.analog_gain * 50;
    if(pConfig->stDcgInfo.Normal.support_en)
        customAe->exp_real_params.dcg_mode = pConfig->stDcgInfo.Normal.dcg_mode.Coeff[0];
    else
        customAe->exp_real_params.dcg_mode = -1;

    customAe->meas_win = pConfig->aeHwConfig.ae_meas.rawae0.win;

    memcpy(&customAe->meas_weight,
           &pConfig->aeHwConfig.hist_meas.rawhist0.weight, sizeof(customAe->meas_weight));
}

static XCamReturn AeDemoPrepare(RkAiqAlgoCom* params)
{
    LOGD_AEC_SUBM(0xff, "%s ENTER", __func__);
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoContext* algo_ctx = params->ctx;

    if (!algo_ctx->cutomAeInit) {
        algo_ctx->cbs.pfn_ae_init(algo_ctx->aiq_ctx);
        algo_ctx->cutomAeInit = true;
    }

    algo_ctx->rkCfg.Working_mode = params->u.prepare.working_mode;
    algo_ctx->rkCfg.RawWidth = params->u.prepare.sns_op_width;
    algo_ctx->rkCfg.RawHeight = params->u.prepare.sns_op_height;

    initAecHwConfig(&algo_ctx->rkCfg);

    if(!(params->u.prepare.conf_type & RK_AIQ_ALGO_CONFTYPE_UPDATECALIB)) {
        initCustomAeRes(&algo_ctx->customRes, &algo_ctx->rkCfg);
        algo_ctx->updateCalib = false;
    } else {
        algo_ctx->updateCalib = true;
    }

    RkAiqAlgoConfigAe* AeCfgParam = (RkAiqAlgoConfigAe*)params;

    algo_ctx->rkCfg.LinePeriodsPerField = AeCfgParam->LinePeriodsPerField;
    algo_ctx->rkCfg.PixelClockFreqMHZ = AeCfgParam->PixelClockFreqMHZ;
    algo_ctx->rkCfg.PixelPeriodsPerLine = AeCfgParam->PixelPeriodsPerLine;
    algo_ctx->rkCfg.stNRswitch = AeCfgParam->nr_switch;

    LOGD_AEC_SUBM(0xff, "%s EXIT", __func__);

    return ret;
}

static
void _rkAeStats2CustomAeStats(rk_aiq_rkAe_config_t * pConfig,
                              rk_aiq_customAe_stats_t* customAe,
                              RKAiqAecStats_t* rkAe)
{
    LOGD_AEC_SUBM(0xff, "%s ENTER", __func__);
    // only support normal mode now
    // use ch0 as big mode
    customAe->rawae_stat.rawae_big = rkAe->ae_data.chn[0].rawae_big;
    customAe->rawae_stat.rawhist_big = rkAe->ae_data.chn[0].rawhist_big;
    customAe->extra.rawae_big = rkAe->ae_data.extra.rawae_big;
    customAe->extra.rawhist_big = rkAe->ae_data.extra.rawhist_big;


    AeReg2RealConv(pConfig, rkAe->ae_exp.LinearExp.exp_sensor_params.analog_gain_code_global,
                   rkAe->ae_exp.LinearExp.exp_sensor_params.coarse_integration_time,
                   rkAe->ae_exp.LinearExp.exp_real_params.dcg_mode,
                   rkAe->ae_exp.LinearExp.exp_real_params.analog_gain,
                   rkAe->ae_exp.LinearExp.exp_real_params.integration_time);

    customAe->exp_real_params = rkAe->ae_exp.LinearExp.exp_real_params;



    LOGD_AEC_SUBM(0xff, "%s EXIT", __func__);
}

static void _rkCISFeature
(
    rk_aiq_rkAe_config_t* pConfig,
    RkAiqAlgoProcResAe* rkAeProcRes
) {

    if(pConfig->stNRswitch.valid == true) {

        float up_thres = (float)pConfig->stNRswitch.up_thres / (float)pConfig->stNRswitch.div_coeff;
        float down_thres = (float)pConfig->stNRswitch.down_thres / (float)pConfig->stNRswitch.div_coeff;

        if(rkAeProcRes->ae_proc_res.exp_set_tbl[0].LinearExp.exp_real_params.analog_gain >= up_thres)
            rkAeProcRes->ae_proc_res.exp_set_tbl[0].CISFeature.SNR = (pConfig->stNRswitch.direct == 0) ? 1 : 0;
        if(rkAeProcRes->ae_proc_res.exp_set_tbl[0].LinearExp.exp_real_params.analog_gain < down_thres)
            rkAeProcRes->ae_proc_res.exp_set_tbl[0].CISFeature.SNR = (pConfig->stNRswitch.direct == 0) ? 0 : 1;

    } else {

        //LCG/HCG => SNR
        rkAeProcRes->ae_proc_res.exp_set_tbl[0].CISFeature.SNR = rkAeProcRes->ae_proc_res.exp_set_tbl[0].LinearExp.exp_real_params.dcg_mode > 0 ? 1 : 0;
    }
}

static
void _customAeRes2rkAeRes(rk_aiq_rkAe_config_t* pConfig, RkAiqAlgoProcResAe* rkAeProcRes,
                          rk_aiq_customeAe_results_t* customAeProcRes)
{

    rkAeProcRes->ae_proc_res.IsConverged = customAeProcRes->IsConverged;
    rkAeProcRes->ae_proc_res.LumaDeviation = customAeProcRes->LumaDeviation;
    rkAeProcRes->ae_proc_res.MeanLuma = customAeProcRes->MeanLuma;

    AeDcgConv(pConfig, customAeProcRes->exp_real_params.analog_gain, &customAeProcRes->exp_real_params.dcg_mode);

    rkAeProcRes->new_ae_exp.LinearExp.exp_real_params = customAeProcRes->exp_real_params;
    AeReal2RegConv(pConfig, customAeProcRes->exp_real_params.integration_time,
                   customAeProcRes->exp_real_params.analog_gain,
                   &rkAeProcRes->new_ae_exp.LinearExp.exp_sensor_params.coarse_integration_time,
                   &rkAeProcRes->new_ae_exp.LinearExp.exp_sensor_params.analog_gain_code_global,
                   &customAeProcRes->exp_real_params.dcg_mode);

    rkAeProcRes->new_ae_exp.line_length_pixels = pConfig->PixelPeriodsPerLine;
    rkAeProcRes->new_ae_exp.pixel_clock_freq_mhz = pConfig->PixelClockFreqMHZ;

    //vts check
    if(rkAeProcRes->new_ae_exp.LinearExp.exp_sensor_params.coarse_integration_time > pConfig->LinePeriodsPerField)
        rkAeProcRes->new_ae_exp.frame_length_lines = round((float)(rkAeProcRes->new_ae_exp.LinearExp.exp_sensor_params.coarse_integration_time
                + pConfig->stSensorInfo.CISLinTimeRegMaxFac.fCoeff[1]) / (float)pConfig->stSensorInfo.CISLinTimeRegMaxFac.fCoeff[0]);
    else
        rkAeProcRes->new_ae_exp.frame_length_lines = pConfig->LinePeriodsPerField;


    rkAeProcRes->ae_proc_res.exp_set_cnt = 1;
    rkAeProcRes->ae_proc_res.exp_set_tbl[0] = rkAeProcRes->new_ae_exp;

    _rkCISFeature(pConfig, rkAeProcRes);

    if (customAeProcRes->meas_win.h_size > 0 &&
            customAeProcRes->meas_win.v_size > 0) {
        rkAeProcRes->ae_meas.rawae0.win = customAeProcRes->meas_win;
        rkAeProcRes->ae_meas.rawae1.win = customAeProcRes->meas_win;
        rkAeProcRes->ae_meas.rawae2.win = customAeProcRes->meas_win;
        rkAeProcRes->ae_meas.rawae3.win = customAeProcRes->meas_win;
        rkAeProcRes->ae_meas.yuvae.win = customAeProcRes->meas_win;
        rkAeProcRes->hist_meas.rawhist0.win = customAeProcRes->meas_win;
        rkAeProcRes->hist_meas.rawhist1.win = customAeProcRes->meas_win;
        rkAeProcRes->hist_meas.rawhist2.win = customAeProcRes->meas_win;
        rkAeProcRes->hist_meas.rawhist3.win = customAeProcRes->meas_win;
        rkAeProcRes->hist_meas.sihist.win_cfg[0].win = customAeProcRes->meas_win;
    }

    if (customAeProcRes->meas_weight[0] > 0) {
        memcpy(rkAeProcRes->hist_meas.rawhist1.weight, customAeProcRes->meas_weight, 15 * 15 * sizeof(unsigned char));
        memcpy(rkAeProcRes->hist_meas.rawhist2.weight, customAeProcRes->meas_weight, 15 * 15 * sizeof(unsigned char));
        memcpy(rkAeProcRes->hist_meas.rawhist3.weight, customAeProcRes->meas_weight, 15 * 15 * sizeof(unsigned char));
        memcpy(rkAeProcRes->hist_meas.sihist.hist_weight, customAeProcRes->meas_weight, 15 * 15 * sizeof(unsigned char));
    }
}

static XCamReturn AeDemoPreProcess(const RkAiqAlgoCom* inparams, RkAiqAlgoResCom* outparams)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    LOGD_AEC_SUBM(0xff, "%s ENTER", __func__);
    RkAiqAlgoPreAe* AePreParams = (RkAiqAlgoPreAe*)inparams;
    RkAiqAlgoPreResAe* AePreResParams = (RkAiqAlgoPreResAe*)outparams;
    RkAiqAlgoContext* algo_ctx = inparams->ctx;
    AecPreResult_t* AePreResult = &AePreResParams->ae_pre_res;

    if(!inparams->u.proc.init) {
        // get current used ae exp
        AePreResult->LinearExp = AePreParams->ispAeStats->ae_exp.LinearExp;
        ret = AeReg2RealConv(&algo_ctx->rkCfg, AePreResult->LinearExp.exp_sensor_params.analog_gain_code_global,
                             AePreResult->LinearExp.exp_sensor_params.coarse_integration_time,
                             AePreResult->LinearExp.exp_real_params.dcg_mode,
                             AePreResult->LinearExp.exp_real_params.analog_gain,
                             AePreResult->LinearExp.exp_real_params.integration_time);

    } else {
        if (algo_ctx->updateCalib) {
            LOGD_AEC_SUBM(0xff, "updateCalib, no need re-init");
            return ret;
        }

        // init ae exp
        AePreResult->LinearExp.exp_real_params.integration_time = 0.01f;
        AePreResult->LinearExp.exp_real_params.analog_gain = 1.0f;
        AePreResult->LinearExp.exp_real_params.digital_gain = 1.0f;
        AePreResult->LinearExp.exp_real_params.isp_dgain = 1.0f;
        AePreResult->LinearExp.exp_real_params.iso = AePreResult->LinearExp.exp_real_params.analog_gain * 50;

        if(algo_ctx->rkCfg.stDcgInfo.Normal.support_en)
            AePreResult->LinearExp.exp_real_params.dcg_mode = algo_ctx->rkCfg.stDcgInfo.Normal.dcg_mode.Coeff[0];
        else
            AePreResult->LinearExp.exp_real_params.dcg_mode = -1;

        ret = AeReal2RegConv(&algo_ctx->rkCfg,
                             AePreResult->LinearExp.exp_real_params.integration_time,
                             AePreResult->LinearExp.exp_real_params.analog_gain,
                             &AePreResult->LinearExp.exp_sensor_params.coarse_integration_time,
                             &AePreResult->LinearExp.exp_sensor_params.analog_gain_code_global,
                             &AePreResult->LinearExp.exp_real_params.dcg_mode);
    }

    LOGD_AEC_SUBM(0xff, "%s EXIT", __func__);

    return ret;
}

static XCamReturn AeDemoProcessing(const RkAiqAlgoCom* inparams, RkAiqAlgoResCom* outparams)
{

    LOGD_AEC_SUBM(0xff, "%s ENTER", __func__);
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoProcAe* AeProcParams = (RkAiqAlgoProcAe*)inparams;
    RkAiqAlgoProcResAe* AeProcResParams = (RkAiqAlgoProcResAe*)outparams;
    RkAiqAlgoContext* algo_ctx = inparams->ctx;

    if(!inparams->u.proc.init) { // init=ture, stats=null
        rk_aiq_customAe_stats_t customStats;
        _rkAeStats2CustomAeStats(&algo_ctx->rkCfg, &customStats, AeProcParams->ispAeStats);

        if (algo_ctx->cbs.pfn_ae_run)
            algo_ctx->cbs.pfn_ae_run(algo_ctx->aiq_ctx,
                                     &customStats,
                                     &algo_ctx->customRes
                                    );
    } else {
        if (algo_ctx->updateCalib) {
            LOGD_AEC_SUBM(0xff, "updateCalib, no need re-init");
            return ret;
        }
        // do nothing now set initial rk result, init custom Res was done in initCustomAeRes
        initCustomAeRes(&algo_ctx->customRes, &algo_ctx->rkCfg);
    }

    // gen patrt of proc results which is not contained in customRes
    updateAecHwConfig(AeProcResParams, &algo_ctx->rkCfg);
    // gen part of proc result which is from customRes
    _customAeRes2rkAeRes(&algo_ctx->rkCfg, AeProcResParams, &algo_ctx->customRes);

    LOGD_AEC_SUBM(0xff, "%s EXIT", __func__);
    return XCAM_RETURN_NO_ERROR;
}

static XCamReturn AeDemoPostProcess(const RkAiqAlgoCom* inparams, RkAiqAlgoResCom* outparams)
{
    RESULT ret = RK_AIQ_RET_SUCCESS;
    RkAiqAlgoContext* algo_ctx = inparams->ctx;

    if (algo_ctx->updateCalib) {
        algo_ctx->updateCalib = false;
    }

    return XCAM_RETURN_NO_ERROR;
}

static std::map<rk_aiq_sys_ctx_t*, RkAiqAlgoDescription*> g_customAe_desc_map;

XCamReturn
rk_aiq_uapi_customAE_register(const rk_aiq_sys_ctx_t* ctx, rk_aiq_customeAe_cbs_t* cbs)
{

    LOGD_AEC_SUBM(0xff, "%s ENTER", __func__);
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (!cbs)
        return XCAM_RETURN_ERROR_PARAM;

    RkAiqAlgoDescription* desc = NULL;
    rk_aiq_sys_ctx_t* cast_ctx = const_cast<rk_aiq_sys_ctx_t*>(ctx);

    std::map<rk_aiq_sys_ctx_t*, RkAiqAlgoDescription*>::iterator it =
        g_customAe_desc_map.find(cast_ctx);

    if (it == g_customAe_desc_map.end()) {
        desc = new RkAiqAlgoDescription();
        g_customAe_desc_map[cast_ctx] = desc;
    } else {
        desc = it->second;
    }

    desc->common.version = RKISP_ALGO_AE_DEMO_VERSION;
    desc->common.vendor  = RKISP_ALGO_AE_DEMO_VENDOR;
    desc->common.description = RKISP_ALGO_AE_DEMO_DESCRIPTION;
    desc->common.type    = RK_AIQ_ALGO_TYPE_AE;
    desc->common.id      = 0;
    desc->common.create_context  = AeDemoCreateCtx;
    desc->common.destroy_context = AeDemoDestroyCtx;
    desc->prepare = AeDemoPrepare;
    desc->pre_process = AeDemoPreProcess;
    desc->processing = AeDemoProcessing;
    desc->post_process = AeDemoPostProcess;

    ret = rk_aiq_uapi_sysctl_regLib(ctx, &desc->common);
    if (ret != XCAM_RETURN_NO_ERROR) {
        LOGE_AEC_SUBM(0xff, "register %d failed !", desc->common.id);
        return ret;
    }

    RkAiqAlgoContext* algoCtx =
        rk_aiq_uapi_sysctl_getAxlibCtx(ctx,
                                       desc->common.type,
                                       desc->common.id);
    if (algoCtx == NULL) {
        LOGE_AEC_SUBM(0xff, "can't get custom ae algo %d ctx!", desc->common.id);
        return XCAM_RETURN_ERROR_FAILED;
    }

    algoCtx->cbs = *cbs;
    algoCtx->aiq_ctx = const_cast<rk_aiq_sys_ctx_t*>(ctx);

    algoCtx->pcalib = rk_aiq_uapi_sysctl_getCurCalib(ctx);

    algoCtx->rkCfg.stSensorInfo = algoCtx->pcalib->sensor;

    if(algoCtx->rkCfg.stSensorInfo.GainRange.IsLinear)
        algoCtx->rkCfg.stSensorInfo.GainRange.GainMode = RKAIQ_EXPGAIN_MODE_LINEAR;
    else
        algoCtx->rkCfg.stSensorInfo.GainRange.GainMode = RKAIQ_EXPGAIN_MODE_NONLINEAR_DB;

    algoCtx->rkCfg.stDcgInfo = algoCtx->pcalib->sysContrl.dcg;

    LOGD_AEC_SUBM(0xff, "register custom ae algo sucess for sys_ctx %p, lib_id %d !",
                  ctx,
                  desc->common.id);
    LOGD_AEC_SUBM(0xff, "%s EXIT", __func__);

    return ret;
}

XCamReturn
rk_aiq_uapi_customAE_enable(const rk_aiq_sys_ctx_t* ctx, bool enable)
{

    LOGD_AEC_SUBM(0xff, "%s ENTER", __func__);
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoDescription* desc = NULL;
    rk_aiq_sys_ctx_t* cast_ctx = const_cast<rk_aiq_sys_ctx_t*>(ctx);

    std::map<rk_aiq_sys_ctx_t*, RkAiqAlgoDescription*>::iterator it =
        g_customAe_desc_map.find(cast_ctx);

    if (it == g_customAe_desc_map.end()) {
        LOGE_AEC_SUBM(0xff, "can't find custom ae algo for sys_ctx %p !", ctx);
        return XCAM_RETURN_ERROR_FAILED;
    } else {
        desc = it->second;
    }

    ret = rk_aiq_uapi_sysctl_enableAxlib(ctx,
                                         desc->common.type,
                                         desc->common.id,
                                         enable);
    if (ret != XCAM_RETURN_NO_ERROR) {
        LOGE_AEC_SUBM(0xff, "enable custom ae lib id %d failed !");
        return ret;
    }

    if (!enable)
        ret = rk_aiq_uapi_sysctl_enableAxlib(ctx,
                                             desc->common.type,
                                             0,
                                             !enable);

    LOGD_AEC_SUBM(0xff, "enable custom ae algo sucess for sys_ctx %p, lib_id %d !",
                  ctx,
                  desc->common.id);
    LOGD_AEC_SUBM(0xff, "%s EXIT", __func__);
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_uapi_customAE_unRegister(const rk_aiq_sys_ctx_t* ctx)
{

    LOGD_AEC_SUBM(0xff, "%s ENTER", __func__);
    RkAiqAlgoDescription* desc = NULL;
    rk_aiq_sys_ctx_t* cast_ctx = const_cast<rk_aiq_sys_ctx_t*>(ctx);

    std::map<rk_aiq_sys_ctx_t*, RkAiqAlgoDescription*>::iterator it =
        g_customAe_desc_map.find(cast_ctx);

    if (it == g_customAe_desc_map.end()) {
        LOGE_AEC_SUBM(0xff, "can't find custom ae algo for sys_ctx %p !", ctx);
        return XCAM_RETURN_ERROR_FAILED;
    } else {
        desc = it->second;
    }

    rk_aiq_uapi_sysctl_unRegLib(ctx,
                                desc->common.type,
                                desc->common.id);

    LOGD_AEC_SUBM(0xff, "unregister custom ae algo sucess for sys_ctx %p, lib_id %d !",
                  ctx,
                  desc->common.id);

    g_customAe_desc_map.erase(it);
    delete it->second;

    LOGD_AEC_SUBM(0xff, "%s EXIT", __func__);
    return XCAM_RETURN_NO_ERROR;
}

RKAIQ_END_DECLARE
