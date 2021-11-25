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
#ifndef __RK_AIQ_AHDR_ALGO_MERGE_H__
#define __RK_AIQ_AHDR_ALGO_MERGE_H__

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "rk_aiq_types_ahdr_algo_prvt.h"

#ifdef __cplusplus
extern "C"
{
#endif

void CalibrateOECurve(float smooth, float offset, unsigned short *OECurve);
void CalibrateMDCurve(float smooth, float offset, unsigned short *MDCurve);
RESULT MergeGetCurrIOData(AhdrHandle_t pAhdrCtx);
RESULT MergeProcessing(AhdrHandle_t pAhdrCtx);

#ifdef __cplusplus
}
#endif

#endif
