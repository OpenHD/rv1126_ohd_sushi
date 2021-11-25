
#include "rk_aiq_anr_algo.h"
#include "rk_aiq_algo_anr_itf.h"

RKAIQ_BEGIN_DECLARE

ANRresult_t ANRStart(ANRContext_t *pANRCtx) 
{
    LOGI_ANR( "%s:enter!\n", __FUNCTION__);

    // initial checks
    if (pANRCtx == NULL) {
        return (ANR_RET_NULL_POINTER);
    }

    if ((ANR_STATE_RUNNING == pANRCtx->eState)
            || (ANR_STATE_LOCKED == pANRCtx->eState)) {
        return (ANR_RET_FAILURE);
    }

    pANRCtx->eState = ANR_STATE_RUNNING;

    LOGI_ANR( "%s:exit!\n", __FUNCTION__);
    return (ANR_RET_SUCCESS);
}


ANRresult_t ANRStop(ANRContext_t *pANRCtx) 
{
    LOGI_ANR( "%s:enter!\n", __FUNCTION__);

    // initial checks
    if (pANRCtx == NULL) {
        return (ANR_RET_NULL_POINTER);
    }

    if (ANR_STATE_LOCKED == pANRCtx->eState) {
        return (ANR_RET_FAILURE);
    }

    pANRCtx->eState = ANR_STATE_STOPPED;

    LOGI_ANR( "%s:exit!\n", __FUNCTION__);
    return (ANR_RET_SUCCESS);
}


//anr inint
ANRresult_t ANRInit(ANRContext_t **ppANRCtx, CamCalibDbContext_t *pCalibDb)
{
    ANRContext_t * pANRCtx;

    LOGI_ANR("%s(%d): enter!\n", __FUNCTION__, __LINE__);

    pANRCtx = (ANRContext_t *)malloc(sizeof(ANRContext_t));
    if(pANRCtx == NULL) {
        LOGE_ANR("%s(%d): malloc fail\n", __FUNCTION__, __LINE__);
        return ANR_RET_NULL_POINTER;
    }

    memset(pANRCtx, 0x00, sizeof(ANRContext_t));

    //gain state init
    pANRCtx->stGainState.gain_stat_full_last = -1;
    pANRCtx->stGainState.gainState = -1;
    pANRCtx->stGainState.gainState_last = -1;
    pANRCtx->stGainState.gain_th0[0]    = 2.0;
    pANRCtx->stGainState.gain_th1[0]    = 4.0;
    pANRCtx->stGainState.gain_th0[1]    = 32.0;
    pANRCtx->stGainState.gain_th1[1]    = 64.0;
	
	pANRCtx->fLuma_SF_Strength = 1.0;
	pANRCtx->fLuma_TF_Strength = 1.0;
	pANRCtx->fChroma_SF_Strength = 1.0;
	pANRCtx->fChroma_TF_Strength = 1.0;
	pANRCtx->fRawnr_SF_Strength = 1.0;

    pANRCtx->eState = ANR_STATE_INITIALIZED;
    *ppANRCtx = pANRCtx;

    pANRCtx->refYuvBit = 8;
    pANRCtx->eMode = ANR_OP_MODE_AUTO;
	pANRCtx->isIQParaUpdate = false;
	pANRCtx->isGrayMode = false;

#if ANR_USE_XML_FILE
    //read v1 params from xml
    pANRCtx->stBayernrCalib = pCalibDb->bayerNr;
    pANRCtx->stUvnrCalib = pCalibDb->uvnr;
    pANRCtx->stYnrCalib = pCalibDb->ynr;
    pANRCtx->stMfnrCalib = pCalibDb->mfnr;
#endif

#if RK_SIMULATOR_HW
    //just for v2 params from html
    FILE *fp2 = fopen("rkaiq_anr_html_params.bin", "rb");
    if(fp2 != NULL) {
        memset(&pANRCtx->stAuto.stBayernrParams, 0, sizeof(RKAnr_Bayernr_Params_t));
        fread(&pANRCtx->stAuto.stBayernrParams, 1, sizeof(RKAnr_Bayernr_Params_t), fp2);
        memset(&pANRCtx->stAuto.stMfnrParams, 0, sizeof(RKAnr_Mfnr_Params_t));
        fread(&pANRCtx->stAuto.stMfnrParams, 1, sizeof(RKAnr_Mfnr_Params_t), fp2);
        memset(&pANRCtx->stAuto.stUvnrParams, 0, sizeof(RKAnr_Uvnr_Params_t));
        fread(&pANRCtx->stAuto.stUvnrParams, 1, sizeof(RKAnr_Uvnr_Params_t), fp2);
        memset(&pANRCtx->stAuto.stYnrParams, 0, sizeof(RKAnr_Ynr_Params_t));
        fread(&pANRCtx->stAuto.stYnrParams, 1, sizeof(RKAnr_Ynr_Params_t), fp2);
        LOGD_ANR("oyyf: %s:(%d) read anr param html file sucess! \n", __FUNCTION__, __LINE__);
    } else {
        LOGE_ANR("oyyf: %s:(%d) read anr param html file failed! \n", __FUNCTION__, __LINE__);
        return ANR_RET_FAILURE;
    }
#endif

#if ANR_USE_XML_FILE
	pANRCtx->stExpInfo.snr_mode = 0;
	pANRCtx->eParamMode = ANR_PARAM_MODE_NORMAL;
	ANRConfigSettingParam(pANRCtx, pANRCtx->eParamMode, pANRCtx->stExpInfo.snr_mode);
#endif

    LOGD_ANR("%s(%d): bayernr %f %f %f %d %d %f", __FUNCTION__, __LINE__,
             pANRCtx->stAuto.stBayernrParams.filtpar[0],
             pANRCtx->stAuto.stBayernrParams.filtpar[4],
             pANRCtx->stAuto.stBayernrParams.filtpar[8],
             pANRCtx->stAuto.stBayernrParams.peaknoisesigma,
             pANRCtx->stAuto.stBayernrParams.sw_bayernr_edge_filter_en,
             pANRCtx->stAuto.stBayernrParams.sw_bayernr_filter_strength[0]);


    LOGI_ANR("%s(%d): exit!\n", __FUNCTION__, __LINE__);
    return ANR_RET_SUCCESS;
}

//anr release
ANRresult_t ANRRelease(ANRContext_t *pANRCtx)
{
	ANRresult_t result = ANR_RET_SUCCESS;
    LOGI_ANR("%s(%d): enter!\n", __FUNCTION__, __LINE__);
    if(pANRCtx == NULL) {
        LOGE_ANR("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
        return ANR_RET_NULL_POINTER;
    }

	result = ANRStop(pANRCtx);
    if (result != ANR_RET_SUCCESS) {
        LOGE_ANR( "%s: ANRStop() failed!\n", __FUNCTION__);
        return (result);
    }

    // check state
    if ((ANR_STATE_RUNNING == pANRCtx->eState)
            || (ANR_STATE_LOCKED == pANRCtx->eState)) {
        return (ANR_RET_BUSY);
    }
	
    memset(pANRCtx, 0x00, sizeof(ANRContext_t));
    free(pANRCtx);

    LOGI_ANR("%s(%d): exit!\n", __FUNCTION__, __LINE__);
    return ANR_RET_SUCCESS;
}

//anr config
ANRresult_t ANRPrepare(ANRContext_t *pANRCtx, ANRConfig_t* pANRConfig)
{
    LOGI_ANR("%s(%d): enter!\n", __FUNCTION__, __LINE__);

    if(pANRCtx == NULL) {
        LOGE_ANR("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
        return ANR_RET_INVALID_PARM;
    }

    if(pANRConfig == NULL) {
        LOGE_ANR("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
        return ANR_RET_INVALID_PARM;
    }

    //pANRCtx->eMode = pANRConfig->eMode;
    //pANRCtx->eState = pANRConfig->eState;
    //pANRCtx->refYuvBit = pANRConfig->refYuvBit;

	//update iq calibdb to context
	if(!!(pANRCtx->prepare_type & RK_AIQ_ALGO_CONFTYPE_UPDATECALIB)){	
		ANRIQParaUpdate(pANRCtx);
	}
	
    ANRStart(pANRCtx);

    LOGI_ANR("%s(%d): exit!\n", __FUNCTION__, __LINE__);
    return ANR_RET_SUCCESS;
}

//anr reconfig
ANRresult_t ANRReConfig(ANRContext_t *pANRCtx, ANRConfig_t* pANRConfig)
{
    LOGI_ANR("%s(%d): enter!\n", __FUNCTION__, __LINE__);
    //need todo what?

    LOGI_ANR("%s(%d): exit!\n", __FUNCTION__, __LINE__);
    return ANR_RET_SUCCESS;
}

//anr reconfig
ANRresult_t ANRIQParaUpdate(ANRContext_t *pANRCtx)
{
    LOGI_ANR("%s(%d): enter!\n", __FUNCTION__, __LINE__);
    //need todo what?

	if(pANRCtx->isIQParaUpdate){
		LOGD_ANR("IQ data reconfig\n");
		ANRConfigSettingParam(pANRCtx, pANRCtx->eParamMode, pANRCtx->stExpInfo.snr_mode);
		pANRCtx->isIQParaUpdate = false;
	}

    LOGI_ANR("%s(%d): exit!\n", __FUNCTION__, __LINE__);
    return ANR_RET_SUCCESS;
}


//anr preprocess
ANRresult_t ANRPreProcess(ANRContext_t *pANRCtx)
{
    LOGI_ANR("%s(%d): enter!\n", __FUNCTION__, __LINE__);
    //need todo what?

	ANRIQParaUpdate(pANRCtx);
	
    LOGI_ANR("%s(%d): exit!\n", __FUNCTION__, __LINE__);
    return ANR_RET_SUCCESS;
}

//anr process
ANRresult_t ANRProcess(ANRContext_t *pANRCtx, ANRExpInfo_t *pExpInfo)
{
    LOGI_ANR("%s(%d): enter!\n", __FUNCTION__, __LINE__);
	ANRParamMode_t mode = ANR_PARAM_MODE_INVALID;
	
    if(pANRCtx == NULL) {
        LOGE_ANR("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
        return ANR_RET_INVALID_PARM;
    }

    if(pExpInfo == NULL) {
        LOGE_ANR("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
        return ANR_RET_INVALID_PARM;
    }

	if(pANRCtx->eState != ANR_STATE_RUNNING){
		return ANR_RET_SUCCESS;
	}
	
    ANRGainRatioProcess(&pANRCtx->stGainState, pExpInfo);

    ANRParamModeProcess(pANRCtx, pExpInfo, &mode);   
	pExpInfo->mfnr_mode_3to1 = pANRCtx->stMfnrCalib.mode_3to1;
	if(pExpInfo->mfnr_mode_3to1){
		pExpInfo->snr_mode = pExpInfo->pre_snr_mode;
	}else{
		pExpInfo->snr_mode = pExpInfo->cur_snr_mode;
	}
	
    if(pANRCtx->eMode == ANR_OP_MODE_AUTO) {

        LOGD_ANR("%s(%d): refYuvBit:%d\n", __FUNCTION__, __LINE__, pANRCtx->refYuvBit);

		#if ANR_USE_XML_FILE
		if(pExpInfo->snr_mode != pANRCtx->stExpInfo.snr_mode || pANRCtx->eParamMode != mode){
			LOGD_ANR("param mode:%d snr_mode:%d\n", mode, pExpInfo->snr_mode);
			pANRCtx->eParamMode = mode;
			ANRConfigSettingParam(pANRCtx, pANRCtx->eParamMode, pExpInfo->snr_mode);	
		}
		#endif
        
        //select param
        select_bayernr_params_by_ISO(&pANRCtx->stAuto.stBayernrParams, &pANRCtx->stAuto.stBayernrParamSelect, pExpInfo);
        select_mfnr_params_by_ISO(&pANRCtx->stAuto.stMfnrParams, &pANRCtx->stAuto.stMfnrParamSelect, pExpInfo, pANRCtx->refYuvBit);
        select_ynr_params_by_ISO(&pANRCtx->stAuto.stYnrParams, &pANRCtx->stAuto.stYnrParamSelect, pExpInfo, pANRCtx->refYuvBit);
        select_uvnr_params_by_ISO(&pANRCtx->stAuto.stUvnrParams, &pANRCtx->stAuto.stUvnrParamSelect, pExpInfo);
	 mfnr_dynamic_calc(&pANRCtx->stAuto.stMfnr_dynamic, pExpInfo);

    } else if(pANRCtx->eMode == ANR_OP_MODE_MANUAL) {
        //TODO
    }

    memcpy(&pANRCtx->stExpInfo, pExpInfo, sizeof(ANRExpInfo_t));

    LOGI_ANR("%s(%d): exit!\n", __FUNCTION__, __LINE__);
    return ANR_RET_SUCCESS;

}

ANRresult_t ANRSetGainMode(ANRProcResult_t* pANRResult)
{
    LOGI_ANR("%s(%d): enter!\n", __FUNCTION__, __LINE__);

    if(pANRResult == NULL) {
        LOGE_ANR("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
        return ANR_RET_INVALID_PARM;
    }

    if(pANRResult->stGainFix.gain_table_en) {
        pANRResult->stMfnrFix.gain_en = 0;
        pANRResult->stUvnrFix.nr_gain_en = 0;

    } else {
        pANRResult->stMfnrFix.gain_en = 1;
        pANRResult->stUvnrFix.nr_gain_en = 1;
    }

    return ANR_RET_SUCCESS;
}


//anr get result
ANRresult_t ANRGetProcResult(ANRContext_t *pANRCtx, ANRProcResult_t* pANRResult)
{
    LOGI_ANR("%s(%d): enter!\n", __FUNCTION__, __LINE__);

    if(pANRCtx == NULL) {
        LOGE_ANR("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
        return ANR_RET_INVALID_PARM;
    }

    if(pANRResult == NULL) {
        LOGE_ANR("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
        return ANR_RET_INVALID_PARM;
    }

    if(pANRCtx->eMode == ANR_OP_MODE_AUTO) {

        pANRResult->stBayernrParamSelect = pANRCtx->stAuto.stBayernrParamSelect;
        pANRResult->stMfnrParamSelect = pANRCtx->stAuto.stMfnrParamSelect;
        pANRResult->stYnrParamSelect = pANRCtx->stAuto.stYnrParamSelect;
        pANRResult->stUvnrParamSelect = pANRCtx->stAuto.stUvnrParamSelect;

        pANRResult->bayernrEn = pANRCtx->stAuto.bayernrEn;
        pANRResult->mfnrEn = pANRCtx->stAuto.mfnrEn;
        pANRResult->ynrEN = pANRCtx->stAuto.ynrEn;
        pANRResult->uvnrEn = pANRCtx->stAuto.uvnrEn;

	 if(pANRCtx->stAuto.mfnrEn && pANRCtx->stAuto.stMfnr_dynamic.enable){
		pANRResult->mfnrEn = pANRCtx->stAuto.stMfnr_dynamic.mfnr_enable_state;
	 }

    } else if(pANRCtx->eMode == ANR_OP_MODE_MANUAL) {
        //TODO
        pANRResult->bayernrEn = pANRCtx->stManual.bayernrEn;
        pANRResult->stBayernrParamSelect = pANRCtx->stManual.stBayernrParamSelect;
        pANRResult->mfnrEn = pANRCtx->stManual.mfnrEn;
        pANRResult->stMfnrParamSelect = pANRCtx->stManual.stMfnrParamSelect;
        pANRResult->ynrEN = pANRCtx->stManual.ynrEn;
        pANRResult->stYnrParamSelect = pANRCtx->stManual.stYnrParamSelect;
        pANRResult->uvnrEn = pANRCtx->stManual.uvnrEn;
        pANRResult->stUvnrParamSelect = pANRCtx->stManual.stUvnrParamSelect;
 	 pANRCtx->fLuma_TF_Strength = 1.0;
 	 pANRCtx->fLuma_SF_Strength = 1.0;
	 pANRCtx->fChroma_SF_Strength = 1.0;
	 pANRCtx->fChroma_TF_Strength = 1.0;
	 pANRCtx->fRawnr_SF_Strength = 1.0;
    }

	//for bw setting
	if(pANRCtx->isGrayMode){
		LOGD_ANR("anr: set gray mode!\n");
		for(int i=0; i<MFNR_MAX_LVL_UV; i++){
			pANRResult->stMfnrParamSelect.weight_limit_uv[i] = MFNR_MAX_WEIGHT_LIMIT_UV;
		}
	}
		
    //transfer to reg value
    bayernr_fix_tranfer(&pANRResult->stBayernrParamSelect, &pANRResult->stBayernrFix, pANRCtx->fRawnr_SF_Strength);
    mfnr_fix_transfer(&pANRResult->stMfnrParamSelect, &pANRResult->stMfnrFix, &pANRCtx->stExpInfo, pANRCtx->stGainState.ratio, pANRCtx->fLuma_TF_Strength,  pANRCtx->fChroma_TF_Strength);
    ynr_fix_transfer(&pANRResult->stYnrParamSelect, &pANRResult->stYnrFix, pANRCtx->stGainState.ratio, pANRCtx->fLuma_SF_Strength);
    uvnr_fix_transfer(&pANRResult->stUvnrParamSelect, &pANRResult->stUvnrFix, &pANRCtx->stExpInfo, pANRCtx->stGainState.ratio, pANRCtx->fChroma_SF_Strength);
    gain_fix_transfer(&pANRResult->stMfnrParamSelect, &pANRResult->stGainFix, &pANRCtx->stExpInfo, pANRCtx->stGainState.ratio);
    pANRResult->stBayernrFix.rawnr_en = pANRResult->bayernrEn;
    pANRResult->stMfnrFix.tnr_en = pANRResult->mfnrEn;
    pANRResult->stMfnrFix.mode = pANRCtx->stMfnrCalib.mode_3to1;
    pANRResult->stYnrFix.ynr_en = pANRResult->ynrEN;
    pANRResult->stUvnrFix.uvnr_en = pANRResult->uvnrEn;
    pANRResult->stGainFix.gain_table_en = pANRCtx->stMfnrCalib.local_gain_en;
    ANRSetGainMode(pANRResult);

	//for bw setting
	if(pANRCtx->isGrayMode){
		//uvnr disable
		pANRResult->stUvnrFix.uvnr_step1_en = 0;
		pANRResult->stUvnrFix.uvnr_step2_en = 0;
	}

    LOGD_ANR("%s:%d xml:local:%d mode:%d  reg: local gain:%d  mfnr gain:%d mode:%d\n",
             __FUNCTION__, __LINE__,
             pANRCtx->stMfnrCalib.local_gain_en,
             pANRCtx->stMfnrCalib.mode_3to1,
             pANRResult->stGainFix.gain_table_en,
             pANRResult->stMfnrFix.gain_en,
             pANRResult->stMfnrFix.mode);

	 //select motion params
	int mode_idx = 0;
	if(pANRCtx->eParamMode == ANR_PARAM_MODE_NORMAL){
		mfnr_get_mode_cell_idx_by_name(&pANRCtx->stMfnrCalib, "normal", &mode_idx);
	}else if(pANRCtx->eParamMode == ANR_PARAM_MODE_HDR){
		mfnr_get_mode_cell_idx_by_name(&pANRCtx->stMfnrCalib, "hdr", &mode_idx);
	}else if(pANRCtx->eParamMode == ANR_PARAM_MODE_GRAY){
		mfnr_get_mode_cell_idx_by_name(&pANRCtx->stMfnrCalib, "gray", &mode_idx);
	}else{
		LOGE_ANR("%s(%d): not support param mode!\n", __FUNCTION__, __LINE__);
	}
	pANRResult->stMotionParam.stMotion = pANRCtx->stMfnrCalib.mode_cell[mode_idx].motion;
	pANRResult->stMotionParam.gain_ratio = pANRCtx->stGainState.ratio;
    LOGI_ANR("%s(%d): exit!\n", __FUNCTION__, __LINE__);
    return ANR_RET_SUCCESS;
}

ANRresult_t ANRGainRatioProcess(ANRGainState_t *pGainState, ANRExpInfo_t *pExpInfo)
{
    LOGI_ANR("%s(%d): enter!\n", __FUNCTION__, __LINE__);

    if(pGainState == NULL) {
        LOGE_ANR("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
        return ANR_RET_INVALID_PARM;
    }

    if(pExpInfo == NULL) {
        LOGE_ANR("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
        return ANR_RET_INVALID_PARM;
    }

    float gain_cur = pExpInfo->arAGain[pExpInfo->hdr_mode] * pExpInfo->arDGain[pExpInfo->hdr_mode];
    float th[2];
    float gain_th0[2];
    float gain_th1[2];
    for(int i = 0; i < 2; i++){
        gain_th0[i]     = pGainState->gain_th0[i];
        gain_th1[i]     = pGainState->gain_th1[i];
        th[i]           = pow(2.0, (log2(gain_th0[i])+log2(gain_th1[i])) / 2);
    }
	
    pGainState->gain_cur = gain_cur;


	int gain_stat_full = -1;
	int gain_stat_full_last = pGainState->gain_stat_full_last;
	int gain_stat_last = pGainState->gainState_last;
	int gain_stat_cur  = -1;
    	int gain_stat  = -1;
        if(gain_cur <= gain_th0[0])
    	{
            gain_stat_full = 0;
            gain_stat_cur = 0;
    	}
        else if(gain_cur <= gain_th1[0] && gain_cur >= gain_th0[0])
            gain_stat_full = 1;
        else if(gain_cur <= gain_th0[1] && gain_cur >= gain_th1[0])
        {
            gain_stat_full = 2;
            gain_stat_cur = 1;
      	 }
        else if(gain_cur <= gain_th1[1] && gain_cur >= gain_th0[1])
            gain_stat_full = 3;
        else if(gain_cur >= gain_th1[1])
    	{
            gain_stat_full = 4;
            gain_stat_cur = 2;
    	}
       if(gain_stat_last == -1 || (abs(gain_stat_full - gain_stat_full_last) >= 2 && gain_stat_cur == -1))
       {
            if(gain_cur <= th[0])
                gain_stat_cur = 0;
            else if(gain_cur <= th[1])
                gain_stat_cur = 1;
            else
                gain_stat_cur = 2;
        }

        if (gain_stat_cur != -1)
    	 {
	        gain_stat_last      = gain_stat_cur;
	        gain_stat_full_last = gain_stat_full;
	        gain_stat       = gain_stat_cur;
    	 }
        else
            gain_stat       = gain_stat_last;
            
        if (gain_stat ==0)
            pGainState->ratio = 16;
        else if (gain_stat == 1)
            pGainState->ratio = 1;
        else
            pGainState->ratio = 1.0/16.0;
        





    LOGD_ANR("%s:%d gain_cur:%f gain th %f %fd %f %f ratio:%f gain_state:%d %d full    %d %d\n",
             __FUNCTION__, __LINE__,
             gain_cur, 
             gain_th0[0],gain_th0[1],
             gain_th1[0],gain_th1[1],
             pGainState->ratio,
             pGainState->gainState_last,
             pGainState->gainState,
             pGainState->gain_stat_full_last,
             gain_stat_full);

	pGainState->gain_stat_full_last 	= gain_stat_full_last;
	pGainState->gainState 		= gain_stat;
	pGainState->gainState_last 	= gain_stat_last;


    

    LOGI_ANR("%s(%d): exit!\n", __FUNCTION__, __LINE__);

    return ANR_RET_SUCCESS;
}

ANRresult_t ANRConfigSettingParam(ANRContext_t *pANRCtx, ANRParamMode_t eParamMode, int snr_mode)
{
	char snr_name[CALIBDB_NR_SHARP_NAME_LENGTH];
	char param_mode_name[CALIBDB_MAX_MODE_NAME_LENGTH];
	memset(param_mode_name, 0x00, sizeof(param_mode_name));
	memset(snr_name, 0x00, sizeof(snr_name));
	
	if(pANRCtx == NULL) {
		LOGE_ANR("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
		return ANR_RET_INVALID_PARM;
	}

	 //select param mode first
	if(eParamMode == ANR_PARAM_MODE_NORMAL){
		sprintf(param_mode_name, "%s", "normal");
	}else if(eParamMode == ANR_PARAM_MODE_HDR){
		sprintf(param_mode_name, "%s", "hdr");
	}else if(eParamMode == ANR_PARAM_MODE_GRAY){
		sprintf(param_mode_name, "%s", "gray");
	}else{
		LOGE_ANR("%s(%d): not support param mode!\n", __FUNCTION__, __LINE__);
		sprintf(param_mode_name, "%s", "normal");
	}
	

	 //then select snr mode next
	if(snr_mode == 1){
		sprintf(snr_name, "%s", "HSNR");
	}else if(snr_mode == 0){
		sprintf(snr_name, "%s", "LSNR");
	}else{
		LOGE_ANR("%s(%d): not support snr mode!\n", __FUNCTION__, __LINE__);
		sprintf(snr_name, "%s", "LSNR");
	}
	
	pANRCtx->stAuto.bayernrEn = pANRCtx->stBayernrCalib.enable;
	bayernr_config_setting_param(&pANRCtx->stAuto.stBayernrParams, &pANRCtx->stBayernrCalib, param_mode_name, snr_name);

	pANRCtx->stAuto.uvnrEn = pANRCtx->stUvnrCalib.enable;
	uvnr_config_setting_param(&pANRCtx->stAuto.stUvnrParams, &pANRCtx->stUvnrCalib, param_mode_name, snr_name);
	
	pANRCtx->stAuto.ynrEn = pANRCtx->stYnrCalib.enable;
	ynr_config_setting_param(&pANRCtx->stAuto.stYnrParams, &pANRCtx->stYnrCalib, param_mode_name, snr_name);
	
	pANRCtx->stAuto.mfnrEn = pANRCtx->stMfnrCalib.enable;
	mfnr_config_setting_param(&pANRCtx->stAuto.stMfnrParams, &pANRCtx->stMfnrCalib, param_mode_name, snr_name);
	mfnr_config_dynamic_param(&pANRCtx->stAuto.stMfnr_dynamic, &pANRCtx->stMfnrCalib, param_mode_name);
	return ANR_RET_SUCCESS; 
}

ANRresult_t ANRParamModeProcess(ANRContext_t *pANRCtx, ANRExpInfo_t *pExpInfo, ANRParamMode_t *mode){
	ANRresult_t res  = ANR_RET_SUCCESS;
	*mode = pANRCtx->eParamMode;
		
	if(pANRCtx == NULL) {
    	LOGE_ANR("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
    	return ANR_RET_INVALID_PARM;
	}

	if(pANRCtx->isGrayMode){
		*mode = ANR_PARAM_MODE_GRAY;
	}else if(pExpInfo->hdr_mode == 0){
		*mode = ANR_PARAM_MODE_NORMAL;
	}else if(pExpInfo->hdr_mode >= 1){
		*mode = ANR_PARAM_MODE_HDR;
	}else{
		*mode = ANR_PARAM_MODE_NORMAL;
	}

	return res;
}


RKAIQ_END_DECLARE


