/******************************************************************************
 *
 * Copyright 2019, Fuzhou Rockchip Electronics Co.Ltd . All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Fuzhou Rockchip Electronics Co.Ltd .
 *
 *
 *****************************************************************************/

#include "base/xcam_common.h"
#include "rk_aiq_user_api_ae.h"
#include "RkAiqHandleInt.h"

RKAIQ_BEGIN_DECLARE

#ifdef RK_SIMULATOR_HW
#define CHECK_USER_API_ENABLE
#endif

XCamReturn rk_aiq_user_api_ae_setExpSwAttr
(
    const rk_aiq_sys_ctx_t* sys_ctx,
    const Uapi_ExpSwAttr_t expSwAttr
) {
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    CHECK_USER_API_ENABLE(RK_AIQ_ALGO_TYPE_AE);
    RKAIQ_API_SMART_LOCK(sys_ctx);

    RkAiqAeHandleInt* algo_handle =
        algoHandle<RkAiqAeHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_AE);

    if (algo_handle) {
        return algo_handle->setExpSwAttr(expSwAttr);
    }

    return (ret);

}
XCamReturn rk_aiq_user_api_ae_getExpSwAttr
(
    const rk_aiq_sys_ctx_t* sys_ctx,
    Uapi_ExpSwAttr_t*        pExpSwAttr
) {
    RKAIQ_API_SMART_LOCK(sys_ctx);
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAeHandleInt* algo_handle =
        algoHandle<RkAiqAeHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_AE);

    if (algo_handle) {
        return algo_handle->getExpSwAttr(pExpSwAttr);
    }

    return (ret);
}
XCamReturn rk_aiq_user_api_ae_setLinAeDayRouteAttr
(
    const rk_aiq_sys_ctx_t* sys_ctx,
    const Uapi_LinAeRouteAttr_t linAeDayRouteAttr
) {
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    CHECK_USER_API_ENABLE(RK_AIQ_ALGO_TYPE_AE);
    RKAIQ_API_SMART_LOCK(sys_ctx);

    RkAiqAeHandleInt* algo_handle =
        algoHandle<RkAiqAeHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_AE);

    if (algo_handle) {
        return algo_handle->setLinAeDayRouteAttr(linAeDayRouteAttr);
    }

    return(ret);
}
XCamReturn rk_aiq_user_api_ae_getLinAeDayRouteAttr
(
    const rk_aiq_sys_ctx_t* sys_ctx,
    Uapi_LinAeRouteAttr_t* pLinAeDayRouteAttr
) {
    RKAIQ_API_SMART_LOCK(sys_ctx);
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAeHandleInt* algo_handle =
        algoHandle<RkAiqAeHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_AE);

    if (algo_handle) {
        return algo_handle->getLinAeDayRouteAttr(pLinAeDayRouteAttr);
    }

    return(ret);

}
XCamReturn rk_aiq_user_api_ae_setHdrAeDayRouteAttr
(
    const rk_aiq_sys_ctx_t* sys_ctx,
    const Uapi_HdrAeRouteAttr_t hdrAeDayRouteAttr
) {
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    CHECK_USER_API_ENABLE(RK_AIQ_ALGO_TYPE_AE);
    RKAIQ_API_SMART_LOCK(sys_ctx);

    RkAiqAeHandleInt* algo_handle =
        algoHandle<RkAiqAeHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_AE);

    if (algo_handle) {
        return algo_handle->setHdrAeDayRouteAttr(hdrAeDayRouteAttr);
    }

    return(ret);

}
XCamReturn rk_aiq_user_api_ae_getHdrAeDayRouteAttr
(
    const rk_aiq_sys_ctx_t* sys_ctx,
    Uapi_HdrAeRouteAttr_t* pHdrAeDayRouteAttr
) {
    RKAIQ_API_SMART_LOCK(sys_ctx);
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAeHandleInt* algo_handle =
        algoHandle<RkAiqAeHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_AE);

    if (algo_handle) {
        return algo_handle->getHdrAeDayRouteAttr(pHdrAeDayRouteAttr);
    }

    return(ret);

}
XCamReturn rk_aiq_user_api_ae_setLinAeNightRouteAttr
(
    const rk_aiq_sys_ctx_t* sys_ctx,
    const Uapi_LinAeRouteAttr_t linAeNightRouteAttr
) {
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    CHECK_USER_API_ENABLE(RK_AIQ_ALGO_TYPE_AE);
    RKAIQ_API_SMART_LOCK(sys_ctx);

    RkAiqAeHandleInt* algo_handle =
        algoHandle<RkAiqAeHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_AE);

    if (algo_handle) {
        return algo_handle->setLinAeNightRouteAttr(linAeNightRouteAttr);
    }

    return(ret);
}
XCamReturn rk_aiq_user_api_ae_getLinAeNightRouteAttr
(
    const rk_aiq_sys_ctx_t* sys_ctx,
    Uapi_LinAeRouteAttr_t* pLinAeNightRouteAttr
) {
    RKAIQ_API_SMART_LOCK(sys_ctx);
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAeHandleInt* algo_handle =
        algoHandle<RkAiqAeHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_AE);

    if (algo_handle) {
        return algo_handle->getLinAeNightRouteAttr(pLinAeNightRouteAttr);
    }

    return(ret);

}
XCamReturn rk_aiq_user_api_ae_setHdrAeNightRouteAttr
(
    const rk_aiq_sys_ctx_t* sys_ctx,
    const Uapi_HdrAeRouteAttr_t hdrAeNightRouteAttr
) {
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    CHECK_USER_API_ENABLE(RK_AIQ_ALGO_TYPE_AE);
    RKAIQ_API_SMART_LOCK(sys_ctx);

    RkAiqAeHandleInt* algo_handle =
        algoHandle<RkAiqAeHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_AE);

    if (algo_handle) {
        return algo_handle->setHdrAeNightRouteAttr(hdrAeNightRouteAttr);
    }

    return(ret);

}
XCamReturn rk_aiq_user_api_ae_getHdrAeNightRouteAttr
(
    const rk_aiq_sys_ctx_t* sys_ctx,
    Uapi_HdrAeRouteAttr_t* pHdrAeNightRouteAttr
) {
    RKAIQ_API_SMART_LOCK(sys_ctx);
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAeHandleInt* algo_handle =
        algoHandle<RkAiqAeHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_AE);

    if (algo_handle) {
        return algo_handle->getHdrAeNightRouteAttr(pHdrAeNightRouteAttr);
    }

    return(ret);

}

XCamReturn rk_aiq_user_api_ae_queryExpResInfo
(
    const rk_aiq_sys_ctx_t* sys_ctx,
    Uapi_ExpQueryInfo_t* pExpResInfo
) {
    RKAIQ_API_SMART_LOCK(sys_ctx);
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAeHandleInt* algo_handle =
        algoHandle<RkAiqAeHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_AE);

    if (algo_handle) {
        return algo_handle->queryExpInfo(pExpResInfo);
    }

    return(ret);

}

XCamReturn rk_aiq_user_api_ae_setLinExpAttr
(
    const rk_aiq_sys_ctx_t* sys_ctx,
    const Uapi_LinExpAttr_t linExpAttr
) {
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    CHECK_USER_API_ENABLE(RK_AIQ_ALGO_TYPE_AE);
    RKAIQ_API_SMART_LOCK(sys_ctx);

    RkAiqAeHandleInt* algo_handle =
        algoHandle<RkAiqAeHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_AE);

    if (algo_handle) {
        return algo_handle->setLinExpAttr(linExpAttr);
    }

    return(ret);

}
XCamReturn rk_aiq_user_api_ae_getLinExpAttr
(
    const rk_aiq_sys_ctx_t* sys_ctx,
    Uapi_LinExpAttr_t* pLinExpAttr
) {
    RKAIQ_API_SMART_LOCK(sys_ctx);
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAeHandleInt* algo_handle =
        algoHandle<RkAiqAeHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_AE);

    if (algo_handle) {
        return algo_handle->getLinExpAttr(pLinExpAttr);
    }

    return(ret);

}
XCamReturn rk_aiq_user_api_ae_setHdrExpAttr
(
    const rk_aiq_sys_ctx_t* sys_ctx,
    const Uapi_HdrExpAttr_t hdrExpAttr
) {
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    CHECK_USER_API_ENABLE(RK_AIQ_ALGO_TYPE_AE);
    RKAIQ_API_SMART_LOCK(sys_ctx);

    RkAiqAeHandleInt* algo_handle =
        algoHandle<RkAiqAeHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_AE);

    if (algo_handle) {
        return algo_handle->setHdrExpAttr(hdrExpAttr);
    }

    return(ret);

}
XCamReturn rk_aiq_user_api_ae_getHdrExpAttr
(
    const rk_aiq_sys_ctx_t* sys_ctx,
    Uapi_HdrExpAttr_t* pHdrExpAttr
) {
    RKAIQ_API_SMART_LOCK(sys_ctx);
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAeHandleInt* algo_handle =
        algoHandle<RkAiqAeHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_AE);

    if (algo_handle) {
        return algo_handle->getHdrExpAttr(pHdrExpAttr);
    }

    return(ret);

}

XCamReturn rk_aiq_user_api_ae_setExpWinAttr
(
    const rk_aiq_sys_ctx_t* sys_ctx,
    const Uapi_ExpWin_t ExpWin
) {
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    CHECK_USER_API_ENABLE(RK_AIQ_ALGO_TYPE_AE);
    RKAIQ_API_SMART_LOCK(sys_ctx);

    RkAiqAeHandleInt* algo_handle =
        algoHandle<RkAiqAeHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_AE);

    if (algo_handle) {
        return algo_handle->setExpWinAttr(ExpWin);
    }

    return(ret);

}
XCamReturn rk_aiq_user_api_ae_getExpWinAttr
(
    const rk_aiq_sys_ctx_t* sys_ctx,
    Uapi_ExpWin_t* pExpWin
) {
    RKAIQ_API_SMART_LOCK(sys_ctx);
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAeHandleInt* algo_handle =
        algoHandle<RkAiqAeHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_AE);

    if (algo_handle) {
        return algo_handle->getExpWinAttr(pExpWin);
    }

    return(ret);

}

RKAIQ_END_DECLARE

