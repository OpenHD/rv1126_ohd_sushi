#include "rk_aiq_uapi_adehaze_int.h"
#include "rk_aiq_types_adehaze_algo_prvt.h"
#include "xcam_log.h"

XCamReturn
rk_aiq_uapi_adehaze_SetAttrib(RkAiqAlgoContext *ctx,
                              adehaze_sw_t attr,
                              bool need_sync)
{
    AdehazeHandle_t * AdehazeHandle = (AdehazeHandle_t *)ctx;
    LOGD_ADEHAZE("%s: setMode:%d", __func__, attr.mode);
    AdehazeHandle->AdehazeAtrr.byPass = attr.byPass;
    AdehazeHandle->AdehazeAtrr.mode = attr.mode;
    if(attr.mode == RK_AIQ_DEHAZE_MODE_AUTO)
        memcpy(&AdehazeHandle->AdehazeAtrr.stAuto, &attr.stAuto, sizeof(CalibDb_Dehaze_t));
    else if(attr.mode == RK_AIQ_DEHAZE_MODE_MANUAL)
        memcpy(&AdehazeHandle->AdehazeAtrr.stManual, &attr.stManual, sizeof(rk_aiq_dehaze_M_attrib_t));
    else if(attr.mode == RK_AIQ_DEHAZE_MODE_OFF)
        memcpy(&AdehazeHandle->AdehazeAtrr.stEnhance, &attr.stEnhance, sizeof(rk_aiq_dehaze_enhance_t));
    else if(attr.mode == RK_AIQ_DEHAZE_MODE_TOOL)
        memcpy(&AdehazeHandle->AdehazeAtrr.stTool, &attr.stTool, sizeof(CalibDb_Dehaze_t));
    else {
        LOGE_ADEHAZE("invalid mode!");
    }
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_uapi_adehaze_GetAttrib(RkAiqAlgoContext *ctx, adehaze_sw_t *attr)
{
    AdehazeHandle_t * AdehazeHandle = (AdehazeHandle_t *)ctx;
	
	attr->byPass = AdehazeHandle->AdehazeAtrr.byPass;
    attr->mode = AdehazeHandle->AdehazeAtrr.mode;
    //if (AdehazeHandle->AdehazeAtrr.mode == RK_AIQ_DEHAZE_MODE_AUTO)
        memcpy(&attr->stAuto, &AdehazeHandle->AdehazeAtrr.stAuto, sizeof(CalibDb_Dehaze_t));
    //else if (AdehazeHandle->AdehazeAtrr.mode == RK_AIQ_DEHAZE_MODE_MANUAL)
        memcpy(&attr->stManual, &AdehazeHandle->AdehazeAtrr.stManual, sizeof(rk_aiq_dehaze_M_attrib_t));
    //else if (AdehazeHandle->AdehazeAtrr.mode == RK_AIQ_DEHAZE_MODE_OFF)
        memcpy(&attr->stEnhance, &AdehazeHandle->AdehazeAtrr.stEnhance, sizeof(rk_aiq_dehaze_enhance_t));
    //else if (AdehazeHandle->AdehazeAtrr.mode == RK_AIQ_DEHAZE_MODE_TOOL)
        memcpy(&attr->stTool, &AdehazeHandle->AdehazeAtrr.stTool, sizeof(CalibDb_Dehaze_t));
	
    return XCAM_RETURN_NO_ERROR;
}

