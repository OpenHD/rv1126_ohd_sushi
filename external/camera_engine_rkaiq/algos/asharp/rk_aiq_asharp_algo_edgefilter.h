
#ifndef __RKAIQ_ASHARP_EDGEFILTER_H__
#define __RKAIQ_ASHARP_EDGEFILTER_H__

#include "stdio.h"
#include "string.h"
#include "math.h"
#include "base/xcam_log.h"
#include "rk_aiq_comm.h"
#include "RkAiqCalibDbTypes.h"
#include "rk_aiq_types_asharp_algo_prvt.h"


RKAIQ_BEGIN_DECLARE

AsharpResult_t edgefilter_get_mode_cell_idx_by_name(CalibDb_EdgeFilter_2_t *pCalibdb, char *name, int *mode_idx);

AsharpResult_t edgefilter_get_setting_idx_by_name(CalibDb_EdgeFilter_2_t *pCalibdb, char *name, int mode_idx, int *setting_idx);

AsharpResult_t edgefilter_config_setting_param(RKAsharp_EdgeFilter_Params_t *pParams, CalibDb_EdgeFilter_2_t *pCalibdb, char* param_mode, char* snr_name);

AsharpResult_t init_edgefilter_params(RKAsharp_EdgeFilter_Params_t *pParams, CalibDb_EdgeFilter_2_t *pCalibdb, int mode_idx, int setting_idx);

AsharpResult_t select_edgefilter_params_by_ISO(RKAsharp_EdgeFilter_Params_t *strkedgefilterParams,      RKAsharp_EdgeFilter_Params_Select_t *strkedgefilterParamsSelected, AsharpExpInfo_t *pExpInfo);

AsharpResult_t edgefilter_fix_transfer(RKAsharp_EdgeFilter_Params_Select_t* edgeflt, RKAsharp_Edgefilter_Fix_t* pEdgefilterCfg , float fPercent);


RKAIQ_END_DECLARE

#endif

