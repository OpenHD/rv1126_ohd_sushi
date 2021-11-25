#include "rk_aiq_uapi_adegamma_int.h"
#include "adegamma/rk_aiq_types_adegamma_algo_prvt.h"

XCamReturn
rk_aiq_uapi_adegamma_SetAttrib(RkAiqAlgoContext *ctx,
                               rk_aiq_degamma_attrib_t attr,
                               bool need_sync)
{
    AdegammaHandle_t *degamma_handle = (AdegammaHandle_t *)ctx;
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    if(attr.mode == RK_AIQ_DEGAMMA_MODE_MANUAL)
        memcpy(&degamma_handle->adegammaAttr.stManual, &attr.stManual, sizeof(Adegamma_api_manual_t));
    if(attr.mode == RK_AIQ_DEGAMMA_MODE_TOOL)
        memcpy(&degamma_handle->adegammaAttr.stTool, &attr.stTool, sizeof(CalibDb_Degamma_t));

    return ret;
}

XCamReturn
rk_aiq_uapi_adegamma_GetAttrib(const RkAiqAlgoContext *ctx,
                               rk_aiq_degamma_attrib_t *attr)
{

    AdegammaHandle_t* degamma_handle = (AdegammaHandle_t*)ctx;

    memcpy(attr, &degamma_handle->adegammaAttr, sizeof(rk_aiq_degamma_attr_t));

    return XCAM_RETURN_NO_ERROR;
}




