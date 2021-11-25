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
#ifndef __RK_AIQ_AWDR_ALGO_H__
#define __RK_AIQ_AWDR_ALGO_H__

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "ae/rk_aiq_types_ae_algo_int.h"
#include "rk_aiq_uapi_awdr_int.h"
#include "rk_aiq_types_awdr_algo_prvt.h"


RESULT AwdrStart(AwdrHandle_t pAhdrCtx);
RESULT AwdrStop(AwdrHandle_t pAhdrCtx);
void AwdrUpdateConfig(AwdrHandle_t pAhdrCtx, CalibDb_Awdr_Para_t* pConfig, int mode);
void AwdrProcessing(AwdrHandle_t pAhdrCtx, AecPreResult_t AecHdrPreResult);
void AwdrSetProcRes( RkAiqAwdrProcResult_t* pHalProc, AwdrProcResData_t* pAgoProc);
RESULT AwdrInit(AwdrInstanceConfig_t* pInstConfig, CamCalibDbContext_t* pCalibDb) ;
RESULT AwdrRelease(AwdrHandle_t pAhdrCtx) ;


#endif
