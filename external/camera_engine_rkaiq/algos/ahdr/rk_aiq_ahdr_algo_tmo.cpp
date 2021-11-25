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
 * @file tmo.cpp
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include "math.h"
#include "rk_aiq_types_ahdr_algo_int.h"
#include "rk_aiq_ahdr_algo_tmo.h"

/******************************************************************************
 * GetSetLgmean()
 *****************************************************************************/
unsigned short GetSetLgmean(AhdrHandle_t pAhdrCtx)
{
    LOG1_AHDR( "%s:enter!\n", __FUNCTION__);

    float value = 0;
    float value_default = 20000;
    unsigned short returnValue;
    int iir_frame = 0;

    if(pAhdrCtx->AhdrConfig.tmo_para.global.iir < IIRMAX) {
        iir_frame = (int)(pAhdrCtx->AhdrConfig.tmo_para.global.iir);
        int iir_frame_real = MIN(pAhdrCtx->frameCnt + 1, iir_frame);

        float PrevLgMean = pAhdrCtx->AhdrPrevData.ro_hdrtmo_lgmean / 2048.0;
        float CurrLgMean = pAhdrCtx->CurrHandleData.CurrLgMean;

        CurrLgMean *= 1 + 0.5 - pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobalTmoStrength;
        value_default *= 1 + 0.5 - pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobalTmoStrength;

        value = pAhdrCtx->frameCnt == 0 ? value_default :
                (iir_frame_real - 1) * PrevLgMean / iir_frame_real + CurrLgMean / iir_frame_real;
        returnValue = (int)SHIFT11BIT(value) ;
    }
    else
        returnValue = value_default;

    LOG1_AHDR( "%s: frameCnt:%d iir_frame:%d set_lgmean_float:%f set_lgmean_return:%d\n", __FUNCTION__,
               pAhdrCtx->frameCnt, iir_frame, value, returnValue);

    LOG1_AHDR( "%s:exit!\n", __FUNCTION__);

    return returnValue;
}
/******************************************************************************
 * GetSetLgAvgMax()
 *****************************************************************************/
unsigned short GetSetLgAvgMax(AhdrHandle_t pAhdrCtx, float set_lgmin, float set_lgmax)
{
    LOG1_AHDR("%s:Enter!\n", __FUNCTION__);
    float WeightKey = pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_weightkey / 256.0;
    float value = 0.0;
    float set_lgmean = pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_lgmean / 2048.0;
    float lgrange1 = pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_lgrange1 / 2048.0;
    unsigned short returnValue;

    value = WeightKey * set_lgmax + (1 - WeightKey) * set_lgmean;
    value = MIN(value, lgrange1);
    returnValue = (int)SHIFT11BIT(value);

    LOG1_AHDR( "%s: set_lgmin:%f set_lgmax:%f set_lgmean:%f lgrange1:%f value:%f returnValue:%d\n", __FUNCTION__, set_lgmin, set_lgmax, set_lgmean, lgrange1, value, returnValue);

    return returnValue;
    LOG1_AHDR("%s:Eixt!\n", __FUNCTION__);

}
/******************************************************************************
 * GetSetLgRange0()
 *****************************************************************************/
unsigned short GetSetLgRange0(AhdrHandle_t pAhdrCtx, float set_lgmin, float set_lgmax)
{
    LOG1_AHDR("%s:Enter!\n", __FUNCTION__);
    float value = 0.0;
    float clipratio0 = (float)(pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_clipratio0) / 256.0;
    float clipgap0 = pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_clipgap0 / 4.0;
    unsigned short returnValue;


    value = set_lgmin * (1 - clipratio0) + set_lgmax * clipratio0;
    value = MIN(value, (set_lgmin + clipgap0));
    returnValue = (int)SHIFT11BIT(value);

    LOG1_AHDR( "%s: set_lgmin:%f set_lgmax:%f clipratio0:%f clipgap0:%f value:%f returnValue:%d\n", __FUNCTION__, set_lgmin, set_lgmax, clipratio0, clipgap0, value, returnValue);

    return returnValue;
    LOG1_AHDR("%s:Eixt!\n", __FUNCTION__);

}
/******************************************************************************
 * GetSetLgRange1()
 *****************************************************************************/
unsigned short GetSetLgRange1(AhdrHandle_t pAhdrCtx, float set_lgmin, float set_lgmax)
{
    LOG1_AHDR("%s:Enter!\n", __FUNCTION__);
    float value = 0.0;
    float clipratio1 = pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_clipratio1 / 256.0;
    float clipgap1 = pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_clipgap1 / 4.0;
    unsigned short returnValue;

    value = set_lgmin * (1 - clipratio1) + set_lgmax * clipratio1;
    value = MAX(value, (set_lgmax - clipgap1));
    returnValue = (int)SHIFT11BIT(value);

    LOG1_AHDR( "%s: set_lgmin:%f set_lgmax:%f clipratio1:%f clipgap1:%f value:%f returnValue:%d\n", __FUNCTION__, set_lgmin, set_lgmax, clipratio1, clipgap1, value, returnValue);

    return returnValue;
    LOG1_AHDR("%s:Eixt!\n", __FUNCTION__);

}
/******************************************************************************
 * GetSetPalhpa()
 *****************************************************************************/
unsigned short GetSetPalhpa(AhdrHandle_t pAhdrCtx, float set_lgmin, float set_lgmax)
{
    LOG1_AHDR("%s:Enter!\n", __FUNCTION__);

    float index = 0.0;
    float value = 0.0;
    float set_lgmean = pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_lgmean / 2048.0;
    float palpha_0p18 = pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_palpha_0p18 / 1024.0;
    int value_int = 0;
    unsigned short returnValue;

    index = 2 * set_lgmean - set_lgmin - set_lgmax;
    index = index / (set_lgmax - set_lgmin);
    value = palpha_0p18 * pow(4, index);
    value_int = (int)SHIFT10BIT(value);
    value_int = MIN(value_int, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_maxpalpha);
    returnValue = value_int;

    LOG1_AHDR( "%s: set_lgmin:%f set_lgmax:%f set_lgmean:%f palpha_0p18:%f value:%f returnValue:%d\n", __FUNCTION__, set_lgmin, set_lgmax, set_lgmean, palpha_0p18, value, returnValue);

    return returnValue;
    LOG1_AHDR("%s:Eixt!\n", __FUNCTION__);

}
/******************************************************************************
 * GetCurrIOData()
 *****************************************************************************/
void TmoGetCurrIOData
(
    AhdrHandle_t pAhdrCtx
)
{
    LOG1_AHDR("%s:Enter!\n", __FUNCTION__);

    //default IO data
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_clipratio0 = 64;
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_clipratio1 = 166;
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_clipgap0 = 12;
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_clipgap1 = 12;
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_ratiol = 32;
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_hist_high = (int)(pAhdrCtx->height * pAhdrCtx->width * 0.01 / 16);
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_hist_min = 0;
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_hist_low = 0;
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_hist_0p3 = 0;
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_hist_shift = 3;
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_gain_ld_off1 = 10;
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_gain_ld_off2 = 5;
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_newhist_en = 1;
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_cnt_mode = 1;
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_cnt_vsize = (int)(pAhdrCtx->height - 256);
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_cfg_alpha = 255;

    //IO data from IQ
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_maxpalpha = (int)(pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeMaxLuma + 0.5);
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_palpha_0p18 =  (int)(pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeLuma + 0.5);
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_palpha_lw0p5 = (int)(pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsHighLight + 0.5);
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_weightkey = (int)(pAhdrCtx->CurrHandleData.CurrTmoHandleData.LocalTmoStrength + 0.5);

    float lwscl = pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsLowLight;
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_palpha_lwscl = (int)(lwscl + 0.5);
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_maxgain = (int)(SHIFT12BIT(lwscl / 16) + 0.5);

    //calc other IO data
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_big_en = pAhdrCtx->width > BIGMODE ? 1 : 0;
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_nobig_en = (int)(1 - pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_big_en);
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_expl_lgratio = (int)SHIFT11BIT(log(pAhdrCtx->AhdrPrevData.PreLExpo / pAhdrCtx->CurrHandleData.CurrLExpo) / log(2));
    if(pAhdrCtx->AhdrConfig.tmo_para.isLinearTmo)
        pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_lgscl_ratio = 128;
    else
        pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_lgscl_ratio = (int)SHIFT7BIT(log(pAhdrCtx->CurrHandleData.CurrL2S_Ratio) / log(pAhdrCtx->AhdrPrevData.PreL2S_ratio));
    float lgmax = 12 + log(pAhdrCtx->CurrHandleData.CurrL2S_Ratio) / log(2);
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_lgmax = (int)SHIFT11BIT(lgmax);
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_lgscl = (int)SHIFT12BIT(16 / lgmax);
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_lgscl_inv = (int)SHIFT12BIT(lgmax / 16);
    float set_lgmin = 0;
    float set_lgmax = lgmax;
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_lgmin = (int)SHIFT11BIT(set_lgmin) ;
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_lgmax = pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_lgmax;
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_gainoff = pow(2, set_lgmin);
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_lgmean = GetSetLgmean(pAhdrCtx);
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_lgrange0 = GetSetLgRange0(pAhdrCtx, set_lgmin, set_lgmax);
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_lgrange1 = GetSetLgRange1(pAhdrCtx, set_lgmin, set_lgmax);
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_lgavgmax = GetSetLgAvgMax(pAhdrCtx, set_lgmin, set_lgmax);
    pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_palpha = GetSetPalhpa(pAhdrCtx, set_lgmin, set_lgmax);

    //for avoid tmo flicker
    pAhdrCtx->AhdrProcRes.TmoFlicker.cnt_mode = pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_cnt_mode;
    pAhdrCtx->AhdrProcRes.TmoFlicker.cnt_vsize = pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_cnt_vsize;
    if(pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobalTmoStrength > 0.5)
        pAhdrCtx->AhdrProcRes.TmoFlicker.GlobalTmoStrengthDown = false;
    else
        pAhdrCtx->AhdrProcRes.TmoFlicker.GlobalTmoStrengthDown = true;
    pAhdrCtx->AhdrProcRes.TmoFlicker.GlobalTmoStrength = pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobalTmoStrength - 0.5;
    pAhdrCtx->AhdrProcRes.TmoFlicker.GlobalTmoStrength = pAhdrCtx->AhdrProcRes.TmoFlicker.GlobalTmoStrength < 0 ? (1 - pAhdrCtx->AhdrProcRes.TmoFlicker.GlobalTmoStrength)
            : (1 + pAhdrCtx->AhdrProcRes.TmoFlicker.GlobalTmoStrength);

    pAhdrCtx->AhdrProcRes.TmoFlicker.iir = (int)(pAhdrCtx->AhdrConfig.tmo_para.global.iir) +
                                           3 * pAhdrCtx->CurrAeResult.AecDelayframe;
    pAhdrCtx->AhdrProcRes.TmoFlicker.iirmax = IIRMAX;
    pAhdrCtx->AhdrProcRes.TmoFlicker.height = pAhdrCtx->height;
    pAhdrCtx->AhdrProcRes.TmoFlicker.width = pAhdrCtx->width;
    pAhdrCtx->AhdrProcRes.TmoFlicker.PredictK.correction_factor = 1.05;
    pAhdrCtx->AhdrProcRes.TmoFlicker.PredictK.correction_offset = 0;
    pAhdrCtx->AhdrProcRes.TmoFlicker.PredictK.Hdr3xLongPercent = 0.5;
    pAhdrCtx->AhdrProcRes.TmoFlicker.PredictK.UseLongLowTh = 1.02;
    pAhdrCtx->AhdrProcRes.TmoFlicker.PredictK.UseLongUpTh = 0.98;
    if(pAhdrCtx->FrameNumber == 1)
        pAhdrCtx->AhdrProcRes.TmoFlicker.LumaDeviation[0] = pAhdrCtx->CurrAeResult.LumaDeviationLinear;
    else if(pAhdrCtx->FrameNumber == 2) {
        pAhdrCtx->AhdrProcRes.TmoFlicker.LumaDeviation[0] = pAhdrCtx->CurrAeResult.LumaDeviationS;
        pAhdrCtx->AhdrProcRes.TmoFlicker.LumaDeviation[1] = pAhdrCtx->CurrAeResult.LumaDeviationL;
    }
    else if(pAhdrCtx->FrameNumber == 3) {
        pAhdrCtx->AhdrProcRes.TmoFlicker.LumaDeviation[0] = pAhdrCtx->CurrAeResult.LumaDeviationS;
        pAhdrCtx->AhdrProcRes.TmoFlicker.LumaDeviation[1] = pAhdrCtx->CurrAeResult.LumaDeviationM;
        pAhdrCtx->AhdrProcRes.TmoFlicker.LumaDeviation[2] = pAhdrCtx->CurrAeResult.LumaDeviationL;
    }
    pAhdrCtx->AhdrProcRes.TmoFlicker.StableThr = 0.1;

    LOGV_AHDR("%s:  Tmo set IOdata to register:\n", __FUNCTION__);
    LOGV_AHDR("%s:  float lgmax:%f\n", __FUNCTION__, lgmax);
    LOGV_AHDR("%s:  sw_hdrtmo_lgmax:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_lgmax);
    LOGV_AHDR("%s:  sw_hdrtmo_lgscl:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_lgscl);
    LOGV_AHDR("%s:  sw_hdrtmo_lgscl_inv:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_lgscl_inv);
    LOGV_AHDR("%s:  sw_hdrtmo_clipratio0:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_clipratio0);
    LOGV_AHDR("%s:  sw_hdrtmo_clipratio1:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_clipratio1);
    LOGV_AHDR("%s:  sw_hdrtmo_clipgap0:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_clipgap0);
    LOGV_AHDR("%s:  sw_hdrtmo_clipgap:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_clipgap1);
    LOGV_AHDR("%s:  sw_hdrtmo_ratiol:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_ratiol);
    LOGV_AHDR("%s:  sw_hdrtmo_hist_min:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_hist_min);
    LOGV_AHDR("%s:  sw_hdrtmo_hist_low:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_hist_low);
    LOGV_AHDR("%s:  sw_hdrtmo_hist_high:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_hist_high);
    LOGV_AHDR("%s:  sw_hdrtmo_hist_0p3:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_hist_0p3);
    LOGV_AHDR("%s:  sw_hdrtmo_hist_shift:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_hist_shift);
    LOGV_AHDR("%s:  sw_hdrtmo_palpha_0p18:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_palpha_0p18);
    LOGV_AHDR("%s:  sw_hdrtmo_palpha_lw0p5:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_palpha_lw0p5);
    LOGV_AHDR("%s:  sw_hdrtmo_palpha_lwscl:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_palpha_lwscl);
    LOGV_AHDR("%s:  sw_hdrtmo_maxpalpha:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_maxpalpha);
    LOGV_AHDR("%s:  sw_hdrtmo_maxgain:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_maxgain);
    LOGV_AHDR("%s:  sw_hdrtmo_cfg_alpha:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_cfg_alpha);
    LOGV_AHDR("%s:  sw_hdrtmo_set_gainoff:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_gainoff);
    LOGV_AHDR("%s:  sw_hdrtmo_set_lgmin:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_lgmin);
    LOGV_AHDR("%s:  sw_hdrtmo_set_lgmax:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_lgmax);
    LOGV_AHDR("%s:  sw_hdrtmo_set_lgmean:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_lgmean);
    LOGV_AHDR("%s:  sw_hdrtmo_set_weightkey:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_weightkey);
    LOGV_AHDR("%s:  sw_hdrtmo_set_lgrange0:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_lgrange0);
    LOGV_AHDR("%s:  sw_hdrtmo_set_lgrange1:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_lgrange1);
    LOGV_AHDR("%s:  sw_hdrtmo_set_lgavgmax:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_lgavgmax);
    LOGV_AHDR("%s:  sw_hdrtmo_set_palpha:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_palpha);
    LOGV_AHDR("%s:  sw_hdrtmo_big_en:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_big_en);
    LOGV_AHDR("%s:  sw_hdrtmo_nobig_en:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_nobig_en);
    LOGV_AHDR("%s:  sw_hdrtmo_newhist_en:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_newhist_en);
    LOGV_AHDR("%s:  sw_hdrtmo_cnt_mode:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_cnt_mode);
    LOGV_AHDR("%s:  sw_hdrtmo_cnt_vsize:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_cnt_vsize);
    LOGV_AHDR("%s:  sw_hdrtmo_expl_lgratio:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_expl_lgratio);
    LOGV_AHDR("%s:  sw_hdrtmo_lgscl_ratio:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_lgscl_ratio);
    LOGV_AHDR("%s:  sw_hdrtmo_gain_ld_off1:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_gain_ld_off1);
    LOGV_AHDR("%s: sw_hdrtmo_gain_ld_off2:%d\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_gain_ld_off2);
    LOGV_AHDR("%s: LumaDeviation:%f %f %f\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.TmoFlicker.LumaDeviation[0],
              pAhdrCtx->AhdrProcRes.TmoFlicker.LumaDeviation[1], pAhdrCtx->AhdrProcRes.TmoFlicker.LumaDeviation[2]);

    LOG1_AHDR("%s:Eixt!\n", __FUNCTION__);
}

/******************************************************************************
 * TMOProcessing()
 *****************************************************************************/
void TmoProcessing
(
    AhdrHandle_t pAhdrCtx
)
{
    LOG1_AHDR("%s:Enter!\n", __FUNCTION__);
    float tmo_damp = pAhdrCtx->CurrHandleData.TmoDamp;

    bool enDampLuma;
    bool enDampDtlsHighLgt;
    bool enDampDtlslowLgt;
    bool enDampLocal;
    bool enDampGlobal;
    bool ifHDRModeChange = pAhdrCtx->CurrHandleData.MergeMode == pAhdrCtx->AhdrPrevData.MergeMode ? false : true;

    if(pAhdrCtx->hdrAttr.opMode == HDR_OpMode_Api_OFF && pAhdrCtx->frameCnt != 0 && !ifHDRModeChange)
    {
        float diff = ABS(pAhdrCtx->CurrHandleData.CurrEnvLv - pAhdrCtx->AhdrPrevData.PreEnvlv);
        diff = diff / pAhdrCtx->AhdrPrevData.PreEnvlv;
        if (diff < pAhdrCtx->AhdrConfig.tmo_para.Luma.Tolerance)
            enDampLuma = false;
        else
            enDampLuma = true;

        int DetailsHighLight_mode = (int)pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLightMode;
        if(DetailsHighLight_mode == 0)
        {
            diff = ABS(pAhdrCtx->CurrHandleData.CurrOEPdf - pAhdrCtx->AhdrPrevData.PreOEPdf);
            diff = diff / pAhdrCtx->AhdrPrevData.PreOEPdf;
        }
        else if(DetailsHighLight_mode == 1)
        {
            diff = ABS(pAhdrCtx->CurrHandleData.CurrEnvLv - pAhdrCtx->AhdrPrevData.PreEnvlv);
            diff = diff / pAhdrCtx->AhdrPrevData.PreEnvlv;
        }
        if (diff < pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.Tolerance)
            enDampDtlsHighLgt = false;
        else
            enDampDtlsHighLgt = true;

        int DetailsLowLight_mode = (int)pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLightMode;
        if(DetailsLowLight_mode == 0)
        {
            diff = ABS(pAhdrCtx->CurrHandleData.CurrTotalFocusLuma - pAhdrCtx->AhdrPrevData.PreTotalFocusLuma);
            diff = diff / pAhdrCtx->AhdrPrevData.PreTotalFocusLuma;
        }
        else if(DetailsLowLight_mode == 1)
        {
            diff = ABS(pAhdrCtx->CurrHandleData.CurrDarkPdf - pAhdrCtx->AhdrPrevData.PreDarkPdf);
            diff = diff / pAhdrCtx->AhdrPrevData.PreDarkPdf;
        }
        else if(DetailsLowLight_mode == 2)
        {
            diff = ABS(pAhdrCtx->CurrHandleData.CurrISO - pAhdrCtx->AhdrPrevData.PreISO);
            diff = diff / pAhdrCtx->AhdrPrevData.PreISO;
        }
        if (diff < pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.Tolerance)
            enDampDtlslowLgt = false;
        else
            enDampDtlslowLgt = true;

        int LocalMode = (int)pAhdrCtx->AhdrConfig.tmo_para.local.localtmoMode;
        if(LocalMode == 0) {
            diff = ABS(pAhdrCtx->CurrHandleData.CurrDynamicRange - pAhdrCtx->AhdrPrevData.PreDynamicRange);
            diff = diff / pAhdrCtx->AhdrPrevData.PreDynamicRange;
        }
        else if(LocalMode == 1)
        {
            diff = ABS(pAhdrCtx->CurrHandleData.CurrEnvLv - pAhdrCtx->AhdrPrevData.PreEnvlv);
            diff = diff / pAhdrCtx->AhdrPrevData.PreEnvlv;
        }
        if (diff < pAhdrCtx->AhdrConfig.tmo_para.local.Tolerance)
            enDampLocal = false;
        else
            enDampLocal = true;

        int GlobalMode = (int)pAhdrCtx->AhdrConfig.tmo_para.global.mode;
        if(GlobalMode == 0) {
            diff = ABS(pAhdrCtx->CurrHandleData.CurrDynamicRange - pAhdrCtx->AhdrPrevData.PreDynamicRange);
            diff = diff / pAhdrCtx->AhdrPrevData.PreDynamicRange;
        }
        else if(GlobalMode == 1)
        {
            diff = ABS(pAhdrCtx->CurrHandleData.CurrEnvLv - pAhdrCtx->AhdrPrevData.PreEnvlv);
            diff = diff / pAhdrCtx->AhdrPrevData.PreEnvlv;
        }
        if (diff < pAhdrCtx->AhdrConfig.tmo_para.global.Tolerance)
            enDampGlobal = false;
        else
            enDampGlobal = true;


        //get finnal cfg data by damp
        if (enDampLuma == true)
        {
            pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeMaxLuma = tmo_damp * pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeMaxLuma
                    + (1 - tmo_damp) * pAhdrCtx->AhdrPrevData.PrevTmoHandleData.GlobeMaxLuma;
            pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeLuma = tmo_damp * pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeLuma
                    + (1 - tmo_damp) * pAhdrCtx->AhdrPrevData.PrevTmoHandleData.GlobeLuma;
        }

        if (enDampDtlsHighLgt == true)
            pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsHighLight = tmo_damp * pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsHighLight
                    + (1 - tmo_damp) * pAhdrCtx->AhdrPrevData.PrevTmoHandleData.DetailsHighLight;

        if (enDampDtlslowLgt == true)
            pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsLowLight = tmo_damp * pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsLowLight
                    + (1 - tmo_damp) * pAhdrCtx->AhdrPrevData.PrevTmoHandleData.DetailsLowLight;

        if (enDampLocal == true)
            pAhdrCtx->CurrHandleData.CurrTmoHandleData.LocalTmoStrength = tmo_damp * pAhdrCtx->CurrHandleData.CurrTmoHandleData.LocalTmoStrength
                    + (1 - tmo_damp) * pAhdrCtx->AhdrPrevData.PrevTmoHandleData.LocalTmoStrength;

        if (enDampGlobal == true)
            pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobalTmoStrength = tmo_damp * pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobalTmoStrength
                    + (1 - tmo_damp) * pAhdrCtx->AhdrPrevData.PrevTmoHandleData.GlobalTmoStrength;

    }


    //get current IO data
    TmoGetCurrIOData(pAhdrCtx);

    LOGD_AHDR("%s:Current damp GlobeLuma:%f GlobeMaxLuma:%f DetailsHighLight:%f DetailsLowLight:%f LocalTmoStrength:%f GlobalTmoStrength:%f\n", __FUNCTION__, pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeLuma
              , pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeMaxLuma, pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsHighLight, pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsLowLight
              , pAhdrCtx->CurrHandleData.CurrTmoHandleData.LocalTmoStrength, pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobalTmoStrength);

    LOG1_AHDR("%s:Eixt!\n", __FUNCTION__);
}
