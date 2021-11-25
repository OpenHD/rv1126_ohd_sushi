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
#ifndef __RK_AIQ_AHDR_ALGO_TMO_H__
#define __RK_AIQ_AHDR_ALGO_TMO_H__

#include "rk_aiq_types_ahdr_algo_prvt.h"


#include <math.h>
#include <string.h>
#include <stdlib.h>


#ifdef __cplusplus
extern "C"
{
#endif

unsigned short GetSetLgmean(AhdrHandle_t pAhdrCtx);
unsigned short GetSetLgAvgMax(AhdrHandle_t pAhdrCtx, float set_lgmin, float set_lgmax);
unsigned short GetSetLgRange0(AhdrHandle_t pAhdrCtx, float set_lgmin, float set_lgmax);
unsigned short GetSetLgRange1(AhdrHandle_t pAhdrCtx, float set_lgmin, float set_lgmax);
unsigned short GetSetPalhpa(AhdrHandle_t pAhdrCtx, float set_lgmin, float set_lgmax);
void TmoGetCurrIOData(AhdrHandle_t pAhdrCtx);
void TmoProcessing(AhdrHandle_t pAhdrCtx);

#ifdef __cplusplus
}
#endif


#endif
