#include "socket_server.h"

#ifdef LOG_TAG
    #undef LOG_TAG
#endif
#define LOG_TAG "rkaiq_tool_ae.cpp"


int setExpSwAttr(rk_aiq_sys_ctx_t* ctx, char* data) {
    return rk_aiq_user_api_ae_setExpSwAttr(ctx, *(Uapi_ExpSwAttr_t*)data);
}

int getExpSwAttr(rk_aiq_sys_ctx_t* ctx, char* data) {
    return rk_aiq_user_api_ae_getExpSwAttr(ctx, (Uapi_ExpSwAttr_t*)data);
}

int setLinAeDayRouteAttr(rk_aiq_sys_ctx_t* ctx, char* data) {
    return rk_aiq_user_api_ae_setLinAeDayRouteAttr(ctx, *(Uapi_LinAeRouteAttr_t*)data);
}

int getLinAeDayRouteAttr(rk_aiq_sys_ctx_t* ctx, char* data) {
    return rk_aiq_user_api_ae_getLinAeDayRouteAttr(ctx, (Uapi_LinAeRouteAttr_t*)data);
}

int setLinAeNightRouteAttr(rk_aiq_sys_ctx_t* ctx, char* data) {
    return rk_aiq_user_api_ae_setLinAeNightRouteAttr(ctx, *(Uapi_LinAeRouteAttr_t*)data);
}

int getLinAeNightRouteAttr(rk_aiq_sys_ctx_t* ctx, char* data) {
    return rk_aiq_user_api_ae_getLinAeNightRouteAttr(ctx, (Uapi_LinAeRouteAttr_t*)data);
}

int setHdrAeDayRouteAttr(rk_aiq_sys_ctx_t* ctx, char* data) {
    return rk_aiq_user_api_ae_setHdrAeDayRouteAttr(ctx, *(Uapi_HdrAeRouteAttr_t*)data);
}

int getHdrAeDayRouteAttr(rk_aiq_sys_ctx_t* ctx, char* data) {
    return rk_aiq_user_api_ae_getHdrAeDayRouteAttr(ctx, (Uapi_HdrAeRouteAttr_t*)data);
}

int setHdrAeNightRouteAttr(rk_aiq_sys_ctx_t* ctx, char* data) {
    return rk_aiq_user_api_ae_setHdrAeNightRouteAttr(ctx, *(Uapi_HdrAeRouteAttr_t*)data);
}

int getHdrAeNightRouteAttr(rk_aiq_sys_ctx_t* ctx, char* data) {
    return rk_aiq_user_api_ae_getHdrAeNightRouteAttr(ctx, (Uapi_HdrAeRouteAttr_t*)data);
}

int queryExpResInfo(rk_aiq_sys_ctx_t* ctx, char* data) {
    return rk_aiq_user_api_ae_queryExpResInfo(ctx, (Uapi_ExpQueryInfo_t*)data);
}

int setLinExpAttr(rk_aiq_sys_ctx_t* ctx, char* data) {
    return rk_aiq_user_api_ae_setLinExpAttr(ctx, *(Uapi_LinExpAttr_t*)data);
}

int getLinExpAttr(rk_aiq_sys_ctx_t* ctx, char* data) {
    return rk_aiq_user_api_ae_getLinExpAttr(ctx, (Uapi_LinExpAttr_t*)data);
}

int setHdrExpAttr(rk_aiq_sys_ctx_t* ctx, char* data) {
    return rk_aiq_user_api_ae_setHdrExpAttr(ctx, *(Uapi_HdrExpAttr_t*)data);
}

int getHdrExpAttr(rk_aiq_sys_ctx_t* ctx, char* data) {
    return rk_aiq_user_api_ae_getHdrExpAttr(ctx, (Uapi_HdrExpAttr_t*)data);
}
