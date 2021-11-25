#include "rk_aiq_uapi_awdr_int.h"
#include "rk_aiq_types_awdr_algo_prvt.h"
#include "xcam_log.h"

XCamReturn
rk_aiq_uapi_awdr_SetAttrib(RkAiqAlgoContext *ctx,
                           awdr_attrib_t attr,
                           bool need_sync)
{
    LOG1_AWDR("%s:Enter!\n", __FUNCTION__);
    AwdrHandle_t  AwdrHandle = (AwdrHandle_t )ctx;

    AwdrHandle->wdrAttr.mode = attr.mode;
    if(attr.mode == WDR_OPMODE_AUTO)
        memcpy(&AwdrHandle->wdrAttr.stAuto, &attr.stAuto, sizeof(CalibDb_Awdr_Para_t));
    else if(attr.mode == WDR_OPMODE_MANU)
        memcpy(&AwdrHandle->wdrAttr.stManu, &attr.stManu, sizeof(wdrAttrStManu_t));
    else {
        LOGE_AWDR("invalid mode!");
    }

    LOG1_AWDR("%s:Exit!\n", __FUNCTION__);
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_uapi_awdr_GetAttrib(RkAiqAlgoContext *ctx, awdr_attrib_t *attr)
{
    LOG1_AWDR("%s:Enter!\n", __FUNCTION__);
    AwdrHandle_t  AwdrHandle = (AwdrHandle_t )ctx;

    attr->mode = AwdrHandle->wdrAttr.mode;
    memcpy(&attr->stAuto, &AwdrHandle->wdrAttr.stAuto, sizeof(CalibDb_Awdr_Para_t));
    memcpy(&attr->stManu, &AwdrHandle->wdrAttr.stManu, sizeof(wdrAttrStManu_t));
    memcpy(&attr->CtlInfo, &AwdrHandle->wdrAttr.CtlInfo, sizeof(WdrCurrCtlData_t));

    LOG1_AWDR("%s:Exit!\n", __FUNCTION__);
    return XCAM_RETURN_NO_ERROR;
}

