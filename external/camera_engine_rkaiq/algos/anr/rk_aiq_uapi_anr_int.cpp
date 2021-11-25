#include "rk_aiq_uapi_anr_int.h"
#include "anr/rk_aiq_types_anr_algo_prvt.h"

#define NR_STRENGTH_MAX_PERCENT (50.0)
#define NR_LUMA_TF_STRENGTH_MAX_PERCENT NR_STRENGTH_MAX_PERCENT
#define NR_LUMA_SF_STRENGTH_MAX_PERCENT (100.0)
#define NR_CHROMA_TF_STRENGTH_MAX_PERCENT NR_STRENGTH_MAX_PERCENT
#define NR_CHROMA_SF_STRENGTH_MAX_PERCENT NR_STRENGTH_MAX_PERCENT
#define NR_RAWNR_SF_STRENGTH_MAX_PERCENT (80.0)

XCamReturn
rk_aiq_uapi_anr_SetAttrib(RkAiqAlgoContext *ctx,
                          rk_aiq_nr_attrib_t *attr,
                          bool need_sync)
{

    ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;

    pAnrCtx->eMode = attr->eMode;
    pAnrCtx->stAuto = attr->stAuto;
    pAnrCtx->stManual = attr->stManual;

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_uapi_anr_GetAttrib(const RkAiqAlgoContext *ctx,
                          rk_aiq_nr_attrib_t *attr)
{

    ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;

    attr->eMode = pAnrCtx->eMode;
    memcpy(&attr->stAuto, &pAnrCtx->stAuto, sizeof(ANR_Auto_Attr_t));
    memcpy(&attr->stManual, &pAnrCtx->stManual, sizeof(ANR_Manual_Attr_t));

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_uapi_anr_SetIQPara(RkAiqAlgoContext *ctx,
                          rk_aiq_nr_IQPara_t *pPara,
                          bool need_sync)
{

    ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;

	if(pPara->module_bits & (1 << ANR_MODULE_BAYERNR)){
		//pAnrCtx->stBayernrCalib = pPara->stBayernrPara;
		pAnrCtx->stBayernrCalib.enable = pPara->stBayernrPara.enable;
		memcpy(pAnrCtx->stBayernrCalib.version, pPara->stBayernrPara.version, sizeof(pPara->stBayernrPara.version));
		for(int i=0; i<pAnrCtx->stBayernrCalib.mode_num; i++){
			pAnrCtx->stBayernrCalib.mode_cell[i] = pPara->stBayernrPara.mode_cell[i];
		}
		pAnrCtx->isIQParaUpdate = true;
	}

	if(pPara->module_bits & (1 << ANR_MODULE_MFNR)){
		//pAnrCtx->stMfnrCalib = pPara->stMfnrPara;
		pAnrCtx->stMfnrCalib.enable = pPara->stMfnrPara.enable;
		memcpy(pAnrCtx->stMfnrCalib.version, pPara->stMfnrPara.version, sizeof(pPara->stMfnrPara.version));
		pAnrCtx->stMfnrCalib.local_gain_en = pPara->stMfnrPara.local_gain_en;
		pAnrCtx->stMfnrCalib.motion_detect_en = pPara->stMfnrPara.motion_detect_en;
		pAnrCtx->stMfnrCalib.mode_3to1 = pPara->stMfnrPara.mode_3to1;
		pAnrCtx->stMfnrCalib.max_level = pPara->stMfnrPara.max_level;
		pAnrCtx->stMfnrCalib.max_level_uv = pPara->stMfnrPara.max_level_uv;
		pAnrCtx->stMfnrCalib.back_ref_num = pPara->stMfnrPara.back_ref_num;
		for(int i=0; i<4; i++){
			pAnrCtx->stMfnrCalib.uv_ratio[i] = pPara->stMfnrPara.uv_ratio[i];
		}
		for(int i=0; i<pAnrCtx->stMfnrCalib.mode_num; i++){
			pAnrCtx->stMfnrCalib.mode_cell[i] = pPara->stMfnrPara.mode_cell[i];
		}
		pAnrCtx->isIQParaUpdate = true;
	}

	if(pPara->module_bits & (1 << ANR_MODULE_UVNR)){
		//pAnrCtx->stUvnrCalib = pPara->stUvnrPara;
		pAnrCtx->stUvnrCalib.enable = pPara->stUvnrPara.enable;
		memcpy(pAnrCtx->stUvnrCalib.version, pPara->stUvnrPara.version, sizeof(pPara->stUvnrPara.version));
		for(int i=0; i<pAnrCtx->stUvnrCalib.mode_num; i++){
			pAnrCtx->stUvnrCalib.mode_cell[i] = pPara->stUvnrPara.mode_cell[i];
		}
		pAnrCtx->isIQParaUpdate = true;
	}

	if(pPara->module_bits & (1 << ANR_MODULE_YNR)){
		//pAnrCtx->stYnrCalib = pPara->stYnrPara;
		pAnrCtx->stYnrCalib.enable = pPara->stYnrPara.enable;
		memcpy(pAnrCtx->stYnrCalib.version, pPara->stYnrPara.version, sizeof(pPara->stYnrPara.version));
		for(int i=0; i<pAnrCtx->stYnrCalib.mode_num; i++){
			pAnrCtx->stYnrCalib.mode_cell[i] = pPara->stYnrPara.mode_cell[i];
		}
		pAnrCtx->isIQParaUpdate = true;
	}

    return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_uapi_anr_GetIQPara(RkAiqAlgoContext *ctx,
                          rk_aiq_nr_IQPara_t *pPara)
{

	ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;

	//pPara->stBayernrPara = pAnrCtx->stBayernrCalib;
	memset(&pPara->stBayernrPara, 0x00, sizeof(CalibDb_BayerNr_t));
	pPara->stBayernrPara.enable = pAnrCtx->stBayernrCalib.enable;
	memcpy(pPara->stBayernrPara.version, pAnrCtx->stBayernrCalib.version, sizeof(pPara->stBayernrPara.version));
	for(int i=0; i<pAnrCtx->stBayernrCalib.mode_num; i++){
		pPara->stBayernrPara.mode_cell[i] = pAnrCtx->stBayernrCalib.mode_cell[i];
	}
	
	//pPara->stMfnrPara = pAnrCtx->stMfnrCalib;
	memset(&pPara->stMfnrPara, 0x00, sizeof(CalibDb_MFNR_t));
	pPara->stMfnrPara.enable = pAnrCtx->stMfnrCalib.enable;
	memcpy(pPara->stMfnrPara.version, pAnrCtx->stMfnrCalib.version, sizeof(pPara->stMfnrPara.version));
	pPara->stMfnrPara.local_gain_en = pAnrCtx->stMfnrCalib.local_gain_en;
	pPara->stMfnrPara.motion_detect_en = pAnrCtx->stMfnrCalib.motion_detect_en;
	pPara->stMfnrPara.mode_3to1 = pAnrCtx->stMfnrCalib.mode_3to1;
	pPara->stMfnrPara.max_level = pAnrCtx->stMfnrCalib.max_level;
	pPara->stMfnrPara.max_level_uv = pAnrCtx->stMfnrCalib.max_level_uv;
	pPara->stMfnrPara.back_ref_num = pAnrCtx->stMfnrCalib.back_ref_num;
	for(int i=0; i<4; i++){
		pPara->stMfnrPara.uv_ratio[i] = pAnrCtx->stMfnrCalib.uv_ratio[i];
	}
	for(int i=0; i<pAnrCtx->stMfnrCalib.mode_num; i++){
		pPara->stMfnrPara.mode_cell[i] = pAnrCtx->stMfnrCalib.mode_cell[i];
	}

	
	//pPara->stUvnrPara = pAnrCtx->stUvnrCalib;
	memset(&pPara->stUvnrPara, 0x00, sizeof(CalibDb_UVNR_t));
	pPara->stUvnrPara.enable = pAnrCtx->stUvnrCalib.enable;
	memcpy(pPara->stUvnrPara.version, pAnrCtx->stUvnrCalib.version, sizeof(pPara->stUvnrPara.version));
	for(int i=0; i<pAnrCtx->stUvnrCalib.mode_num; i++){
		pPara->stUvnrPara.mode_cell[i] = pAnrCtx->stUvnrCalib.mode_cell[i];
	}

	
	//pPara->stYnrPara = pAnrCtx->stYnrCalib;
	memset(&pPara->stYnrPara, 0x00, sizeof(CalibDb_YNR_t));
	pPara->stYnrPara.enable = pAnrCtx->stYnrCalib.enable;
	memcpy(pPara->stYnrPara.version, pAnrCtx->stYnrCalib.version, sizeof(pPara->stYnrPara.version));
	for(int i=0; i<pAnrCtx->stYnrCalib.mode_num; i++){
		pPara->stYnrPara.mode_cell[i] = pAnrCtx->stYnrCalib.mode_cell[i];
	}
	
    return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_uapi_anr_SetLumaSFStrength(const RkAiqAlgoContext *ctx,
                          float fPercent)
{
	ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;

	float fStrength = 1.0f;
	float fMax = NR_LUMA_SF_STRENGTH_MAX_PERCENT;

    
	if(fPercent <= 0.5){
		fStrength =  fPercent /0.5;
	}else{
		fStrength = (fPercent - 0.5)*(fMax - 1)*2 + 1;
	}

	if(fStrength > 1){
		pAnrCtx->fRawnr_SF_Strength = fStrength / 2.0;
		pAnrCtx->fLuma_SF_Strength = 1;
	}else{
		pAnrCtx->fRawnr_SF_Strength = fStrength;
		pAnrCtx->fLuma_SF_Strength = fStrength;
	}

	return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_uapi_anr_SetLumaTFStrength(const RkAiqAlgoContext *ctx,
                          float fPercent)
{
	ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;
	
	float fStrength = 1.0;
	float fMax = NR_LUMA_TF_STRENGTH_MAX_PERCENT;

	if(fPercent <= 0.5){
		fStrength =  fPercent /0.5;
	}else{
		fStrength = (fPercent - 0.5)*(fMax - 1) * 2 + 1;
	}

	pAnrCtx->fLuma_TF_Strength = fStrength;

	return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_uapi_anr_GetLumaSFStrength(const RkAiqAlgoContext *ctx,
                          float *pPercent)
{
	ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;

	float fStrength = 1.0f;
	float fMax = NR_LUMA_SF_STRENGTH_MAX_PERCENT;

	
	fStrength = pAnrCtx->fLuma_SF_Strength;

	if(fStrength <= 1){
		*pPercent = fStrength * 0.5;
	}else{
		*pPercent = (fStrength - 1)/((fMax - 1) * 2)+ 0.5;
	}

	return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_uapi_anr_GetLumaTFStrength(const RkAiqAlgoContext *ctx,
                          float *pPercent)
{
	ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;
	
	float fStrength = 1.0;
	float fMax = NR_LUMA_TF_STRENGTH_MAX_PERCENT;

	fStrength = pAnrCtx->fLuma_TF_Strength;

	if(fStrength <= 1){
		*pPercent = fStrength * 0.5;
	}else{
		*pPercent = (fStrength - 1)/((fMax - 1) * 2)+ 0.5;
	}

	return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_uapi_anr_SetChromaSFStrength(const RkAiqAlgoContext *ctx,
                          float fPercent)
{
	ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;

	float fStrength = 1.0f;
	float fMax = NR_CHROMA_SF_STRENGTH_MAX_PERCENT;

	if(fPercent <= 0.5){
		fStrength =  fPercent /0.5;
	}else{
		fStrength = (fPercent - 0.5)*(fMax - 1) * 2 + 1;
	}

	pAnrCtx->fChroma_SF_Strength = fStrength;

	return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_uapi_anr_SetChromaTFStrength(const RkAiqAlgoContext *ctx,
                          float fPercent)
{
	ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;
	
	float fStrength = 1.0;
	float fMax = NR_CHROMA_TF_STRENGTH_MAX_PERCENT;

	if(fPercent <= 0.5){
		fStrength =  fPercent /0.5;
	}else{
		fStrength = (fPercent - 0.5)*(fMax - 1) * 2 + 1;
	}

	pAnrCtx->fChroma_TF_Strength = fStrength;

	return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_uapi_anr_GetChromaSFStrength(const RkAiqAlgoContext *ctx,
                          float *pPercent)
{
	ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;

	float fStrength = 1.0f;
	float fMax = NR_CHROMA_SF_STRENGTH_MAX_PERCENT;
	
	fStrength = pAnrCtx->fChroma_SF_Strength;
	
	
	if(fStrength <= 1){
		*pPercent = fStrength * 0.5;
	}else{
		*pPercent = (fStrength - 1)/((fMax - 1) * 2)+ 0.5;
	}
	

	return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_uapi_anr_GetChromaTFStrength(const RkAiqAlgoContext *ctx,
                          float *pPercent)
{
	ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;
	
	float fStrength = 1.0;
	float fMax = NR_CHROMA_TF_STRENGTH_MAX_PERCENT;

	fStrength = pAnrCtx->fChroma_TF_Strength;

	if(fStrength <= 1){
		*pPercent = fStrength * 0.5;
	}else{
		*pPercent = (fStrength - 1)/((fMax - 1) * 2)+ 0.5;
	}

	return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_uapi_anr_SetRawnrSFStrength(const RkAiqAlgoContext *ctx,
                          float fPercent)
{
	ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;
	
	float fStrength = 1.0;
	float fMax = NR_RAWNR_SF_STRENGTH_MAX_PERCENT;

	if(fPercent <= 0.5){
		fStrength =  fPercent /0.5;
	}else{
		fStrength = (fPercent - 0.5)*(fMax - 1) * 2 + 1;
	}

	pAnrCtx->fRawnr_SF_Strength = fStrength;

	return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_uapi_anr_GetRawnrSFStrength(const RkAiqAlgoContext *ctx,
                          float *pPercent)
{
	ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;

	float fStrength = 1.0f;
	float fMax = NR_RAWNR_SF_STRENGTH_MAX_PERCENT;
	
	fStrength = pAnrCtx->fRawnr_SF_Strength;
	
	
	if(fStrength <= 1){
		*pPercent = fStrength * 0.5;
	}else{
		*pPercent = (fStrength - 1)/((fMax - 1) * 2)+ 0.5;
	}
	

	return XCAM_RETURN_NO_ERROR;
}


