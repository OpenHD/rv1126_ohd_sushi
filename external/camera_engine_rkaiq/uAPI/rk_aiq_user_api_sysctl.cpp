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

#include "rk_aiq_user_api_sysctl.h"
#include "RkAiqManager.h"
#include "socket_server.h"
#ifdef RK_SIMULATOR_HW
#include "simulator/CamHwSimulator.h"
#else
#include "isp20/CamHwIsp20.h"
#include "fakecamera/FakeCamHwIsp20.h"
#include "isp20/Isp20_module_dbg.h"
#endif

using namespace RkCam;
using namespace XCam;

typedef struct rk_aiq_sys_ctx_s {
    const char* _sensor_entity_name;
    SmartPtr<RkAiqManager> _rkAiqManager;
    SmartPtr<ICamHw> _camHw;
    SmartPtr<RkAiqCore> _analyzer;
    SmartPtr<RkLumaCore> _lumaAnalyzer;
    CamCalibDbContext_t *_calibDb;
    SocketServer *  _socket;
    SmartPtr<Mutex> _apiMutex;
} rk_aiq_sys_ctx_t;

#define RKAIQSYS_CHECK_RET(cond, ret, format, ...) \
    if ((cond)) { \
        LOGE(format, ##__VA_ARGS__); \
        return ret; \
    }

#define RKAIQ_DEFAULT_IQ_PATH "/etc/iqfiles/"

RKAIQ_BEGIN_DECLARE

#define RKAIQ_API_SMART_LOCK(ctx) \
    SmartLock lock (*ctx->_apiMutex.ptr());

typedef struct rk_aiq_sys_preinit_cfg_s {
    rk_aiq_working_mode_t mode;
    std::string force_iq_file;
} rk_aiq_sys_preinit_cfg_t;

static std::map<std::string, rk_aiq_sys_preinit_cfg_t> g_rk_aiq_sys_preinit_cfg_map;

XCamReturn
rk_aiq_uapi_sysctl_preInit(const char* sns_ent_name,
                           rk_aiq_working_mode_t mode,
                           const char* force_iq_file)
{
    std::string sns_ent_name_str(sns_ent_name);
    rk_aiq_sys_preinit_cfg_t cfg;

    cfg.mode = mode;
    if (force_iq_file)
        cfg.force_iq_file = force_iq_file;
    g_rk_aiq_sys_preinit_cfg_map[sns_ent_name_str] = cfg;

    return XCAM_RETURN_NO_ERROR;
}

static void
rk_aiq_uapi_sysctl_deinit_locked(rk_aiq_sys_ctx_t* ctx);

rk_aiq_sys_ctx_t*
rk_aiq_uapi_sysctl_init(const char* sns_ent_name,
                        const char* config_file_dir,
                        rk_aiq_error_cb err_cb,
                        rk_aiq_metas_cb metas_cb)
{
    ENTER_XCORE_FUNCTION();
    char config_file[256];

    XCAM_ASSERT(sns_ent_name);

    bool is_ent_name = true;
    if (sns_ent_name[0] != 'm' || sns_ent_name[3] != '_')
        is_ent_name = false;

    if (!is_ent_name) {
        if (config_file_dir && (strlen(config_file_dir) > 0))
            sprintf(config_file, "%s/%s.xml", config_file_dir, sns_ent_name);
        else
            sprintf(config_file, "%s/%s.xml", RKAIQ_DEFAULT_IQ_PATH, sns_ent_name);
    }
    rk_aiq_sys_ctx_t* ctx = new rk_aiq_sys_ctx_t();
    RKAIQSYS_CHECK_RET(!ctx, NULL, "malloc ctx error !");

    ctx->_apiMutex = new Mutex();
    RKAIQ_API_SMART_LOCK(ctx);

    ctx->_sensor_entity_name = strndup(sns_ent_name, 128);
    RKAIQSYS_CHECK_RET(!ctx->_sensor_entity_name, NULL, "dup sensor name error !");

    ctx->_rkAiqManager = new RkAiqManager(ctx->_sensor_entity_name,
                                          err_cb,
                                          metas_cb);
#ifdef RK_SIMULATOR_HW
    ctx->_camHw = new CamHwSimulator();
#else
    if (0 == strcmp(sns_ent_name, "FakeCamera")) {
        ctx->_camHw = new FakeCamHwIsp20();
    } else {
        ctx->_camHw = new CamHwIsp20();
    }

    // use user defined iq file
    std::map<std::string, rk_aiq_sys_preinit_cfg_t>::iterator it =
        g_rk_aiq_sys_preinit_cfg_map.find(std::string(ctx->_sensor_entity_name));
    int user_hdr_mode = -1;
    bool user_spec_iq = false;
    if (it != g_rk_aiq_sys_preinit_cfg_map.end()) {
        if (!it->second.force_iq_file.empty()) {
            sprintf(config_file, "%s/%s", config_file_dir, it->second.force_iq_file.c_str());
            LOGI("use user sepcified iq file %s", config_file);
            user_spec_iq = true;
        } else {
            user_hdr_mode = it->second.mode;
            LOGI("selected by user sepcified hdr mode %d", user_hdr_mode);
        }
    }

    // use auto selected iq file
    if (is_ent_name && !user_spec_iq) {
        char iq_file[128] = {'\0'};
        CamHwIsp20::selectIqFile(sns_ent_name, iq_file);

        char* hdr_mode = getenv("HDR_MODE");
        int start = strlen(iq_file) - strlen(".xml");

        if (user_hdr_mode != -1) {
            iq_file[start] = '\0';
            if (user_hdr_mode == RK_AIQ_WORKING_MODE_ISP_HDR3)
                strcat(iq_file, "-hdr3.xml");
            else
                strcat(iq_file, "_normal.xml");
        } else if (hdr_mode) {
            iq_file[start] = '\0';
            if (strstr(hdr_mode, "32"))
                strcat(iq_file, "-hdr3.xml");
            else
                strcat(iq_file, "_normal.xml");
        }

        if (config_file_dir) {
            sprintf(config_file, "%s/%s", config_file_dir, iq_file);
        } else {
            sprintf(config_file, "%s/%s", RKAIQ_DEFAULT_IQ_PATH, iq_file);
        }

        // use default iq file
        if (hdr_mode && access(config_file, F_OK)) {
            LOGW("%s not exist, will use the default !", config_file);
            if (strstr(hdr_mode, "32"))
                start = strlen(config_file) - strlen("-hdr3.xml");
            else
                start = strlen(config_file) - strlen("_normal.xml");
            config_file[start] = '\0';
            strcat(config_file, ".xml");
        }
        LOGI("use iq file %s", config_file);
    }
#endif
    ctx->_rkAiqManager->setCamHw(ctx->_camHw);
    ctx->_analyzer = new RkAiqCore();
    if (is_ent_name && config_file_dir) {
        ctx->_analyzer->setResrcPath(config_file_dir);
    } else
        ctx->_analyzer->setResrcPath(RKAIQ_DEFAULT_IQ_PATH);
    ctx->_rkAiqManager->setAnalyzer(ctx->_analyzer);
    ctx->_lumaAnalyzer = new RkLumaCore();
    ctx->_rkAiqManager->setLumaAnalyzer(ctx->_lumaAnalyzer);
    ctx->_calibDb = RkAiqCalibDb::createCalibDb(config_file);
    ctx->_socket  = new SocketServer();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (!ctx->_calibDb)
        goto error;
    ctx->_rkAiqManager->setAiqCalibDb(ctx->_calibDb);
    ret = ctx->_rkAiqManager->init();
    ctx->_socket->Process(ctx, sns_ent_name);
    if (ret)
        goto error;

    EXIT_XCORE_FUNCTION();

    return ctx;

error:
    LOGE("_rkAiqManager init error!");
    rk_aiq_uapi_sysctl_deinit_locked(ctx);
    return NULL;
}

void
rk_aiq_uapi_sysctl_deinit_locked(rk_aiq_sys_ctx_t* ctx)
{
    std::map<std::string, rk_aiq_sys_preinit_cfg_t>::iterator it =
        g_rk_aiq_sys_preinit_cfg_map.find(std::string(ctx->_sensor_entity_name));
    if (it != g_rk_aiq_sys_preinit_cfg_map.end()) {
        g_rk_aiq_sys_preinit_cfg_map.erase(it);
        LOGI("unset user specific iq file.");
    }

    if (ctx->_rkAiqManager.ptr())
        ctx->_rkAiqManager->deInit();

    ctx->_socket->Deinit();
    delete(ctx->_socket);
    ctx->_camHw.release();
    ctx->_analyzer.release();
    ctx->_lumaAnalyzer.release();
    ctx->_rkAiqManager.release();

    if (ctx->_sensor_entity_name)
        xcam_free((void*)(ctx->_sensor_entity_name));

    //RkAiqCalibDb::releaseCalibDb();
}

void
rk_aiq_uapi_sysctl_deinit(rk_aiq_sys_ctx_t* ctx)
{
    ENTER_XCORE_FUNCTION();
    {
        RKAIQ_API_SMART_LOCK(ctx);
        rk_aiq_uapi_sysctl_deinit_locked(ctx);
    }
    delete ctx;
    EXIT_XCORE_FUNCTION();
}

XCamReturn
rk_aiq_uapi_sysctl_prepare(const rk_aiq_sys_ctx_t* ctx,
                           uint32_t  width, uint32_t  height,
                           rk_aiq_working_mode_t mode)
{
    ENTER_XCORE_FUNCTION();
    RKAIQ_API_SMART_LOCK(ctx);

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = ctx->_rkAiqManager->prepare(width, height, mode);
    RKAIQSYS_CHECK_RET(ret, ret, "prepare failed !");

    EXIT_XCORE_FUNCTION();

    return ret;
}

XCamReturn
rk_aiq_uapi_sysctl_start(const rk_aiq_sys_ctx_t* ctx)
{
    ENTER_XCORE_FUNCTION();
    RKAIQ_API_SMART_LOCK(ctx);

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = ctx->_rkAiqManager->start();

    EXIT_XCORE_FUNCTION();

    return ret;
}

XCamReturn
rk_aiq_uapi_sysctl_stop(const rk_aiq_sys_ctx_t* ctx, bool keep_ext_hw_st)
{
    ENTER_XCORE_FUNCTION();
    RKAIQ_API_SMART_LOCK(ctx);

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = ctx->_rkAiqManager->stop(keep_ext_hw_st);

    EXIT_XCORE_FUNCTION();

    return ret;
}

XCamReturn
rk_aiq_uapi_sysctl_getStaticMetas(const char* sns_ent_name, rk_aiq_static_info_t* static_info)
{
    if (!sns_ent_name || !static_info)
        return XCAM_RETURN_ERROR_FAILED;
#ifdef RK_SIMULATOR_HW
    /* nothing to do now*/
    static_info = NULL;
#else
    memcpy(static_info, CamHwIsp20::getStaticCamHwInfo(sns_ent_name), sizeof(rk_aiq_static_info_t));
#endif
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_uapi_sysctl_enumStaticMetas(int index, rk_aiq_static_info_t* static_info)
{
    if (!static_info)
        return XCAM_RETURN_ERROR_FAILED;
#ifdef RK_SIMULATOR_HW
    /* nothing to do now*/
    static_info = NULL;
#else
    rk_aiq_static_info_t* tmp =  CamHwIsp20::getStaticCamHwInfo(NULL, index);
    if (tmp)
        memcpy(static_info, tmp, sizeof(rk_aiq_static_info_t));
    else
        return XCAM_RETURN_ERROR_OUTOFRANGE;
#endif
    return XCAM_RETURN_NO_ERROR;
}

const char*
rk_aiq_uapi_sysctl_getBindedSnsEntNmByVd(const char* vd)
{
#ifndef RK_SIMULATOR_HW
    return CamHwIsp20::getBindedSnsEntNmByVd(vd);
#endif
    return NULL;
}

XCamReturn
rk_aiq_uapi_sysctl_getMetaData(const rk_aiq_sys_ctx_t* ctx, uint32_t frame_id, rk_aiq_metas_t* metas)
{
    // TODO
    return XCAM_RETURN_ERROR_FAILED;
}

XCamReturn
rk_aiq_uapi_sysctl_regLib(const rk_aiq_sys_ctx_t* ctx,
                          RkAiqAlgoDesComm* algo_lib_des)
{
    RKAIQ_API_SMART_LOCK(ctx);
    return ctx->_analyzer->addAlgo(*algo_lib_des);
}

XCamReturn
rk_aiq_uapi_sysctl_unRegLib(const rk_aiq_sys_ctx_t* ctx,
                            const int algo_type,
                            const int lib_id)
{
    RKAIQ_API_SMART_LOCK(ctx);
    return ctx->_analyzer->rmAlgo(algo_type, lib_id);
}

XCamReturn
rk_aiq_uapi_sysctl_enableAxlib(const rk_aiq_sys_ctx_t* ctx,
                               const int algo_type,
                               const int lib_id,
                               bool enable)
{
    RKAIQ_API_SMART_LOCK(ctx);
    return ctx->_analyzer->enableAlgo(algo_type, lib_id, enable);
}

bool
rk_aiq_uapi_sysctl_getAxlibStatus(const rk_aiq_sys_ctx_t* ctx,
                                  const int algo_type,
                                  const int lib_id)
{
    RKAIQ_API_SMART_LOCK(ctx);
    return ctx->_analyzer->getAxlibStatus(algo_type, lib_id);
}

RkAiqAlgoContext*
rk_aiq_uapi_sysctl_getEnabledAxlibCtx(const rk_aiq_sys_ctx_t* ctx, const int algo_type)
{
    RKAIQ_API_SMART_LOCK(ctx);
    return ctx->_analyzer->getEnabledAxlibCtx(algo_type);
}

RkAiqAlgoContext*
rk_aiq_uapi_sysctl_getAxlibCtx(const rk_aiq_sys_ctx_t* ctx, const int algo_type, const int lib_id)
{
    RKAIQ_API_SMART_LOCK(ctx);
    return ctx->_analyzer->getAxlibCtx(algo_type, lib_id);
}

XCamReturn
rk_aiq_uapi_sysctl_get3AStats(const rk_aiq_sys_ctx_t* ctx,
                              rk_aiq_isp_stats_t *stats)
{
    RKAIQ_API_SMART_LOCK(ctx);
    return ctx->_analyzer->get3AStatsFromCachedList(*stats);
}

XCamReturn
rk_aiq_uapi_sysctl_get3AStatsBlk(const rk_aiq_sys_ctx_t* ctx,
                                 rk_aiq_isp_stats_t **stats, int timeout_ms)
{
    // blocked API, add lock ?
    //RKAIQ_API_SMART_LOCK(ctx);
    return ctx->_analyzer->get3AStatsFromCachedList(stats, timeout_ms);
}

void
rk_aiq_uapi_sysctl_release3AStatsRef(const rk_aiq_sys_ctx_t* ctx,
                                     rk_aiq_isp_stats_t *stats)
{
    RKAIQ_API_SMART_LOCK(ctx);
    ctx->_analyzer->release3AStatsRef(stats);
}

RKAIQ_END_DECLARE

template<typename T> static T*
algoHandle(const rk_aiq_sys_ctx_t* ctx, const int algo_type)
{
    T* algo_handle = NULL;

    RkCam::RkAiqHandle* handle =
        const_cast<RkCam::RkAiqHandle*>(ctx->_analyzer->getAiqAlgoHandle(algo_type));

    XCAM_ASSERT(handle);

    int algo_id = handle->getAlgoId();

    if (algo_id == 0)
        algo_handle = dynamic_cast<T*>(handle);

    return algo_handle;
}

#include "RkAiqVersion.h"
#include "RkAiqCalibVersion.h"
#include "rk_aiq_user_api_awb.cpp"
#include "rk_aiq_user_api_adebayer.cpp"
#include "rk_aiq_user_api_ahdr.cpp"
#include "rk_aiq_user_api_alsc.cpp"
#include "rk_aiq_user_api_accm.cpp"
#include "rk_aiq_user_api_a3dlut.cpp"
#include "rk_aiq_user_api_adehaze.cpp"
#include "rk_aiq_user_api_agamma.cpp"
#include "rk_aiq_user_api_adegamma.cpp"
#include "rk_aiq_user_api_ablc.cpp"
#include "rk_aiq_user_api_adpcc.cpp"
#include "rk_aiq_user_api_ae.cpp"
#include "rk_aiq_user_api_anr.cpp"
#include "rk_aiq_user_api_asharp.cpp"
#include "rk_aiq_user_api_imgproc.cpp"
#include "rk_aiq_user_api_afec.cpp"
#include "rk_aiq_user_api_af.cpp"
#include "rk_aiq_user_api_asd.cpp"
#include "rk_aiq_user_api_aldch.cpp"
#include "rk_aiq_user_api_acp.cpp"
#include "rk_aiq_user_api_aie.cpp"
#include "rk_aiq_user_api_custom_ae.cpp"

#define RK_AIQ_ALGO_TYPE_MODULES (RK_AIQ_ALGO_TYPE_MAX + 1)

XCamReturn
rk_aiq_uapi_sysctl_setModuleCtl(const rk_aiq_sys_ctx_t* ctx, rk_aiq_module_id_t mId, bool mod_en)
{
    ENTER_XCORE_FUNCTION();
    CHECK_USER_API_ENABLE(RK_AIQ_ALGO_TYPE_MODULES);
    RKAIQ_API_SMART_LOCK(ctx);

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (mId > RK_MODULE_INVAL && mId < RK_MODULE_MAX) {
        if (mId == RK_MODULE_FEC) {
            rk_aiq_fec_attrib_t fecAttr;
            rk_aiq_user_api_afec_GetAttrib(ctx, &fecAttr);
            fecAttr.en = mod_en;
            if(XCAM_RETURN_NO_ERROR != rk_aiq_user_api_afec_SetAttrib(ctx, fecAttr))
                LOGE("enable fec failed! maybe fec not enable in xml.");
        } else {
            ret = ctx->_rkAiqManager->setModuleCtl(mId, mod_en);
        }
    } else {
        ret = XCAM_RETURN_ERROR_FAILED;
    }

    EXIT_XCORE_FUNCTION();

    return ret;
}

int32_t
rk_aiq_uapi_sysctl_getModuleCtl(const rk_aiq_sys_ctx_t* ctx, rk_aiq_module_id_t mId, bool *mod_en)
{
    ENTER_XCORE_FUNCTION();
    RKAIQ_API_SMART_LOCK(ctx);

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    bool en;
    ret = ctx->_rkAiqManager->getModuleCtl(mId, en);
    *mod_en = en;
    EXIT_XCORE_FUNCTION();

    return ret;
}

XCamReturn
rk_aiq_uapi_sysctl_setCpsLtCfg(const rk_aiq_sys_ctx_t* ctx,
                               rk_aiq_cpsl_cfg_t* cfg)
{
    RKAIQ_API_SMART_LOCK(ctx);
    return ctx->_analyzer->setCpsLtCfg(*cfg);
}

XCamReturn
rk_aiq_uapi_sysctl_getCpsLtInfo(const rk_aiq_sys_ctx_t* ctx,
                                rk_aiq_cpsl_info_t* info)
{
    RKAIQ_API_SMART_LOCK(ctx);
    return ctx->_analyzer->getCpsLtInfo(*info);
}

XCamReturn
rk_aiq_uapi_sysctl_queryCpsLtCap(const rk_aiq_sys_ctx_t* ctx,
                                 rk_aiq_cpsl_cap_t* cap)
{
    RKAIQ_API_SMART_LOCK(ctx);
    return ctx->_analyzer->queryCpsLtCap(*cap);
}

extern RkAiqAlgoDescription g_RkIspAlgoDescA3dlut;
extern RkAiqAlgoDescription g_RkIspAlgoDescAblc;
extern RkAiqAlgoDescription g_RkIspAlgoDescAccm;
extern RkAiqAlgoDescription g_RkIspAlgoDescAcgc;
extern RkAiqAlgoDescription g_RkIspAlgoDescAcp;
extern RkAiqAlgoDescription g_RkIspAlgoDescAdebayer;
extern RkAiqAlgoDescription g_RkIspAlgoDescAdhaz;
extern RkAiqAlgoDescription g_RkIspAlgoDescAdpcc;
extern RkAiqAlgoDescription g_RkIspAlgoDescAe;
extern RkAiqAlgoDescription g_RkIspAlgoDescAf;
extern RkAiqAlgoDescription g_RkIspAlgoDescAfec;
extern RkAiqAlgoDescription g_RkIspAlgoDescAgamma;
extern RkAiqAlgoDescription g_RkIspAlgoDescAgic;
extern RkAiqAlgoDescription g_RkIspAlgoDescAhdr;
extern RkAiqAlgoDescription g_RkIspAlgoDescAie;
extern RkAiqAlgoDescription g_RkIspAlgoDescAldch;
extern RkAiqAlgoDescription g_RkIspAlgoDescAlsc;
extern RkAiqAlgoDescription g_RkIspAlgoDescAnr;
extern RkAiqAlgoDescription g_RkIspAlgoDescAorb;
extern RkAiqAlgoDescription g_RkIspAlgoDescAr2y;
extern RkAiqAlgoDescription g_RkIspAlgoDescAsd;
extern RkAiqAlgoDescription g_RkIspAlgoDescAsharp;
extern RkAiqAlgoDescription g_RkIspAlgoDescAwb;
extern RkAiqAlgoDescription g_RkIspAlgoDescAwdr;

static void _print_versions()
{
    LOGI("\n"
         "************************** VERSION INFOS **************************\n"
         "version release date: %s\n"
         "         AIQ:       %s\n"
         "   IQ PARSER:       %s\n"
         "   RK INTEGRATED ALGO MODULES:\n"
         "         AWB:       %s\n"
         "         AEC:       %s\n"
         "          AF:       %s\n"
         "        AHDR:       %s\n"
         "         ANR:       %s\n"
         "      ASHARP:       %s\n"
         "     ADEHAZE:       %s\n"
         "      AGAMMA:       %s\n"
         "      A3DLUT:       %s\n"
         "        ABLC:       %s\n"
         "        ACCM:       %s\n"
         "        ACGC:       %s\n"
         "         ACP:       %s\n"
         "    ADEBAYER:       %s\n"
         "       ADPCC:       %s\n"
         "        AGIC:       %s\n"
         "         AIE:       %s\n"
         "       ALDCH:       %s\n"
         "        ALSC:       %s\n"
         "        AORB:       %s\n"
         "        AR2Y:       %s\n"
         "         ASD:       %s\n"
         "        AWDR:       %s\n"
         "************************ VERSION INFOS END ************************\n"
         , RK_AIQ_RELEASE_DATE
         , RK_AIQ_VERSION
         , RK_AIQ_CALIB_VERSION
         , g_RkIspAlgoDescAwb.common.version
         , g_RkIspAlgoDescAe.common.version
         , g_RkIspAlgoDescAf.common.version
         , g_RkIspAlgoDescAhdr.common.version
         , g_RkIspAlgoDescAnr.common.version
         , g_RkIspAlgoDescAsharp.common.version
         , g_RkIspAlgoDescAdhaz.common.version
         , g_RkIspAlgoDescAgamma.common.version
         , g_RkIspAlgoDescA3dlut.common.version
         , g_RkIspAlgoDescAblc.common.version
         , g_RkIspAlgoDescAccm.common.version
         , g_RkIspAlgoDescAcgc.common.version
         , g_RkIspAlgoDescAcp.common.version
         , g_RkIspAlgoDescAdebayer.common.version
         , g_RkIspAlgoDescAdpcc.common.version
         , g_RkIspAlgoDescAfec.common.version
         , g_RkIspAlgoDescAgic.common.version
         , g_RkIspAlgoDescAie.common.version
         , g_RkIspAlgoDescAldch.common.version
         , g_RkIspAlgoDescAlsc.common.version
         , g_RkIspAlgoDescAorb.common.version
         , g_RkIspAlgoDescAr2y.common.version
         , g_RkIspAlgoDescAsd.common.version
         , g_RkIspAlgoDescAwdr.common.version
        );
}

void rk_aiq_uapi_get_version_info(rk_aiq_ver_info_t* vers)
{
    uint32_t iq_parser_magic_code;

    xcam_mem_clear (*vers);
    const char* ver_str = RK_AIQ_CALIB_VERSION;
    const char* start = ver_str +  strlen(RK_AIQ_CALIB_VERSION_HEAD);
    const char* stop = strstr(ver_str, RK_AIQ_CALIB_VERSION_MAGIC_JOINT);

    // TODO: use strncpy instead of memcpy, but has compile warning now
    memcpy(vers->iq_parser_ver, start, stop - start);

    start = strstr(ver_str, RK_AIQ_CALIB_VERSION_MAGIC_CODE_HEAD) +
            strlen(RK_AIQ_CALIB_VERSION_MAGIC_CODE_HEAD);

    vers->iq_parser_magic_code = atoi(start);

    ver_str = RK_AIQ_VERSION;
    start = ver_str +  strlen(RK_AIQ_VERSION_HEAD);
    strcpy(vers->aiq_ver, start);

    strcpy(vers->awb_algo_ver, g_RkIspAlgoDescAwb.common.version);
    strcpy(vers->ae_algo_ver, g_RkIspAlgoDescAe.common.version);
    strcpy(vers->af_algo_ver, g_RkIspAlgoDescAf.common.version);
    strcpy(vers->ahdr_algo_ver, g_RkIspAlgoDescAhdr.common.version);

    LOGD("aiq ver %s, parser ver %s, magic code %d, awb ver %s\n"
         "ae ver %s, af ver %s, ahdr ver %s", vers->aiq_ver,
         vers->iq_parser_ver, vers->iq_parser_magic_code,
         vers->awb_algo_ver, vers->ae_algo_ver,
         vers->af_algo_ver, vers->ahdr_algo_ver);
}

static void rk_aiq_init_lib(void) __attribute__((constructor));
static void rk_aiq_init_lib(void)
{
    xcam_get_log_level();
    ENTER_XCORE_FUNCTION();
#ifdef RK_SIMULATOR_HW
    /* nothing to do now */
#else
    CamHwIsp20::initCamHwInfos();
#endif
    _print_versions();
    EXIT_XCORE_FUNCTION();

}
static void rk_aiq_deinit_lib(void) __attribute__((destructor));
static void rk_aiq_deinit_lib(void)
{
    ENTER_XCORE_FUNCTION();
#ifdef RK_SIMULATOR_HW
    /* nothing to do now */
#else
    RkAiqCalibDb::releaseCalibDb();
    CamHwIsp20::clearStaticCamHwInfo();
#endif
    EXIT_XCORE_FUNCTION();
}

XCamReturn
rk_aiq_uapi_sysctl_enqueueRkRawBuf(const rk_aiq_sys_ctx_t* ctx, void *rawdata, bool sync)
{
    ENTER_XCORE_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ret = ctx->_rkAiqManager->enqueueRawBuffer(rawdata, sync);
    EXIT_XCORE_FUNCTION();

    return ret;
}

XCamReturn
rk_aiq_uapi_sysctl_enqueueRkRawFile(const rk_aiq_sys_ctx_t* ctx, const char *path)
{
    ENTER_XCORE_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ret = ctx->_rkAiqManager->enqueueRawFile(path);
    EXIT_XCORE_FUNCTION();

    return ret;
}

XCamReturn
rk_aiq_uapi_sysctl_registRkRawCb(const rk_aiq_sys_ctx_t* ctx, void (*callback)(void*))
{
    ENTER_XCORE_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (callback == NULL)
        ret = XCAM_RETURN_ERROR_PARAM;
    else
        ret = ctx->_rkAiqManager->registRawdataCb(callback);
    EXIT_XCORE_FUNCTION();
    return ret;
}

XCamReturn
rk_aiq_uapi_sysctl_prepareRkRaw(const rk_aiq_sys_ctx_t* ctx, rk_aiq_raw_prop_t prop)
{
    ENTER_XCORE_FUNCTION();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ret = ctx->_rkAiqManager->rawdataPrepare(prop);
    EXIT_XCORE_FUNCTION();

    return ret;
}

XCamReturn
rk_aiq_uapi_sysctl_setSharpFbcRotation(const rk_aiq_sys_ctx_t* ctx, rk_aiq_rotation_t rot)
{
    ENTER_XCORE_FUNCTION();
    RKAIQ_API_SMART_LOCK(ctx);
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ret = ctx->_rkAiqManager->setSharpFbcRotation(rot);
    EXIT_XCORE_FUNCTION();
    return ret;
}

/*!
 * \brief switch working mode dynamically
 * this aims to switch the isp pipeline working mode fast, and can be called on
 * streaming status. On non streaming status, should call rk_aiq_uapi_sysctl_prepare
 * instead of this to set working mode.
 */
XCamReturn
rk_aiq_uapi_sysctl_swWorkingModeDyn(const rk_aiq_sys_ctx_t* ctx, rk_aiq_working_mode_t mode)
{
    ENTER_XCORE_FUNCTION();
    RKAIQ_API_SMART_LOCK(ctx);
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    /* ret = ctx->_rkAiqManager->swWorkingModeDyn(mode); */
    ret = ctx->_rkAiqManager->swWorkingModeDyn_msg(mode);
    EXIT_XCORE_FUNCTION();
    return ret;
}

void
rk_aiq_uapi_sysctl_setMulCamConc(const rk_aiq_sys_ctx_t* ctx, bool cc)
{
    ENTER_XCORE_FUNCTION();
    RKAIQ_API_SMART_LOCK(ctx);
    ctx->_rkAiqManager->setMulCamConc(cc);
    EXIT_XCORE_FUNCTION();
}

XCamReturn
rk_aiq_uapi_sysctl_setCrop(const rk_aiq_sys_ctx_t* sys_ctx, rk_aiq_rect_t rect)
{
    RKAIQ_API_SMART_LOCK(sys_ctx);
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ret = sys_ctx->_camHw->setSensorCrop(rect);
    return ret;
}

XCamReturn
rk_aiq_uapi_sysctl_getCrop(const rk_aiq_sys_ctx_t* sys_ctx, rk_aiq_rect_t* rect)
{
    RKAIQ_API_SMART_LOCK(sys_ctx);
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ret = sys_ctx->_camHw->getSensorCrop(*rect);

    return ret;
}

XCamReturn
rk_aiq_uapi_sysctl_updateIq(const rk_aiq_sys_ctx_t* sys_ctx, char* iqfile)
{
    RKAIQ_API_SMART_LOCK(sys_ctx);
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    LOGI("applying new iq file:%s\n", iqfile);

    if (!sys_ctx) {
        LOGE("%s: sys_ctx is invalied\n", __func__);
        return XCAM_RETURN_ERROR_FAILED;
    }

    auto newCalibDb = RkAiqCalibDb::createCalibDb(iqfile);
    if (!newCalibDb) {
        LOGE("failed to create new CalibDb for:%s\n", iqfile);
        return XCAM_RETURN_ERROR_FAILED;
    }

    ret = sys_ctx->_rkAiqManager->updateCalibDb(newCalibDb);

    if (ret) {
        LOGE("failed to update iqfile\n");
        ret = XCAM_RETURN_ERROR_FAILED;
    }

    const_cast<rk_aiq_sys_ctx_t*>(sys_ctx)->_calibDb = newCalibDb;

    return ret;
}

XCamReturn
rk_aiq_uapi_sysctl_getSensorDiscrib(const rk_aiq_sys_ctx_t* sys_ctx, rk_aiq_exposure_sensor_descriptor *sensorDes)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ret = sys_ctx->_rkAiqManager->getSensorDiscrib(sensorDes);

    return ret;
}

CamCalibDbContext_t*
rk_aiq_uapi_sysctl_getCurCalib(const rk_aiq_sys_ctx_t* ctx)
{
    RKAIQ_API_SMART_LOCK(ctx);
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    return  ctx->_calibDb;
}

XCamReturn
rk_aiq_uapi_sysctl_upateCalib(const rk_aiq_sys_ctx_t* ctx, CamCalibDbContext_t* calib)
{
    RKAIQ_API_SMART_LOCK(ctx);
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    if (calib != ctx->_calibDb)
        LOGW("new calibdb is not equal with the current one\n");

    ret = ctx->_rkAiqManager->updateCalibDb(calib);

    if (ret) {
        LOGE("failed to update iqfile\n");
        ret = XCAM_RETURN_ERROR_FAILED;
    }
    LOGD("update calibdb success\n");

    return ret;
}

