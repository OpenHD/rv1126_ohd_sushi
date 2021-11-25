/******************************************************************************
 *
 * Copyright 2019, Fuzhou Rockchip Electronics Co.Ltd. All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Fuzhou Rockchip Electronics Co.Ltd .
 *
 *
 *****************************************************************************/
/**
 * @file ahdr.cpp
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include "math.h"
#include "rk_aiq_types_awdr_algo_int.h"
#include "rk_aiq_types_awdr_algo_prvt.h"

#include "xcam_log.h"

/******************************************************************************
 * AwdrStart()
 *****************************************************************************/
RESULT AwdrStart
(
    AwdrHandle_t pAwdrCtx
) {

    LOG1_AWDR( "%s:enter!\n", __FUNCTION__);

    // initial checks
    if (pAwdrCtx == NULL) {
        return (AWDR_RET_WRONG_HANDLE);
    }

    if ((AWDR_STATE_RUNNING == pAwdrCtx->state)
            || (AWDR_STATE_LOCKED == pAwdrCtx->state)) {
        return (AWDR_RET_WRONG_STATE);
    }

    pAwdrCtx->state = AWDR_STATE_RUNNING;

    LOG1_AWDR( "%s:exit!\n", __FUNCTION__);
    return (AWDR_RET_SUCCESS);
}
/******************************************************************************
 * AwdrStop()
 *****************************************************************************/
RESULT AwdrStop
(
    AwdrHandle_t pAwdrCtx
) {

    LOG1_AWDR( "%s:enter!\n", __FUNCTION__);

    // initial checks
    if (pAwdrCtx == NULL) {
        return (AWDR_RET_WRONG_HANDLE);
    }

    // before stopping, unlock the AHDR if locked
    if (AWDR_STATE_LOCKED == pAwdrCtx->state) {
        return (AWDR_RET_WRONG_STATE);
    }

    pAwdrCtx->state = AWDR_STATE_STOPPED;

    LOG1_AWDR( "%s:exit!\n", __FUNCTION__);
    return (AWDR_RET_SUCCESS);
}
/******************************************************************************
 * AwdrGetCurrPara()
 *****************************************************************************/
float AwdrGetCurrPara
(
    float           inPara,
    float         inMatrixX[AWDR_MAX_IQ_DOTS],
    float         inMatrixY[AWDR_MAX_IQ_DOTS]
) {
    LOG1_AWDR( "%s:enter!\n", __FUNCTION__);
    float x1 = 0.0f;
    float x2 = 0.0f;
    float value1 = 0;
    float value2 = 0;
    float outPara = 0;

    if(inPara < inMatrixX[0])
        outPara = inMatrixY[0];
    else if (inPara >= inMatrixX[AWDR_MAX_IQ_DOTS - 1])
        outPara = inMatrixY[AWDR_MAX_IQ_DOTS - 1];
    else
        for(int i = 0; i < AWDR_MAX_IQ_DOTS - 1; i++)
        {
            if(inPara >= inMatrixX[i] && inPara < inMatrixX[i + 1])
            {
                x1 = inMatrixX[i];
                x2 = inMatrixX[i + 1];
                value1 = inMatrixY[i];
                value2 = inMatrixY[i + 1];
                outPara = value1 + (inPara - x1) * (value1 - value2) / (x1 - x2);
                break;
            }
            else
                continue;
        }

    LOG1_AWDR( "%s:exit!\n", __FUNCTION__);
    return outPara;
}
/*used by gloabl mode*/
static uint32_t cam_ia_wdr_def_global_y[CAMERIC_WDR_CURVE_SIZE] = {
    0x0000, 0x00a2, 0x00a2, 0x016a,
    0x01e6, 0x029c, 0x02e6, 0x0368,
    0x03d9, 0x049b, 0x049b, 0x058d,
    0x0619, 0x069a, 0x0712, 0x0783,
    0x07ed, 0x0852, 0x090e, 0x0967,
    0x0a0f, 0x0a5f, 0x0af9, 0x0af9,
    0x0af9, 0x0c9a, 0x0d1e, 0x0d69,
    0x0e21, 0x0e9f, 0x0ef4, 0x0f74,
    0x1000,

};

/*used by block mode*/
static uint16_t cam_ia_wdr_def_block_y[CAMERIC_WDR_CURVE_SIZE] = {
    0x0000, 0x011c, 0x011c, 0x02d8,
    0x0375, 0x0478, 0x054f, 0x0609,
    0x06b0, 0x07d6, 0x07d6, 0x09b9,
    0x0b49, 0x0ca5, 0x0ddc, 0x0ef8,
    0x0fff, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000,
};

//each value means 2^(val+3)
static uint8_t  cam_ia_wdr_def_segment[CAMERIC_WDR_CURVE_SIZE - 1] = {
    0x0, 0x1, 0x1, 0x2, 0x3, 0x2, 0x3, 0x3, //0x33232110
    0x4, 0x3, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, //0x44444434
    0x4, 0x5, 0x4, 0x5, 0x4, 0x5, 0x5, 0x5, //0x55545454
    0x5, 0x5, 0x4, 0x5, 0x4, 0x3, 0x3, 0x2, //0x23345455
};

/******************************************************************************
 * AwdrConfig()
 *set default Config data
 *****************************************************************************/
void AwdrConfig
(
    AwdrConfig_t*           pAwdrConfig
) {
    LOGI_AHDR( "%s:enter!\n", __FUNCTION__);

    // initial checks
    DCT_ASSERT(pAwdrCtx != NULL);

    //defalt setting
    pAwdrConfig->enable = false;
    pAwdrConfig->mode = CAMERIC_WDR_MODE_GLOBAL;

    //strenght
    pAwdrConfig->WdrStrength.EnvLv[0] = 0;
    pAwdrConfig->WdrStrength.EnvLv[1] = 0.005 ;
    pAwdrConfig->WdrStrength.EnvLv[2] = 0.01 ;
    pAwdrConfig->WdrStrength.EnvLv[3] = 0.05 ;
    pAwdrConfig->WdrStrength.EnvLv[4] = 0.1 ;
    pAwdrConfig->WdrStrength.EnvLv[5] = 0.15 ;
    pAwdrConfig->WdrStrength.EnvLv[6] = 0.2;
    pAwdrConfig->WdrStrength.EnvLv[7] = 0.3 ;
    pAwdrConfig->WdrStrength.EnvLv[8] = 0.4 ;
    pAwdrConfig->WdrStrength.EnvLv[9] = 0.5 ;
    pAwdrConfig->WdrStrength.EnvLv[10] = 0.6 ;
    pAwdrConfig->WdrStrength.EnvLv[11] = 0.8 ;
    pAwdrConfig->WdrStrength.EnvLv[12] = 1.0 ;
    pAwdrConfig->WdrStrength.Level[0] = 1.0;
    pAwdrConfig->WdrStrength.Level[1] = 1.0;
    pAwdrConfig->WdrStrength.Level[2] = 1.0;
    pAwdrConfig->WdrStrength.Level[3] = 1.0;
    pAwdrConfig->WdrStrength.Level[4] = 1.0;
    pAwdrConfig->WdrStrength.Level[5] = 1.0;
    pAwdrConfig->WdrStrength.Level[6] = 1.0;
    pAwdrConfig->WdrStrength.Level[7] = 1.0;
    pAwdrConfig->WdrStrength.Level[8] = 1.0;
    pAwdrConfig->WdrStrength.Level[9] = 1.0;
    pAwdrConfig->WdrStrength.Level[10] = 1.0;
    pAwdrConfig->WdrStrength.Level[11] = 1.0;
    pAwdrConfig->WdrStrength.Level[12] = 1.0;
    pAwdrConfig->WdrStrength.Level_Final = 4.0;
    pAwdrConfig->WdrStrength.damp = 0.9;

    //config
    int index = 0;
    for (index = 0; index < (CAMERIC_WDR_CURVE_SIZE - 1); index++)
        pAwdrConfig->WdrConfig.segment[index] = cam_ia_wdr_def_segment[index];

    for (index = 0; index < (CAMERIC_WDR_CURVE_SIZE); index++)
        pAwdrConfig->WdrConfig.LocalCurve[index] = cam_ia_wdr_def_block_y[index];

    for (index = 0; index < (CAMERIC_WDR_CURVE_SIZE); index++)
        pAwdrConfig->WdrConfig.GlobalCurve[index] = cam_ia_wdr_def_global_y[index];

    pAwdrConfig->WdrConfig.wdr_noiseratio = 0x00ee;
    pAwdrConfig->WdrConfig.wdr_bestlight = 0x0ccc;
    pAwdrConfig->WdrConfig.wdr_gain_off1 = 0x000000cd;
    pAwdrConfig->WdrConfig.wdr_pym_cc = 0x3;
    pAwdrConfig->WdrConfig.wdr_epsilon = 0xc;
    pAwdrConfig->WdrConfig.wdr_lvl_en = 0xf;
    pAwdrConfig->WdrConfig.wdr_flt_sel = 0x1;
    pAwdrConfig->WdrConfig.wdr_gain_max_clip_enable = 0x1;
    pAwdrConfig->WdrConfig.wdr_bavg_clip = 0x3;
    pAwdrConfig->WdrConfig.wdr_nonl_segm = 0x0;
    pAwdrConfig->WdrConfig.wdr_nonl_open = 0x1;
    pAwdrConfig->WdrConfig.wdr_nonl_mode1 = 0x0;
    pAwdrConfig->WdrConfig.wdr_coe0 = 0x00000036;
    pAwdrConfig->WdrConfig.wdr_coe1 = 0x000000b7;
    pAwdrConfig->WdrConfig.wdr_coe2 = 0x00000012;
    pAwdrConfig->WdrConfig.wdr_coe_off = 0x0;

    LOG1_AWDR( "%s:exit!\n", __FUNCTION__);
}
void AwdrGetAeResult
(
    AwdrHandle_t           pAwdrCtx,
    AecPreResult_t  AecHdrPreResult
) {
    LOG1_AWDR( "%s:enter!\n", __FUNCTION__);

    //get Ae Pre Result
    pAwdrCtx->CurrAeResult.GlobalEnvLv = AecHdrPreResult.GlobalEnvLv[AecHdrPreResult.NormalIndex];

    //transfer CurrAeResult data into AwdrHandle
    switch (pAwdrCtx->FrameNumber)
    {
    case 1:
        pAwdrCtx->CurrAeResult.ISO = AecHdrPreResult.LinearExp.exp_real_params.analog_gain * 50.0;
        break;
    case 2:
        pAwdrCtx->CurrAeResult.ISO = AecHdrPreResult.HdrExp[1].exp_real_params.analog_gain * 50.0;
        break;
    case 3:
        pAwdrCtx->CurrAeResult.ISO = AecHdrPreResult.HdrExp[2].exp_real_params.analog_gain * 50.0;
        break;
    default:
        LOGE_AWDR("%s:  Wrong frame number in HDR mode!!!\n", __FUNCTION__);
        break;
    }

    //Normalize the current envLv for AEC
    float maxEnvLuma = 65 / 10;
    float minEnvLuma = 0;
    pAwdrCtx->CurrData.EnvLv = (pAwdrCtx->CurrAeResult.GlobalEnvLv  - minEnvLuma) / (maxEnvLuma - minEnvLuma);
    pAwdrCtx->CurrData.EnvLv = LIMIT_VALUE(pAwdrCtx->CurrData.EnvLv, AWDRENVLVMAX, AWDRENVLVMIN);

    //get iso
    pAwdrCtx->CurrData.ISO = pAwdrCtx->CurrAeResult.ISO;

    LOG1_AWDR( "%s:exit!\n", __FUNCTION__);
}

/******************************************************************************
 * AwdrUpdateConfig()
 *transfer html parameter into handle
 ***************************************************************************/
void AwdrUpdateConfig
(
    AwdrHandle_t           pAwdrCtx,
    CalibDb_Awdr_Para_t*         pCalibDb,
    int mode
) {
    LOG1_AWDR( "%s:enter!\n", __FUNCTION__);

    // initial checks
    DCT_ASSERT(pAwdrCtx != NULL);
    DCT_ASSERT(pCalibDb != NULL);

    pAwdrCtx->Config.enable = pCalibDb->Enbale && pCalibDb->Mode[mode].SceneEnbale;
    pAwdrCtx->Config.mode = pCalibDb->Mode[mode].mode == 0 ? CAMERIC_WDR_MODE_GLOBAL : CAMERIC_WDR_MODE_BLOCK;

    //config
    pAwdrCtx->Config.WdrConfig.wdr_noiseratio = (int)LIMIT_VALUE(pCalibDb->Mode[mode].WdrConfig.wdr_noiseratio, AWDR8BITMAX, AWDR8BITMIN);
    pAwdrCtx->Config.WdrConfig.wdr_bestlight = (int)LIMIT_VALUE(pCalibDb->Mode[mode].WdrConfig.wdr_bestlight, AWDR8BITMAX, AWDR8BITMIN);
    pAwdrCtx->Config.WdrConfig.wdr_gain_off1 = (int)(pCalibDb->Mode[mode].WdrConfig.wdr_gain_off1);
    pAwdrCtx->Config.WdrConfig.wdr_pym_cc = (int)LIMIT_VALUE(pCalibDb->Mode[mode].WdrConfig.wdr_pym_cc, AWDR8BITMAX, AWDR8BITMIN);
    pAwdrCtx->Config.WdrConfig.wdr_epsilon = (int)LIMIT_VALUE(pCalibDb->Mode[mode].WdrConfig.wdr_epsilon, AWDR8BITMAX, AWDR8BITMIN);
    pAwdrCtx->Config.WdrConfig.wdr_lvl_en = (int)LIMIT_VALUE(pCalibDb->Mode[mode].WdrConfig.wdr_lvl_en, AWDRLVLMAX, AWDRLVLMIN);
    pAwdrCtx->Config.WdrConfig.wdr_flt_sel = (int)LIMIT_VALUE(pCalibDb->Mode[mode].WdrConfig.wdr_flt_sel, AWDRSWITCHMAX, AWDRSWITCHMIN);
    pAwdrCtx->Config.WdrConfig.wdr_gain_max_clip_enable = (int)LIMIT_VALUE(pCalibDb->Mode[mode].WdrConfig.wdr_gain_max_clip_enable, AWDRSWITCHMAX, AWDRSWITCHMIN);
    pAwdrCtx->Config.WdrConfig.wdr_bavg_clip = (int)LIMIT_VALUE(pCalibDb->Mode[mode].WdrConfig.wdr_bavg_clip, AWDRBAVGMAX, AWDRBAVGMIN);
    pAwdrCtx->Config.WdrConfig.wdr_nonl_segm = (int)LIMIT_VALUE(pCalibDb->Mode[mode].WdrConfig.wdr_nonl_segm, AWDRSWITCHMAX, AWDRSWITCHMIN);
    pAwdrCtx->Config.WdrConfig.wdr_nonl_open = (int)LIMIT_VALUE(pCalibDb->Mode[mode].WdrConfig.wdr_nonl_open, AWDRSWITCHMAX, AWDRSWITCHMIN);
    pAwdrCtx->Config.WdrConfig.wdr_nonl_mode1 = (int)LIMIT_VALUE(pCalibDb->Mode[mode].WdrConfig.wdr_nonl_mode1, AWDRSWITCHMAX, AWDRSWITCHMIN);
    pAwdrCtx->Config.WdrConfig.wdr_coe0 = (int)LIMIT_VALUE(pCalibDb->Mode[mode].WdrConfig.wdr_coe0, AWDRCOEFMAX, AWDRCOEFMIN);
    pAwdrCtx->Config.WdrConfig.wdr_coe1 = (int)LIMIT_VALUE(pCalibDb->Mode[mode].WdrConfig.wdr_coe1, AWDRCOEFMAX, AWDRCOEFMIN);
    pAwdrCtx->Config.WdrConfig.wdr_coe2 = (int)LIMIT_VALUE(pCalibDb->Mode[mode].WdrConfig.wdr_coe2, AWDRCOEFMAX, AWDRCOEFMIN);
    pAwdrCtx->Config.WdrConfig.wdr_coe_off = (int)LIMIT_VALUE(pCalibDb->Mode[mode].WdrConfig.wdr_coe_off, AWDRCOEFOFFMAX, AWDRCOEFOFFMIN);
    for (int i = 0; i < CAMERIC_WDR_CURVE_SIZE; i++ ) {
        pAwdrCtx->Config.WdrConfig.LocalCurve[i] = (int)LIMIT_VALUE(pCalibDb->Mode[mode].WdrConfig.LocalCurve[i], LOCALCURVEMAX, LOCALCURVEMIN);
        pAwdrCtx->Config.WdrConfig.GlobalCurve[i] = (int)LIMIT_VALUE(pCalibDb->Mode[mode].WdrConfig.GlobalCurve[i], GLOBALCURVEMAX, GLOBALCURVEMIN);
    }

    //strength
    pAwdrCtx->Config.WdrStrength.damp = LIMIT_VALUE(pCalibDb->Mode[mode].WdrStrength.damp, AWDRDAMPMAX, AWDRDAMPMIN);
    pAwdrCtx->Config.WdrStrength.Tolerance = LIMIT_VALUE(pCalibDb->Mode[mode].WdrStrength.Tolerance, AWDRTOLERANCEMAX, AWDRTOLERANCEMIN);
    for(int i = 0; i < 13; i++) {
        pAwdrCtx->Config.WdrStrength.EnvLv[i] = LIMIT_VALUE(pCalibDb->Mode[mode].WdrStrength.Envlv[i], AWDRENVLVMAX, AWDRENVLVMIN);
        pAwdrCtx->Config.WdrStrength.Level[i] = LIMIT_VALUE(pCalibDb->Mode[mode].WdrStrength.Level[i], AWDRLEVELMAX, AWDRLEVELMin);
    }

    LOG1_AWDR( "%s:enable:%d mode:%d\n", __FUNCTION__, pAwdrCtx->Config.enable, pAwdrCtx->Config.mode);
    for(int i = 0; i < 13; i++)
        LOG1_AWDR( "%s:WdrStrength EnvLv[%d]:%f Level[%d]:%d\n", __FUNCTION__, i,
                   pAwdrCtx->Config.WdrStrength.EnvLv[i], i, pAwdrCtx->Config.WdrStrength.Level[i]);
    LOG1_AWDR( "%s:WdrStrength damp:%f Tolerance:%f\n", __FUNCTION__,
               pAwdrCtx->Config.WdrStrength.damp, pAwdrCtx->Config.WdrStrength.Tolerance);
    LOG1_AWDR( "%s:WdrConfig wdr_noiseratio:%d wdr_bestlight:%d wdr_gain_off1:%d\n", __FUNCTION__,
               pAwdrCtx->Config.WdrConfig.wdr_noiseratio, pAwdrCtx->Config.WdrConfig.wdr_bestlight, pAwdrCtx->Config.WdrConfig.wdr_gain_off1);
    LOG1_AWDR( "%s:WdrConfig wdr_pym_cc:%d wdr_epsilon:%d wdr_lvl_en:%d\n", __FUNCTION__,
               pAwdrCtx->Config.WdrConfig.wdr_pym_cc, pAwdrCtx->Config.WdrConfig.wdr_epsilon, pAwdrCtx->Config.WdrConfig.wdr_lvl_en);
    LOG1_AWDR( "%s:WdrConfig wdr_flt_sel:%d wdr_gain_max_clip_enable:%d wdr_bavg_clip:%d\n", __FUNCTION__,
               pAwdrCtx->Config.WdrConfig.wdr_flt_sel, pAwdrCtx->Config.WdrConfig.wdr_gain_max_clip_enable, pAwdrCtx->Config.WdrConfig.wdr_bavg_clip);
    LOG1_AWDR( "%s:WdrConfig wdr_nonl_segm:%d wdr_nonl_open:%d wdr_nonl_mode1:%d\n", __FUNCTION__,
               pAwdrCtx->Config.WdrConfig.wdr_nonl_segm, pAwdrCtx->Config.WdrConfig.wdr_nonl_open, pAwdrCtx->Config.WdrConfig.wdr_nonl_mode1);
    LOG1_AWDR( "%s:WdrConfig wdr_coe0:%d wdr_coe1:%d wdr_coe2:%d wdr_coe_off:%d\n", __FUNCTION__,
               pAwdrCtx->Config.WdrConfig.wdr_coe0, pAwdrCtx->Config.WdrConfig.wdr_coe1, pAwdrCtx->Config.WdrConfig.wdr_coe2
               , pAwdrCtx->Config.WdrConfig.wdr_coe_off);
    for(int i = 0; i < CAMERIC_WDR_CURVE_SIZE; i++)
        LOG1_AWDR( "%s:WdrConfig LocalCurve[%d]:%d GlobalCurve[%d]:%d\n", __FUNCTION__, i,
                   pAwdrCtx->Config.WdrConfig.LocalCurve[i], i, pAwdrCtx->Config.WdrConfig.GlobalCurve[i]);

    LOG1_AWDR( "%s:exit!\n", __FUNCTION__);
}

/******************************************************************************
 * AwdrGetProcRes()
 *****************************************************************************/
void AwdrGetProcRes
(
    AwdrHandle_t pAwdrCtx
) {

    LOG1_AWDR( "%s:enter!\n", __FUNCTION__);

    //damp
    bool enDamp = false;
    float damp = pAwdrCtx->Config.WdrStrength.damp;
    float diff = ABS(pAwdrCtx->CurrData.EnvLv - pAwdrCtx->PreData.EnvLv);
    diff = diff / pAwdrCtx->PreData.EnvLv;
    if (diff < pAwdrCtx->Config.WdrStrength.Tolerance)
        enDamp = false;
    else
        enDamp = true;

    if (enDamp) {
        pAwdrCtx->Config.WdrStrength.Level_Final = damp * pAwdrCtx->Config.WdrStrength.Level_Final
                + (1 - damp) * pAwdrCtx->PreData.Level_Final;
        LOGD_AWDR("%s:	After damping EnvLv:%f ISO:%f WdrStrength:%f \n", __FUNCTION__,  pAwdrCtx->CurrData.EnvLv,
                  pAwdrCtx->CurrData.ISO, pAwdrCtx->Config.WdrStrength.Level_Final);
    }

    //store in proc res
    pAwdrCtx->ProcRes.enable = pAwdrCtx->Config.enable;
    pAwdrCtx->ProcRes.mode = pAwdrCtx->Config.mode;
    for(int i = 0; i < (CAMERIC_WDR_CURVE_SIZE - 1); i++)
        pAwdrCtx->ProcRes.segment[i] = pAwdrCtx->Config.WdrConfig.segment[i];
    for(int i = 0; i < CAMERIC_WDR_CURVE_SIZE; i++) {
        pAwdrCtx->ProcRes.wdr_global_y[i] = pAwdrCtx->Config.WdrConfig.GlobalCurve[i];
        pAwdrCtx->ProcRes.wdr_block_y[i] = pAwdrCtx->Config.WdrConfig.LocalCurve[i];
    }
    pAwdrCtx->ProcRes.wdr_noiseratio = pAwdrCtx->Config.WdrConfig.wdr_noiseratio;
    pAwdrCtx->ProcRes.wdr_bestlight = pAwdrCtx->Config.WdrConfig.wdr_bestlight;
    pAwdrCtx->ProcRes.wdr_gain_off1 = pAwdrCtx->Config.WdrConfig.wdr_gain_off1;
    pAwdrCtx->ProcRes.wdr_pym_cc = pAwdrCtx->Config.WdrConfig.wdr_pym_cc;
    pAwdrCtx->ProcRes.wdr_epsilon = pAwdrCtx->Config.WdrConfig.wdr_epsilon;
    pAwdrCtx->ProcRes.wdr_lvl_en = pAwdrCtx->Config.WdrConfig.wdr_lvl_en;
    pAwdrCtx->ProcRes.wdr_flt_sel = pAwdrCtx->Config.WdrConfig.wdr_flt_sel;
    pAwdrCtx->ProcRes.wdr_gain_max_clip_enable = pAwdrCtx->Config.WdrConfig.wdr_gain_max_clip_enable;
    pAwdrCtx->ProcRes.wdr_gain_max_value = ((int)(pAwdrCtx->Config.WdrStrength.Level_Final + 0.5)) << 4;
    pAwdrCtx->ProcRes.wdr_bavg_clip = pAwdrCtx->Config.WdrConfig.wdr_bavg_clip;
    pAwdrCtx->ProcRes.wdr_nonl_segm = pAwdrCtx->Config.WdrConfig.wdr_nonl_segm;
    pAwdrCtx->ProcRes.wdr_nonl_open = pAwdrCtx->Config.WdrConfig.wdr_nonl_open;
    pAwdrCtx->ProcRes.wdr_nonl_mode1 = pAwdrCtx->Config.WdrConfig.wdr_nonl_mode1;
    pAwdrCtx->ProcRes.wdr_coe0 = pAwdrCtx->Config.WdrConfig.wdr_coe0;
    pAwdrCtx->ProcRes.wdr_coe1 = pAwdrCtx->Config.WdrConfig.wdr_coe1;
    pAwdrCtx->ProcRes.wdr_coe2 = pAwdrCtx->Config.WdrConfig.wdr_coe2;
    pAwdrCtx->ProcRes.wdr_coe_off = pAwdrCtx->Config.WdrConfig.wdr_coe_off;

    LOGD_AWDR("%s:	After damping Enable:%d\n", __FUNCTION__,  pAwdrCtx->ProcRes.enable);
    LOGD_AWDR("%s:	After damping Level:%f wdr_gain_max_value:%d\n", __FUNCTION__, pAwdrCtx->Config.WdrStrength.Level_Final,
              pAwdrCtx->ProcRes.wdr_gain_max_value);

    // store current handle data to pre data for next loop
    pAwdrCtx->PreData.EnvLv = pAwdrCtx->CurrData.EnvLv;
    pAwdrCtx->PreData.ISO = pAwdrCtx->CurrData.ISO;
    pAwdrCtx->PreData.Level_Final = pAwdrCtx->Config.WdrStrength.Level_Final;
    ++pAwdrCtx->frameCnt;

    LOG1_AWDR( "%s:exit!\n", __FUNCTION__);
}

/******************************************************************************
 * AwdrProcessing()
 *get handle para by config and current variate
 *****************************************************************************/
void AwdrProcessing
(
    AwdrHandle_t     pAwdrCtx,
    AecPreResult_t  AecHdrPreResult
)
{
    LOG1_AWDR("%s:enter!\n", __FUNCTION__);

    //get current ae data from AecPreRes
    AwdrGetAeResult(pAwdrCtx, AecHdrPreResult);

    //get Current wdr strength
    pAwdrCtx->Config.WdrStrength.Level_Final = AwdrGetCurrPara(pAwdrCtx->CurrData.EnvLv,
            pAwdrCtx->Config.WdrStrength.EnvLv, pAwdrCtx->Config.WdrStrength.Level);

    if(pAwdrCtx->wdrAttr.mode == WDR_OPMODE_API_OFF)
        LOGD_AWDR("%s:  Awdr api OFF!! Current Handle data:\n", __FUNCTION__);
    else if(pAwdrCtx->wdrAttr.mode == WDR_OPMODE_AUTO)
        LOGD_AWDR("%s:  Awdr api Auto!! Current Handle data:\n", __FUNCTION__);
    else if(pAwdrCtx->wdrAttr.mode == WDR_OPMODE_MANU) {
        LOGD_AWDR("%s:  Awdr api Manual!! Current Handle data:\n", __FUNCTION__);
        pAwdrCtx->Config.WdrStrength.Level_Final = LIMIT_VALUE(pAwdrCtx->wdrAttr.stManu.level, AWDRLEVELMAX, AWDRLEVELMin);
    }
    else
        LOGE_AWDR("%s:  Awdr wrong mode!!!\n", __FUNCTION__);

    LOGD_AWDR("%s:	Enable:%d\n", __FUNCTION__,  pAwdrCtx->Config.enable);
    LOGD_AWDR("%s:	Current EnvLv:%f ISO:%f WdrStrength:%f \n", __FUNCTION__,  pAwdrCtx->CurrData.EnvLv,
              pAwdrCtx->CurrData.ISO, pAwdrCtx->Config.WdrStrength.Level_Final);

    //transfer data to api
    pAwdrCtx->wdrAttr.CtlInfo.Envlv = pAwdrCtx->CurrData.EnvLv;
    pAwdrCtx->wdrAttr.CtlInfo.ISO = pAwdrCtx->CurrData.ISO;

    //get proc res
    AwdrGetProcRes(pAwdrCtx);

    LOG1_AWDR( "%s:exit!\n", __FUNCTION__);
}

/******************************************************************************
 * AwdrSetProcRes()
 *get handle para by config and current variate
 *****************************************************************************/
void AwdrSetProcRes
(
    RkAiqAwdrProcResult_t*     pHalProc,
    AwdrProcResData_t*  pAgoProc
)
{
    LOG1_AWDR("%s:enter!\n", __FUNCTION__);

    int regi = 0, i = 0;
    pHalProc->enable = pAgoProc->enable;
    if (pAgoProc->mode == CAMERIC_WDR_MODE_BLOCK)
        pHalProc->mode = WDR_MODE_BLOCK;
    else
        pHalProc->mode = WDR_MODE_GLOBAL;
    //TODO
    /*offset 0x2a00*/
    pHalProc->c_wdr[0] = 0x00000812;

    //ISP_WDR_2A00_WDR_TONECURVE_DYN1~ISP_WDR_2A00_WDR_TONECURVE_DYN14   offset:0x2a04 - 0x2a10
#if 1
    for (regi = 1; regi < 5; regi++) {
        pHalProc->c_wdr[regi] = 0;
        for (int i = 0; i < 8; i++)
            pHalProc->c_wdr[regi] |=
                (uint32_t)(pAgoProc->segment[i + (regi - 1) * 8 ]) << (4 * i);
    }
#else
    int data[8] = {0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0x10};
    for (regi = 1; regi < 5; regi++) {
        pHalProc->c_wdr[regi] = 0;
        for (int i = 0; i < 8; i++)
            pHalProc->c_wdr[regi] |= data(i);
    }
#endif

    //ISP_WDR_2A00_TONECURVE_YM_0~ ISP_WDR_2A00_TONECURVE_YM_32   offset:0x2a14 - 0x2a94
    for (regi = 5; regi < 38; regi++)
        pHalProc->c_wdr[regi] =
            ((uint32_t)(pAgoProc->wdr_block_y[regi - 5]) << 16) |
            (uint32_t)(pAgoProc->wdr_global_y[regi - 5]);

    //ISP_WDR_2A00_OFFSET   offset:0x2a98
    pHalProc->c_wdr[38] = 0x0;

    //offset:0x2a9c
    pHalProc->c_wdr[39] = 0x0;
#if 0
    /*offset 0x2b50 - 0x2b6c*/
    pHalProc->c_wdr[40] = 0x00030cf0;
    pHalProc->c_wdr[41] = 0x000140d3;
    pHalProc->c_wdr[42] = 0x000000cd;
    pHalProc->c_wdr[43] = 0x0ccc00ee;
    pHalProc->c_wdr[44] = 0x00000036;
    pHalProc->c_wdr[45] = 0x000000b7;
    pHalProc->c_wdr[46] = 0x00000012;
    pHalProc->c_wdr[47] = 0x0;
#else
    //ISP_WDR_2A00_CTRL0   offset:0x2b50
    pHalProc->c_wdr[40] =
        ((uint32_t)(pAgoProc->wdr_pym_cc) << 16) |
        ((uint32_t)(pAgoProc->wdr_epsilon) << 8) |
        ((uint32_t)(pAgoProc->wdr_lvl_en) << 4) ;

    //ISP_WDR_2A00_CTRL1   offset:0x2b54
    pHalProc->c_wdr[41] =
        ((uint32_t)(pAgoProc->wdr_gain_max_clip_enable) << 16) |
        ((uint32_t)(pAgoProc->wdr_gain_max_value) << 8) |
        ((uint32_t)(pAgoProc->wdr_bavg_clip) << 6) |
        ((uint32_t)(pAgoProc->wdr_nonl_segm) << 5) |
        ((uint32_t)(pAgoProc->wdr_nonl_open) << 4) |
        ((uint32_t)(pAgoProc->wdr_nonl_mode1) << 3) |
        ((uint32_t)(pAgoProc->wdr_flt_sel) << 1) |
        pAgoProc->mode ;

    //ISP_WDR_2A00_BLKOFF0   offset:0x2b58
    pHalProc->c_wdr[42] = pAgoProc->wdr_gain_off1;

    //ISP_WDR_2A00_AVGCLIP   offset:0x2b5c
    pHalProc->c_wdr[43] =
        ((uint32_t)(pAgoProc->wdr_bestlight) << 20) |
        (uint32_t)(pAgoProc->wdr_noiseratio << 4);

    //ISP_WDR_2A00_COE_0   offset:0x2b60
    pHalProc->c_wdr[44] = pAgoProc->wdr_coe0;

    //ISP_WDR_2A00_COE_1   offset:0x2b64
    pHalProc->c_wdr[45] =  pAgoProc->wdr_coe1;

    //ISP_WDR_2A00_COE_2   offset:0x2b68
    pHalProc->c_wdr[46] =  pAgoProc->wdr_coe2;

    //ISP_WDR_2A00_COE_OFF   offset:0x2b6c
    pHalProc->c_wdr[47] =  pAgoProc->wdr_coe_off;
#endif

    LOG1_AWDR( "%s:ISP_WDR_2A00_CTRL0[0x%x] ISP_WDR_2A00_CTRL1[0x%x]\n", __FUNCTION__,
               pHalProc->c_wdr[40], pHalProc->c_wdr[41]);
    LOG1_AWDR( "%s:ISP_WDR_2A00_BLKOFF0[0x%x] ISP_WDR_2A00_AVGCLIP[0x%x]\n", __FUNCTION__,
               pHalProc->c_wdr[42], pHalProc->c_wdr[43]);
    LOG1_AWDR( "%s:ISP_WDR_2A00_COE_0[0x%x] ISP_WDR_2A00_COE_1[0x%x]\n", __FUNCTION__,
               pHalProc->c_wdr[44], pHalProc->c_wdr[45]);
    LOG1_AWDR( "%s:ISP_WDR_2A00_COE_2[0x%x] ISP_WDR_2A00_COE_OFF[0x%x]\n", __FUNCTION__,
               pHalProc->c_wdr[46], pHalProc->c_wdr[47]);

    LOG1_AWDR( "%s:exit!\n", __FUNCTION__);
}

/******************************************************************************
 * AwdrInit()
 *****************************************************************************/
RESULT AwdrInit
(
    AwdrInstanceConfig_t* pInstConfig,
    CamCalibDbContext_t* pCalibDb
) {

    AwdrContext_s *pAwdrCtx;

    LOG1_AWDR("%s:enter!\n", __FUNCTION__);

    RESULT result = AWDR_RET_SUCCESS;

    // initial checks
    if (pInstConfig == NULL)
        return (AWDR_RET_INVALID_PARM);

    // allocate AWDR control context
    pAwdrCtx = (AwdrContext_s*)malloc(sizeof(AwdrContext_s));
    if (NULL == pAwdrCtx) {
        LOGE_AWDR( "%s: Can't allocate AWDR context\n",  __FUNCTION__);
        return (AWDR_RET_OUTOFMEM);
    }

    // pre-initialize context
    memset(pAwdrCtx, 0x00, sizeof(*pAwdrCtx));
    pAwdrCtx->state = AWDR_STATE_INITIALIZED;
    pAwdrCtx->wdrAttr.mode = WDR_OPMODE_API_OFF;
    AwdrConfig(&pAwdrCtx->Config);//set default para
    //default api info
    pAwdrCtx->wdrAttr.CtlInfo.Envlv = 0;
    pAwdrCtx->wdrAttr.CtlInfo.ISO = 50;
    //set pre data
    pAwdrCtx->PreData.ISO = 50;
    pAwdrCtx->PreData.EnvLv = 0;
    pAwdrCtx->PreData.Level_Final = 1;
    memcpy(&pAwdrCtx->pCalibDB, &pCalibDb->awdr, sizeof(CalibDb_Awdr_Para_t));//load iq paras
    memcpy(&pAwdrCtx->wdrAttr.stAuto, &pCalibDb->awdr, sizeof(CalibDb_Awdr_Para_t));
    pInstConfig->hAwdr = (AwdrHandle_t)pAwdrCtx;

    LOG1_AWDR( "%s:exit!\n", __FUNCTION__);

    return (AWDR_RET_SUCCESS);
}
/******************************************************************************
 * AwdrRelease()
 *****************************************************************************/
RESULT AwdrRelease
(
    AwdrHandle_t pAwdrCtx
) {

    LOG1_AWDR( "%s:enter!\n", __FUNCTION__);
    RESULT result = AWDR_RET_SUCCESS;

    // initial checks
    if (NULL == pAwdrCtx) {
        return (AWDR_RET_WRONG_HANDLE);
    }

    result = AwdrStop(pAwdrCtx);
    if (result != AWDR_RET_SUCCESS) {
        LOGE_AWDR( "%s: AWDR Stop() failed!\n", __FUNCTION__);
        return (result);
    }

    // check state
    if ((AWDR_STATE_RUNNING == pAwdrCtx->state)
            || (AWDR_STATE_LOCKED == pAwdrCtx->state)) {
        return (AWDR_RET_BUSY);
    }

    memset(pAwdrCtx, 0, sizeof(AwdrContext_s));
    free(pAwdrCtx);
    pAwdrCtx = NULL;

    LOG1_AWDR( "%s:exit!\n", __FUNCTION__);

    return (AWDR_RET_SUCCESS);
}
