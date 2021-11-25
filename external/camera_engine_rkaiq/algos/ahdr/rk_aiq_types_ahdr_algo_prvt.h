/******************************************************************************
 *
 * Copyright 2019, Fuzhou Rockchip Electronics Co.Ltd . All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Fuzhou Rockchip Electronics Co.Ltd .
 *
 *
 *****************************************************************************/
#ifndef __RK_AIQ_TYPES_AHDR_ALGO_PRVT_H__
#define __RK_AIQ_TYPES_AHDR_ALGO_PRVT_H__

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "ae/rk_aiq_types_ae_algo_int.h"
#include "af/rk_aiq_types_af_algo_int.h"
#include "rk_aiq_algo_types.h"
//#include "RkAiqCalibDbTypes.h"
#include "xcam_log.h"
//#include "ae/rk_aiq_types_ae_algo_int.h"
//#include "af/rk_aiq_af_hw_v200.h"
#include "rk_aiq_types_ahdr_stat_v200.h"
//#include "rk_aiq_types_ahdr_algo_int.h"



#define LIMIT_VALUE(value,max_value,min_value)      (value > max_value? max_value : value < min_value ? min_value : value)
#define SHIFT6BIT(A)         (A*64)
#define SHIFT7BIT(A)         (A*128)
#define SHIFT10BIT(A)         (A*1024)
#define SHIFT11BIT(A)         (A*2048)
#define SHIFT12BIT(A)         (A*4096)

#define LIMIT_PARA(a,b,c,d,e)      (c+(a-e)*(b-c)/(d -e))


#define AHDR_RET_SUCCESS             0   //!< this has to be 0, if clauses rely on it
#define AHDR_RET_FAILURE             1   //!< general failure
#define AHDR_RET_NOTSUPP             2   //!< feature not supported
#define AHDR_RET_BUSY                3   //!< there's already something going on...
#define AHDR_RET_CANCELED            4   //!< operation canceled
#define AHDR_RET_OUTOFMEM            5   //!< out of memory
#define AHDR_RET_OUTOFRANGE          6   //!< parameter/value out of range
#define AHDR_RET_IDLE                7   //!< feature/subsystem is in idle state
#define AHDR_RET_WRONG_HANDLE        8   //!< handle is wrong
#define AHDR_RET_NULL_POINTER        9   //!< the/one/all parameter(s) is a(are) NULL pointer(s)
#define AHDR_RET_NOTAVAILABLE       10   //!< profile not available
#define AHDR_RET_DIVISION_BY_ZERO   11   //!< a divisor equals ZERO
#define AHDR_RET_WRONG_STATE        12   //!< state machine in wrong state
#define AHDR_RET_INVALID_PARM       13   //!< invalid parameter
#define AHDR_RET_PENDING            14   //!< command pending
#define AHDR_RET_WRONG_CONFIG       15   //!< given configuration is invalid

#define BIGMODE     (2560)
#define MAXLUMAK     (1.5)
#define MAXLUMAB     (30)

#define ENVLVMAX     (1.0)
#define ENVLVMIN     (0.0)
#define MOVECOEFMAX     (1.0)
#define MOVECOEFMIN     (0.0)
#define OEPDFMAX     (1.0)
#define OEPDFMIN     (0.0)
#define FOCUSLUMAMAX     (100)
#define FOCUSLUMAMIN     (1)
#define OECURVESMOOTHMAX     (200)
#define OECURVESMOOTHMIN     (20)
#define OECURVEOFFSETMAX     (280)
#define OECURVEOFFSETMIN     (108)
#define MDCURVESMOOTHMAX     (200)
#define MDCURVESMOOTHMIN     (20)
#define MDCURVEOFFSETMAX     (100)
#define MDCURVEOFFSETMIN     (26)
#define DAMPMAX     (1.0)
#define DAMPMIN     (0.0)
#define GLOBALLUMAMODEMAX     (1.0)
#define GLOBALLUMAMODEMIN     (0.0)
#define DETAILSLOWLIGHTMODEMAX     (2.0)
#define DETAILSLOWLIGHTMODEMIN     (0.0)
#define DETAILSHIGHLIGHTMODEMAX     (1.0)
#define DETAILSHIGHLIGHTMODEMIN     (0.0)
#define TMOCONTRASTMODEMAX     (1.0)
#define TMOCONTRASTMODEMIN     (0.0)
#define FASTMODELEVELMAX     (100.0)
#define FASTMODELEVELMIN     (1.0)
#define IIRMAX     (1000)
#define IIRMIN     (0)


#define DAYTHMAX     (1.0)
#define DAYTHMIN     (0.0)
#define DARKPDFTHMAX     (1.0)
#define DARKPDFTHMIN     (0.0)
#define TOLERANCEMAX     (20.0)
#define TOLERANCEMIN     (0.0)
#define GLOBEMAXLUMAMAX     (1023)
#define GLOBEMAXLUMAMIN     (51)
#define GLOBELUMAMAX     (737)
#define GLOBELUMAMIN     (51)
#define DETAILSHIGHLIGHTMAX     (1023)
#define DETAILSHIGHLIGHTMIN     (51)
#define DARKPDFMAX     (1)
#define DARKPDFMIN     (0)
#define ISOMIN     (50)
#define ISOMAX     (204800)
#define DETAILSLOWLIGHTMAX     (63)
#define DETAILSLOWLIGHTMIN     (16)
#define DYNAMICRANGEMAX     (84)
#define DYNAMICRANGEMIN     (1)
#define TMOCONTRASTMAX     (255)
#define TMOCONTRASTMIN     (0)
#define IQPARAMAX     (1)
#define IQPARAMIN     (0)
#define IQDETAILSLOWLIGHTMAX     (4)
#define IQDETAILSLOWLIGHTMIN     (1)

#define AHDR_MAX_IQ_DOTS (13)




typedef enum AhdrState_e {
    AHDR_STATE_INVALID       = 0,
    AHDR_STATE_INITIALIZED   = 1,
    AHDR_STATE_STOPPED       = 2,
    AHDR_STATE_RUNNING       = 3,
    AHDR_STATE_LOCKED        = 4,
    AHDR_STATE_MAX
} AhdrState_t;

typedef struct TmoHandleData_s
{
    float GlobeMaxLuma;
    float GlobeLuma;
    float DetailsHighLight;
    float DetailsLowLight;
    float LocalTmoStrength;
    float GlobalTmoStrength;

} TmoHandleData_t;

typedef struct MergeHandleData_s
{
    float OECurve_smooth;
    float OECurve_offset;
    float MDCurveLM_smooth;
    float MDCurveLM_offset;
    float MDCurveMS_smooth;
    float MDCurveMS_offset;
} MergeHandleData_t;

typedef struct AhdrPrevData_s
{
    int MergeMode;
    float PreL2S_ratio;
    float PreLExpo;
    float PreEnvlv;
    float PreMoveCoef;
    float PreOEPdf;
    float PreTotalFocusLuma;
    float PreDarkPdf;
    float PreISO;
    float PreDynamicRange;
    MergeHandleData_t PrevMergeHandleData;
    TmoHandleData_t PrevTmoHandleData;
    unsigned short ro_hdrtmo_lgmean;
} AhdrPrevData_t;

typedef struct CurrAeResult_s {
    //TODO
    float MeanLuma[3];
    float LfrmDarkLuma;
    float LfrmDarkPdf;
    float LfrmOverExpPdf;
    float SfrmMaxLuma;
    float SfrmMaxLumaPdf;
    float GlobalEnvLv;
    float L2M_Ratio;
    float M2S_Ratio;
    float DynamicRange;
    float OEPdf; //the pdf of over exposure in long frame
    float DarkPdf; //the pdf of dark region in long frame
    float ISO; //use long frame

    float Lv_fac;
    float DarkPdf_fac;
    float Contrast_fac;
    float BlockLumaS[225];
    float BlockLumaM[25];
    float BlockLumaL[225];

    //aec delay frame
    int AecDelayframe;

	//aec LumaDeviation
	float LumaDeviationL;
	float LumaDeviationM;
	float LumaDeviationS;
	float LumaDeviationLinear;
} CurrAeResult_t;

typedef struct {
    unsigned char valid;
    int id;
    int depth;
} AfDepthInfo_t;

typedef struct CurrAfResult_s {
    unsigned int CurrAfTargetPos;
    unsigned int CurrAfTargetWidth;
    unsigned int CurrAfTargetHeight;
    AfDepthInfo_t AfDepthInfo[225];
    unsigned int GlobalSharpnessCompensated[225];
} CurrAfResult_t;

typedef struct CurrHandleData_s
{
    int MergeMode;
    float CurrL2S_Ratio;
    float CurrL2M_Ratio;
    float CurrL2L_Ratio;
    float CurrLExpo;
    float CurrEnvLv;
    float CurrMoveCoef;
    float CurrDynamicRange;
    float CurrOEPdf;
    float CurrDarkPdf;
    float CurrISO;
    float DayTh;
    float DarkPdfTH;
    float CurrTotalFocusLuma;
    float TmoDamp;
    float CurrLgMean;
    float LumaWeight[225];
    float MergeOEDamp;
    float MergeMDDampLM;
    float MergeMDDampMS;
    MergeHandleData_t CurrMergeHandleData;
    TmoHandleData_t CurrTmoHandleData;
} CurrHandleData_t;

typedef struct AhdrProcResData_s
{
    TmoProcRes_t TmoProcRes;
    MgeProcRes_t MgeProcRes;
    bool LongFrameMode;
    bool isHdrGlobalTmo;
    bool bTmoEn;
    bool isLinearTmo;
    TmoFlickerPara_t TmoFlicker;
} AhdrProcResData_t;

typedef struct SensorInfo_s
{
    bool  LongFrmMode;
    float HdrMinGain[MAX_HDR_FRAMENUM];
    float HdrMaxGain[MAX_HDR_FRAMENUM];
    float HdrMinIntegrationTime[MAX_HDR_FRAMENUM];
    float HdrMaxIntegrationTime[MAX_HDR_FRAMENUM];

    float MaxExpoL;
    float MinExpoL;
    float MaxExpoM;
    float MinExpoM;
    float MaxExpoS;
    float MinExpoS;
} SensorInfo_t;

typedef struct AhdrContext_s
{
    //api
    hdrAttr_t hdrAttr;

    CalibDb_Ahdr_Para_t pCalibDB;
    AhdrState_t state;
    AhdrConfig_t AhdrConfig;
    AhdrPrevData_t AhdrPrevData ;
    AhdrProcResData_t AhdrProcRes;
    CurrAeResult_t CurrAeResult;
    CurrAfResult_t CurrAfResult;
    CurrHandleData_t CurrHandleData;
    rkisp_ahdr_stats_t CurrStatsData;
    SensorInfo_t SensorInfo;
    uint32_t width;
    uint32_t height;
    int frameCnt;
    int FrameNumber;
    int sence_mode;
} AhdrContext_t;

typedef AhdrContext_t* AhdrHandle_t;

typedef struct AhdrInstanceConfig_s {
    AhdrHandle_t              hAhdr;
} AhdrInstanceConfig_t;

typedef struct _RkAiqAlgoContext {
    AhdrInstanceConfig_t AhdrInstConfig;
    //void* place_holder[0];
} RkAiqAlgoContext;

#endif
