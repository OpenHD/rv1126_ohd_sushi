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
#ifndef __RK_AIQ_TYPES_AWDR_ALGO_PRVT_H__
#define __RK_AIQ_TYPES_AWDR_ALGO_PRVT_H__

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "ae/rk_aiq_types_ae_algo_int.h"
#include "af/rk_aiq_types_af_algo_int.h"
#include "rk_aiq_algo_types.h"
#include "xcam_log.h"


#define CAMERIC_WDR_CURVE_SIZE      33

#define LIMIT_VALUE(value,max_value,min_value)      (value > max_value? max_value : value < min_value ? min_value : value)
#define SHIFT6BIT(A)         (A*64)
#define SHIFT7BIT(A)         (A*128)
#define SHIFT10BIT(A)         (A*1024)
#define SHIFT11BIT(A)         (A*2048)
#define SHIFT12BIT(A)         (A*4096)

#define LIMIT_PARA(a,b,c,d,e)      (c+(a-e)*(b-c)/(d -e))


#define AWDR_RET_SUCCESS             0   //!< this has to be 0, if clauses rely on it
#define AWDR_RET_FAILURE             1   //!< general failure
#define AWDR_RET_NOTSUPP             2   //!< feature not supported
#define AWDR_RET_BUSY                3   //!< there's already something going on...
#define AWDR_RET_CANCELED            4   //!< operation canceled
#define AWDR_RET_OUTOFMEM            5   //!< out of memory
#define AWDR_RET_OUTOFRANGE          6   //!< parameter/value out of range
#define AWDR_RET_IDLE                7   //!< feature/subsystem is in idle state
#define AWDR_RET_WRONG_HANDLE        8   //!< handle is wrong
#define AWDR_RET_NULL_POINTER        9   //!< the/one/all parameter(s) is a(are) NULL pointer(s)
#define AWDR_RET_NOTAVAILABLE       10   //!< profile not available
#define AWDR_RET_DIVISION_BY_ZERO   11   //!< a divisor equals ZERO
#define AWDR_RET_WRONG_STATE        12   //!< state machine in wrong state
#define AWDR_RET_INVALID_PARM       13   //!< invalid parameter
#define AWDR_RET_PENDING            14   //!< command pending
#define AWDR_RET_WRONG_CONFIG       15   //!< given configuration is invalid

#define AWDRLEVELMAX     (15)
#define AWDRLEVELMin     (0)
#define AWDRENVLVMAX     (1.0)
#define AWDRENVLVMIN     (0.0)
#define GLOBALCURVEMAX     (4096)
#define GLOBALCURVEMIN     (0)
#define LOCALCURVEMAX     (4095)
#define LOCALCURVEMIN     (0)
#define AWDRTOLERANCEMAX     (1.0)
#define AWDRTOLERANCEMIN     (0.0)
#define AWDRDAMPMAX     (1.0)
#define AWDRDAMPMIN     (0)
#define AWDRSWITCHMAX     (1)
#define AWDRSWITCHMIN     (0)
#define AWDRBAVGMAX     (3)
#define AWDRBAVGMIN     (0)
#define AWDRLVLMAX     (15)
#define AWDRLVLMIN     (0)
#define AWDR8BITMAX     (255)
#define AWDR8BITMIN     (0)

#define AWDRCOEFMAX     (511)
#define AWDRCOEFMIN     (0)
#define AWDRCOEFOFFMAX     (2097151)
#define AWDRCOEFOFFMIN     (0)


#define AWDR_MAX_IQ_DOTS (13)


typedef enum AwdrState_e {
    AWDR_STATE_INVALID       = 0,
    AWDR_STATE_INITIALIZED   = 1,
    AWDR_STATE_STOPPED       = 2,
    AWDR_STATE_RUNNING       = 3,
    AWDR_STATE_LOCKED        = 4,
    AWDR_STATE_MAX
} AwdrState_t;


typedef struct AwdrCurrAeResult_s {
    float GlobalEnvLv;
    float ISO; //use long frame
} AwdrCurrAeResult_t;

typedef struct AwdrCurrHandleData_s {
    float EnvLv;
    float ISO;
} AwdrCurrHandleData_t;

typedef struct AwdrPreHandleData_s {
    float EnvLv;
    float ISO;
    float Level_Final;
} AwdrPreHandleData_t;

typedef enum WdrMode_e {
    CAMERIC_WDR_MODE_GLOBAL = 0,
    CAMERIC_WDR_MODE_BLOCK = 1
} WdrMode_t;

typedef struct AwdrProcResData_s
{
    bool          enable;                            /**< measuring enabled */
    WdrMode_t mode;
    int     segment[CAMERIC_WDR_CURVE_SIZE - 1];    /**< x_i segment size */
    int    wdr_global_y[CAMERIC_WDR_CURVE_SIZE];
    int    wdr_block_y[CAMERIC_WDR_CURVE_SIZE];
    int wdr_noiseratio;
    int wdr_bestlight;
    int wdr_gain_off1;
    int wdr_pym_cc;
    int wdr_epsilon;
    int wdr_lvl_en;
    int wdr_flt_sel;
    int wdr_gain_max_clip_enable;
    int wdr_gain_max_value;
    int wdr_bavg_clip;
    int wdr_nonl_segm;
    int wdr_nonl_open;
    int wdr_nonl_mode1;
    int wdr_coe0;
    int wdr_coe1;
    int wdr_coe2;
    int wdr_coe_off;
} AwdrProcResData_t;

typedef struct AwdrStrength_s {
    float  Level_Final;
    float EnvLv[13];
    float Level[13];
    float damp;
    float Tolerance;
} AwdrStrength_t;

typedef struct AwdrConfig_para_s {
    int segment[CAMERIC_WDR_CURVE_SIZE - 1];
    int                   LocalCurve[CAMERIC_WDR_CURVE_SIZE];
    int                   GlobalCurve[CAMERIC_WDR_CURVE_SIZE];
    int                   wdr_noiseratio;
    int                   wdr_bestlight;
    int                   wdr_gain_off1;
    int                   wdr_pym_cc;
    int                    wdr_epsilon;
    int                    wdr_lvl_en;
    int                    wdr_flt_sel;
    int                    wdr_gain_max_clip_enable;
    int                    wdr_bavg_clip;
    int                    wdr_nonl_segm;
    int                    wdr_nonl_open;
    int                    wdr_nonl_mode1;
    int                   wdr_coe0;
    int                   wdr_coe1;
    int                   wdr_coe2;
    int                   wdr_coe_off;
} AwdrConfig_para_t;

typedef struct AwdrConfig_s {
    bool enable;
    WdrMode_t mode;
    AwdrConfig_para_t WdrConfig;
    AwdrStrength_t WdrStrength;
} AwdrConfig_t;


typedef struct AwdrContext_s
{
    //api
    wdrAttr_t wdrAttr;
    AwdrState_t state;
    CalibDb_Awdr_Para_t pCalibDB;
    AwdrConfig_t Config;
    AwdrProcResData_t ProcRes;
    AwdrCurrAeResult_t CurrAeResult;
    AwdrCurrHandleData_t CurrData;
    AwdrPreHandleData_t PreData;
    int frameCnt;
    int FrameNumber;
    int sence_mode;
} AwdrContext_t;

typedef AwdrContext_t* AwdrHandle_t;

typedef struct AwdrInstanceConfig_s {
    AwdrHandle_t              hAwdr;
} AwdrInstanceConfig_t;

typedef struct _RkAiqAlgoContext {
    AwdrInstanceConfig_t AwdrInstConfig;
    //void* place_holder[0];
} RkAiqAlgoContext;

#endif
