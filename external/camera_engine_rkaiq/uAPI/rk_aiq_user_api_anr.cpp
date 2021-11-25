/*
 *  Copyright (c) 2019 Rockchip Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "rk_aiq_user_api_anr.h"
#include "RkAiqHandleInt.h"

RKAIQ_BEGIN_DECLARE

#ifdef RK_SIMULATOR_HW
#define CHECK_USER_API_ENABLE
#endif

XCamReturn
rk_aiq_user_api_anr_SetAttrib(const rk_aiq_sys_ctx_t* sys_ctx, rk_aiq_nr_attrib_t *attr)
{
    CHECK_USER_API_ENABLE(RK_AIQ_ALGO_TYPE_ANR);
    RKAIQ_API_SMART_LOCK(sys_ctx);
    RkAiqAnrHandleInt* algo_handle =
        algoHandle<RkAiqAnrHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_ANR);

    if (algo_handle) {
        return algo_handle->setAttrib(attr);
    }

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_user_api_anr_GetAttrib(const rk_aiq_sys_ctx_t* sys_ctx, rk_aiq_nr_attrib_t *attr)
{
    RKAIQ_API_SMART_LOCK(sys_ctx);
    RkAiqAnrHandleInt* algo_handle =
        algoHandle<RkAiqAnrHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_ANR);

    if (algo_handle) {
        return algo_handle->getAttrib(attr);
    }

    return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_user_api_anr_SetIQPara(const rk_aiq_sys_ctx_t* sys_ctx, rk_aiq_nr_IQPara_t *para)
{
    RKAIQ_API_SMART_LOCK(sys_ctx);
    RkAiqAnrHandleInt* algo_handle =
        algoHandle<RkAiqAnrHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_ANR);

    if (algo_handle) {
        return algo_handle->setIQPara(para);
    }

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_user_api_anr_GetIQPara(const rk_aiq_sys_ctx_t* sys_ctx, rk_aiq_nr_IQPara_t *para)
{
    RKAIQ_API_SMART_LOCK(sys_ctx);
    RkAiqAnrHandleInt* algo_handle =
        algoHandle<RkAiqAnrHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_ANR);

    if (algo_handle) {
        return algo_handle->getIQPara(para);
    }

    return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_user_api_anr_SetLumaSFStrength(const rk_aiq_sys_ctx_t* sys_ctx, float fPercnt)
{
    RKAIQ_API_SMART_LOCK(sys_ctx);
    RkAiqAnrHandleInt* algo_handle =
        algoHandle<RkAiqAnrHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_ANR);

    if (algo_handle) {
        return algo_handle->setLumaSFStrength(fPercnt);
    }

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_user_api_anr_SetLumaTFStrength(const rk_aiq_sys_ctx_t* sys_ctx, float fPercnt)
{
    RKAIQ_API_SMART_LOCK(sys_ctx);
    RkAiqAnrHandleInt* algo_handle =
        algoHandle<RkAiqAnrHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_ANR);

    if (algo_handle) {
        return algo_handle->setLumaTFStrength(fPercnt);
    }

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_user_api_anr_GetLumaSFStrength(const rk_aiq_sys_ctx_t* sys_ctx, float *pPercnt)
{
    RKAIQ_API_SMART_LOCK(sys_ctx);
    RkAiqAnrHandleInt* algo_handle =
        algoHandle<RkAiqAnrHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_ANR);

    if (algo_handle) {
        return algo_handle->getLumaSFStrength(pPercnt);
    }

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_user_api_anr_GetLumaTFStrength(const rk_aiq_sys_ctx_t* sys_ctx, float *pPercnt)
{
    RKAIQ_API_SMART_LOCK(sys_ctx);
    RkAiqAnrHandleInt* algo_handle =
        algoHandle<RkAiqAnrHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_ANR);

    if (algo_handle) {
        return algo_handle->getLumaTFStrength(pPercnt);
    }

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_user_api_anr_SetChromaSFStrength(const rk_aiq_sys_ctx_t* sys_ctx, float fPercnt)
{
    RKAIQ_API_SMART_LOCK(sys_ctx);
    RkAiqAnrHandleInt* algo_handle =
        algoHandle<RkAiqAnrHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_ANR);

    if (algo_handle) {
        return algo_handle->setChromaSFStrength(fPercnt);
    }

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_user_api_anr_SetChromaTFStrength(const rk_aiq_sys_ctx_t* sys_ctx, float fPercnt)
{
    RKAIQ_API_SMART_LOCK(sys_ctx);
    RkAiqAnrHandleInt* algo_handle =
        algoHandle<RkAiqAnrHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_ANR);

    if (algo_handle) {
        return algo_handle->setChromaTFStrength(fPercnt);
    }

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_user_api_anr_GetChromaSFStrength(const rk_aiq_sys_ctx_t* sys_ctx, float *pPercnt)
{
    RKAIQ_API_SMART_LOCK(sys_ctx);
    RkAiqAnrHandleInt* algo_handle =
        algoHandle<RkAiqAnrHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_ANR);

    if (algo_handle) {
        return algo_handle->getChromaSFStrength(pPercnt);
    }

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_user_api_anr_GetChromaTFStrength(const rk_aiq_sys_ctx_t* sys_ctx, float *pPercnt)
{
    RKAIQ_API_SMART_LOCK(sys_ctx);
    RkAiqAnrHandleInt* algo_handle =
        algoHandle<RkAiqAnrHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_ANR);

    if (algo_handle) {
        return algo_handle->getChromaTFStrength(pPercnt);
    }

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_user_api_anr_SetRawnrSFStrength(const rk_aiq_sys_ctx_t* sys_ctx, float fPercnt)
{
    RKAIQ_API_SMART_LOCK(sys_ctx);
    RkAiqAnrHandleInt* algo_handle =
        algoHandle<RkAiqAnrHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_ANR);

    if (algo_handle) {
        return algo_handle->setRawnrSFStrength(fPercnt);
    }

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_user_api_anr_GetRawnrSFStrength(const rk_aiq_sys_ctx_t* sys_ctx, float *pPercnt)
{
    RKAIQ_API_SMART_LOCK(sys_ctx);
    RkAiqAnrHandleInt* algo_handle =
        algoHandle<RkAiqAnrHandleInt>(sys_ctx, RK_AIQ_ALGO_TYPE_ANR);

    if (algo_handle) {
        return algo_handle->getRawnrSFStrength(pPercnt);
    }

    return XCAM_RETURN_NO_ERROR;
}

RKAIQ_END_DECLARE
