#include "rk_aiq_uapi_ablc_int.h"
#include "ablc/rk_aiq_types_ablc_algo_prvt.h"

XCamReturn
rk_aiq_uapi_ablc_SetAttrib(RkAiqAlgoContext *ctx,
                           rk_aiq_blc_attrib_t *attr,
                           bool need_sync)
{

    AblcContext_t* pAblcCtx = (AblcContext_t*)ctx;
    pAblcCtx->eMode = attr->eMode;
    pAblcCtx->stAuto = attr->stAuto;
    pAblcCtx->stManual = attr->stManual;

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_uapi_ablc_GetAttrib(const RkAiqAlgoContext *ctx,
                           rk_aiq_blc_attrib_t *attr)
{

    AblcContext_t* pAblcCtx = (AblcContext_t*)ctx;

    attr->eMode = pAblcCtx->eMode;
    memcpy(&attr->stAuto, &pAblcCtx->stAuto, sizeof(AblcAutoAttr_t));
    memcpy(&attr->stManual, &pAblcCtx->stManual, sizeof(AblcManualAttr_t));

    return XCAM_RETURN_NO_ERROR;
}



