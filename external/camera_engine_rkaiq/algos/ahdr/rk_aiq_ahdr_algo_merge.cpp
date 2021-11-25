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
 * @file merge.cpp
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include "math.h"
#include "rk_aiq_types_ahdr_algo_int.h"
#include "rk_aiq_ahdr_algo_merge.h"

/******************************************************************************
* CalibrateOECurve()
*****************************************************************************/
void CalibrateOECurve
(
    float smooth, float offset, unsigned short *OECurve
)
{
    LOG1_AHDR("%s:Enter!\n", __FUNCTION__);

    int step = 32 ;
    float curve = 0;
    float k = 511;

    for(int i = 0; i < 17; ++i)
    {
        curve = 1 + exp(-smooth * (k / 1023 - offset / 256));
        curve = 1024 / curve ;
        OECurve[i] = round(curve) ;
        OECurve[i] = MIN(OECurve[i], 1023);
        k += step ;
    }

    LOG1_AHDR("%s:Eixt!\n", __FUNCTION__);

}
/******************************************************************************
* CalibrateMDCurve()
*****************************************************************************/
void CalibrateMDCurve
(
    float smooth, float offset, unsigned short *MDCurve
)
{
    LOG1_AHDR("%s:Enter!\n", __FUNCTION__);

    int step = 16;
    float curve;
    float k = 0;

    for (int i = 0; i < 17; ++i)
    {
        curve = 1 + exp(-smooth * (k / 1023 - offset / 256));
        curve = 1024 / curve ;
        MDCurve[i] = round(curve) ;
        MDCurve[i] = MIN(MDCurve[i], 1023);
        k += step ;
    }

    LOG1_AHDR("%s:Eixt!\n", __FUNCTION__);
}
/******************************************************************************
 * GetCurrIOData()
 *****************************************************************************/
RESULT MergeGetCurrIOData
(
    AhdrHandle_t pAhdrCtx
)
{
    LOG1_AHDR("%s:Enter!\n", __FUNCTION__);
    RESULT result = AHDR_RET_SUCCESS;
    int OECurve[17];

    pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_mode = pAhdrCtx->CurrHandleData.MergeMode;
    pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_lm_dif_0p9 = 230;
    pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_ms_dif_0p8 = 205;
    pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_lm_dif_0p15 = (int)pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_offset;
    pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_ms_dif_0p15 = (int)pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_offset;


    CalibrateOECurve(pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_smooth,
                     pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_offset, pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_e_y) ;
    CalibrateMDCurve(pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_smooth,
                     pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_offset, pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_l1_y);
    CalibrateMDCurve(pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_smooth,
                     pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_offset, pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_l0_y);

    if(pAhdrCtx->SensorInfo.LongFrmMode == true) {
        for(int i = 0; i < 17; i++)
            pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_e_y[i] = 0;
    }

    //when gainX = 1, gainX_inv = 1/gainX -1
    pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_gain0 = (int)SHIFT6BIT(pAhdrCtx->CurrHandleData.CurrL2S_Ratio);
    if(pAhdrCtx->CurrHandleData.CurrL2S_Ratio == 1)
        pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_gain0_inv = (int)(SHIFT12BIT(1 / pAhdrCtx->CurrHandleData.CurrL2S_Ratio) - 1);
    else
        pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_gain0_inv = (int)SHIFT12BIT(1 / pAhdrCtx->CurrHandleData.CurrL2S_Ratio);

    pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_gain1 = (int)SHIFT6BIT(pAhdrCtx->CurrHandleData.CurrL2M_Ratio);
    if(pAhdrCtx->CurrHandleData.CurrL2M_Ratio == 1)
        pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_gain1_inv = (int)(SHIFT12BIT(1 / pAhdrCtx->CurrHandleData.CurrL2M_Ratio) - 1);
    else
        pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_gain1_inv = (int)SHIFT12BIT(1 / pAhdrCtx->CurrHandleData.CurrL2M_Ratio);

    pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_gain2 = (int)SHIFT6BIT(pAhdrCtx->CurrHandleData.CurrL2L_Ratio);

    LOGV_AHDR("%s:  Merge set IOdata to register:\n", __FUNCTION__);
    LOGV_AHDR("%s:  sw_hdrmge_mode:%d \n", __FUNCTION__, pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_mode);
    LOGV_AHDR("%s:  sw_hdrmge_ms_dif_0p8:%d \n", __FUNCTION__, pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_ms_dif_0p8);
    LOGV_AHDR("%s:  sw_hdrmge_lm_dif_0p9:%d \n", __FUNCTION__, pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_lm_dif_0p9);
    LOGV_AHDR("%s:  sw_hdrmge_ms_dif_0p15:%d \n", __FUNCTION__, pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_ms_dif_0p15);
    LOGV_AHDR("%s:  sw_hdrmge_gain1:%d \n", __FUNCTION__, pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_gain1);
    LOGV_AHDR("%s:  sw_hdrmge_gain1_inv:%d \n", __FUNCTION__, pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_gain1_inv);
    LOGV_AHDR("%s:  sw_hdrmge_gain2:%d \n", __FUNCTION__, pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_gain2);
    LOGV_AHDR("%s:  sw_hdrmge_gain0:%d \n", __FUNCTION__, pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_gain0);
    LOGV_AHDR("%s:  sw_hdrmge_gain0_inv:%d \n", __FUNCTION__, pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_gain0_inv);
    LOGV_AHDR("%s:  sw_hdrmge_lm_dif_0p15:%d \n", __FUNCTION__, pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_lm_dif_0p15);
    for(int i = 0; i < 17; i++)
        LOGV_AHDR("%s:  sw_hdrmge_e_y[%d]:%d \n", __FUNCTION__, i, pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_e_y[i]);
    for(int i = 0; i < 17; i++)
        LOGV_AHDR("%s:  sw_hdrmge_l0_y[%d]:%d \n", __FUNCTION__, i, pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_l0_y[i]);
    for(int i = 0; i < 17; i++)
        LOGV_AHDR("%s:  sw_hdrmge_l1_y[%d]:%d \n", __FUNCTION__, i, pAhdrCtx->AhdrProcRes.MgeProcRes.sw_hdrmge_l1_y[i]);

    LOG1_AHDR("%s:Eixt!\n", __FUNCTION__);
    return(result);
}

/******************************************************************************
 * MergeProcess()
 *****************************************************************************/
RESULT MergeProcessing
(
    AhdrHandle_t pAhdrCtx
)
{
    LOG1_AHDR("%s:Enter!\n", __FUNCTION__);
    RESULT result = AHDR_RET_SUCCESS;

    float OEDamp = pAhdrCtx->CurrHandleData.MergeOEDamp;
    float MDDampLM = pAhdrCtx->CurrHandleData.MergeMDDampLM;
    float MDDampMS = pAhdrCtx->CurrHandleData.MergeMDDampMS;

    bool ifHDRModeChange = pAhdrCtx->CurrHandleData.MergeMode == pAhdrCtx->AhdrPrevData.MergeMode ? false : true;

    //get finnal current data
    if (pAhdrCtx->hdrAttr.opMode == HDR_OpMode_Api_OFF && pAhdrCtx->frameCnt != 0 && !ifHDRModeChange)
    {
        pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_smooth = OEDamp * pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_smooth
                + (1 - OEDamp) * pAhdrCtx->AhdrPrevData.PrevMergeHandleData.OECurve_smooth;
        pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_offset = OEDamp * pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_offset
                + (1 - OEDamp) * pAhdrCtx->AhdrPrevData.PrevMergeHandleData.OECurve_offset;
        pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_smooth = MDDampLM * pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_smooth
                + (1 - MDDampLM) * pAhdrCtx->AhdrPrevData.PrevMergeHandleData.MDCurveLM_smooth;
        pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_offset = MDDampLM * pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_offset
                + (1 - MDDampLM) * pAhdrCtx->AhdrPrevData.PrevMergeHandleData.MDCurveLM_offset;
        pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_smooth = MDDampMS * pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_smooth
                + (1 - MDDampMS) * pAhdrCtx->AhdrPrevData.PrevMergeHandleData.MDCurveMS_smooth;
        pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_offset = MDDampMS * pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_offset
                + (1 - MDDampMS) * pAhdrCtx->AhdrPrevData.PrevMergeHandleData.MDCurveMS_offset;
    }

    //get current IO data
    result = MergeGetCurrIOData(pAhdrCtx);

    LOGD_AHDR("%s:  Current damp OECurve_smooth:%f OECurve_offset:%f \n", __FUNCTION__, pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_smooth,
              pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_offset);
    LOGD_AHDR("%s:  Current damp MDCurveLM_smooth:%f MDCurveLM_offset:%f \n", __FUNCTION__, pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_smooth,
              pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_offset);
    LOGD_AHDR("%s:  Current damp MDCurveMS_smooth:%f MDCurveMS_offset:%f\n", __FUNCTION__, pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_smooth
              , pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_offset);

    LOG1_AHDR("%s:Eixt!\n", __FUNCTION__);
    return(result);
}
