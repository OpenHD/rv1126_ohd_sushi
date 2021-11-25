#include "rk_aiq_uapi_agic_int.h"
#include "rk_aiq_types_algo_agic_prvt.h"

XCamReturn
rk_aiq_uapi_agic_SetAttrib
(
    RkAiqAlgoContext* ctx,
    agic_attrib_t attr,
    bool need_sync
)
{
    if(ctx == NULL) {
        LOGE_AGIC("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_ERROR_PARAM;
    }
    AgicContext_t* pAgicCtx = (AgicContext_t*)&ctx->agicCtx;

    pAgicCtx->full_param.edge_en = attr.edge_en;
    pAgicCtx->full_param.gr_ration = attr.gr_ration;
    pAgicCtx->full_param.noise_cut_en = attr.noise_cut_en;
    for(int i = 0; i < 9; i++)
    {
        pAgicCtx->full_param.gic_iso[i].min_busy_thre = attr.min_busy_thre[i];
        pAgicCtx->full_param.gic_iso[i].min_grad_thr1 = attr.min_grad_thr1[i];
        pAgicCtx->full_param.gic_iso[i].min_grad_thr2 = attr.min_grad_thr2[i];
        pAgicCtx->full_param.gic_iso[i].k_grad1 = attr.k_grad1[i];
        pAgicCtx->full_param.gic_iso[i].k_grad2 = attr.k_grad2[i];
        //pAgicCtx->full_param.gic_iso[i].smoothness_gb = attr.smoothness_gb[i];
        // pAgicCtx->full_param.gic_iso[i].smoothness_gb_weak = attr.smoothness_gb_weak[i];
        pAgicCtx->full_param.gic_iso[i].gb_thre = attr.gb_thre[i];
        pAgicCtx->full_param.gic_iso[i].maxCorV = attr.maxCorV[i];
        pAgicCtx->full_param.gic_iso[i].maxCorVboth = attr.maxCorVboth[i];
        //pAgicCtx->full_param.gic_iso[i].maxCutV = attr.maxCutV[i];
        pAgicCtx->full_param.gic_iso[i].dark_thre = attr.dark_thre[i];
        pAgicCtx->full_param.gic_iso[i].dark_threHi = attr.dark_threHi[i];
        pAgicCtx->full_param.gic_iso[i].k_grad1_dark = attr.k_grad1_dark[i];
        pAgicCtx->full_param.gic_iso[i].k_grad2_dark = attr.k_grad2_dark[i];
        pAgicCtx->full_param.gic_iso[i].min_grad_thr_dark1 = attr.min_grad_thr_dark1[i];
        pAgicCtx->full_param.gic_iso[i].min_grad_thr_dark2 = attr.min_grad_thr_dark2[i];
        pAgicCtx->full_param.gic_iso[i].GValueLimitLo = attr.GValueLimitLo[i];
        pAgicCtx->full_param.gic_iso[i].GValueLimitHi = attr.GValueLimitHi[i];
        pAgicCtx->full_param.gic_iso[i].textureStrength = attr.textureStrength[i];
        pAgicCtx->full_param.gic_iso[i].ScaleLo = attr.ScaleLo[i];
        pAgicCtx->full_param.gic_iso[i].ScaleHi = attr.ScaleHi[i];
        pAgicCtx->full_param.gic_iso[i].noiseCurve_0 = attr.noiseCurve_0[i];
        pAgicCtx->full_param.gic_iso[i].noiseCurve_1 = attr.noiseCurve_1[i];
        pAgicCtx->full_param.gic_iso[i].globalStrength = attr.globalStrength[i];
        pAgicCtx->full_param.gic_iso[i].noise_coea = attr.noise_coea[i];
        pAgicCtx->full_param.gic_iso[i].noise_coeb = attr.noise_coeb[i];
        pAgicCtx->full_param.gic_iso[i].diff_clip = attr.diff_clip[i];
    }
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_uapi_agic_GetAttrib
(
    RkAiqAlgoContext*  ctx,
    agic_attrib_t* attr
)
{
    if(ctx == NULL || attr == NULL) {
        LOGE_AGIC("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_ERROR_PARAM;
    }

    AgicContext_t* pAgicCtx = (AgicContext_t*)&ctx->agicCtx;

    attr->edge_en = pAgicCtx->full_param.edge_en;
    attr->gr_ration = pAgicCtx->full_param.gr_ration;
    attr->noise_cut_en = pAgicCtx->full_param.noise_cut_en;
    for(int i = 0; i < 9; i++)
    {
        attr->min_busy_thre[i] = pAgicCtx->full_param.gic_iso[i].min_busy_thre;
        attr->min_grad_thr1[i] = pAgicCtx->full_param.gic_iso[i].min_grad_thr1;
        attr->min_grad_thr2[i] = pAgicCtx->full_param.gic_iso[i].min_grad_thr2;
        attr->k_grad1[i] = pAgicCtx->full_param.gic_iso[i].k_grad1;
        attr->k_grad2[i] = pAgicCtx->full_param.gic_iso[i].k_grad2;
        //  attr->smoothness_gb[i] = pAgicCtx->full_param.gic_iso[i].smoothness_gb;
        //   attr->smoothness_gb_weak[i] = pAgicCtx->full_param.gic_iso[i].smoothness_gb_weak;
        attr->gb_thre[i] = pAgicCtx->full_param.gic_iso[i].gb_thre;
        attr->maxCorV[i] = pAgicCtx->full_param.gic_iso[i].maxCorV;
        attr->maxCorVboth[i] = pAgicCtx->full_param.gic_iso[i].maxCorVboth;
        //  attr->maxCutV[i] = pAgicCtx->full_param.gic_iso[i].maxCutV;
        attr->dark_thre[i] = pAgicCtx->full_param.gic_iso[i].dark_thre;
        attr->dark_threHi[i] = pAgicCtx->full_param.gic_iso[i].dark_threHi;
        attr->k_grad1_dark[i] = pAgicCtx->full_param.gic_iso[i].k_grad1_dark;
        attr->k_grad2_dark[i] = pAgicCtx->full_param.gic_iso[i].k_grad2_dark;
        attr->min_grad_thr_dark1[i] = pAgicCtx->full_param.gic_iso[i].min_grad_thr_dark1;
        attr->min_grad_thr_dark2[i] = pAgicCtx->full_param.gic_iso[i].min_grad_thr_dark2;
        attr->GValueLimitLo[i] = pAgicCtx->full_param.gic_iso[i].GValueLimitLo;
        attr->GValueLimitHi[i] = pAgicCtx->full_param.gic_iso[i].GValueLimitHi;
        attr->textureStrength[i] = pAgicCtx->full_param.gic_iso[i].textureStrength;
        attr->ScaleLo[i] = pAgicCtx->full_param.gic_iso[i].ScaleLo;
        attr->ScaleHi[i] = pAgicCtx->full_param.gic_iso[i].ScaleHi;
        attr->noiseCurve_0[i] = pAgicCtx->full_param.gic_iso[i].noiseCurve_0;
        attr->noiseCurve_1[i] = pAgicCtx->full_param.gic_iso[i].noiseCurve_1;
        attr->globalStrength[i] = pAgicCtx->full_param.gic_iso[i].globalStrength;
        attr->noise_coea[i] = pAgicCtx->full_param.gic_iso[i].noise_coea;
        attr->noise_coeb[i] = pAgicCtx->full_param.gic_iso[i].noise_coeb;
        attr->diff_clip[i] = pAgicCtx->full_param.gic_iso[i].diff_clip;
    }

    return XCAM_RETURN_NO_ERROR;
}

