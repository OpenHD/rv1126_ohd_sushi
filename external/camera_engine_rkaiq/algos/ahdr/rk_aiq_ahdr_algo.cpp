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
#include "rk_aiq_types_ahdr_algo_int.h"
#include "rk_aiq_ahdr_algo_merge.h"
#include "rk_aiq_ahdr_algo_tmo.h"
#include "xcam_log.h"

/******************************************************************************
 * AhdrStart()
 *****************************************************************************/
RESULT AhdrStart
(
    AhdrHandle_t pAhdrCtx
) {

    LOG1_AHDR( "%s:enter!\n", __FUNCTION__);

    // initial checks
    if (pAhdrCtx == NULL) {
        return (AHDR_RET_WRONG_HANDLE);
    }

    if ((AHDR_STATE_RUNNING == pAhdrCtx->state)
            || (AHDR_STATE_LOCKED == pAhdrCtx->state)) {
        return (AHDR_RET_WRONG_STATE);
    }

    pAhdrCtx->state = AHDR_STATE_RUNNING;

    LOG1_AHDR( "%s:exit!\n", __FUNCTION__);
    return (AHDR_RET_SUCCESS);
}
/******************************************************************************
 * AhdrStop()
 *****************************************************************************/
RESULT AhdrStop
(
    AhdrHandle_t pAhdrCtx
) {

    LOG1_AHDR( "%s:enter!\n", __FUNCTION__);

    // initial checks
    if (pAhdrCtx == NULL) {
        return (AHDR_RET_WRONG_HANDLE);
    }

    // before stopping, unlock the AHDR if locked
    if (AHDR_STATE_LOCKED == pAhdrCtx->state) {
        return (AHDR_RET_WRONG_STATE);
    }

    pAhdrCtx->state = AHDR_STATE_STOPPED;

    LOG1_AHDR( "%s:exit!\n", __FUNCTION__);

    return (AHDR_RET_SUCCESS);
}
/******************************************************************************
 * GetCurrPara()
 *****************************************************************************/
float GetCurrPara
(
    float           inPara,
    float         inMatrixX[AHDR_MAX_IQ_DOTS],
    float         inMatrixY[AHDR_MAX_IQ_DOTS]
) {
    LOG1_AHDR( "%s:enter!\n", __FUNCTION__);
    float x1 = 0.0f;
    float x2 = 0.0f;
    float value1 = 0.0f;
    float value2 = 0.0f;
    float outPara = 0.0f;

    if(inPara < inMatrixX[0])
        outPara = inMatrixY[0];
    else if (inPara >= inMatrixX[AHDR_MAX_IQ_DOTS - 1])
        outPara = inMatrixY[AHDR_MAX_IQ_DOTS - 1];
    else
        for(int i = 0; i < AHDR_MAX_IQ_DOTS - 1; i++)
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

    return outPara;
    LOG1_AHDR( "%s:exit!\n", __FUNCTION__);
}
/******************************************************************************
 * AhdrApiOffConfig()
 *set default AhdrConfig data
 *****************************************************************************/
void AhdrApiOffConfig
(
    AhdrHandle_t           pAhdrCtx
) {
    LOG1_AHDR( "%s:enter!\n", __FUNCTION__);

    //merge
    pAhdrCtx->AhdrConfig.merge_para.MergeMode = 1;
    pAhdrCtx->AhdrConfig.merge_para.EnvLv[0] = 0;
    pAhdrCtx->AhdrConfig.merge_para.EnvLv[1] = 0.005 ;
    pAhdrCtx->AhdrConfig.merge_para.EnvLv[2] = 0.01 ;
    pAhdrCtx->AhdrConfig.merge_para.EnvLv[3] = 0.05 ;
    pAhdrCtx->AhdrConfig.merge_para.EnvLv[4] = 0.1 ;
    pAhdrCtx->AhdrConfig.merge_para.EnvLv[5] = 0.15 ;
    pAhdrCtx->AhdrConfig.merge_para.EnvLv[6] = 0.2;
    pAhdrCtx->AhdrConfig.merge_para.EnvLv[7] = 0.3 ;
    pAhdrCtx->AhdrConfig.merge_para.EnvLv[8] = 0.4 ;
    pAhdrCtx->AhdrConfig.merge_para.EnvLv[9] = 0.5 ;
    pAhdrCtx->AhdrConfig.merge_para.EnvLv[10] = 0.6 ;
    pAhdrCtx->AhdrConfig.merge_para.EnvLv[11] = 0.8 ;
    pAhdrCtx->AhdrConfig.merge_para.EnvLv[12] = 1.0 ;
    pAhdrCtx->AhdrConfig.merge_para.MoveCoef[0] = 0;
    pAhdrCtx->AhdrConfig.merge_para.MoveCoef[1] = 0.005 ;
    pAhdrCtx->AhdrConfig.merge_para.MoveCoef[2] = 0.01 ;
    pAhdrCtx->AhdrConfig.merge_para.MoveCoef[3] = 0.05 ;
    pAhdrCtx->AhdrConfig.merge_para.MoveCoef[4] = 0.1 ;
    pAhdrCtx->AhdrConfig.merge_para.MoveCoef[5] = 0.15 ;
    pAhdrCtx->AhdrConfig.merge_para.MoveCoef[6] = 0.2;
    pAhdrCtx->AhdrConfig.merge_para.MoveCoef[7] = 0.3 ;
    pAhdrCtx->AhdrConfig.merge_para.MoveCoef[8] = 0.4 ;
    pAhdrCtx->AhdrConfig.merge_para.MoveCoef[9] = 0.5 ;
    pAhdrCtx->AhdrConfig.merge_para.MoveCoef[10] = 0.6 ;
    pAhdrCtx->AhdrConfig.merge_para.MoveCoef[11] = 0.8 ;
    pAhdrCtx->AhdrConfig.merge_para.MoveCoef[12] = 1.0 ;
    for(int i = 0; i < 13; ++i)
    {
        pAhdrCtx->AhdrConfig.merge_para.OECurve_smooth[i] = 80 ;
        pAhdrCtx->AhdrConfig.merge_para.OECurve_offset[i] = 205 ;
        pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_smooth[i] = 80 ;
        pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_offset[i] = 38 ;
        pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_smooth[i] = 80 ;
        pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_offset[i] = 38 ;
    }
    pAhdrCtx->AhdrConfig.merge_para.OECurve_damp = 0.7;
    pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_damp = 0.7;
    pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_damp = 0.7;

    //TMO
    pAhdrCtx->AhdrConfig.tmo_para.damp = 0.7 ;
    pAhdrCtx->AhdrConfig.tmo_para.Luma.EnvLv[0] = 0;
    pAhdrCtx->AhdrConfig.tmo_para.Luma.EnvLv[1] = 0.005 ;
    pAhdrCtx->AhdrConfig.tmo_para.Luma.EnvLv[2] = 0.01 ;
    pAhdrCtx->AhdrConfig.tmo_para.Luma.EnvLv[3] = 0.05 ;
    pAhdrCtx->AhdrConfig.tmo_para.Luma.EnvLv[4] = 0.1 ;
    pAhdrCtx->AhdrConfig.tmo_para.Luma.EnvLv[5] = 0.15 ;
    pAhdrCtx->AhdrConfig.tmo_para.Luma.EnvLv[6] = 0.2;
    pAhdrCtx->AhdrConfig.tmo_para.Luma.EnvLv[7] = 0.3 ;
    pAhdrCtx->AhdrConfig.tmo_para.Luma.EnvLv[8] = 0.4 ;
    pAhdrCtx->AhdrConfig.tmo_para.Luma.EnvLv[9] = 0.5 ;
    pAhdrCtx->AhdrConfig.tmo_para.Luma.EnvLv[10] = 0.6 ;
    pAhdrCtx->AhdrConfig.tmo_para.Luma.EnvLv[11] = 0.8 ;
    pAhdrCtx->AhdrConfig.tmo_para.Luma.EnvLv[12] = 1.0 ;
    pAhdrCtx->AhdrConfig.tmo_para.Luma.Tolerance = 0 ;
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLightMode = 0;
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.OEPdf[0] = 0;
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.OEPdf[1] = 0.05;
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.OEPdf[2] = 0.1;
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.OEPdf[3] = 0.15;
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.OEPdf[4] = 0.2;
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.OEPdf[5] = 0.25;
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.OEPdf[6] = 0.3;
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.OEPdf[7] = 0.35;
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.OEPdf[8] = 0.4;
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.OEPdf[9] = 0.45;
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.OEPdf[10] = 0.5;
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.OEPdf[11] = 0.8;
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.OEPdf[12] = 1;
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.EnvLv[0] = 0;
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.EnvLv[1] = 0.005 ;
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.EnvLv[2] = 0.01 ;
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.EnvLv[3] = 0.05 ;
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.EnvLv[4] = 0.1 ;
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.EnvLv[5] = 0.15 ;
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.EnvLv[6] = 0.2;
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.EnvLv[7] = 0.3 ;
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.EnvLv[8] = 0.4 ;
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.EnvLv[9] = 0.5 ;
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.EnvLv[10] = 0.6 ;
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.EnvLv[11] = 0.8 ;
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.EnvLv[12] = 1.0 ;
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.Tolerance = 0;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLightMode = 0;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.Tolerance = 0;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.FocusLuma[0] = 1;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.FocusLuma[1] = 15;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.FocusLuma[2] = 20;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.FocusLuma[3] = 25;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.FocusLuma[4] = 30;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.FocusLuma[5] = 35;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.FocusLuma[6] = 40;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.FocusLuma[7] = 50;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.FocusLuma[8] = 60;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.FocusLuma[9] = 70;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.FocusLuma[10] = 80;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.FocusLuma[11] = 90;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.FocusLuma[12] = 100;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DarkPdf[0] = 0;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DarkPdf[1] = 0.005 ;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DarkPdf[2] = 0.01 ;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DarkPdf[3] = 0.05 ;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DarkPdf[4] = 0.1 ;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DarkPdf[5] = 0.15 ;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DarkPdf[6] = 0.2;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DarkPdf[7] = 0.3 ;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DarkPdf[8] = 0.4 ;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DarkPdf[9] = 0.5 ;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DarkPdf[10] = 0.6 ;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DarkPdf[11] = 0.8 ;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DarkPdf[12] = 1.0 ;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.ISO[0] = 50;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.ISO[1] = 100;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.ISO[2] = 200;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.ISO[3] = 400;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.ISO[4] = 800;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.ISO[5] = 1600;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.ISO[6] = 3200;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.ISO[7] = 6400;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.ISO[8] = 12800;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.ISO[9] = 25600;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.ISO[10] = 51200;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.ISO[11] = 102400;
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.ISO[12] = 204800;
    pAhdrCtx->AhdrConfig.tmo_para.global.mode = 0;
    pAhdrCtx->AhdrConfig.tmo_para.global.iir = 64;
    pAhdrCtx->AhdrConfig.tmo_para.global.Tolerance = 0;
    pAhdrCtx->AhdrConfig.tmo_para.global.DynamicRange[0] = 1;
    pAhdrCtx->AhdrConfig.tmo_para.global.DynamicRange[1] = 20;
    pAhdrCtx->AhdrConfig.tmo_para.global.DynamicRange[2] = 30;
    pAhdrCtx->AhdrConfig.tmo_para.global.DynamicRange[3] = 44;
    pAhdrCtx->AhdrConfig.tmo_para.global.DynamicRange[4] = 48;
    pAhdrCtx->AhdrConfig.tmo_para.global.DynamicRange[5] = 55;
    pAhdrCtx->AhdrConfig.tmo_para.global.DynamicRange[6] = 60;
    pAhdrCtx->AhdrConfig.tmo_para.global.DynamicRange[7] = 66;
    pAhdrCtx->AhdrConfig.tmo_para.global.DynamicRange[8] = 68;
    pAhdrCtx->AhdrConfig.tmo_para.global.DynamicRange[9] = 72;
    pAhdrCtx->AhdrConfig.tmo_para.global.DynamicRange[10] = 78;
    pAhdrCtx->AhdrConfig.tmo_para.global.DynamicRange[11] = 80;
    pAhdrCtx->AhdrConfig.tmo_para.global.DynamicRange[12] = 84;
    pAhdrCtx->AhdrConfig.tmo_para.global.EnvLv[0] = 0;
    pAhdrCtx->AhdrConfig.tmo_para.global.EnvLv[1] = 0.005 ;
    pAhdrCtx->AhdrConfig.tmo_para.global.EnvLv[2] = 0.01 ;
    pAhdrCtx->AhdrConfig.tmo_para.global.EnvLv[3] = 0.05 ;
    pAhdrCtx->AhdrConfig.tmo_para.global.EnvLv[4] = 0.1 ;
    pAhdrCtx->AhdrConfig.tmo_para.global.EnvLv[5] = 0.15 ;
    pAhdrCtx->AhdrConfig.tmo_para.global.EnvLv[6] = 0.2;
    pAhdrCtx->AhdrConfig.tmo_para.global.EnvLv[7] = 0.3 ;
    pAhdrCtx->AhdrConfig.tmo_para.global.EnvLv[8] = 0.4 ;
    pAhdrCtx->AhdrConfig.tmo_para.global.EnvLv[9] = 0.5 ;
    pAhdrCtx->AhdrConfig.tmo_para.global.EnvLv[10] = 0.6 ;
    pAhdrCtx->AhdrConfig.tmo_para.global.EnvLv[11] = 0.8 ;
    pAhdrCtx->AhdrConfig.tmo_para.global.EnvLv[12] = 1.0 ;
    pAhdrCtx->AhdrConfig.tmo_para.local.localtmoMode = 0;
    pAhdrCtx->AhdrConfig.tmo_para.local.Tolerance = 0;
    pAhdrCtx->AhdrConfig.tmo_para.local.DynamicRange[0] = 1;
    pAhdrCtx->AhdrConfig.tmo_para.local.DynamicRange[1] = 20;
    pAhdrCtx->AhdrConfig.tmo_para.local.DynamicRange[2] = 30;
    pAhdrCtx->AhdrConfig.tmo_para.local.DynamicRange[3] = 44;
    pAhdrCtx->AhdrConfig.tmo_para.local.DynamicRange[4] = 48;
    pAhdrCtx->AhdrConfig.tmo_para.local.DynamicRange[5] = 55;
    pAhdrCtx->AhdrConfig.tmo_para.local.DynamicRange[6] = 60;
    pAhdrCtx->AhdrConfig.tmo_para.local.DynamicRange[7] = 66;
    pAhdrCtx->AhdrConfig.tmo_para.local.DynamicRange[8] = 68;
    pAhdrCtx->AhdrConfig.tmo_para.local.DynamicRange[9] = 72;
    pAhdrCtx->AhdrConfig.tmo_para.local.DynamicRange[10] = 78;
    pAhdrCtx->AhdrConfig.tmo_para.local.DynamicRange[11] = 80;
    pAhdrCtx->AhdrConfig.tmo_para.local.DynamicRange[12] = 84;
    pAhdrCtx->AhdrConfig.tmo_para.local.EnvLv[0] = 0;
    pAhdrCtx->AhdrConfig.tmo_para.local.EnvLv[1] = 0.005 ;
    pAhdrCtx->AhdrConfig.tmo_para.local.EnvLv[2] = 0.01 ;
    pAhdrCtx->AhdrConfig.tmo_para.local.EnvLv[3] = 0.05 ;
    pAhdrCtx->AhdrConfig.tmo_para.local.EnvLv[4] = 0.1 ;
    pAhdrCtx->AhdrConfig.tmo_para.local.EnvLv[5] = 0.15 ;
    pAhdrCtx->AhdrConfig.tmo_para.local.EnvLv[6] = 0.2;
    pAhdrCtx->AhdrConfig.tmo_para.local.EnvLv[7] = 0.3 ;
    pAhdrCtx->AhdrConfig.tmo_para.local.EnvLv[8] = 0.4 ;
    pAhdrCtx->AhdrConfig.tmo_para.local.EnvLv[9] = 0.5 ;
    pAhdrCtx->AhdrConfig.tmo_para.local.EnvLv[10] = 0.6 ;
    pAhdrCtx->AhdrConfig.tmo_para.local.EnvLv[11] = 0.8 ;
    pAhdrCtx->AhdrConfig.tmo_para.local.EnvLv[12] = 1.0 ;
    for(int i = 0; i < 13; ++i)
    {
        pAhdrCtx->AhdrConfig.tmo_para.Luma.GlobeLuma[i] = 0.25 * GLOBELUMAMAX ;
        pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[i] = 0.5 * DETAILSHIGHLIGHTMAX;
        pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLight[i] = 1 * DETAILSLOWLIGHTMIN;
        pAhdrCtx->AhdrConfig.tmo_para.global.GlobalTmoStrength[i] = 0.5;
        pAhdrCtx->AhdrConfig.tmo_para.local.LocalTmoStrength[i] = 0.5 * TMOCONTRASTMAX;
    }

    LOGI_AHDR( "%s:exit!\n", __FUNCTION__);
}

/******************************************************************************
 * AhdrConfig()
 *set default AhdrConfig data
 *****************************************************************************/
void AhdrConfig
(
    AhdrHandle_t           pAhdrCtx
) {
    LOGI_AHDR( "%s:enter!\n", __FUNCTION__);

    // initial checks
    DCT_ASSERT(pAhdrCtx != NULL);

    if(pAhdrCtx->hdrAttr.opMode == HDR_OpMode_Api_OFF)
    {
        AhdrApiOffConfig(pAhdrCtx);
        LOGD_AHDR("%s: Ahdr Api is OFF!!!:\n", __FUNCTION__);
    }
    else
        LOGD_AHDR("%s: Ahdr Api is ON!!!:\n", __FUNCTION__);

    //config default AhdrPrevData data
    pAhdrCtx->AhdrPrevData.ro_hdrtmo_lgmean = 20000;
    pAhdrCtx->AhdrPrevData.PrevMergeHandleData.OECurve_smooth = 80;
    pAhdrCtx->AhdrPrevData.PrevMergeHandleData.OECurve_offset = 210;
    pAhdrCtx->AhdrPrevData.PrevMergeHandleData.MDCurveLM_smooth = 80;
    pAhdrCtx->AhdrPrevData.PrevMergeHandleData.MDCurveLM_offset = 38;
    pAhdrCtx->AhdrPrevData.PrevMergeHandleData.MDCurveMS_smooth = 80;
    pAhdrCtx->AhdrPrevData.PrevMergeHandleData.MDCurveMS_offset = 38;
    pAhdrCtx->AhdrPrevData.PrevTmoHandleData.GlobeLuma = 0.18;
    pAhdrCtx->AhdrPrevData.PrevTmoHandleData.GlobeMaxLuma = 0.3;
    pAhdrCtx->AhdrPrevData.PrevTmoHandleData.DetailsHighLight = 0.5;
    pAhdrCtx->AhdrPrevData.PrevTmoHandleData.DetailsLowLight = 1;
    pAhdrCtx->AhdrPrevData.PrevTmoHandleData.LocalTmoStrength = 0.3;
    pAhdrCtx->AhdrPrevData.PrevTmoHandleData.GlobalTmoStrength = 0.5;

    LOG1_AHDR( "%s:exit!\n", __FUNCTION__);
}
void AhdrGetStats
(
    AhdrHandle_t           pAhdrCtx,
    rkisp_ahdr_stats_t*         ROData
) {
    LOG1_AHDR( "%s:enter!\n", __FUNCTION__);

    pAhdrCtx->CurrStatsData.tmo_stats.ro_hdrtmo_lglow = ROData->tmo_stats.ro_hdrtmo_lglow;
    pAhdrCtx->CurrStatsData.tmo_stats.ro_hdrtmo_lgmin = ROData->tmo_stats.ro_hdrtmo_lgmin;
    pAhdrCtx->CurrStatsData.tmo_stats.ro_hdrtmo_lgmax = ROData->tmo_stats.ro_hdrtmo_lgmax;
    pAhdrCtx->CurrStatsData.tmo_stats.ro_hdrtmo_lghigh = ROData->tmo_stats.ro_hdrtmo_lghigh;
    pAhdrCtx->CurrStatsData.tmo_stats.ro_hdrtmo_lgmean = ROData->tmo_stats.ro_hdrtmo_lgmean;
    pAhdrCtx->CurrStatsData.tmo_stats.ro_hdrtmo_weightkey = ROData->tmo_stats.ro_hdrtmo_weightkey;
    pAhdrCtx->CurrStatsData.tmo_stats.ro_hdrtmo_lgrange0 = ROData->tmo_stats.ro_hdrtmo_lgrange0;
    pAhdrCtx->CurrStatsData.tmo_stats.ro_hdrtmo_lgrange1 = ROData->tmo_stats.ro_hdrtmo_lgrange1;
    pAhdrCtx->CurrStatsData.tmo_stats.ro_hdrtmo_lgavgmax = ROData->tmo_stats.ro_hdrtmo_lgavgmax;
    pAhdrCtx->CurrStatsData.tmo_stats.ro_hdrtmo_palpha = ROData->tmo_stats.ro_hdrtmo_palpha;
    pAhdrCtx->CurrStatsData.tmo_stats.ro_hdrtmo_linecnt = ROData->tmo_stats.ro_hdrtmo_linecnt;
    for(int i = 0; i < 32; i++)
        pAhdrCtx->CurrStatsData.tmo_stats.ro_array_min_max[i] = ROData->tmo_stats.ro_array_min_max[i];

    //get other stats from stats
    for(int i = 0; i < 225; i++)
    {
        pAhdrCtx->CurrStatsData.other_stats.short_luma[i] = ROData->other_stats.short_luma[i];
        pAhdrCtx->CurrStatsData.other_stats.long_luma[i] = ROData->other_stats.long_luma[i];
        pAhdrCtx->CurrStatsData.other_stats.tmo_luma[i] = ROData->other_stats.tmo_luma[i];
    }

    if(pAhdrCtx->FrameNumber == 3)
    {
        for(int i = 0; i < 25; i++)
            pAhdrCtx->CurrStatsData.other_stats.middle_luma[i] = ROData->other_stats.middle_luma[i];
    }

    LOGV_AHDR("%s:  Ahdr RO data from register:\n", __FUNCTION__);
    LOGV_AHDR("%s:  ro_hdrtmo_lglow:%d:\n", __FUNCTION__, pAhdrCtx->CurrStatsData.tmo_stats.ro_hdrtmo_lglow);
    LOGV_AHDR("%s:  ro_hdrtmo_lgmin:%d:\n", __FUNCTION__, pAhdrCtx->CurrStatsData.tmo_stats.ro_hdrtmo_lgmin);
    LOGV_AHDR("%s:  ro_hdrtmo_lgmax:%d:\n", __FUNCTION__, pAhdrCtx->CurrStatsData.tmo_stats.ro_hdrtmo_lgmax);
    LOGV_AHDR("%s:  ro_hdrtmo_lghigh:%d:\n", __FUNCTION__, pAhdrCtx->CurrStatsData.tmo_stats.ro_hdrtmo_lghigh);
    LOGV_AHDR("%s:  ro_hdrtmo_weightkey:%d:\n", __FUNCTION__, pAhdrCtx->CurrStatsData.tmo_stats.ro_hdrtmo_weightkey);
    LOGV_AHDR("%s:  ro_hdrtmo_lgmean:%d:\n", __FUNCTION__, pAhdrCtx->CurrStatsData.tmo_stats.ro_hdrtmo_lgmean);
    LOGV_AHDR("%s:  ro_hdrtmo_lgrange0:%d:\n", __FUNCTION__, pAhdrCtx->CurrStatsData.tmo_stats.ro_hdrtmo_lgrange0);
    LOGV_AHDR("%s:  ro_hdrtmo_lgrange1:%d:\n", __FUNCTION__, pAhdrCtx->CurrStatsData.tmo_stats.ro_hdrtmo_lgrange1);
    LOGV_AHDR("%s:  ro_hdrtmo_lgavgmax:%d:\n", __FUNCTION__, pAhdrCtx->CurrStatsData.tmo_stats.ro_hdrtmo_lgavgmax);
    LOGV_AHDR("%s:  ro_hdrtmo_palpha:%d:\n", __FUNCTION__, pAhdrCtx->CurrStatsData.tmo_stats.ro_hdrtmo_palpha);
    LOGV_AHDR("%s:  ro_hdrtmo_linecnt:%d:\n", __FUNCTION__, pAhdrCtx->CurrStatsData.tmo_stats.ro_hdrtmo_linecnt);
    for(int i = 0; i < 32; i++)
        LOGV_AHDR("%s:  ro_array_min_max[%d]:%d:\n", __FUNCTION__, i, pAhdrCtx->CurrStatsData.tmo_stats.ro_array_min_max[i]);

    LOG1_AHDR( "%s:exit!\n", __FUNCTION__);
}
void AhdrGetAeResult
(
    AhdrHandle_t           pAhdrCtx,
    AecPreResult_t  AecHdrPreResult
) {
    LOG1_AHDR( "%s:enter!\n", __FUNCTION__);

    //get Ae Pre Result
    pAhdrCtx->CurrAeResult.GlobalEnvLv = AecHdrPreResult.GlobalEnvLv[AecHdrPreResult.NormalIndex];
    pAhdrCtx->CurrAeResult.M2S_Ratio = AecHdrPreResult.M2S_ExpRatio;
    pAhdrCtx->CurrAeResult.M2S_Ratio = pAhdrCtx->CurrAeResult.M2S_Ratio < 1 ? 1 : pAhdrCtx->CurrAeResult.M2S_Ratio;
    pAhdrCtx->CurrAeResult.L2M_Ratio = AecHdrPreResult.L2M_ExpRatio;
    pAhdrCtx->CurrAeResult.L2M_Ratio = pAhdrCtx->CurrAeResult.L2M_Ratio < 1 ? 1 : pAhdrCtx->CurrAeResult.L2M_Ratio;
    pAhdrCtx->CurrAeResult.DynamicRange = AecHdrPreResult.DynamicRange;
    pAhdrCtx->CurrAeResult.OEPdf = AecHdrPreResult.OverExpROIPdf[1];
    pAhdrCtx->CurrAeResult.DarkPdf = AecHdrPreResult.LowLightROIPdf[1];
    for(int i = 0; i < 225; i++)
    {
        pAhdrCtx->CurrAeResult.BlockLumaS[i] = pAhdrCtx->CurrStatsData.other_stats.short_luma[i];
        pAhdrCtx->CurrAeResult.BlockLumaL[i] = pAhdrCtx->CurrStatsData.other_stats.long_luma[i];
    }
    if(pAhdrCtx->FrameNumber == 3)
        for(int i = 0; i < 25; i++)
            pAhdrCtx->CurrAeResult.BlockLumaM[i] = pAhdrCtx->CurrStatsData.other_stats.middle_luma[i];
    else
        for(int i = 0; i < 25; i++)
            pAhdrCtx->CurrAeResult.BlockLumaM[i] = 0;

    //transfer CurrAeResult data into AhdrHandle
    switch (pAhdrCtx->CurrHandleData.MergeMode)
    {
    case 0:
        pAhdrCtx->CurrHandleData.CurrLExpo = AecHdrPreResult.LinearExp.exp_real_params.analog_gain * AecHdrPreResult.LinearExp.exp_real_params.integration_time;
        pAhdrCtx->CurrHandleData.CurrL2S_Ratio = 1;
        pAhdrCtx->CurrHandleData.CurrL2M_Ratio = 1;
        pAhdrCtx->CurrHandleData.CurrL2L_Ratio = 1;
        pAhdrCtx->CurrAeResult.ISO = AecHdrPreResult.LinearExp.exp_real_params.analog_gain * 50.0;
        pAhdrCtx->CurrAeResult.GlobalEnvLv = AecHdrPreResult.GlobalEnvLv[0];
        pAhdrCtx->CurrAeResult.OEPdf = AecHdrPreResult.OverExpROIPdf[0];
        pAhdrCtx->CurrAeResult.DarkPdf = AecHdrPreResult.LowLightROIPdf[0];
        break;
    case 1:
        pAhdrCtx->CurrHandleData.CurrL2S_Ratio = pAhdrCtx->CurrAeResult.M2S_Ratio;
        pAhdrCtx->CurrHandleData.CurrL2M_Ratio = 1;
        pAhdrCtx->CurrHandleData.CurrL2L_Ratio = 1;
        pAhdrCtx->CurrHandleData.CurrLExpo = AecHdrPreResult.HdrExp[1].exp_real_params.analog_gain * AecHdrPreResult.HdrExp[1].exp_real_params.integration_time;
        pAhdrCtx->CurrAeResult.ISO = AecHdrPreResult.HdrExp[1].exp_real_params.analog_gain * 50.0;
        pAhdrCtx->CurrAeResult.GlobalEnvLv = AecHdrPreResult.GlobalEnvLv[1];
        pAhdrCtx->CurrAeResult.OEPdf = AecHdrPreResult.OverExpROIPdf[1];
        pAhdrCtx->CurrAeResult.DarkPdf = AecHdrPreResult.LowLightROIPdf[1];
        break;
    case 2:
        pAhdrCtx->CurrHandleData.CurrL2S_Ratio = pAhdrCtx->CurrAeResult.L2M_Ratio * pAhdrCtx->CurrAeResult.M2S_Ratio;
        pAhdrCtx->CurrHandleData.CurrL2M_Ratio = pAhdrCtx->CurrAeResult.L2M_Ratio;
        pAhdrCtx->CurrHandleData.CurrL2L_Ratio = 1;
        pAhdrCtx->CurrHandleData.CurrLExpo = AecHdrPreResult.HdrExp[2].exp_real_params.analog_gain * AecHdrPreResult.HdrExp[2].exp_real_params.integration_time;
        pAhdrCtx->CurrAeResult.ISO = AecHdrPreResult.HdrExp[2].exp_real_params.analog_gain * 50.0;
        pAhdrCtx->CurrAeResult.GlobalEnvLv = AecHdrPreResult.GlobalEnvLv[2];
        pAhdrCtx->CurrAeResult.OEPdf = AecHdrPreResult.OverExpROIPdf[1];
        pAhdrCtx->CurrAeResult.DarkPdf = AecHdrPreResult.LowLightROIPdf[1];
        break;
    default:
        LOGE_AHDR("%s:  Wrong frame number in HDR mode!!!\n", __FUNCTION__);
        break;
    }

    //Normalize the current envLv for AEC
    float maxEnvLuma = 65 / 10;
    float minEnvLuma = 0;
    pAhdrCtx->CurrHandleData.CurrEnvLv = (pAhdrCtx->CurrAeResult.GlobalEnvLv  - minEnvLuma) / (maxEnvLuma - minEnvLuma);
    pAhdrCtx->CurrHandleData.CurrEnvLv = LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrEnvLv, 1, 0);

    LOGD_AHDR("%s:  Current CurrL2S_Ratio:%f CurrL2M_Ratio:%f CurrL2L_Ratio:%f\n", __FUNCTION__, pAhdrCtx->CurrHandleData.CurrL2S_Ratio,
              pAhdrCtx->CurrHandleData.CurrL2M_Ratio, pAhdrCtx->CurrHandleData.CurrL2L_Ratio);

    LOG1_AHDR( "%s:exit!\n", __FUNCTION__);
}

void AhdrGetCurrMergeData
(
    AhdrHandle_t           pAhdrCtx
) {
    LOG1_AHDR( "%s:enter!\n", __FUNCTION__);

    //get Current merge OECurve
    pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_smooth = GetCurrPara(pAhdrCtx->CurrHandleData.CurrEnvLv,
            pAhdrCtx->AhdrConfig.merge_para.EnvLv,
            pAhdrCtx->AhdrConfig.merge_para.OECurve_smooth);
    pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_offset = GetCurrPara(pAhdrCtx->CurrHandleData.CurrEnvLv,
            pAhdrCtx->AhdrConfig.merge_para.EnvLv,
            pAhdrCtx->AhdrConfig.merge_para.OECurve_offset);

    //get Current merge MDCurve
    pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_smooth = GetCurrPara(pAhdrCtx->CurrHandleData.CurrMoveCoef,
            pAhdrCtx->AhdrConfig.merge_para.MoveCoef,
            pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_smooth);
    pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_offset = GetCurrPara(pAhdrCtx->CurrHandleData.CurrMoveCoef,
            pAhdrCtx->AhdrConfig.merge_para.MoveCoef,
            pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_offset);
    pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_smooth = GetCurrPara(pAhdrCtx->CurrHandleData.CurrMoveCoef,
            pAhdrCtx->AhdrConfig.merge_para.MoveCoef,
            pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_smooth);
    pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_offset = GetCurrPara(pAhdrCtx->CurrHandleData.CurrMoveCoef,
            pAhdrCtx->AhdrConfig.merge_para.MoveCoef,
            pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_offset);

    pAhdrCtx->CurrHandleData.MergeOEDamp = pAhdrCtx->AhdrConfig.merge_para.OECurve_damp;
    pAhdrCtx->CurrHandleData.MergeMDDampLM = pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_damp;
    pAhdrCtx->CurrHandleData.MergeMDDampMS = pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_damp;

    LOG1_AHDR( "%s:exit!\n", __FUNCTION__);
}

void AhdrGetCurrTmoData
(
    AhdrHandle_t           pAhdrCtx
) {
    LOG1_AHDR( "%s:enter!\n", __FUNCTION__);

    //get Current tmo GlobeLuma GlobeMaxLuma
    int GlobalLuma_mode = (int)pAhdrCtx->AhdrConfig.tmo_para.Luma.globalLumaMode;
    if(GlobalLuma_mode == 0)
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeLuma = GetCurrPara(pAhdrCtx->CurrHandleData.CurrEnvLv,
                pAhdrCtx->AhdrConfig.tmo_para.Luma.EnvLv,
                pAhdrCtx->AhdrConfig.tmo_para.Luma.GlobeLuma);
    else if(GlobalLuma_mode == 1)
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeLuma = GetCurrPara(pAhdrCtx->CurrHandleData.CurrISO,
                pAhdrCtx->AhdrConfig.tmo_para.Luma.ISO,
                pAhdrCtx->AhdrConfig.tmo_para.Luma.GlobeLuma);

    float GlobeLuma = pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeLuma;
    pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeMaxLuma = MAXLUMAK * GlobeLuma + MAXLUMAB;
    pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeMaxLuma = LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeMaxLuma, GLOBEMAXLUMAMAX, GLOBEMAXLUMAMIN);

    //get Current tmo strength
    int LocalMode = (int)pAhdrCtx->AhdrConfig.tmo_para.local.localtmoMode;
    if(LocalMode == 0)
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.LocalTmoStrength = GetCurrPara(pAhdrCtx->CurrHandleData.CurrDynamicRange,
                pAhdrCtx->AhdrConfig.tmo_para.local.DynamicRange,
                pAhdrCtx->AhdrConfig.tmo_para.local.LocalTmoStrength);
    else if(LocalMode == 1)
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.LocalTmoStrength = GetCurrPara(pAhdrCtx->CurrHandleData.CurrEnvLv,
                pAhdrCtx->AhdrConfig.tmo_para.local.EnvLv,
                pAhdrCtx->AhdrConfig.tmo_para.local.LocalTmoStrength);

    if(pAhdrCtx->AhdrConfig.tmo_para.global.isHdrGlobalTmo)
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.LocalTmoStrength = 0;

    //get Current tmo BabdPrior
    int GlobalMode = (int)pAhdrCtx->AhdrConfig.tmo_para.global.mode;
    if(GlobalMode == 0)
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobalTmoStrength = GetCurrPara(pAhdrCtx->CurrHandleData.CurrDynamicRange,
                pAhdrCtx->AhdrConfig.tmo_para.global.DynamicRange,
                pAhdrCtx->AhdrConfig.tmo_para.global.GlobalTmoStrength);
    else if(GlobalMode == 1)
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobalTmoStrength = GetCurrPara(pAhdrCtx->CurrHandleData.CurrEnvLv,
                pAhdrCtx->AhdrConfig.tmo_para.global.EnvLv,
                pAhdrCtx->AhdrConfig.tmo_para.global.GlobalTmoStrength);

    //get Current tmo DetailsHighLight
    int DetailsHighLight_mode = (int)pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLightMode;
    if(DetailsHighLight_mode == 0)
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsHighLight = GetCurrPara(pAhdrCtx->CurrHandleData.CurrOEPdf,
                pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.OEPdf,
                pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight);
    else if(DetailsHighLight_mode == 1)
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsHighLight = GetCurrPara(pAhdrCtx->CurrHandleData.CurrEnvLv,
                pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.EnvLv,
                pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight);

    //get Current tmo DetailsLowLight
    int DetailsLowLight_mode = (int)pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLightMode;
    if (DetailsLowLight_mode == 0)
    {
#if 0
        int focs = pAhdrCtx->CurrAfResult.CurrAfTargetPos;
        int focs_width = pAhdrCtx->CurrAfResult.CurrAfTargetWidth / (pAhdrCtx->width / 15);
        int focs_height = pAhdrCtx->CurrAfResult.CurrAfTargetHeight / (pAhdrCtx->height / 15);
        float focs_luma = 0;
        for(int i = 0; i < focs_height; i++)
            for(int j = 0; j < focs_width; j++)
                focs_luma += pAhdrCtx->CurrAeResult.BlockLumaL[focs + i + 15 * j];
        focs_luma = focs_luma / (focs_width * focs_height);
        pAhdrCtx->CurrHandleData.CurrTotalFocusLuma = focs_luma / 15;
        pAhdrCtx->CurrHandleData.CurrTotalFocusLuma = LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrTotalFocusLuma, FOCUSLUMAMAX, FOCUSLUMAMIN);
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsLowLight = GetCurrPara(pAhdrCtx->CurrHandleData.CurrTotalFocusLuma,
                pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.FocusLuma,
                pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLight);

#endif
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsLowLight = pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLight[0];

    }
    else if (DetailsLowLight_mode == 1)
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsLowLight = GetCurrPara(pAhdrCtx->CurrHandleData.CurrDarkPdf,
                pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DarkPdf,
                pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLight);
    else if (DetailsLowLight_mode == 2)
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsLowLight = GetCurrPara(pAhdrCtx->CurrHandleData.CurrISO,
                pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.ISO,
                pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLight);

    LOG1_AHDR( "%s:exit!\n", __FUNCTION__);
}

void AhdrGetSensorInfo
(
    AhdrHandle_t     pAhdrCtx,
    AecProcResult_t  AecHdrProcResult
) {
    LOG1_AHDR( "%s:enter!\n", __FUNCTION__);

    pAhdrCtx->SensorInfo.LongFrmMode = AecHdrProcResult.LongFrmMode && (pAhdrCtx->FrameNumber != 1);

    for(int i = 0; i < 3; i++)
    {
        pAhdrCtx->SensorInfo.HdrMinGain[i] = AecHdrProcResult.HdrMinGain[i];
        pAhdrCtx->SensorInfo.HdrMaxGain[i] = AecHdrProcResult.HdrMaxGain[i];
        pAhdrCtx->SensorInfo.HdrMinIntegrationTime[i] = AecHdrProcResult.HdrMinIntegrationTime[i];
        pAhdrCtx->SensorInfo.HdrMaxIntegrationTime[i] = AecHdrProcResult.HdrMaxIntegrationTime[i];
    }

    if(pAhdrCtx->FrameNumber == 1)
    {
        //pAhdrCtx->SensorInfo.MaxExpoL = pAhdrCtx->SensorInfo.HdrMaxGain[1] * pAhdrCtx->SensorInfo.HdrMaxIntegrationTime[1];
        //pAhdrCtx->SensorInfo.MinExpoL = pAhdrCtx->SensorInfo.HdrMinGain[1] * pAhdrCtx->SensorInfo.HdrMinIntegrationTime[1];
        //pAhdrCtx->SensorInfo.MaxExpoM = 0;
        //pAhdrCtx->SensorInfo.MinExpoM = 0;

        pAhdrCtx->CurrAeResult.LumaDeviationLinear = AecHdrProcResult.LumaDeviation;
        pAhdrCtx->CurrAeResult.LumaDeviationLinear = abs(pAhdrCtx->CurrAeResult.LumaDeviationLinear);
    }
    else if(pAhdrCtx->FrameNumber == 2)
    {
        pAhdrCtx->SensorInfo.MaxExpoL = pAhdrCtx->SensorInfo.HdrMaxGain[1] * pAhdrCtx->SensorInfo.HdrMaxIntegrationTime[1];
        pAhdrCtx->SensorInfo.MinExpoL = pAhdrCtx->SensorInfo.HdrMinGain[1] * pAhdrCtx->SensorInfo.HdrMinIntegrationTime[1];
        pAhdrCtx->SensorInfo.MaxExpoM = 0;
        pAhdrCtx->SensorInfo.MinExpoM = 0;

        pAhdrCtx->CurrAeResult.LumaDeviationL = AecHdrProcResult.HdrLumaDeviation[1];
        pAhdrCtx->CurrAeResult.LumaDeviationL = abs(pAhdrCtx->CurrAeResult.LumaDeviationL);
        pAhdrCtx->CurrAeResult.LumaDeviationS = AecHdrProcResult.HdrLumaDeviation[0];
        pAhdrCtx->CurrAeResult.LumaDeviationS = abs(pAhdrCtx->CurrAeResult.LumaDeviationS);
    }
    else if(pAhdrCtx->FrameNumber == 3)
    {
        pAhdrCtx->SensorInfo.MaxExpoL = pAhdrCtx->SensorInfo.HdrMaxGain[2] * pAhdrCtx->SensorInfo.HdrMaxIntegrationTime[2];
        pAhdrCtx->SensorInfo.MinExpoL = pAhdrCtx->SensorInfo.HdrMinGain[2] * pAhdrCtx->SensorInfo.HdrMinIntegrationTime[2];
        pAhdrCtx->SensorInfo.MaxExpoM = pAhdrCtx->SensorInfo.HdrMaxGain[1] * pAhdrCtx->SensorInfo.HdrMaxIntegrationTime[1];
        pAhdrCtx->SensorInfo.MinExpoM = pAhdrCtx->SensorInfo.HdrMinGain[1] * pAhdrCtx->SensorInfo.HdrMinIntegrationTime[1];

        pAhdrCtx->CurrAeResult.LumaDeviationL = AecHdrProcResult.HdrLumaDeviation[2];
        pAhdrCtx->CurrAeResult.LumaDeviationL = abs(pAhdrCtx->CurrAeResult.LumaDeviationL);
        pAhdrCtx->CurrAeResult.LumaDeviationM = AecHdrProcResult.HdrLumaDeviation[1];
        pAhdrCtx->CurrAeResult.LumaDeviationM = abs(pAhdrCtx->CurrAeResult.LumaDeviationM);
        pAhdrCtx->CurrAeResult.LumaDeviationS = AecHdrProcResult.HdrLumaDeviation[0];
        pAhdrCtx->CurrAeResult.LumaDeviationS = abs(pAhdrCtx->CurrAeResult.LumaDeviationS);
    }

    pAhdrCtx->SensorInfo.MaxExpoS = pAhdrCtx->SensorInfo.HdrMaxGain[0] * pAhdrCtx->SensorInfo.HdrMaxIntegrationTime[0];
    pAhdrCtx->SensorInfo.MinExpoS = pAhdrCtx->SensorInfo.HdrMinGain[0] * pAhdrCtx->SensorInfo.HdrMinIntegrationTime[0];

    LOG1_AHDR( "%s:exit!\n", __FUNCTION__);
}

/******************************************************************************
 * AhdrApiSetLevel()
 *
 *****************************************************************************/
void AhdrApiSetLevel
(
    AhdrHandle_t     pAhdrCtx
)
{
    LOG1_AHDR("%s:enter!\n", __FUNCTION__);

    //tmo data
    pAhdrCtx->hdrAttr.stSetLevel.level = LIMIT_VALUE(pAhdrCtx->hdrAttr.stSetLevel.level, FASTMODELEVELMAX, FASTMODELEVELMIN);

    float level = ((float)(pAhdrCtx->hdrAttr.stSetLevel.level)) / FASTMODELEVELMAX;
    float level_default = 0.5;
    float level_diff = level - level_default;

    pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeLuma *= 1 + level_diff;
    float GlobeLuma = pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeLuma;
    pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeMaxLuma = MAXLUMAK * GlobeLuma + MAXLUMAB;
    pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeMaxLuma =
        LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeMaxLuma, GLOBEMAXLUMAMAX, GLOBEMAXLUMAMIN);

    pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsHighLight *= 1 + level_diff;
    pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsHighLight =
        LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsHighLight, DETAILSHIGHLIGHTMAX, DETAILSHIGHLIGHTMIN);

    pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsLowLight *= 1 + level_diff;
    pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsLowLight =
        LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsLowLight, DETAILSLOWLIGHTMAX, DETAILSLOWLIGHTMIN);

    pAhdrCtx->AhdrConfig.tmo_para.bTmoEn = true;
    pAhdrCtx->AhdrProcRes.isLinearTmo = pAhdrCtx->FrameNumber == 1 ;

    /*
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.TmoContrast *= 1 + level_diff;
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.TmoContrast =
            LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrTmoHandleData.LocalTmoStrength, TMOCONTRASTMAX, TMOCONTRASTMIN);
    */
    //paras after updating
    LOGD_AHDR("%s:  AHDR_OpMode_Fast set level:%d\n", __FUNCTION__, pAhdrCtx->hdrAttr.stSetLevel.level);
    LOGD_AHDR("%s:  After fast mode GlobeLuma:%f GlobeMaxLuma:%f DetailsHighLight:%f DetailsLowLight:%f LocalTmoStrength:%f \n", __FUNCTION__,
              pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeLuma, pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeMaxLuma,
              pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsHighLight, pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsLowLight,
              pAhdrCtx->CurrHandleData.CurrTmoHandleData.LocalTmoStrength);


    LOG1_AHDR( "%s:exit!\n", __FUNCTION__);
}

/******************************************************************************
 * AhdrApiOffProcess()
 *
 *****************************************************************************/
void AhdrApiOffProcess
(
    AhdrHandle_t     pAhdrCtx,
    AecPreResult_t  AecHdrPreResult,
    af_preprocess_result_t AfPreResult
)
{
    LOG1_AHDR("%s:enter!\n", __FUNCTION__);

    //get current merge data
    AhdrGetCurrMergeData(pAhdrCtx);

    //get cuurrent tmo data
    AhdrGetCurrTmoData(pAhdrCtx);

    //get Current tmo TmoDamp
    pAhdrCtx->CurrHandleData.TmoDamp = pAhdrCtx->AhdrConfig.tmo_para.damp;

    LOG1_AHDR( "%s:exit!\n", __FUNCTION__);
}

/******************************************************************************
 * AhdrTranferData2Api()
 *
 *****************************************************************************/
void AhdrTranferData2Api
(
    AhdrHandle_t     pAhdrCtx
)
{
    LOG1_AHDR("%s:enter!\n", __FUNCTION__);

    //transfer control data to api
    pAhdrCtx->hdrAttr.CtlInfo.SceneMode = pAhdrCtx->sence_mode;
    pAhdrCtx->hdrAttr.CtlInfo.GlobalLumaMode = pAhdrCtx->AhdrConfig.tmo_para.Luma.globalLumaMode;
    pAhdrCtx->hdrAttr.CtlInfo.DetailsHighLightMode = pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLightMode;
    pAhdrCtx->hdrAttr.CtlInfo.DetailsLowLightMode = pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLightMode;
    pAhdrCtx->hdrAttr.CtlInfo.GlobalTmoMode = pAhdrCtx->AhdrConfig.tmo_para.global.mode;
    pAhdrCtx->hdrAttr.CtlInfo.LocalTMOMode = pAhdrCtx->AhdrConfig.tmo_para.local.localtmoMode;
    pAhdrCtx->hdrAttr.CtlInfo.Envlv = pAhdrCtx->CurrHandleData.CurrEnvLv;
    pAhdrCtx->hdrAttr.CtlInfo.MoveCoef = pAhdrCtx->CurrHandleData.CurrMoveCoef;
    pAhdrCtx->hdrAttr.CtlInfo.ISO = pAhdrCtx->CurrHandleData.CurrISO;
    pAhdrCtx->hdrAttr.CtlInfo.OEPdf = pAhdrCtx->CurrHandleData.CurrOEPdf;
    pAhdrCtx->hdrAttr.CtlInfo.DarkPdf = pAhdrCtx->CurrHandleData.CurrDarkPdf;
    pAhdrCtx->hdrAttr.CtlInfo.FocusLuma = pAhdrCtx->CurrHandleData.CurrTotalFocusLuma;
    pAhdrCtx->hdrAttr.CtlInfo.DynamicRange = pAhdrCtx->CurrHandleData.CurrDynamicRange;

    //transfer register data to api
    pAhdrCtx->hdrAttr.RegInfo.OECurve_smooth = pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_smooth;
    pAhdrCtx->hdrAttr.RegInfo.OECurve_offset = pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_offset;
    pAhdrCtx->hdrAttr.RegInfo.MDCurveLM_smooth = pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_smooth;
    pAhdrCtx->hdrAttr.RegInfo.MDCurveLM_offset = pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_offset;
    pAhdrCtx->hdrAttr.RegInfo.MDCurveMS_smooth = pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_smooth;
    pAhdrCtx->hdrAttr.RegInfo.MDCurveMS_offset = pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_offset;

    pAhdrCtx->hdrAttr.RegInfo.GlobalLuma = pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeLuma / GLOBELUMAMAX;
    pAhdrCtx->hdrAttr.RegInfo.DetailsLowlight = pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsLowLight / DETAILSLOWLIGHTMIN;
    pAhdrCtx->hdrAttr.RegInfo.DetailsHighlight = pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsHighLight / DETAILSHIGHLIGHTMAX;
    pAhdrCtx->hdrAttr.RegInfo.LocalTmoStrength = pAhdrCtx->CurrHandleData.CurrTmoHandleData.LocalTmoStrength / TMOCONTRASTMAX;
    pAhdrCtx->hdrAttr.RegInfo.GlobaltmoStrength = pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobalTmoStrength;


    LOG1_AHDR( "%s:exit!\n", __FUNCTION__);
}

/******************************************************************************
 * AhdrApiAutoUpdate()
 *
 *****************************************************************************/
void AhdrApiAutoUpdate
(
    AhdrHandle_t     pAhdrCtx
)
{
    LOG1_AHDR("%s:enter!\n", __FUNCTION__);

    if (pAhdrCtx->hdrAttr.stAuto.bUpdateMge == true)
    {
        //get oe cruve
        pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_smooth =
            LIMIT_PARA(pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stOECurve.stCoef, pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stOECurve.stSmthMax,
                       pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stOECurve.stSmthMin, pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stOECurve.stCoefMax,
                       pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stOECurve.stCoefMin);
        pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_offset =
            LIMIT_PARA(pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stOECurve.stCoef, pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stOECurve.stOfstMax,
                       pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stOECurve.stOfstMin, pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stOECurve.stCoefMax,
                       pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stOECurve.stCoefMin);

        //get md cruve ms
        pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_smooth =
            LIMIT_PARA(pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stMDCurveMS.stCoef, pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stMDCurveMS.stSmthMax,
                       pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stMDCurveMS.stSmthMin, pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stMDCurveMS.stCoefMax,
                       pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stMDCurveMS.stCoefMin);
        pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_offset =
            LIMIT_PARA(pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stMDCurveMS.stCoef, pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stMDCurveMS.stOfstMax,
                       pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stMDCurveMS.stOfstMin, pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stMDCurveMS.stCoefMax,
                       pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stMDCurveMS.stCoefMin);

        //get md cruve lm
        pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_smooth =
            LIMIT_PARA(pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stMDCurveLM.stCoef, pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stMDCurveLM.stSmthMax,
                       pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stMDCurveLM.stSmthMin, pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stMDCurveLM.stCoefMax,
                       pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stMDCurveLM.stCoefMin);
        pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_offset =
            LIMIT_PARA(pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stMDCurveLM.stCoef, pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stMDCurveLM.stOfstMax,
                       pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stMDCurveLM.stOfstMin, pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stMDCurveLM.stCoefMax,
                       pAhdrCtx->hdrAttr.stAuto.stMgeAuto.stMDCurveLM.stCoefMin);

    }
    else
        AhdrGetCurrMergeData(pAhdrCtx);

    //update tmo data in Auto mode
    if (pAhdrCtx->hdrAttr.stAuto.bUpdateTmo == true)
    {
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsLowLight =
            LIMIT_PARA(pAhdrCtx->hdrAttr.stAuto.stTmoAuto.stDtlsLL.stCoef, pAhdrCtx->hdrAttr.stAuto.stTmoAuto.stDtlsLL.stMax, pAhdrCtx->hdrAttr.stAuto.stTmoAuto.stDtlsLL.stMin,
                       pAhdrCtx->hdrAttr.stAuto.stTmoAuto.stDtlsLL.stCoefMax, pAhdrCtx->hdrAttr.stAuto.stTmoAuto.stDtlsLL.stMin);
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsLowLight = LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsLowLight
                , IQDETAILSLOWLIGHTMAX, IQDETAILSLOWLIGHTMIN);

        pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsHighLight =
            LIMIT_PARA(pAhdrCtx->hdrAttr.stAuto.stTmoAuto.stDtlsHL.stCoef, pAhdrCtx->hdrAttr.stAuto.stTmoAuto.stDtlsHL.stMax, pAhdrCtx->hdrAttr.stAuto.stTmoAuto.stDtlsHL.stMin,
                       pAhdrCtx->hdrAttr.stAuto.stTmoAuto.stDtlsHL.stCoefMax, pAhdrCtx->hdrAttr.stAuto.stTmoAuto.stDtlsHL.stMin);
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsHighLight = LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsHighLight
                , DETAILSHIGHLIGHTMAX, DETAILSHIGHLIGHTMIN);

        pAhdrCtx->CurrHandleData.CurrTmoHandleData.LocalTmoStrength =
            LIMIT_PARA(pAhdrCtx->hdrAttr.stAuto.stTmoAuto.stLocalTMO.stCoef, pAhdrCtx->hdrAttr.stAuto.stTmoAuto.stLocalTMO.stMax, pAhdrCtx->hdrAttr.stAuto.stTmoAuto.stLocalTMO.stMin,
                       pAhdrCtx->hdrAttr.stAuto.stTmoAuto.stLocalTMO.stCoefMax, pAhdrCtx->hdrAttr.stAuto.stTmoAuto.stLocalTMO.stMin);
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.LocalTmoStrength = LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrTmoHandleData.LocalTmoStrength
                , TMOCONTRASTMAX, TMOCONTRASTMIN);

        pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeLuma =
            LIMIT_PARA(pAhdrCtx->hdrAttr.stAuto.stTmoAuto.stGlobeLuma.stCoef, pAhdrCtx->hdrAttr.stAuto.stTmoAuto.stGlobeLuma.stMax, pAhdrCtx->hdrAttr.stAuto.stTmoAuto.stGlobeLuma.stMin,
                       pAhdrCtx->hdrAttr.stAuto.stTmoAuto.stGlobeLuma.stCoefMax, pAhdrCtx->hdrAttr.stAuto.stTmoAuto.stGlobeLuma.stMin);
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeLuma = LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeLuma
                , GLOBELUMAMAX, GLOBELUMAMIN);

        float GlobeLuma = pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeLuma;
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeMaxLuma = MAXLUMAK * GlobeLuma + MAXLUMAB;
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeMaxLuma = LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeMaxLuma,
                GLOBEMAXLUMAMAX, GLOBEMAXLUMAMIN);

        if(pAhdrCtx->hdrAttr.stAuto.stTmoAuto.stGlobalTMO.en)
        {
            float strength = LIMIT_PARA(pAhdrCtx->hdrAttr.stAuto.stTmoAuto.stGlobalTMO.stCoef, pAhdrCtx->hdrAttr.stAuto.stTmoAuto.stGlobalTMO.stMax, pAhdrCtx->hdrAttr.stAuto.stTmoAuto.stGlobalTMO.stMin,
                                        pAhdrCtx->hdrAttr.stAuto.stTmoAuto.stGlobalTMO.stCoefMax, pAhdrCtx->hdrAttr.stAuto.stTmoAuto.stGlobalTMO.stMin);
            pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobalTmoStrength = strength;
        }
        else
            pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobalTmoStrength = 0.5;

    }
    else
        AhdrGetCurrTmoData(pAhdrCtx);

    //paras after updating
    LOGD_AHDR("%s:	Current MDCurveMS_smooth:%f MDCurveMS_offset:%f MDCurveLM_smooth:%f MDCurveLM_offset:%f OECurve_smooth:%f OECurve_offset:%f\n", __FUNCTION__,
              pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_smooth, pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_offset,
              pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_smooth, pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_offset,
              pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_smooth, pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_offset);
    LOGD_AHDR("%s:	Current GlobeLuma:%f GlobeMaxLuma:%f DetailsHighLight:%f DetailsLowLight:%f GlobalTmoStrength:%f LocalTmoStrength:%f\n", __FUNCTION__,
              pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeLuma, pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeMaxLuma,
              pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsHighLight, pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsLowLight,
              pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobalTmoStrength, pAhdrCtx->CurrHandleData.CurrTmoHandleData.LocalTmoStrength);

    LOG1_AHDR( "%s:exit!\n", __FUNCTION__);
}

/******************************************************************************
 * AhdrApiManualUpdate()
 *
 *****************************************************************************/
void AhdrApiManualUpdate
(
    AhdrHandle_t     pAhdrCtx
)
{
    LOG1_AHDR("%s:enter!\n", __FUNCTION__);

    //update merge data in manual mode
    if (pAhdrCtx->hdrAttr.stManual.bUpdateMge == true)
    {
        pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_smooth = pAhdrCtx->hdrAttr.stManual.stMgeManual.MDCurveMS_smooth * MDCURVESMOOTHMAX;
        pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_smooth = LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_smooth,
                MDCURVESMOOTHMAX, MDCURVESMOOTHMIN);
        pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_offset = pAhdrCtx->hdrAttr.stManual.stMgeManual.MDCurveMS_offset * MDCURVEOFFSETMAX;
        pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_offset = LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_offset,
                MDCURVEOFFSETMAX, MDCURVEOFFSETMIN);

        pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_smooth = pAhdrCtx->hdrAttr.stManual.stMgeManual.MDCurveLM_smooth * MDCURVESMOOTHMAX;
        pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_smooth = LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_smooth,
                MDCURVESMOOTHMAX, MDCURVESMOOTHMIN);
        pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_offset = pAhdrCtx->hdrAttr.stManual.stMgeManual.MDCurveLM_offset * MDCURVEOFFSETMAX;
        pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_offset = LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_offset,
                MDCURVEOFFSETMAX, MDCURVEOFFSETMIN);

        pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_smooth = pAhdrCtx->hdrAttr.stManual.stMgeManual.OECurve_smooth * OECURVESMOOTHMAX;
        pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_smooth = LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_smooth,
                OECURVESMOOTHMAX, OECURVESMOOTHMIN);
        pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_offset = pAhdrCtx->hdrAttr.stManual.stMgeManual.OECurve_offset;
        pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_offset = LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_offset,
                OECURVEOFFSETMAX, OECURVEOFFSETMIN);


        pAhdrCtx->CurrHandleData.MergeOEDamp = pAhdrCtx->hdrAttr.stManual.stMgeManual.dampOE;
        pAhdrCtx->CurrHandleData.MergeMDDampLM = pAhdrCtx->hdrAttr.stManual.stMgeManual.dampMDLM;
        pAhdrCtx->CurrHandleData.MergeMDDampMS = pAhdrCtx->hdrAttr.stManual.stMgeManual.dampMDMS;

    }
    else
        AhdrGetCurrMergeData(pAhdrCtx);

    //update tmo data in manual mode
    if (pAhdrCtx->hdrAttr.stManual.bUpdateTmo == true)
    {
        pAhdrCtx->AhdrConfig.tmo_para.bTmoEn = pAhdrCtx->hdrAttr.stManual.stTmoManual.Enable;
        pAhdrCtx->AhdrConfig.tmo_para.isLinearTmo = pAhdrCtx->AhdrProcRes.bTmoEn && pAhdrCtx->FrameNumber == 1;
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsLowLight = pAhdrCtx->hdrAttr.stManual.stTmoManual.stDtlsLL * DETAILSLOWLIGHTMIN ;
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsLowLight = LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsLowLight
                , DETAILSLOWLIGHTMAX, DETAILSLOWLIGHTMIN);

        pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsHighLight = pAhdrCtx->hdrAttr.stManual.stTmoManual.stDtlsHL * DETAILSHIGHLIGHTMAX ;
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsHighLight = LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsHighLight
                , DETAILSHIGHLIGHTMAX, DETAILSHIGHLIGHTMIN);

        pAhdrCtx->CurrHandleData.CurrTmoHandleData.LocalTmoStrength = pAhdrCtx->hdrAttr.stManual.stTmoManual.stLocalTMOStrength * TMOCONTRASTMAX;
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.LocalTmoStrength = LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrTmoHandleData.LocalTmoStrength
                , TMOCONTRASTMAX, TMOCONTRASTMIN);

        pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeLuma = pAhdrCtx->hdrAttr.stManual.stTmoManual.stGlobeLuma * GLOBELUMAMAX;
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeLuma = LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeLuma
                , GLOBELUMAMAX, GLOBELUMAMIN);

        float GlobeLuma = pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeLuma;
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeMaxLuma = MAXLUMAK * GlobeLuma + MAXLUMAB;
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeMaxLuma = LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeMaxLuma,
                GLOBEMAXLUMAMAX, GLOBEMAXLUMAMIN);

        pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobalTmoStrength = LIMIT_VALUE(pAhdrCtx->hdrAttr.stManual.stTmoManual.stGlobalTMOStrength,
                1, 0);

        pAhdrCtx->CurrHandleData.TmoDamp = pAhdrCtx->hdrAttr.stManual.stTmoManual.damp;



    }
    else
        AhdrGetCurrTmoData(pAhdrCtx);

    //paras after updating
    LOGD_AHDR("%s:	Current MDCurveMS_smooth:%f MDCurveMS_offset:%f MDCurveLM_smooth:%f MDCurveLM_offset:%f OECurve_smooth:%f OECurve_offset:%f\n", __FUNCTION__,
              pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_smooth, pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_offset,
              pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_smooth, pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_offset,
              pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_smooth, pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_offset);
    LOGD_AHDR("%s:  Current GlobeLuma:%f GlobeMaxLuma:%f DetailsHighLight:%f DetailsLowLight:%f GlobalTmoStrength:%f LocalTmoStrength:%f\n", __FUNCTION__,
              pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeLuma, pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeMaxLuma,
              pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsHighLight, pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsLowLight,
              pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobalTmoStrength, pAhdrCtx->CurrHandleData.CurrTmoHandleData.LocalTmoStrength);

    LOG1_AHDR( "%s:exit!\n", __FUNCTION__);
}

/******************************************************************************
 * AhdrUpdateConfig()
 *transfer html parameter into handle
 ***************************************************************************/
void AhdrUpdateConfig
(
    AhdrHandle_t           pAhdrCtx,
    CalibDb_Ahdr_Para_t*         pCalibDb,
    int mode
) {
    LOG1_AHDR( "%s:enter!\n", __FUNCTION__);

    // initial checks
    DCT_ASSERT(pAhdrCtx != NULL);
    DCT_ASSERT(pCalibDb != NULL);

    //merge
    pAhdrCtx->AhdrConfig.merge_para.OECurve_damp = LIMIT_VALUE(pCalibDb->merge.oeCurve_damp, DAMPMAX, DAMPMIN);
    pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_damp = LIMIT_VALUE(pCalibDb->merge.mdCurveLm_damp, DAMPMAX, DAMPMIN);
    pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_damp = LIMIT_VALUE(pCalibDb->merge.mdCurveMs_damp, DAMPMAX, DAMPMIN);
    for (int i = 0; i < 13; i++ )
    {
        pAhdrCtx->AhdrConfig.merge_para.EnvLv[i] = LIMIT_VALUE(pCalibDb->merge.envLevel[i], ENVLVMAX, ENVLVMIN);
        pAhdrCtx->AhdrConfig.merge_para.MoveCoef[i] = LIMIT_VALUE(pCalibDb->merge.moveCoef[i], MOVECOEFMAX, MOVECOEFMIN);
        pAhdrCtx->AhdrConfig.merge_para.OECurve_smooth[i] = LIMIT_VALUE(pCalibDb->merge.oeCurve_smooth[i], IQPARAMAX, IQPARAMIN);
        pAhdrCtx->AhdrConfig.merge_para.OECurve_offset[i] = LIMIT_VALUE(pCalibDb->merge.oeCurve_offset[i], OECURVEOFFSETMAX, OECURVEOFFSETMIN);
        pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_smooth[i] = LIMIT_VALUE(pCalibDb->merge.mdCurveLm_smooth[i], IQPARAMAX, IQPARAMIN);
        pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_offset[i] = LIMIT_VALUE(pCalibDb->merge.mdCurveLm_offset[i], IQPARAMAX, IQPARAMIN);
        pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_smooth[i] = LIMIT_VALUE(pCalibDb->merge.mdCurveMs_smooth[i], IQPARAMAX, IQPARAMIN);
        pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_offset[i] = LIMIT_VALUE(pCalibDb->merge.mdCurveMs_offset[i], IQPARAMAX, IQPARAMIN);
    }

    //TMO
    pAhdrCtx->AhdrConfig.tmo_para.Luma.globalLumaMode = LIMIT_VALUE(pCalibDb->tmo.luma[mode].GlobalLumaMode, GLOBALLUMAMODEMAX, GLOBALLUMAMODEMIN);
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLightMode = LIMIT_VALUE(pCalibDb->tmo.LowLight[mode].DetailsLowLightMode, DETAILSLOWLIGHTMODEMAX, DETAILSLOWLIGHTMODEMIN);
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLightMode = LIMIT_VALUE(pCalibDb->tmo.HighLight[mode].DetailsHighLightMode, DETAILSHIGHLIGHTMODEMAX, DETAILSHIGHLIGHTMODEMIN);
    pAhdrCtx->AhdrConfig.tmo_para.local.localtmoMode = LIMIT_VALUE(pCalibDb->tmo.LocalTMO[mode].LocalTMOMode, TMOCONTRASTMODEMAX, TMOCONTRASTMODEMIN);
    pAhdrCtx->AhdrConfig.tmo_para.damp = LIMIT_VALUE(pCalibDb->tmo.damp, DAMPMAX, DAMPMIN);
    pAhdrCtx->AhdrConfig.tmo_para.Luma.Tolerance = LIMIT_VALUE(pCalibDb->tmo.luma[mode].Tolerance, TOLERANCEMAX, TOLERANCEMIN);
    pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.Tolerance = LIMIT_VALUE(pCalibDb->tmo.HighLight[mode].Tolerance, TOLERANCEMAX, TOLERANCEMIN);
    pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.Tolerance = LIMIT_VALUE(pCalibDb->tmo.LowLight[mode].Tolerance, TOLERANCEMAX, TOLERANCEMIN);
    pAhdrCtx->AhdrConfig.tmo_para.local.Tolerance = LIMIT_VALUE(pCalibDb->tmo.LocalTMO[mode].Tolerance, TOLERANCEMAX, TOLERANCEMIN);
    for(int i = 0; i < 13; i++)
    {
        pAhdrCtx->AhdrConfig.tmo_para.Luma.EnvLv[i] = LIMIT_VALUE(pCalibDb->tmo.luma[mode].envLevel[i], ENVLVMAX, ENVLVMIN);
        pAhdrCtx->AhdrConfig.tmo_para.Luma.ISO[i] = LIMIT_VALUE(pCalibDb->tmo.luma[mode].ISO[i], ISOMAX, ISOMIN);
        pAhdrCtx->AhdrConfig.tmo_para.Luma.GlobeLuma[i] = LIMIT_VALUE(pCalibDb->tmo.luma[mode].globalLuma[i], IQPARAMAX, IQPARAMIN) ;

        pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.OEPdf[i] = LIMIT_VALUE(pCalibDb->tmo.HighLight[mode].OEPdf[i], OEPDFMAX, OEPDFMIN) ;
        pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.EnvLv[i] = LIMIT_VALUE(pCalibDb->tmo.HighLight[mode].EnvLv[i], ENVLVMAX, ENVLVMIN);
        pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[i] = LIMIT_VALUE(pCalibDb->tmo.HighLight[mode].detailsHighLight[i], IQPARAMAX, IQPARAMIN) ;

        pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.FocusLuma[i] = LIMIT_VALUE(pCalibDb->tmo.LowLight[mode].FocusLuma[i], FOCUSLUMAMAX, FOCUSLUMAMIN) ;
        pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DarkPdf[i] = LIMIT_VALUE(pCalibDb->tmo.LowLight[mode].DarkPdf[i], DARKPDFMAX, DARKPDFMIN) ;
        pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.ISO[i] = LIMIT_VALUE(pCalibDb->tmo.LowLight[mode].ISO[i], ISOMAX, ISOMIN) ;
        pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLight[i] = LIMIT_VALUE(pCalibDb->tmo.LowLight[mode].detailsLowLight[i], IQDETAILSLOWLIGHTMAX, IQDETAILSLOWLIGHTMIN) ;

        pAhdrCtx->AhdrConfig.tmo_para.local.DynamicRange[i] = LIMIT_VALUE(pCalibDb->tmo.LocalTMO[mode].DynamicRange[i], DYNAMICRANGEMAX, DYNAMICRANGEMIN) ;
        pAhdrCtx->AhdrConfig.tmo_para.local.EnvLv[i] = LIMIT_VALUE(pCalibDb->tmo.LocalTMO[mode].EnvLv[i], ENVLVMAX, ENVLVMIN) ;
        pAhdrCtx->AhdrConfig.tmo_para.local.LocalTmoStrength[i] = LIMIT_VALUE(pCalibDb->tmo.LocalTMO[mode].Strength[i], IQPARAMAX, IQPARAMIN) ;
    }

    //Global Tmo
    pAhdrCtx->AhdrConfig.tmo_para.global.isHdrGlobalTmo =
        pCalibDb->tmo.GlobaTMO[mode].en == 0 ? false : true;
    pAhdrCtx->AhdrConfig.tmo_para.global.mode = LIMIT_VALUE(pCalibDb->tmo.GlobaTMO[mode].mode, TMOCONTRASTMODEMAX, TMOCONTRASTMODEMIN);
    pAhdrCtx->AhdrConfig.tmo_para.global.Tolerance = LIMIT_VALUE(pCalibDb->tmo.GlobaTMO[mode].Tolerance, TOLERANCEMAX, TOLERANCEMIN);
    for(int i = 0; i < 13; i++)
    {
        pAhdrCtx->AhdrConfig.tmo_para.global.DynamicRange[i] = LIMIT_VALUE(pCalibDb->tmo.GlobaTMO[mode].DynamicRange[i], DYNAMICRANGEMAX, DYNAMICRANGEMIN) ;
        pAhdrCtx->AhdrConfig.tmo_para.global.EnvLv[i] = LIMIT_VALUE(pCalibDb->tmo.GlobaTMO[mode].EnvLv[i], ENVLVMAX, ENVLVMIN) ;
        pAhdrCtx->AhdrConfig.tmo_para.global.GlobalTmoStrength[i] = LIMIT_VALUE(pCalibDb->tmo.GlobaTMO[mode].Strength[i], IQPARAMAX, IQPARAMIN) ;
    }
    pAhdrCtx->AhdrConfig.tmo_para.global.iir = LIMIT_VALUE(pCalibDb->tmo.GlobaTMO[mode].iir, IIRMAX, IIRMIN);

    //tmo En
    if(pAhdrCtx->FrameNumber == 2 || pAhdrCtx->FrameNumber == 3) {
        pAhdrCtx->AhdrConfig.tmo_para.bTmoEn = true;
        pAhdrCtx->AhdrConfig.tmo_para.isLinearTmo = false;
    }
    else if(pAhdrCtx->FrameNumber == 1)
    {
        pAhdrCtx->AhdrConfig.tmo_para.bTmoEn = pCalibDb->tmo.en[mode].en == 0 ? false : true;
        pAhdrCtx->AhdrConfig.tmo_para.isLinearTmo = pAhdrCtx->AhdrConfig.tmo_para.bTmoEn;
    }

    LOG1_AHDR("%s:  Ahdr comfig data from xml:\n", __FUNCTION__);
    LOG1_AHDR("%s:  Merge MergeMode:%d\n", __FUNCTION__, pAhdrCtx->AhdrConfig.merge_para.MergeMode);
    LOG1_AHDR("%s:  Merge EnvLv[0~5]:%f %f %f %f %f %f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.merge_para.EnvLv[0], pAhdrCtx->AhdrConfig.merge_para.EnvLv[1], pAhdrCtx->AhdrConfig.merge_para.EnvLv[2],
              pAhdrCtx->AhdrConfig.merge_para.EnvLv[3], pAhdrCtx->AhdrConfig.merge_para.EnvLv[4], pAhdrCtx->AhdrConfig.merge_para.EnvLv[5]);
    LOG1_AHDR("%s:  Merge EnvLv[6~12]:%f %f %f %f %f %f %f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.merge_para.EnvLv[6], pAhdrCtx->AhdrConfig.merge_para.EnvLv[7], pAhdrCtx->AhdrConfig.merge_para.EnvLv[8],
              pAhdrCtx->AhdrConfig.merge_para.EnvLv[9], pAhdrCtx->AhdrConfig.merge_para.EnvLv[10], pAhdrCtx->AhdrConfig.merge_para.EnvLv[11], pAhdrCtx->AhdrConfig.merge_para.EnvLv[12]);
    LOG1_AHDR("%s:  Merge OECurve_smooth[0~5]:%f %f %f %f %f %f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.merge_para.OECurve_smooth[0], pAhdrCtx->AhdrConfig.merge_para.OECurve_smooth[1], pAhdrCtx->AhdrConfig.merge_para.OECurve_smooth[2],
              pAhdrCtx->AhdrConfig.merge_para.OECurve_smooth[3], pAhdrCtx->AhdrConfig.merge_para.OECurve_smooth[4], pAhdrCtx->AhdrConfig.merge_para.OECurve_smooth[5]);
    LOG1_AHDR("%s:  Merge OECurve_smooth[6~12]:%f %f %f %f %f %f %f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.merge_para.OECurve_smooth[6], pAhdrCtx->AhdrConfig.merge_para.OECurve_smooth[7], pAhdrCtx->AhdrConfig.merge_para.OECurve_smooth[8],
              pAhdrCtx->AhdrConfig.merge_para.OECurve_smooth[9], pAhdrCtx->AhdrConfig.merge_para.OECurve_smooth[10], pAhdrCtx->AhdrConfig.merge_para.OECurve_smooth[11], pAhdrCtx->AhdrConfig.merge_para.OECurve_smooth[12]);
    LOG1_AHDR("%s:  Merge OECurve_offset[0~5]:%f %f %f %f %f %f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.merge_para.OECurve_offset[0], pAhdrCtx->AhdrConfig.merge_para.OECurve_offset[1], pAhdrCtx->AhdrConfig.merge_para.OECurve_offset[2],
              pAhdrCtx->AhdrConfig.merge_para.OECurve_offset[3], pAhdrCtx->AhdrConfig.merge_para.OECurve_offset[4], pAhdrCtx->AhdrConfig.merge_para.OECurve_offset[5]);
    LOG1_AHDR("%s:  Merge OECurve_offset[6~12]:%f %f %f %f %f %f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.merge_para.OECurve_offset[6], pAhdrCtx->AhdrConfig.merge_para.OECurve_offset[7], pAhdrCtx->AhdrConfig.merge_para.OECurve_offset[8],
              pAhdrCtx->AhdrConfig.merge_para.OECurve_offset[9], pAhdrCtx->AhdrConfig.merge_para.OECurve_offset[10], pAhdrCtx->AhdrConfig.merge_para.OECurve_offset[11], pAhdrCtx->AhdrConfig.merge_para.OECurve_offset[12]);
    LOG1_AHDR("%s:  Merge MoveCoef[0~5]:%f %f %f %f %f %f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.merge_para.MoveCoef[0], pAhdrCtx->AhdrConfig.merge_para.MoveCoef[1], pAhdrCtx->AhdrConfig.merge_para.MoveCoef[2],
              pAhdrCtx->AhdrConfig.merge_para.MoveCoef[3], pAhdrCtx->AhdrConfig.merge_para.MoveCoef[4], pAhdrCtx->AhdrConfig.merge_para.MoveCoef[5]);
    LOG1_AHDR("%s:  Merge MoveCoef[6~12]:%f %f %f %f %f %f %f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.merge_para.MoveCoef[6], pAhdrCtx->AhdrConfig.merge_para.MoveCoef[7], pAhdrCtx->AhdrConfig.merge_para.MoveCoef[8],
              pAhdrCtx->AhdrConfig.merge_para.MoveCoef[9], pAhdrCtx->AhdrConfig.merge_para.MoveCoef[10], pAhdrCtx->AhdrConfig.merge_para.MoveCoef[11], pAhdrCtx->AhdrConfig.merge_para.MoveCoef[12]);
    LOG1_AHDR("%s:  Merge MDCurveLM_smooth[0~5]:%f %f %f %f %f %f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_smooth[0], pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_smooth[1], pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_smooth[2],
              pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_smooth[3], pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_smooth[4], pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_smooth[5]);
    LOG1_AHDR("%s:  Merge MDCurveLM_smooth[6~12]:%f %f %f %f %f %f %f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_smooth[6], pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_smooth[7], pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_smooth[8],
              pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_smooth[9], pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_smooth[10], pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_smooth[11], pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_smooth[12]);
    LOG1_AHDR("%s:  Merge MDCurveLM_offset[0~5]:%f %f %f %f %f %f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_offset[0], pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_offset[1], pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_offset[2],
              pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_offset[3], pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_offset[4], pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_offset[5]);
    LOG1_AHDR("%s:  Merge MDCurveLM_offset[6~12]:%f %f %f %f %f %f %f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_offset[6], pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_offset[7], pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_offset[8],
              pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_offset[9], pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_offset[10], pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_offset[11], pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_offset[12]);
    LOG1_AHDR("%s:  Merge MDCurveMS_smooth[0~5]:%f %f %f %f %f %f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_smooth[0], pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_smooth[1], pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_smooth[2],
              pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_smooth[3], pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_smooth[4], pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_smooth[5]);
    LOG1_AHDR("%s:  Merge MDCurveMS_smooth[6~12]:%f %f %f %f %f %f %f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_smooth[6], pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_smooth[7], pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_smooth[8],
              pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_smooth[9], pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_smooth[10], pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_smooth[11], pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_smooth[12]);
    LOG1_AHDR("%s:  Merge MDCurveMS_offset[0~5]:%f %f %f %f %f %f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_offset[0], pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_offset[1], pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_offset[2],
              pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_offset[3], pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_offset[4], pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_offset[5]);
    LOG1_AHDR("%s:  Merge MDCurveMS_offset[6~12]:%f %f %f %f %f %f %f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_offset[6], pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_offset[7], pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_offset[8],
              pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_offset[9], pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_offset[10], pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_offset[11], pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_offset[12]);
    LOG1_AHDR("%s:  Merge OECurve_damp:%f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.merge_para.OECurve_damp);
    LOG1_AHDR("%s:  Merge MDCurveLM_damp:%f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_damp);
    LOG1_AHDR("%s:  Merge MDCurveMS_damp:%f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_damp);

    LOGD_AHDR("%s:  Tmo Em:%d linear Tmo en:%d\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.bTmoEn, pAhdrCtx->AhdrConfig.tmo_para.isLinearTmo);
    LOG1_AHDR("%s:  Tmo GlobalLuma EnvLv:%f %f %f %f %f %f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.Luma.EnvLv[0], pAhdrCtx->AhdrConfig.tmo_para.Luma.EnvLv[1], pAhdrCtx->AhdrConfig.tmo_para.Luma.EnvLv[2]
              , pAhdrCtx->AhdrConfig.tmo_para.Luma.EnvLv[3], pAhdrCtx->AhdrConfig.tmo_para.Luma.EnvLv[4], pAhdrCtx->AhdrConfig.tmo_para.Luma.EnvLv[5]);
    LOG1_AHDR("%s:  Tmo GlobalLuma EnvLv[6~12]:%f %f %f %f %f %f %f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.Luma.EnvLv[6], pAhdrCtx->AhdrConfig.tmo_para.Luma.EnvLv[7], pAhdrCtx->AhdrConfig.tmo_para.Luma.EnvLv[8]
              , pAhdrCtx->AhdrConfig.tmo_para.Luma.EnvLv[9], pAhdrCtx->AhdrConfig.tmo_para.Luma.EnvLv[10], pAhdrCtx->AhdrConfig.tmo_para.Luma.EnvLv[11], pAhdrCtx->AhdrConfig.tmo_para.Luma.EnvLv[12]);
    LOG1_AHDR("%s:  Tmo GlobalLuma ISO[0~5]:%f %f %f %f %f %f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.Luma.ISO[0], pAhdrCtx->AhdrConfig.tmo_para.Luma.ISO[1], pAhdrCtx->AhdrConfig.tmo_para.Luma.ISO[2]
              , pAhdrCtx->AhdrConfig.tmo_para.Luma.ISO[3], pAhdrCtx->AhdrConfig.tmo_para.Luma.ISO[4], pAhdrCtx->AhdrConfig.tmo_para.Luma.ISO[5]);
    LOG1_AHDR("%s:  Tmo GlobalLuma ISO[6~12]:%f %f %f %f %f %f %f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.Luma.ISO[6], pAhdrCtx->AhdrConfig.tmo_para.Luma.ISO[7], pAhdrCtx->AhdrConfig.tmo_para.Luma.ISO[8]
              , pAhdrCtx->AhdrConfig.tmo_para.Luma.ISO[9], pAhdrCtx->AhdrConfig.tmo_para.Luma.ISO[10], pAhdrCtx->AhdrConfig.tmo_para.Luma.ISO[11], pAhdrCtx->AhdrConfig.tmo_para.Luma.ISO[12]);
    LOG1_AHDR("%s:  Tmo EnvLvTolerance:%f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.Luma.Tolerance);
    LOG1_AHDR("%s:  Tmo GlobeLuma[0~5]:%f %f %f %f %f %f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.Luma.GlobeLuma[0], pAhdrCtx->AhdrConfig.tmo_para.Luma.GlobeLuma[1], pAhdrCtx->AhdrConfig.tmo_para.Luma.GlobeLuma[2]
              , pAhdrCtx->AhdrConfig.tmo_para.Luma.GlobeLuma[3], pAhdrCtx->AhdrConfig.tmo_para.Luma.GlobeLuma[4], pAhdrCtx->AhdrConfig.tmo_para.Luma.GlobeLuma[5]);
    LOG1_AHDR("%s:  Tmo GlobeLuma[6~12]:%f %f %f %f %f %f %f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.Luma.GlobeLuma[6], pAhdrCtx->AhdrConfig.tmo_para.Luma.GlobeLuma[7], pAhdrCtx->AhdrConfig.tmo_para.Luma.GlobeLuma[8]
              , pAhdrCtx->AhdrConfig.tmo_para.Luma.GlobeLuma[9], pAhdrCtx->AhdrConfig.tmo_para.Luma.GlobeLuma[10], pAhdrCtx->AhdrConfig.tmo_para.Luma.GlobeLuma[11], pAhdrCtx->AhdrConfig.tmo_para.Luma.GlobeLuma[12]);
    LOG1_AHDR("%s:  Tmo DetailsHighLightMode:%f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLightMode);
    LOG1_AHDR("%s:  Tmo OEPdf[0~5]:%f %f %f %f %f %f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.OEPdf[0], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.OEPdf[1], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.OEPdf[2]
              , pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.OEPdf[3], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.OEPdf[4], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.OEPdf[5]);
    LOG1_AHDR("%s:  Tmo OEPdf[6~12]:%f %f %f %f %f %f %f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.OEPdf[6], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.OEPdf[7], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.OEPdf[8]
              , pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.OEPdf[9], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.OEPdf[10], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.OEPdf[11], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.OEPdf[12]);
    LOG1_AHDR("%s:  Tmo DetailsHighLight EnvLv[0~5]:%f %f %f %f %f %f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.EnvLv[0], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.EnvLv[1], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.EnvLv[2]
              , pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.EnvLv[3], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.EnvLv[4], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.EnvLv[5]);
    LOG1_AHDR("%s:  Tmo DetailsHighLight EnvLv[6~12]:%f %f %f %f %f %f %f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.EnvLv[6], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.EnvLv[7], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.EnvLv[8]
              , pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.EnvLv[9], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.EnvLv[10], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.EnvLv[11], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.EnvLv[11]);
    LOG1_AHDR("%s:  Tmo OETolerance:%f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.Tolerance);
    LOG1_AHDR("%s:  Tmo DetailsHighLight[0~5]:%f %f %f %f %f %f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[0], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[1], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[2]
              , pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[3], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[4], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[5]);
    LOG1_AHDR("%s:  Tmo DetailsHighLight[6~12]:%f %f %f %f %f %f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[6], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[7], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[8]
              , pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[9], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[10], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[11], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[11]);
    LOG1_AHDR("%s:  Tmo DetailsLowLightMode:%f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLightMode);
    LOG1_AHDR("%s:  Tmo FocusLuma:%f %f %f %f %f %f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.FocusLuma[0], pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.FocusLuma[1], pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.FocusLuma[2]
              , pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.FocusLuma[3], pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.FocusLuma[4], pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.FocusLuma[5]);
    LOG1_AHDR("%s:  Tmo DarkPdf:%f %f %f %f %f %f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DarkPdf[0], pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DarkPdf[1], pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DarkPdf[2]
              , pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DarkPdf[3], pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DarkPdf[4], pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DarkPdf[5]);
    LOG1_AHDR("%s:  Tmo ISO:%f %f %f %f %f %f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.ISO[0], pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.ISO[1], pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.ISO[2]
              , pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.ISO[3], pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.ISO[4], pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.ISO[5]);
    LOG1_AHDR("%s:  Tmo DTPdfTolerance:%f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.Tolerance);
    LOG1_AHDR("%s:  Tmo DetailsLowLight:%f %f %f %f %f %f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLight[0], pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLight[1], pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLight[2]
              , pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLight[3], pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLight[4], pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLight[5]);
    LOG1_AHDR("%s:  Tmo LocalTMOMode:%f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.local.localtmoMode);
    LOG1_AHDR("%s:  Tmo LocalTMO DynamicRange:%f %f %f %f %f %f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.local.DynamicRange[0], pAhdrCtx->AhdrConfig.tmo_para.local.DynamicRange[1], pAhdrCtx->AhdrConfig.tmo_para.local.DynamicRange[2]
              , pAhdrCtx->AhdrConfig.tmo_para.local.DynamicRange[3], pAhdrCtx->AhdrConfig.tmo_para.local.DynamicRange[4], pAhdrCtx->AhdrConfig.tmo_para.local.DynamicRange[5]);
    LOG1_AHDR("%s:  Tmo LocalTMO EnvLv:%f %f %f %f %f %f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.local.EnvLv[0], pAhdrCtx->AhdrConfig.tmo_para.local.EnvLv[1], pAhdrCtx->AhdrConfig.tmo_para.local.EnvLv[2]
              , pAhdrCtx->AhdrConfig.tmo_para.local.EnvLv[3], pAhdrCtx->AhdrConfig.tmo_para.local.EnvLv[4], pAhdrCtx->AhdrConfig.tmo_para.local.EnvLv[5]);
    LOG1_AHDR("%s:  Tmo LocalTMO Tolerance:%f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.local.Tolerance);
    LOG1_AHDR("%s:  Tmo LocalTMO:%f %f %f %f %f %f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.local.LocalTmoStrength[0], pAhdrCtx->AhdrConfig.tmo_para.local.LocalTmoStrength[1], pAhdrCtx->AhdrConfig.tmo_para.local.LocalTmoStrength[2]
              , pAhdrCtx->AhdrConfig.tmo_para.local.LocalTmoStrength[3], pAhdrCtx->AhdrConfig.tmo_para.local.LocalTmoStrength[4], pAhdrCtx->AhdrConfig.tmo_para.local.LocalTmoStrength[5]);
    LOG1_AHDR("%s:  Tmo GlobalTMO en%d\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.global.isHdrGlobalTmo);
    LOG1_AHDR("%s:  Tmo GlobalTMO mode:%f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.global.mode);
    LOG1_AHDR("%s:  Tmo GlobalTMO DynamicRange:%f %f %f %f %f %f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.global.DynamicRange[0], pAhdrCtx->AhdrConfig.tmo_para.global.DynamicRange[1], pAhdrCtx->AhdrConfig.tmo_para.global.DynamicRange[2]
              , pAhdrCtx->AhdrConfig.tmo_para.global.DynamicRange[3], pAhdrCtx->AhdrConfig.tmo_para.global.DynamicRange[4], pAhdrCtx->AhdrConfig.tmo_para.global.DynamicRange[5]);
    LOG1_AHDR("%s:  Tmo GlobalTMO EnvLv:%f %f %f %f %f %f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.global.EnvLv[0], pAhdrCtx->AhdrConfig.tmo_para.global.EnvLv[1], pAhdrCtx->AhdrConfig.tmo_para.global.EnvLv[2]
              , pAhdrCtx->AhdrConfig.tmo_para.global.EnvLv[3], pAhdrCtx->AhdrConfig.tmo_para.global.EnvLv[4], pAhdrCtx->AhdrConfig.tmo_para.global.EnvLv[5]);
    LOG1_AHDR("%s:  Tmo GlobalTMO Tolerance:%f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.global.Tolerance);
    LOG1_AHDR("%s:  Tmo GlobalTMO Strength:%f %f %f %f %f %f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.global.GlobalTmoStrength[0], pAhdrCtx->AhdrConfig.tmo_para.global.GlobalTmoStrength[1], pAhdrCtx->AhdrConfig.tmo_para.global.GlobalTmoStrength[2]
              , pAhdrCtx->AhdrConfig.tmo_para.global.GlobalTmoStrength[3], pAhdrCtx->AhdrConfig.tmo_para.global.GlobalTmoStrength[4], pAhdrCtx->AhdrConfig.tmo_para.global.GlobalTmoStrength[5]);
    LOG1_AHDR("%s:  Tmo Damp:%f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.damp);

    //turn the IQ paras into algo paras
    for(int i = 0; i < 13; i++)
    {
        pAhdrCtx->AhdrConfig.merge_para.OECurve_smooth[i] = pAhdrCtx->AhdrConfig.merge_para.OECurve_smooth[i] * OECURVESMOOTHMAX;
        pAhdrCtx->AhdrConfig.merge_para.OECurve_smooth[i] = LIMIT_VALUE(pAhdrCtx->AhdrConfig.merge_para.OECurve_smooth[i], OECURVESMOOTHMAX, OECURVESMOOTHMIN) ;

        pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_smooth[i] = pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_smooth[i] * MDCURVESMOOTHMAX;
        pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_smooth[i] = LIMIT_VALUE(pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_smooth[i], MDCURVESMOOTHMAX, MDCURVESMOOTHMIN) ;
        pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_offset[i] = pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_offset[i] * MDCURVEOFFSETMAX;
        pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_offset[i] = LIMIT_VALUE(pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_offset[i], MDCURVEOFFSETMAX, MDCURVEOFFSETMIN) ;

        pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_smooth[i] = pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_smooth[i] * MDCURVESMOOTHMAX;
        pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_smooth[i] = LIMIT_VALUE(pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_smooth[i], MDCURVESMOOTHMAX, MDCURVESMOOTHMIN) ;
        pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_offset[i] = pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_offset[i] * MDCURVEOFFSETMAX;
        pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_offset[i] = LIMIT_VALUE(pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_offset[i], MDCURVEOFFSETMAX, MDCURVEOFFSETMIN) ;

        pAhdrCtx->AhdrConfig.tmo_para.Luma.GlobeLuma[i] = pAhdrCtx->AhdrConfig.tmo_para.Luma.GlobeLuma[i] * GLOBELUMAMAX;
        pAhdrCtx->AhdrConfig.tmo_para.Luma.GlobeLuma[i] = LIMIT_VALUE(pAhdrCtx->AhdrConfig.tmo_para.Luma.GlobeLuma[i], GLOBELUMAMAX, GLOBELUMAMIN) ;

        pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[i] = pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[i] * DETAILSHIGHLIGHTMAX;
        pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[i] = LIMIT_VALUE(pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[i], DETAILSHIGHLIGHTMAX, DETAILSHIGHLIGHTMIN) ;

        pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLight[i] = pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLight[i] * DETAILSLOWLIGHTMIN;
        pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLight[i] = LIMIT_VALUE(pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLight[i], DETAILSLOWLIGHTMAX, DETAILSLOWLIGHTMIN) ;

        pAhdrCtx->AhdrConfig.tmo_para.local.LocalTmoStrength[i] = pAhdrCtx->AhdrConfig.tmo_para.local.LocalTmoStrength[i] * TMOCONTRASTMAX;
        pAhdrCtx->AhdrConfig.tmo_para.local.LocalTmoStrength[i] = LIMIT_VALUE(pAhdrCtx->AhdrConfig.tmo_para.local.LocalTmoStrength[i], TMOCONTRASTMAX, TMOCONTRASTMIN) ;

    }


    LOG1_AHDR("%s:  Merge algo OECurve_smooth:%f %f %f %f %f %f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.merge_para.OECurve_smooth[0], pAhdrCtx->AhdrConfig.merge_para.OECurve_smooth[1], pAhdrCtx->AhdrConfig.merge_para.OECurve_smooth[2],
              pAhdrCtx->AhdrConfig.merge_para.OECurve_smooth[3], pAhdrCtx->AhdrConfig.merge_para.OECurve_smooth[4], pAhdrCtx->AhdrConfig.merge_para.OECurve_smooth[5]);
    LOG1_AHDR("%s:  Merge algo MDCurveLM_smooth:%f %f %f %f %f %f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_smooth[0], pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_smooth[1], pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_smooth[2],
              pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_smooth[3], pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_smooth[4], pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_smooth[5]);
    LOG1_AHDR("%s:  Merge algo MDCurveLM_offset:%f %f %f %f %f %f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_offset[0], pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_offset[1], pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_offset[2],
              pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_offset[3], pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_offset[4], pAhdrCtx->AhdrConfig.merge_para.MDCurveLM_offset[5]);
    LOG1_AHDR("%s:  Merge algo MDCurveMS_smooth:%f %f %f %f %f %f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_smooth[0], pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_smooth[1], pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_smooth[2],
              pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_smooth[3], pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_smooth[4], pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_smooth[5]);
    LOG1_AHDR("%s:  Merge algo MDCurveMS_offset:%f %f %f %f %f %f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_offset[0], pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_offset[1], pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_offset[2],
              pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_offset[3], pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_offset[4], pAhdrCtx->AhdrConfig.merge_para.MDCurveMS_offset[5]);
    LOG1_AHDR("%s:  Tmo algo GlobeLuma:%f %f %f %f %f %f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.Luma.GlobeLuma[0], pAhdrCtx->AhdrConfig.tmo_para.Luma.GlobeLuma[1], pAhdrCtx->AhdrConfig.tmo_para.Luma.GlobeLuma[2]
              , pAhdrCtx->AhdrConfig.tmo_para.Luma.GlobeLuma[3], pAhdrCtx->AhdrConfig.tmo_para.Luma.GlobeLuma[4], pAhdrCtx->AhdrConfig.tmo_para.Luma.GlobeLuma[5]);
    LOG1_AHDR("%s:  Tmo algo DetailsHighLight[0~5]:%f %f %f %f %f %f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[0], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[1], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[2]
              , pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[3], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[4], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[5]);
    LOG1_AHDR("%s:  Tmo algo DetailsHighLight[6~12]:%f %f %f %f %f %f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[6], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[7], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[8]
              , pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[9], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[10], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[11], pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLight[11]);
    LOG1_AHDR("%s:  Tmo algo DetailsLowLight:%f %f %f %f %f %f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLight[0], pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLight[1], pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLight[2]
              , pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLight[3], pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLight[4], pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLight[5]);
    LOG1_AHDR("%s:  Tmo algo TmoContrast:%f %f %f %f %f %f:\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.local.LocalTmoStrength[0], pAhdrCtx->AhdrConfig.tmo_para.local.LocalTmoStrength[1], pAhdrCtx->AhdrConfig.tmo_para.local.LocalTmoStrength[2]
              , pAhdrCtx->AhdrConfig.tmo_para.local.LocalTmoStrength[3], pAhdrCtx->AhdrConfig.tmo_para.local.LocalTmoStrength[4], pAhdrCtx->AhdrConfig.tmo_para.local.LocalTmoStrength[5]);

    LOG1_AHDR( "%s:exit!\n", __FUNCTION__);
}
/******************************************************************************
 * SetGlobalTMO()
 *****************************************************************************/
bool SetGlobalTMO
(
    AhdrHandle_t pAhdrCtx
) {

    LOG1_AHDR( "%s:enter!\n", __FUNCTION__);

    bool returnValue = false;

    if(pAhdrCtx->AhdrConfig.tmo_para.global.isHdrGlobalTmo == true) {
        returnValue = true;
        pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_weightkey = 0;
    }

    else
        returnValue = false;

    LOGD_AHDR("%s: set GlobalTMO:%d\n", __FUNCTION__, returnValue);

    return returnValue;

    LOG1_AHDR( "%s:exit!\n", __FUNCTION__);
}

/******************************************************************************
 * AhdrGetProcRes()
 *****************************************************************************/
void AhdrGetProcRes
(
    AhdrHandle_t pAhdrCtx
) {

    LOG1_AHDR( "%s:enter!\n", __FUNCTION__);

    MergeProcessing(pAhdrCtx);
    TmoProcessing(pAhdrCtx);

    //tmo enable
    pAhdrCtx->AhdrProcRes.bTmoEn = pAhdrCtx->AhdrConfig.tmo_para.bTmoEn;
    pAhdrCtx->AhdrProcRes.isLinearTmo = pAhdrCtx->AhdrConfig.tmo_para.isLinearTmo;

    // Set Global TMO
    pAhdrCtx->AhdrProcRes.isHdrGlobalTmo = SetGlobalTMO(pAhdrCtx);

    // store current handle data to pre data for next loop
    pAhdrCtx->AhdrPrevData.MergeMode = pAhdrCtx->CurrHandleData.MergeMode;
    pAhdrCtx->AhdrPrevData.ro_hdrtmo_lgmean = pAhdrCtx->AhdrProcRes.TmoProcRes.sw_hdrtmo_set_lgmean;
    pAhdrCtx->AhdrPrevData.PreL2S_ratio = pAhdrCtx->CurrHandleData.CurrL2S_Ratio;
    pAhdrCtx->AhdrPrevData.PreLExpo = pAhdrCtx->CurrHandleData.CurrLExpo;
    pAhdrCtx->AhdrPrevData.PreEnvlv = pAhdrCtx->CurrHandleData.CurrEnvLv;
    pAhdrCtx->AhdrPrevData.PreMoveCoef = pAhdrCtx->CurrHandleData.CurrMoveCoef;
    pAhdrCtx->AhdrPrevData.PreOEPdf = pAhdrCtx->CurrHandleData.CurrOEPdf;
    pAhdrCtx->AhdrPrevData.PreTotalFocusLuma = pAhdrCtx->CurrHandleData.CurrTotalFocusLuma;
    pAhdrCtx->AhdrPrevData.PreDarkPdf = pAhdrCtx->CurrHandleData.CurrDarkPdf;
    pAhdrCtx->AhdrPrevData.PreISO = pAhdrCtx->CurrHandleData.CurrISO;
    pAhdrCtx->AhdrPrevData.PreDynamicRange = pAhdrCtx->CurrHandleData.CurrDynamicRange;
    memcpy(&pAhdrCtx->AhdrPrevData.PrevMergeHandleData, &pAhdrCtx->CurrHandleData.CurrMergeHandleData, sizeof(MergeHandleData_s));
    memcpy(&pAhdrCtx->AhdrPrevData.PrevTmoHandleData, &pAhdrCtx->CurrHandleData.CurrTmoHandleData, sizeof(TmoHandleData_s));
    ++pAhdrCtx->frameCnt;

    LOG1_AHDR( "%s:exit!\n", __FUNCTION__);
}

/******************************************************************************
 * AhdrProcessing()
 *get handle para by config and current variate
 *****************************************************************************/
void AhdrProcessing
(
    AhdrHandle_t     pAhdrCtx,
    AecPreResult_t  AecHdrPreResult,
    af_preprocess_result_t AfPreResult
)
{
    LOG1_AHDR("%s:enter!\n", __FUNCTION__);

    LOGD_AHDR("%s:  Ahdr Current frame cnt:%d:\n",  __FUNCTION__, pAhdrCtx->frameCnt);

    //get Current hdr mode
    pAhdrCtx->CurrHandleData.MergeMode = pAhdrCtx->FrameNumber - 1;
    LOGD_AHDR("%s:  Current MergeMode: %d \n", __FUNCTION__, pAhdrCtx->CurrHandleData.MergeMode);

    //get current ae data from AecPreRes
    AhdrGetAeResult(pAhdrCtx, AecHdrPreResult);

    //transfer ae data to CurrHandle
    pAhdrCtx->CurrHandleData.CurrEnvLv = LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrEnvLv, ENVLVMAX, ENVLVMIN);

    pAhdrCtx->CurrHandleData.CurrMoveCoef = 1;
    pAhdrCtx->CurrHandleData.CurrMoveCoef = LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrMoveCoef, MOVECOEFMAX, MOVECOEFMIN);

    pAhdrCtx->CurrHandleData.CurrISO = pAhdrCtx->CurrAeResult.ISO;
    pAhdrCtx->CurrHandleData.CurrISO = LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrISO, ISOMAX, ISOMIN);

    pAhdrCtx->CurrHandleData.CurrOEPdf = pAhdrCtx->CurrAeResult.OEPdf;
    pAhdrCtx->CurrHandleData.CurrOEPdf = LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrOEPdf, OEPDFMAX, OEPDFMIN);

    pAhdrCtx->CurrHandleData.CurrTotalFocusLuma = 1;
    pAhdrCtx->CurrHandleData.CurrTotalFocusLuma = LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrTotalFocusLuma, FOCUSLUMAMAX, FOCUSLUMAMIN);

    pAhdrCtx->CurrHandleData.CurrDarkPdf = pAhdrCtx->CurrAeResult.DarkPdf;
    pAhdrCtx->CurrHandleData.CurrDarkPdf = LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrDarkPdf, 0.5, 0);

    pAhdrCtx->CurrHandleData.CurrDynamicRange = pAhdrCtx->CurrAeResult.DynamicRange;
    pAhdrCtx->CurrHandleData.CurrDynamicRange = LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrDynamicRange, DYNAMICRANGEMAX, DYNAMICRANGEMIN);

    if(pAhdrCtx->hdrAttr.opMode == HDR_OpMode_Api_OFF)
    {
        LOGD_AHDR("%s:  Ahdr api OFF!! Current Handle data:\n", __FUNCTION__);

        AhdrApiOffProcess(pAhdrCtx, AecHdrPreResult, AfPreResult);

        //log after updating
        LOGD_AHDR("%s:	Current CurrEnvLv:%f OECurve_smooth:%f OECurve_offset:%f \n", __FUNCTION__,  pAhdrCtx->CurrHandleData.CurrEnvLv,
                  pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_smooth, pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_offset);
        LOGD_AHDR("%s:	Current CurrMoveCoef:%f MDCurveLM_smooth:%f MDCurveLM_offset:%f \n", __FUNCTION__, pAhdrCtx->CurrHandleData.CurrMoveCoef,
                  pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_smooth, pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_offset);
        LOGD_AHDR("%s:	Current CurrMoveCoef:%f MDCurveMS_smooth:%f MDCurveMS_offset:%f \n", __FUNCTION__, pAhdrCtx->CurrHandleData.CurrMoveCoef,
                  pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_smooth, pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_offset);
        LOGD_AHDR("%s:  GlobalLumaMode:%f CurrEnvLv:%f CurrISO:%f GlobeLuma:%f GlobeMaxLuma:%f \n", __FUNCTION__,  pAhdrCtx->AhdrConfig.tmo_para.Luma.globalLumaMode,
                  pAhdrCtx->CurrHandleData.CurrEnvLv, pAhdrCtx->CurrHandleData.CurrISO, pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeLuma, pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeMaxLuma);
        LOGD_AHDR("%s:  DetailsHighLightMode:%f CurrOEPdf:%f CurrEnvLv:%f DetailsHighLight:%f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLightMode, pAhdrCtx->CurrHandleData.CurrOEPdf
                  , pAhdrCtx->CurrHandleData.CurrEnvLv, pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsHighLight);
        LOGD_AHDR("%s:  DetailsLowLightMode:%f CurrTotalFocusLuma:%f CurrDarkPdf:%f CurrISO:%f DetailsLowLight:%f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLightMode,
                  pAhdrCtx->CurrHandleData.CurrTotalFocusLuma, pAhdrCtx->CurrHandleData.CurrDarkPdf, pAhdrCtx->CurrHandleData.CurrISO, pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsLowLight);
        LOGD_AHDR("%s:  localtmoMode:%f CurrDynamicRange:%f CurrEnvLv:%f LocalTmoStrength:%f\n", __FUNCTION__,  pAhdrCtx->AhdrConfig.tmo_para.local.localtmoMode, pAhdrCtx->CurrHandleData.CurrDynamicRange,
                  pAhdrCtx->CurrHandleData.CurrEnvLv, pAhdrCtx->CurrHandleData.CurrTmoHandleData.LocalTmoStrength);
        LOGD_AHDR("%s:  GlobalTMO en:%d mode:%f CurrDynamicRange:%f CurrEnvLv:%f Strength:%f\n", __FUNCTION__,  pAhdrCtx->AhdrConfig.tmo_para.global.isHdrGlobalTmo, pAhdrCtx->AhdrConfig.tmo_para.global.mode, pAhdrCtx->CurrHandleData.CurrDynamicRange,
                  pAhdrCtx->CurrHandleData.CurrEnvLv, pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobalTmoStrength);

    }
    else if(pAhdrCtx->hdrAttr.opMode == HDR_OpMode_Auto)
    {
        LOGD_AHDR("%s:  Ahdr api Auto!! Current Handle data:\n", __FUNCTION__);
        AhdrApiAutoUpdate(pAhdrCtx);
    }
    else if(pAhdrCtx->hdrAttr.opMode == HDR_OpMode_MANU)
    {
        LOGD_AHDR("%s:  Ahdr api Manual!! Current Handle data:\n", __FUNCTION__);
        AhdrApiManualUpdate(pAhdrCtx);
    }
    else if(pAhdrCtx->hdrAttr.opMode == HDR_OpMode_SET_LEVEL)
    {
        LOGD_AHDR("%s:  Ahdr api set level!! Current Handle data:\n", __FUNCTION__);

        AhdrApiOffProcess(pAhdrCtx, AecHdrPreResult, AfPreResult);
        //merge log
        LOGD_AHDR("%s:	Current CurrEnvLv:%f OECurve_smooth:%f OECurve_offset:%f \n", __FUNCTION__,  pAhdrCtx->CurrHandleData.CurrEnvLv,
                  pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_smooth, pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_offset);
        LOGD_AHDR("%s:	Current CurrMoveCoef:%f MDCurveLM_smooth:%f MDCurveLM_offset:%f \n", __FUNCTION__, pAhdrCtx->CurrHandleData.CurrMoveCoef,
                  pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_smooth, pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_offset);
        LOGD_AHDR("%s:	Current CurrMoveCoef:%f MDCurveMS_smooth:%f MDCurveMS_offset:%f \n", __FUNCTION__, pAhdrCtx->CurrHandleData.CurrMoveCoef,
                  pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_smooth, pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_offset);
        AhdrApiSetLevel(pAhdrCtx);

    }
    else if(pAhdrCtx->hdrAttr.opMode == HDR_OpMode_DarkArea)
    {
        LOGD_AHDR("%s:  Ahdr api DarkArea!! Current Handle data:\n", __FUNCTION__);
        AhdrApiOffProcess(pAhdrCtx, AecHdrPreResult, AfPreResult);
        //merge log
        LOGD_AHDR("%s:	Current CurrEnvLv:%f OECurve_smooth:%f OECurve_offset:%f \n", __FUNCTION__,  pAhdrCtx->CurrHandleData.CurrEnvLv,
                  pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_smooth, pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_offset);
        LOGD_AHDR("%s:	Current CurrMoveCoef:%f MDCurveLM_smooth:%f MDCurveLM_offset:%f \n", __FUNCTION__, pAhdrCtx->CurrHandleData.CurrMoveCoef,
                  pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_smooth, pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_offset);
        LOGD_AHDR("%s:	Current CurrMoveCoef:%f MDCurveMS_smooth:%f MDCurveMS_offset:%f \n", __FUNCTION__, pAhdrCtx->CurrHandleData.CurrMoveCoef,
                  pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_smooth, pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_offset);

        //tmo
        pAhdrCtx->AhdrConfig.tmo_para.bTmoEn = true;
        pAhdrCtx->AhdrConfig.tmo_para.isLinearTmo = pAhdrCtx->FrameNumber == 1 ? true : false;
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsLowLight *= 1 + (float)(pAhdrCtx->hdrAttr.stDarkArea.level) * 0.4;
        pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsLowLight =
            LIMIT_VALUE(pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsLowLight, DETAILSLOWLIGHTMAX, DETAILSLOWLIGHTMIN);
        LOGD_AHDR("%s: Linear TMO en:%d DetailsLowLightMode:%f CurrTotalFocusLuma:%f CurrDarkPdf:%f CurrISO:%f DetailsLowLight:%f\n", __FUNCTION__, pAhdrCtx->AhdrProcRes.isLinearTmo,
                  pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLightMode, pAhdrCtx->CurrHandleData.CurrTotalFocusLuma, pAhdrCtx->CurrHandleData.CurrDarkPdf,
                  pAhdrCtx->CurrHandleData.CurrISO, pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsLowLight);

    }
    else if(pAhdrCtx->hdrAttr.opMode == HDR_OpMode_Tool)
    {
        LOGD_AHDR("%s:  Ahdr api Tool!! Current Handle data:\n", __FUNCTION__);
        AhdrApiOffProcess(pAhdrCtx, AecHdrPreResult, AfPreResult);

        //tmo en
        pAhdrCtx->AhdrConfig.tmo_para.bTmoEn = pAhdrCtx->AhdrConfig.tmo_para.bTmoEn;
        pAhdrCtx->AhdrConfig.tmo_para.isLinearTmo = pAhdrCtx->AhdrConfig.tmo_para.bTmoEn && pAhdrCtx->FrameNumber == 1;

        //log after updating
        LOGD_AHDR("%s:	Current CurrEnvLv:%f OECurve_smooth:%f OECurve_offset:%f \n", __FUNCTION__,  pAhdrCtx->CurrHandleData.CurrEnvLv,
                  pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_smooth, pAhdrCtx->CurrHandleData.CurrMergeHandleData.OECurve_offset);
        LOGD_AHDR("%s:	Current CurrMoveCoef:%f MDCurveLM_smooth:%f MDCurveLM_offset:%f \n", __FUNCTION__, pAhdrCtx->CurrHandleData.CurrMoveCoef,
                  pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_smooth, pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveLM_offset);
        LOGD_AHDR("%s:	Current CurrMoveCoef:%f MDCurveMS_smooth:%f MDCurveMS_offset:%f \n", __FUNCTION__, pAhdrCtx->CurrHandleData.CurrMoveCoef,
                  pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_smooth, pAhdrCtx->CurrHandleData.CurrMergeHandleData.MDCurveMS_offset);
        LOGD_AHDR("%s:  GlobalLumaMode:%f CurrEnvLv:%f CurrISO:%f GlobeLuma:%f GlobeMaxLuma:%f \n", __FUNCTION__,  pAhdrCtx->AhdrConfig.tmo_para.Luma.globalLumaMode,
                  pAhdrCtx->CurrHandleData.CurrEnvLv, pAhdrCtx->CurrHandleData.CurrISO, pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeLuma, pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobeMaxLuma);
        LOGD_AHDR("%s:  DetailsHighLightMode:%f CurrOEPdf:%f CurrEnvLv:%f DetailsHighLight:%f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.DtsHiLit.DetailsHighLightMode, pAhdrCtx->CurrHandleData.CurrOEPdf
                  , pAhdrCtx->CurrHandleData.CurrEnvLv, pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsHighLight);
        LOGD_AHDR("%s:  DetailsLowLightMode:%f CurrTotalFocusLuma:%f CurrDarkPdf:%f CurrISO:%f DetailsLowLight:%f\n", __FUNCTION__, pAhdrCtx->AhdrConfig.tmo_para.DtsLoLit.DetailsLowLightMode,
                  pAhdrCtx->CurrHandleData.CurrTotalFocusLuma, pAhdrCtx->CurrHandleData.CurrDarkPdf, pAhdrCtx->CurrHandleData.CurrISO, pAhdrCtx->CurrHandleData.CurrTmoHandleData.DetailsLowLight);
        LOGD_AHDR("%s:  localtmoMode:%f CurrDynamicRange:%f CurrEnvLv:%f LocalTmoStrength:%f\n", __FUNCTION__,  pAhdrCtx->AhdrConfig.tmo_para.local.localtmoMode, pAhdrCtx->CurrHandleData.CurrDynamicRange,
                  pAhdrCtx->CurrHandleData.CurrEnvLv, pAhdrCtx->CurrHandleData.CurrTmoHandleData.LocalTmoStrength);
        LOGD_AHDR("%s:  GlobalTMO en:%d mode:%f CurrDynamicRange:%f CurrEnvLv:%f Strength:%f\n", __FUNCTION__,  pAhdrCtx->AhdrConfig.tmo_para.global.isHdrGlobalTmo, pAhdrCtx->AhdrConfig.tmo_para.global.mode, pAhdrCtx->CurrHandleData.CurrDynamicRange,
                  pAhdrCtx->CurrHandleData.CurrEnvLv, pAhdrCtx->CurrHandleData.CurrTmoHandleData.GlobalTmoStrength);

    }
    else
        LOGE_AHDR("%s:  Ahdr wrong mode!!!\n", __FUNCTION__);


    //transfer data to api
    AhdrTranferData2Api(pAhdrCtx);


    //read current rodata
    pAhdrCtx->CurrHandleData.CurrLgMean = pAhdrCtx->CurrStatsData.tmo_stats.ro_hdrtmo_lgmean / 2048.0;

    //calc the current merge luma
    float MergeLuma = (float)pAhdrCtx->CurrStatsData.tmo_stats.ro_hdrtmo_lgmean;
    MergeLuma = MergeLuma / 2048.0;
    float lgmean = MergeLuma;
    MergeLuma = pow(2, MergeLuma);
    MergeLuma =  MergeLuma / 16;

    //get pre frame tmo mean luma
    unsigned long tmo_mean = 0;
    for(int i = 0; i < 225; i++) {
        tmo_mean += pAhdrCtx->CurrStatsData.other_stats.tmo_luma[i];
    }
    tmo_mean = tmo_mean / 225;
    tmo_mean = tmo_mean / 16;

    //calc short middle long frame mean luma
    unsigned long short_mean = 0, middle_mean = 0, long_mean = 0;
    for (int i = 0; i < 225; i++)
    {
        short_mean += pAhdrCtx->CurrAeResult.BlockLumaS[i];
        long_mean += pAhdrCtx->CurrAeResult.BlockLumaL[i];
    }
    short_mean = short_mean / 225;
    long_mean = long_mean / 225;
    short_mean = short_mean / 16;
    long_mean = long_mean / 16;

    for(int i = 0; i < 25; i++)
        middle_mean += pAhdrCtx->CurrAeResult.BlockLumaM[i];
    middle_mean = middle_mean / 25;
    middle_mean = middle_mean / 16;

    LOGD_AHDR("%s:  preFrame lgMergeLuma:%f MergeLuma(8bit):%f TmoLuma(8bit):%d\n", __FUNCTION__, lgmean, MergeLuma, tmo_mean);
    LOGD_AHDR("%s:  preFrame SLuma(8bit):%d MLuma(8bit):%d LLuma(8bit):%d\n", __FUNCTION__, short_mean, middle_mean, long_mean);

    //get proc res
    AhdrGetProcRes(pAhdrCtx);

    LOG1_AHDR( "%s:exit!\n", __FUNCTION__);
}

/******************************************************************************
 * AhdrInit()
 *****************************************************************************/
RESULT AhdrInit
(
    AhdrInstanceConfig_t* pInstConfig
) {

    AhdrContext_s *pAhdrCtx;

    LOG1_AHDR("%s:enter!\n", __FUNCTION__);

    RESULT result = AHDR_RET_SUCCESS;

    // initial checks
    if (pInstConfig == NULL)
        return (AHDR_RET_INVALID_PARM);

    // allocate AHDR control context
    pAhdrCtx = (AhdrContext_s*)malloc(sizeof(AhdrContext_s));
    if (NULL == pAhdrCtx) {
        LOGE_AHDR( "%s: Can't allocate AHDR context\n",  __FUNCTION__);
        return (AHDR_RET_OUTOFMEM);
    }

    // pre-initialize context
    memset(pAhdrCtx, 0x00, sizeof(*pAhdrCtx));
    pAhdrCtx->state = AHDR_STATE_INITIALIZED;
    pInstConfig->hAhdr = (AhdrHandle_t)pAhdrCtx;

    LOG1_AHDR( "%s:exit!\n", __FUNCTION__);

    return (AHDR_RET_SUCCESS);
}
/******************************************************************************
 * AhdrRelease()
 *****************************************************************************/
RESULT AhdrRelease
(
    AhdrHandle_t pAhdrCtx
) {

    LOG1_AHDR( "%s:enter!\n", __FUNCTION__);
    RESULT result = AHDR_RET_SUCCESS;

    // initial checks
    if (NULL == pAhdrCtx) {
        return (AHDR_RET_WRONG_HANDLE);
    }

    result = AhdrStop(pAhdrCtx);
    if (result != AHDR_RET_SUCCESS) {
        LOGE_AHDR( "%s: AHDRStop() failed!\n", __FUNCTION__);
        return (result);
    }

    // check state
    if ((AHDR_STATE_RUNNING == pAhdrCtx->state)
            || (AHDR_STATE_LOCKED == pAhdrCtx->state)) {
        return (AHDR_RET_BUSY);
    }

    memset(pAhdrCtx, 0, sizeof(AhdrContext_s));
    free(pAhdrCtx);
    pAhdrCtx = NULL;

    LOG1_AHDR( "%s:exit!\n", __FUNCTION__);

    return (AHDR_RET_SUCCESS);
}
