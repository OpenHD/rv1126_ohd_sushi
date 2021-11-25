#include "rk_aiq_uapi_ahdr_int.h"
#include "rk_aiq_types_ahdr_algo_prvt.h"


XCamReturn
rk_aiq_uapi_ahdr_SetAttrib
(
    RkAiqAlgoContext* ctx,
    ahdr_attrib_t attr,
    bool need_sync
)
{
    if(ctx == NULL) {
        LOGE_AHDR("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_ERROR_PARAM;
    }

    AhdrContext_t* pAhdrCtx = (AhdrContext_t*)(ctx->AhdrInstConfig.hAhdr);

    pAhdrCtx->hdrAttr.opMode = attr.opMode;
    if(attr.opMode == HDR_OpMode_SET_LEVEL)
        memcpy(&pAhdrCtx->hdrAttr.stSetLevel, &attr.stSetLevel, sizeof(FastMode_t));
    if(attr.opMode == HDR_OpMode_DarkArea)
        memcpy(&pAhdrCtx->hdrAttr.stDarkArea, &attr.stDarkArea, sizeof(DarkArea_t));
    if(attr.opMode == HDR_OpMode_Tool)
        memcpy(&pAhdrCtx->hdrAttr.stTool, &attr.stTool, sizeof(CalibDb_Ahdr_Para_t));

    if(attr.opMode == HDR_OpMode_Auto) {
        pAhdrCtx->hdrAttr.stAuto.bUpdateMge = attr.stAuto.bUpdateMge;
        memcpy(&pAhdrCtx->hdrAttr.stAuto.stMgeAuto, &attr.stAuto.stMgeAuto, sizeof(amgeAttr_t));

        pAhdrCtx->hdrAttr.stAuto.bUpdateTmo = attr.stAuto.bUpdateTmo;
        memcpy(&pAhdrCtx->hdrAttr.stAuto.stTmoAuto, &attr.stAuto.stTmoAuto, sizeof(atmoAttr_t));
    }
    else {
        pAhdrCtx->hdrAttr.stAuto.bUpdateMge = false;
        pAhdrCtx->hdrAttr.stAuto.bUpdateTmo = false;
    }

    if (attr.opMode == HDR_OpMode_MANU ) {
        pAhdrCtx->hdrAttr.stManual.bUpdateMge = attr.stManual.bUpdateMge;
        memcpy(&pAhdrCtx->hdrAttr.stManual.stMgeManual, &attr.stManual.stMgeManual, sizeof(mmgeAttr_t));

        pAhdrCtx->hdrAttr.stManual.bUpdateTmo = attr.stManual.bUpdateTmo;
        memcpy(&pAhdrCtx->hdrAttr.stManual.stTmoManual, &attr.stManual.stTmoManual, sizeof(mtmoAttr_t));
    }
    else {
        pAhdrCtx->hdrAttr.stManual.bUpdateMge = false;
        pAhdrCtx->hdrAttr.stManual.bUpdateTmo = false;
    }

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_uapi_ahdr_GetAttrib
(
    RkAiqAlgoContext*  ctx,
    ahdr_attrib_t* attr
)
{
    if(ctx == NULL || attr == NULL) {
        LOGE_AHDR("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_ERROR_PARAM;
    }

    AhdrContext_t* pAhdrCtx = (AhdrContext_t*)ctx->AhdrInstConfig.hAhdr;

    attr->opMode = pAhdrCtx->hdrAttr.opMode;

    memcpy(&attr->stAuto.stMgeAuto, &pAhdrCtx->hdrAttr.stAuto.stMgeAuto, sizeof(amgeAttr_t));
    memcpy(&attr->stAuto.stTmoAuto, &pAhdrCtx->hdrAttr.stAuto.stTmoAuto, sizeof(atmoAttr_t));
    memcpy(&attr->stManual.stMgeManual, &pAhdrCtx->hdrAttr.stManual.stMgeManual, sizeof(mmgeAttr_t));
    memcpy(&attr->stManual.stTmoManual, &pAhdrCtx->hdrAttr.stManual.stTmoManual, sizeof(mtmoAttr_t));
    memcpy(&attr->stSetLevel, &pAhdrCtx->hdrAttr.stSetLevel, sizeof(FastMode_t));
    memcpy(&attr->stDarkArea, &pAhdrCtx->hdrAttr.stDarkArea, sizeof(DarkArea_t));
    memcpy(&attr->stTool, &pAhdrCtx->hdrAttr.stTool, sizeof(CalibDb_Ahdr_Para_t));
    memcpy(&attr->CtlInfo, &pAhdrCtx->hdrAttr.CtlInfo, sizeof(CurrCtlData_t));
    memcpy(&attr->RegInfo, &pAhdrCtx->hdrAttr.RegInfo, sizeof(CurrRegData_t));

    return XCAM_RETURN_NO_ERROR;
}


