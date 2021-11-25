
#include "rk_aiq_types_algo_agic_prvt.h"


static int
AgicFullParamsInit
(
    AgicContext_t *pAgicCtx
)
{
    memcpy(&pAgicCtx->full_param, &pAgicCtx->pCalibDb->gic, sizeof(pAgicCtx->pCalibDb->gic));
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
AgicInit
(
    AgicContext_t *pAgicCtx
)
{
    LOGI_AGIC("%s(%d): enter!\n", __FUNCTION__, __LINE__);
    if(pAgicCtx == NULL) {
        LOGE_AGIC("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_ERROR_PARAM;
    }
    memset(pAgicCtx, 0, sizeof(AgicContext_t));
    pAgicCtx->state = AGIC_STATE_INITIALIZED;

    LOGI_AGIC("%s(%d): exit!\n", __FUNCTION__, __LINE__);
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
AgicRelease
(
    AgicContext_t *pAgicCtx
)
{
    LOGI_AGIC("%s(%d): enter!\n", __FUNCTION__, __LINE__);
    if(pAgicCtx == NULL) {
        LOGE_AGIC("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_ERROR_PARAM;
    }
    AgicStop(pAgicCtx);

    LOGI_AGIC("%s(%d): exit!\n", __FUNCTION__, __LINE__);
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
AgicStart
(
    AgicContext_t *pAgicCtx
)
{
    LOGI_AGIC("%s(%d): enter!\n", __FUNCTION__, __LINE__);

    if(pAgicCtx == NULL) {
        LOGE_AGIC("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_ERROR_PARAM;
    }
    AgicFullParamsInit(pAgicCtx);
    pAgicCtx->state = AGIC_STATE_RUNNING;
    LOGI_AGIC("%s(%d): exit!\n", __FUNCTION__, __LINE__);
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
AgicStop
(
    AgicContext_t *pAgicCtx
)
{
    LOGI_AGIC("%s(%d): enter!\n", __FUNCTION__, __LINE__);

    if(pAgicCtx == NULL) {
        LOGE_AGIC("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_ERROR_PARAM;
    }
    pAgicCtx->state = AGIC_STATE_STOPPED;
    LOGI_AGIC("%s(%d): exit!\n", __FUNCTION__, __LINE__);
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn AgicPreProcess(AgicContext_t *pAgicCtx)
{
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
AgicProcess
(
    AgicContext_t *pAgicCtx,
    int ISO
)
{
    float ave1 = 0.0f, noiseSigma = 0.0f;
    short LumaPoints[] = { 0, 128, 256, 384, 512, 640, 768, 896, 1024, 1536, 2048, 2560, 3072, 3584, 4096 };
    short ratio = 0;
    short min_busy_threHi = 0, min_busy_threLo = 0;
    short min_grad_thrHi = 0, min_grad_thrLo = 0, min_grad_thr2Hi = 0, min_grad_thr2Lo = 0;
    short k_grad1Hi = 0, k_grad1Lo = 0, k_grad2Hi = 0, k_grad2Lo = 0;
    short smoothness_gbHi = 0, smoothness_gbLo = 0, smoothness_gb_weakHi = 0, smoothness_gb_weakLo = 0;
    short gb_threHi = 0, gb_threLo = 0, maxCorVHi = 0, maxCorVLo = 0, maxCutVHi = 0, maxCutVLo = 0;
    short maxCorVbothHi = 0, maxCorVbothLo = 0;
    short dark_threHi = 0, dark_threLo = 0, dark_threUpHi = 0, dark_threUpLo = 0;
    short k_grad1_darkHi = 0, k_grad1_darkLo = 0, k_grad2_darkHi = 0, k_grad2_darkLo = 0, min_grad_thr_dark1Hi = 0;
    short min_grad_thr_dark1Lo = 0, min_grad_thr_dark2Hi = 0, min_grad_thr_dark2Lo = 0;
    short noiseCoeaHi = 0, noiseCoeaLo = 0, noiseCoebHi = 0, noiseCoebLo = 0, diffClipHi = 0, diffClipLo = 0;
    float pNoiseCurveGicHi_0 = 0.0f, pNoiseCurveGicHi_1 = 0.0f;
    float pNoiseCurveGicLo_0 = 0.0f, pNoiseCurveGicLo_1 = 0.0f;
    float scaleHiHi = 0.0f, scaleHiLo = 0.0f;
    float scaleLoHi = 0.0f, scaleLoLo = 0.0f;
    float strengthHi = 0.0f, strengthLo = 0.0f;
    float strengthGlobalHi = 0.0f, strengthGlobalLo = 0.0f;
    float gValueLimitHiHi = 0.0f, gValueLimitHiLo = 0.0f;
    float gValueLimitLoHi = 0.0f, gValueLimitLoLo = 0.0f;
    int index, iso_hi = 0, iso_lo = 0;

    LOGI_AGIC("%s(%d): enter, ISO=%d\n", __FUNCTION__, __LINE__, ISO);

    if(pAgicCtx == NULL) {
        LOGE_AGIC("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_ERROR_PARAM;
    }

    if(ISO <= 50) {
        index = 0;
        ratio = 0;
    } else if (ISO > 12800) {
        index = CALIBDB_ISO_NUM - 2;
        ratio = (1 << 4);
    } else {
        for (index = 0; index < (CALIBDB_ISO_NUM - 1); index++)
        {
            iso_lo = (int)(pAgicCtx->full_param.gic_iso[index].iso);
            iso_hi = (int)(pAgicCtx->full_param.gic_iso[index + 1].iso);
            LOGD_AGIC("index=%d,  iso_lo=%d, iso_hi=%d\n", index, iso_lo, iso_hi);
            if (ISO > iso_lo && ISO <= iso_hi)
            {
                break;
            }
        }
        ratio = ((ISO - iso_lo) * (1 << 4)) / (iso_hi - iso_lo);
    }
    min_busy_threHi      = pAgicCtx->full_param.gic_iso[index + 1].min_busy_thre;
    min_busy_threLo      = pAgicCtx->full_param.gic_iso[index].min_busy_thre;
    min_grad_thrHi       = pAgicCtx->full_param.gic_iso[index + 1].min_grad_thr1;
    min_grad_thrLo       = pAgicCtx->full_param.gic_iso[index].min_grad_thr1;
    min_grad_thr2Hi      = pAgicCtx->full_param.gic_iso[index + 1].min_grad_thr2;
    min_grad_thr2Lo      = pAgicCtx->full_param.gic_iso[index].min_grad_thr2;
    k_grad1Hi            = pAgicCtx->full_param.gic_iso[index + 1].k_grad1;
    k_grad1Lo            = pAgicCtx->full_param.gic_iso[index].k_grad1;
    k_grad2Hi            = pAgicCtx->full_param.gic_iso[index + 1].k_grad2;
    k_grad2Lo            = pAgicCtx->full_param.gic_iso[index].k_grad2;
    smoothness_gbHi      = pAgicCtx->full_param.gic_iso[index + 1].smoothness_gb;
    smoothness_gbLo      = pAgicCtx->full_param.gic_iso[index].smoothness_gb;
    smoothness_gb_weakHi = pAgicCtx->full_param.gic_iso[index + 1].smoothness_gb_weak;
    smoothness_gb_weakLo = pAgicCtx->full_param.gic_iso[index].smoothness_gb_weak;
    gb_threHi            = pAgicCtx->full_param.gic_iso[index + 1].gb_thre;
    gb_threLo            = pAgicCtx->full_param.gic_iso[index].gb_thre;
    maxCorVHi            = pAgicCtx->full_param.gic_iso[index + 1].maxCorV;
    maxCorVLo            = pAgicCtx->full_param.gic_iso[index].maxCorV;
    maxCorVbothHi        = pAgicCtx->full_param.gic_iso[index + 1].maxCorVboth;
    maxCorVbothLo        = pAgicCtx->full_param.gic_iso[index].maxCorVboth;
    maxCutVHi            = pAgicCtx->full_param.gic_iso[index + 1].maxCutV;
    maxCutVLo            = pAgicCtx->full_param.gic_iso[index].maxCutV;
    dark_threHi          = pAgicCtx->full_param.gic_iso[index + 1].dark_thre;
    dark_threLo          = pAgicCtx->full_param.gic_iso[index].dark_thre;
    dark_threUpHi        = pAgicCtx->full_param.gic_iso[index + 1].dark_threHi;
    dark_threUpLo        = pAgicCtx->full_param.gic_iso[index].dark_threHi;
    k_grad1_darkHi       = pAgicCtx->full_param.gic_iso[index + 1].k_grad1_dark;
    k_grad1_darkLo       = pAgicCtx->full_param.gic_iso[index].k_grad1_dark;
    k_grad2_darkHi       = pAgicCtx->full_param.gic_iso[index + 1].k_grad2_dark;
    k_grad2_darkLo       = pAgicCtx->full_param.gic_iso[index].k_grad2_dark;
    min_grad_thr_dark1Hi = pAgicCtx->full_param.gic_iso[index + 1].min_grad_thr_dark1;
    min_grad_thr_dark1Lo = pAgicCtx->full_param.gic_iso[index].min_grad_thr_dark1;
    min_grad_thr_dark2Hi = pAgicCtx->full_param.gic_iso[index + 1].min_grad_thr_dark2;
    min_grad_thr_dark2Lo = pAgicCtx->full_param.gic_iso[index].min_grad_thr_dark2;
    pNoiseCurveGicHi_0   = pAgicCtx->full_param.gic_iso[index + 1].noiseCurve_0;
    pNoiseCurveGicLo_0   = pAgicCtx->full_param.gic_iso[index].noiseCurve_0;
    pNoiseCurveGicHi_1   = pAgicCtx->full_param.gic_iso[index + 1].noiseCurve_1;
    pNoiseCurveGicLo_1   = pAgicCtx->full_param.gic_iso[index].noiseCurve_1;
    scaleHiHi            = pAgicCtx->full_param.gic_iso[index + 1].ScaleHi;
    scaleHiLo            = pAgicCtx->full_param.gic_iso[index].ScaleHi;
    scaleLoHi            = pAgicCtx->full_param.gic_iso[index + 1].ScaleLo;
    scaleLoLo            = pAgicCtx->full_param.gic_iso[index].ScaleLo;
    strengthHi           = pAgicCtx->full_param.gic_iso[index + 1].textureStrength;
    strengthLo           = pAgicCtx->full_param.gic_iso[index].textureStrength;
    strengthGlobalHi     = pAgicCtx->full_param.gic_iso[index + 1].globalStrength;
    strengthGlobalLo     = pAgicCtx->full_param.gic_iso[index].globalStrength;
    gValueLimitHiHi      = pAgicCtx->full_param.gic_iso[index + 1].GValueLimitHi;
    gValueLimitHiLo      = pAgicCtx->full_param.gic_iso[index].GValueLimitHi;
    gValueLimitLoHi      = pAgicCtx->full_param.gic_iso[index + 1].GValueLimitLo;
    gValueLimitLoLo      = pAgicCtx->full_param.gic_iso[index].GValueLimitLo;
    noiseCoeaHi          = pAgicCtx->full_param.gic_iso[index + 1].noise_coea;
    noiseCoeaLo          = pAgicCtx->full_param.gic_iso[index].noise_coea;
    noiseCoebHi          = pAgicCtx->full_param.gic_iso[index + 1].noise_coeb;
    noiseCoebLo          = pAgicCtx->full_param.gic_iso[index].noise_coeb;
    diffClipHi           = pAgicCtx->full_param.gic_iso[index + 1].diff_clip;
    diffClipLo           = pAgicCtx->full_param.gic_iso[index].diff_clip;

    pAgicCtx->config.regminbusythre = (ratio * (min_busy_threHi - min_busy_threLo) + min_busy_threLo * (1 << 4) + (1 << 3)) >> 4;
    pAgicCtx->config.regmingradthr1 = (ratio * (min_grad_thrHi - min_grad_thrLo) + min_grad_thrLo * (1 << 4) + (1 << 3)) >> 4;
    pAgicCtx->config.regmingradthr2 = (ratio * (min_grad_thr2Hi - min_grad_thr2Lo) + min_grad_thr2Lo * (1 << 4) + (1 << 3)) >> 4;
    pAgicCtx->config.regkgrad1 = (ratio * (k_grad1Hi - k_grad1Lo) + k_grad1Lo * (1 << 4) + (1 << 3)) >> 4;
    pAgicCtx->config.regkgrad2 = (ratio * (k_grad2Hi - k_grad2Lo) + k_grad2Lo * (1 << 4) + (1 << 3)) >> 4;
    pAgicCtx->config.smoothness_gb = (ratio * (smoothness_gbHi - smoothness_gbLo) + smoothness_gbLo * (1 << 4) + (1 << 3)) >> 4;
    pAgicCtx->config.smoothness_gb_weak = (ratio * (smoothness_gb_weakHi - smoothness_gb_weakLo) + smoothness_gb_weakLo * (1 << 4) + (1 << 3)) >> 4;
    pAgicCtx->config.reggbthre = (ratio * (gb_threHi - gb_threLo) + gb_threLo * (1 << 4) + (1 << 3)) >> 4;
    pAgicCtx->config.regmaxcorv = (ratio * (maxCorVHi - maxCorVLo) + maxCorVLo * (1 << 4) + (1 << 3)) >> 4;
    pAgicCtx->config.regmaxcorvboth = (ratio * (maxCorVbothHi - maxCorVbothLo) + maxCorVbothLo * (1 << 4) + (1 << 3)) >> 4;
    pAgicCtx->config.max_cut_v = (ratio * (maxCutVHi - maxCutVLo) + maxCutVLo * (1 << 4) + (1 << 3)) >> 4;
    pAgicCtx->config.regdarkthre = (ratio * (dark_threHi - dark_threLo) + dark_threLo * (1 << 4) + (1 << 3)) >> 4;
    pAgicCtx->config.regdarktthrehi = (ratio * (dark_threUpHi - dark_threUpLo) + dark_threUpLo * (1 << 4) + (1 << 3)) >> 4;
    pAgicCtx->config.regkgrad1dark = (ratio * (k_grad1_darkHi - k_grad1_darkLo) + k_grad1_darkLo * (1 << 4) + (1 << 3)) >> 4;
    pAgicCtx->config.regkgrad2dark = (ratio * (k_grad2_darkHi - k_grad2_darkLo) + k_grad2_darkLo * (1 << 4) + (1 << 3)) >> 4;
    pAgicCtx->config.regmingradthrdark1 = (ratio * (min_grad_thr_dark1Hi - min_grad_thr_dark1Lo) + min_grad_thr_dark1Lo * (1 << 4) + (1 << 3)) >> 4;
    pAgicCtx->config.regmingradthrdark2 = (ratio * (min_grad_thr_dark2Hi - min_grad_thr_dark2Lo) + min_grad_thr_dark2Lo * (1 << 4) + (1 << 3)) >> 4;
    float ratioF = ratio / 16.0f;
    pAgicCtx->config.textureStrength = ratioF * (strengthHi - strengthLo) + strengthLo;
    pAgicCtx->config.dnhiscale = ratioF * (scaleHiHi - scaleHiLo) + scaleHiLo;
    pAgicCtx->config.dnloscale = ratioF * (scaleLoHi - scaleLoLo) + scaleLoLo;
    pAgicCtx->config.globalStrength = ratioF * (strengthGlobalHi - strengthGlobalLo) + strengthGlobalLo;
    pAgicCtx->config.gvaluelimithi = ratioF * (gValueLimitHiHi - gValueLimitHiLo) + gValueLimitHiLo;
    pAgicCtx->config.gvaluelimitlo = ratioF * (gValueLimitLoHi - gValueLimitLoLo) + gValueLimitLoLo;

    pAgicCtx->config.noise_coe_a = (ratio * (noiseCoeaHi - noiseCoeaLo) + noiseCoeaLo * (1 << 4) + (1 << 3)) >> 4;
    pAgicCtx->config.noise_coe_b = (ratio * (noiseCoebHi - noiseCoebLo) + noiseCoebLo * (1 << 4) + (1 << 3)) >> 4;
    pAgicCtx->config.diff_clip = (ratio * (diffClipHi - diffClipLo) +  diffClipLo * (1 << 4) + (1 << 3)) >> 4;
    pAgicCtx->config.noiseCurve_0 = ratioF * (pNoiseCurveGicHi_0 - pNoiseCurveGicLo_0) + pNoiseCurveGicLo_0;
    pAgicCtx->config.noiseCurve_1 =  ratioF * (pNoiseCurveGicHi_1 - pNoiseCurveGicLo_1) + pNoiseCurveGicLo_1;
    pAgicCtx->config.reglumapointsstep = 7;
    pAgicCtx->config.fusionratiohilimt1 = 0.75;

    for (int i = 0; i < 15; i++)
    {
        ave1 = LumaPoints[i];
        noiseSigma = pAgicCtx->config.noiseCurve_0 * sqrt(ave1) +  pAgicCtx->config.noiseCurve_1;
        if (noiseSigma < 0)
        {
            noiseSigma = 0;
        }
        pAgicCtx->config.lumaPoints[i] = LumaPoints[i];
        pAgicCtx->config.sigma_y[i] = noiseSigma;
    }
    short mulBit = 0;
    int bitValue = RKAIQ_GIC_BITS;
    if(bitValue > 10)
    {
        mulBit = 1 << (bitValue - 10);
    }
    else
    {
        mulBit = 1;
    }
    pAgicCtx->config.regminbusythre *= mulBit;
    pAgicCtx->config.regmingradthr1 *= mulBit;
    pAgicCtx->config.regmingradthr2 *= mulBit;
    pAgicCtx->config.smoothness_gb *= mulBit;
    pAgicCtx->config.smoothness_gb_weak *= mulBit;
    pAgicCtx->config.reggbthre *= mulBit;
    pAgicCtx->config.regmaxcorv *= mulBit;
    pAgicCtx->config.max_cut_v *= mulBit;
    pAgicCtx->config.regmaxcorvboth *= mulBit;
    pAgicCtx->config.regdarkthre *= mulBit;
    pAgicCtx->config.regdarktthrehi *= mulBit;
    pAgicCtx->config.regmingradthrdark1 *= mulBit;
    pAgicCtx->config.regmingradthrdark2 *= mulBit;
    pAgicCtx->config.edge_open = pAgicCtx->full_param.edge_en;
    pAgicCtx->config.gr_ratio = pAgicCtx->full_param.gr_ration;
    pAgicCtx->config.noise_cut_en = pAgicCtx->full_param.noise_cut_en;
    pAgicCtx->config.gic_en = pAgicCtx->full_param.gic_en;

    LOGI_AGIC("%s(%d): exit!\n", __FUNCTION__, __LINE__);
    return XCAM_RETURN_NO_ERROR;

}

//debayer get result
XCamReturn
AgicGetProcResult
(
    AgicContext_t*    pAgicCtx,
    AgicProcResult_t* pAgicResult
)
{
    LOGI_AGIC("%s(%d): enter!\n", __FUNCTION__, __LINE__);

    if(pAgicCtx == NULL) {
        LOGE_AGIC("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_ERROR_PARAM;
    }

    if(pAgicResult == NULL) {
        LOGE_AGIC("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_ERROR_PARAM;
    }

    pAgicResult->config = pAgicCtx->config;

    LOGI_AGIC("%s(%d): exit!\n", __FUNCTION__, __LINE__);
    return XCAM_RETURN_NO_ERROR;
}



