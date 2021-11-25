#ifndef __RK_ISP20_MFNR_PARSE_PARAMS_
#define __RK_ISP20_MFNR_PARSE_PARAMS_

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "base/xcam_log.h"
#include "rk_aiq_comm.h"
#include "RkAiqCalibDbTypes.h"
#include "anr/rk_aiq_types_anr_algo_prvt.h"

RKAIQ_BEGIN_DECLARE

ANRresult_t mfnr_get_mode_cell_idx_by_name(CalibDb_MFNR_2_t *pCalibdb, const char *name, int *mode_idx);

ANRresult_t mfnr_get_setting_idx_by_name(CalibDb_MFNR_2_t *pCalibdb, char *name, int mode_idx, int *setting_idx);

ANRresult_t init_mfnr_dynamic_params(RKAnr_Mfnr_Dynamic_t *pDynamic, CalibDb_MFNR_2_t *pCalibdb, int mode_idx);

ANRresult_t mfnr_config_dynamic_param(RKAnr_Mfnr_Dynamic_t *pDynamic,  CalibDb_MFNR_2_t *pCalibdb, char* param_mode);

ANRresult_t mfnr_config_setting_param(RKAnr_Mfnr_Params_t *pParams, CalibDb_MFNR_2_t *pCalibdb, char* param_mode, char* snr_name);

ANRresult_t init_mfnr_params(RKAnr_Mfnr_Params_t *pParams, CalibDb_MFNR_2_t *pCalibdb, int mode_idx, int setting_idx);

ANRresult_t select_mfnr_params_by_ISO(RKAnr_Mfnr_Params_t *stmfnrParams,    RKAnr_Mfnr_Params_Select_t *stmfnrParamsSelected, ANRExpInfo_t *pExpInfo, int bits_proc);

ANRresult_t mfnr_fix_transfer(RKAnr_Mfnr_Params_Select_t* tnr, RKAnr_Mfnr_Fix_t *pMfnrCfg, ANRExpInfo_t *pExpInfo, float gain_ratio, float fLumaStrength, float fChromaStrength);

ANRresult_t mfnr_fix_Printf(RKAnr_Mfnr_Fix_t  * pMfnrCfg);

ANRresult_t mfnr_dynamic_calc(RKAnr_Mfnr_Dynamic_t  * pDynamic, ANRExpInfo_t *pExpInfo);

RKAIQ_END_DECLARE

#endif


