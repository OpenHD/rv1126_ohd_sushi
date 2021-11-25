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

#include "CamHwIsp20.h"
#include "Isp20Evts.h"
#include "Isp20PollThread.h"
#include "Isp20SpThread.h"
#include "rk_isp20_hw.h"
#include "Isp20_module_dbg.h"
#include "mediactl/mediactl-priv.h"
#include <linux/v4l2-subdev.h>
#include <sys/mman.h>
#include <fcntl.h>

namespace RkCam {
std::map<std::string, SmartPtr<rk_aiq_static_info_t>> CamHwIsp20::mCamHwInfos;
std::map<std::string, SmartPtr<rk_sensor_full_info_t>> CamHwIsp20::mSensorHwInfos;
rk_aiq_isp_hw_info_t CamHwIsp20::mIspHwInfos;
rk_aiq_cif_hw_info_t CamHwIsp20::mCifHwInfos;

CamHwIsp20::CamHwIsp20()
    : _is_exit(false)
    , _state(CAM_HW_STATE_INVALID)
    , _hdr_mode(0)
    , _ispp_module_init_ens(0)
    , _sharp_fbc_rotation(RK_AIQ_ROTATION_0)
    , _linked_to_isp(false)
{
    mNormalNoReadBack = false;
    char* valueStr = getenv("normal_no_read_back");
    if (valueStr) {
        mNormalNoReadBack = atoi(valueStr) > 0 ? true : false;
    }
    xcam_mem_clear(_fec_drv_mem_ctx);
    xcam_mem_clear(_ldch_drv_mem_ctx);
    _fec_drv_mem_ctx.type = MEM_TYPE_FEC;
    _fec_drv_mem_ctx.ops_ctx = this;
    _fec_drv_mem_ctx.mem_info = (void*)(fec_mem_info_array);

    _ldch_drv_mem_ctx.type = MEM_TYPE_LDCH;
    _ldch_drv_mem_ctx.ops_ctx = this;
    _ldch_drv_mem_ctx.mem_info = (void*)(ldch_mem_info_array);
    xcam_mem_clear(_crop_rect);
}

CamHwIsp20::~CamHwIsp20()
{}

static XCamReturn get_isp_ver(rk_aiq_isp_hw_info_t *hw_info) {
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    struct v4l2_capability cap;
    V4l2Device vdev(hw_info->isp_info[0].stats_path);


    ret = vdev.open();
    if (ret != XCAM_RETURN_NO_ERROR) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "failed to open dev (%s)", hw_info->isp_info[0].stats_path);
        return XCAM_RETURN_ERROR_FAILED;
    }

    if (vdev.query_cap(cap) == XCAM_RETURN_NO_ERROR) {
        char *p;
        p = strrchr((char*)cap.driver, '_');
        if (p == NULL) {
            ret = XCAM_RETURN_ERROR_FAILED;
            goto out;
        }

        if (*(p + 1) != 'v') {
            ret = XCAM_RETURN_ERROR_FAILED;
            goto out;
        }

        hw_info->hw_ver_info.isp_ver = atoi(p + 2);
        //awb/aec version?
        vdev.close();
        return XCAM_RETURN_NO_ERROR;
    } else {
        ret = XCAM_RETURN_ERROR_FAILED;
        goto out;
    }

out:
    vdev.close();
    LOGE_CAMHW_SUBM(ISP20HW_SUBM, "get isp version failed !");
    return ret;
}

static XCamReturn get_sensor_caps(rk_sensor_full_info_t *sensor_info) {
    struct v4l2_subdev_frame_size_enum fsize_enum;
    struct v4l2_subdev_mbus_code_enum  code_enum;
    std::vector<uint32_t> formats;
    rk_frame_fmt_t frameSize;
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    V4l2SubDevice vdev(sensor_info->device_name.c_str());
    ret = vdev.open();
    if (ret != XCAM_RETURN_NO_ERROR) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "failed to open dev (%s)", sensor_info->device_name.c_str());
        return XCAM_RETURN_ERROR_FAILED;
    }
    //get module info
    struct rkmodule_inf *minfo = &(sensor_info->mod_info);
    if(vdev.io_control(RKMODULE_GET_MODULE_INFO, minfo) < 0) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "@%s %s: Get sensor module info failed", __FUNCTION__, sensor_info->device_name.c_str());
        return XCAM_RETURN_ERROR_FAILED;
    }
    sensor_info->len_name = std::string(minfo->base.lens);

#if 0
    memset(&code_enum, 0, sizeof(code_enum));
    while (vdev.io_control(VIDIOC_SUBDEV_ENUM_MBUS_CODE, &code_enum) == 0) {
        formats.push_back(code_enum.code);
        code_enum.index++;
    };

    //TODO: sensor driver not supported now
    for (auto it = formats.begin(); it != formats.end(); ++it) {
        fsize_enum.pad = 0;
        fsize_enum.index = 0;
        fsize_enum.code = *it;
        while (vdev.io_control(VIDIOC_SUBDEV_ENUM_FRAME_SIZE, &fsize_enum) == 0) {
            frameSize.format = (rk_aiq_format_t)fsize_enum.code;
            frameSize.width = fsize_enum.max_width;
            frameSize.height = fsize_enum.max_height;
            sensor_info->frame_size.push_back(frameSize);
            fsize_enum.index++;
        };
    }
    if(!formats.size() || !sensor_info->frame_size.size()) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "@%s %s: Enum sensor frame size failed", __FUNCTION__, sensor_info->device_name.c_str());
        ret = XCAM_RETURN_ERROR_FAILED;
    }
#endif

    struct v4l2_subdev_frame_interval_enum fie;
    memset(&fie, 0, sizeof(fie));
    while(vdev.io_control(VIDIOC_SUBDEV_ENUM_FRAME_INTERVAL, &fie) == 0) {
        frameSize.format = (rk_aiq_format_t)fie.code;
        frameSize.width = fie.width;
        frameSize.height = fie.height;
        frameSize.fps = fie.interval.denominator / fie.interval.numerator;
        frameSize.hdr_mode = fie.reserved[0];
        sensor_info->frame_size.push_back(frameSize);
        fie.index++;
    }
    if (fie.index == 0)
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "@%s %s: Enum sensor frame interval failed", __FUNCTION__, sensor_info->device_name.c_str());
    vdev.close();

    return ret;
}

static XCamReturn
parse_module_info(rk_sensor_full_info_t *sensor_info)
{
    // sensor entity name format SHOULD be like this:
    // m00_b_ov13850 1-0010
    std::string entity_name(sensor_info->sensor_name);

    if (entity_name.empty())
        return XCAM_RETURN_ERROR_SENSOR;

    int parse_index = 0;

    if (entity_name.at(parse_index) != 'm') {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "%d:parse sensor entity name %s error at %d, please check sensor driver !",
                        __LINE__, entity_name.c_str(), parse_index);
        return XCAM_RETURN_ERROR_SENSOR;
    }

    std::string index_str = entity_name.substr (parse_index, 3);
    sensor_info->module_index_str = index_str;

    parse_index += 3;

    if (entity_name.at(parse_index) != '_') {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "%d:parse sensor entity name %s error at %d, please check sensor driver !",
                        __LINE__, entity_name.c_str(), parse_index);
        return XCAM_RETURN_ERROR_SENSOR;
    }

    parse_index++;

    if (entity_name.at(parse_index) != 'b' &&
            entity_name.at(parse_index) != 'f') {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "%d:parse sensor entity name %s error at %d, please check sensor driver !",
                        __LINE__, entity_name.c_str(), parse_index);
        return XCAM_RETURN_ERROR_SENSOR;
    }
    sensor_info->phy_module_orient = entity_name.at(parse_index);

    parse_index++;

    if (entity_name.at(parse_index) != '_') {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "%d:parse sensor entity name %s error at %d, please check sensor driver !",
                        __LINE__, entity_name.c_str(), parse_index);
        return XCAM_RETURN_ERROR_SENSOR;
    }

    parse_index++;

    std::size_t real_name_end = std::string::npos;
    if ((real_name_end = entity_name.find(' ')) == std::string::npos) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "%d:parse sensor entity name %s error at %d, please check sensor driver !",
                        __LINE__, entity_name.c_str(), parse_index);
        return XCAM_RETURN_ERROR_SENSOR;
    }

    std::string real_name_str = entity_name.substr(parse_index, real_name_end - parse_index);
    sensor_info->module_real_sensor_name = real_name_str;

    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "%s:%d, real sensor name %s, module ori %c, module id %s",
                    __FUNCTION__, __LINE__, sensor_info->module_real_sensor_name.c_str(),
                    sensor_info->phy_module_orient, sensor_info->module_index_str.c_str());

    return XCAM_RETURN_NO_ERROR;
}

static rk_aiq_ispp_t*
get_ispp_subdevs(struct media_device *device, const char *devpath, rk_aiq_ispp_t* ispp_info)
{
    media_entity *entity = NULL;
    const char *entity_name = NULL;
    int index = 0;

    if(!device || !ispp_info || !devpath)
        return NULL;

    for(index = 0; index < MAX_CAM_NUM; index++) {
        if (0 == strlen(ispp_info[index].media_dev_path))
            break;
        if (0 == strncmp(ispp_info[index].media_dev_path, devpath, sizeof(ispp_info[index].media_dev_path))) {
            LOGD_CAMHW_SUBM(ISP20HW_SUBM, "isp info of path %s exists!", devpath);
            return &ispp_info[index];
        }
    }

    if (index >= MAX_CAM_NUM)
        return NULL;

    if (strcmp(device->info.model, "rkispp0") == 0 ||
            strcmp(device->info.model, "rkispp") == 0)
        ispp_info[index].model_idx = 0;
    else if (strcmp(device->info.model, "rkispp1") == 0)
        ispp_info[index].model_idx = 1;
    else if (strcmp(device->info.model, "rkispp2") == 0)
        ispp_info[index].model_idx = 2;
    else if (strcmp(device->info.model, "rkispp3") == 0)
        ispp_info[index].model_idx = 3;
    else
        ispp_info[index].model_idx = -1;

    strncpy(ispp_info[index].media_dev_path, devpath, sizeof(ispp_info[index].media_dev_path));

    entity = media_get_entity_by_name(device, "rkispp_input_image", strlen("rkispp_input_image"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(ispp_info[index].pp_input_image_path, entity_name, sizeof(ispp_info[index].pp_input_image_path));
        }
    }
    entity = media_get_entity_by_name(device, "rkispp_m_bypass", strlen("rkispp_m_bypass"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(ispp_info[index].pp_m_bypass_path, entity_name, sizeof(ispp_info[index].pp_m_bypass_path));
        }
    }
    entity = media_get_entity_by_name(device, "rkispp_scale0", strlen("rkispp_scale0"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(ispp_info[index].pp_scale0_path, entity_name, sizeof(ispp_info[index].pp_scale0_path));
        }
    }
    entity = media_get_entity_by_name(device, "rkispp_scale1", strlen("rkispp_scale1"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(ispp_info[index].pp_scale1_path, entity_name, sizeof(ispp_info[index].pp_scale1_path));
        }
    }
    entity = media_get_entity_by_name(device, "rkispp_scale2", strlen("rkispp_scale2"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(ispp_info[index].pp_scale2_path, entity_name, sizeof(ispp_info[index].pp_scale2_path));
        }
    }
    entity = media_get_entity_by_name(device, "rkispp_input_params", strlen("rkispp_input_params"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(ispp_info[index].pp_input_params_path, entity_name, sizeof(ispp_info[index].pp_input_params_path));
        }
    }
    entity = media_get_entity_by_name(device, "rkispp-stats", strlen("rkispp-stats"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(ispp_info[index].pp_stats_path, entity_name, sizeof(ispp_info[index].pp_stats_path));
        }
    }
    entity = media_get_entity_by_name(device, "rkispp-subdev", strlen("rkispp-subdev"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(ispp_info[index].pp_dev_path, entity_name, sizeof(ispp_info[index].pp_dev_path));
        }
    }

    LOGI_CAMHW_SUBM(ISP20HW_SUBM, "model(%s): ispp_info(%d): ispp-subdev entity name: %s\n",
                    device->info.model, index,
                    ispp_info[index].pp_dev_path);

    return &ispp_info[index];
}

static rk_aiq_isp_t*
get_isp_subdevs(struct media_device *device, const char *devpath, rk_aiq_isp_t* isp_info)
{
    media_entity *entity = NULL;
    const char *entity_name = NULL;
    int index = 0;
    if(!device || !isp_info || !devpath)
        return NULL;

    for(index = 0; index < MAX_CAM_NUM; index++) {
        if (0 == strlen(isp_info[index].media_dev_path))
            break;
        if (0 == strncmp(isp_info[index].media_dev_path, devpath, sizeof(isp_info[index].media_dev_path))) {
            LOGD_CAMHW_SUBM(ISP20HW_SUBM, "isp info of path %s exists!", devpath);
            return &isp_info[index];
        }
    }
    if (index >= MAX_CAM_NUM)
        return NULL;

    if (strcmp(device->info.model, "rkisp0") == 0 ||
            strcmp(device->info.model, "rkisp") == 0)
        isp_info[index].model_idx = 0;
    else if (strcmp(device->info.model, "rkisp1") == 0)
        isp_info[index].model_idx = 1;
    else if (strcmp(device->info.model, "rkisp2") == 0)
        isp_info[index].model_idx = 2;
    else if (strcmp(device->info.model, "rkisp3") == 0)
        isp_info[index].model_idx = 3;
    else
        isp_info[index].model_idx = -1;

    strncpy(isp_info[index].media_dev_path, devpath, sizeof(isp_info[index].media_dev_path));

    entity = media_get_entity_by_name(device, "rkisp-isp-subdev", strlen("rkisp-isp-subdev"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(isp_info[index].isp_dev_path, entity_name, sizeof(isp_info[index].isp_dev_path));
        }
    }

    entity = media_get_entity_by_name(device, "rkisp-csi-subdev", strlen("rkisp-csi-subdev"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(isp_info[index].csi_dev_path, entity_name, sizeof(isp_info[index].csi_dev_path));
        }
    }
    entity = media_get_entity_by_name(device, "rkisp-mpfbc-subdev", strlen("rkisp-mpfbc-subdev"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(isp_info[index].mpfbc_dev_path, entity_name, sizeof(isp_info[index].mpfbc_dev_path));
        }
    }
    entity = media_get_entity_by_name(device, "rkisp_mainpath", strlen("rkisp_mainpath"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(isp_info[index].main_path, entity_name, sizeof(isp_info[index].main_path));
        }
    }
    entity = media_get_entity_by_name(device, "rkisp_selfpath", strlen("rkisp_selfpath"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(isp_info[index].self_path, entity_name, sizeof(isp_info[index].self_path));
        }
    }
    entity = media_get_entity_by_name(device, "rkisp_rawwr0", strlen("rkisp_rawwr0"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(isp_info[index].rawwr0_path, entity_name, sizeof(isp_info[index].rawwr0_path));
        }
    }
    entity = media_get_entity_by_name(device, "rkisp_rawwr1", strlen("rkisp_rawwr1"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(isp_info[index].rawwr1_path, entity_name, sizeof(isp_info[index].rawwr1_path));
        }
    }
    entity = media_get_entity_by_name(device, "rkisp_rawwr2", strlen("rkisp_rawwr2"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(isp_info[index].rawwr2_path, entity_name, sizeof(isp_info[index].rawwr2_path));
        }
    }
    entity = media_get_entity_by_name(device, "rkisp_rawwr3", strlen("rkisp_rawwr3"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(isp_info[index].rawwr3_path, entity_name, sizeof(isp_info[index].rawwr3_path));
        }
    }
    entity = media_get_entity_by_name(device, "rkisp_dmapath", strlen("rkisp_dmapath"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(isp_info[index].dma_path, entity_name, sizeof(isp_info[index].dma_path));
        }
    }
    entity = media_get_entity_by_name(device, "rkisp_rawrd0_m", strlen("rkisp_rawrd0_m"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(isp_info[index].rawrd0_m_path, entity_name, sizeof(isp_info[index].rawrd0_m_path));
        }
    }
    entity = media_get_entity_by_name(device, "rkisp_rawrd1_l", strlen("rkisp_rawrd1_l"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(isp_info[index].rawrd1_l_path, entity_name, sizeof(isp_info[index].rawrd1_l_path));
        }
    }
    entity = media_get_entity_by_name(device, "rkisp_rawrd2_s", strlen("rkisp_rawrd2_s"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(isp_info[index].rawrd2_s_path, entity_name, sizeof(isp_info[index].rawrd2_s_path));
        }
    }
    entity = media_get_entity_by_name(device, "rkisp-statistics", strlen("rkisp-statistics"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(isp_info[index].stats_path, entity_name, sizeof(isp_info[index].stats_path));
        }
    }
    entity = media_get_entity_by_name(device, "rkisp-input-params", strlen("rkisp-input-params"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(isp_info[index].input_params_path, entity_name, sizeof(isp_info[index].input_params_path));
        }
    }
    entity = media_get_entity_by_name(device, "rkisp-mipi-luma", strlen("rkisp-mipi-luma"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(isp_info[index].mipi_luma_path, entity_name, sizeof(isp_info[index].mipi_luma_path));
        }
    }
    entity = media_get_entity_by_name(device, "rockchip-mipi-dphy-rx", strlen("rockchip-mipi-dphy-rx"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(isp_info[index].mipi_dphy_rx_path, entity_name, sizeof(isp_info[index].mipi_dphy_rx_path));
        }
    }

    entity = media_get_entity_by_name(device, "rkcif_dvp", strlen("rkcif_dvp"));
    if(entity)
        isp_info[index].linked_dvp = true;

    entity = media_get_entity_by_name(device, "rkcif_mipi_lvds", strlen("rkcif_mipi_lvds"));
    if(entity)
        isp_info[index].linked_dvp = false;

    if ((entity = media_get_entity_by_name(device, "rkcif_dvp", strlen("rkcif_dvp"))) ||
            (entity = media_get_entity_by_name(device, "rkcif_lite_mipi_lvds", strlen("rkcif_lite_mipi_lvds"))) ||
            (entity = media_get_entity_by_name(device, "rkcif_mipi_lvds", strlen("rkcif_mipi_lvds")))) {
        strncpy(isp_info[index].linked_vicap, entity->info.name, sizeof(isp_info[index].linked_vicap));
    }

    LOGI_CAMHW_SUBM(ISP20HW_SUBM, "model(%s): isp_info(%d): ispp-subdev entity name: %s\n",
                    device->info.model, index,
                    isp_info[index].isp_dev_path);

    return &isp_info[index];
}

static rk_aiq_cif_info_t*
get_cif_subdevs(struct media_device *device, const char *devpath, rk_aiq_cif_info_t* cif_info)
{
    media_entity *entity = NULL;
    const char *entity_name = NULL;
    int index = 0;
    if(!device || !devpath || !cif_info)
        return NULL;

    for(index = 0; index < MAX_CAM_NUM; index++) {
        if (0 == strlen(cif_info[index].media_dev_path))
            break;
        if (0 == strncmp(cif_info[index].media_dev_path, devpath, sizeof(cif_info[index].media_dev_path))) {
            LOGD_CAMHW_SUBM(ISP20HW_SUBM, "isp info of path %s exists!", devpath);
            return &cif_info[index];
        }
    }
    if (index >= MAX_CAM_NUM)
        return NULL;

    cif_info[index].model_idx = index;

    strncpy(cif_info[index].media_dev_path, devpath, sizeof(cif_info[index].media_dev_path));

    entity = media_get_entity_by_name(device, "stream_cif_mipi_id0", strlen("stream_cif_mipi_id0"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(cif_info[index].mipi_id0, entity_name, sizeof(cif_info[index].mipi_id0));
        }
    }

    entity = media_get_entity_by_name(device, "stream_cif_mipi_id1", strlen("stream_cif_mipi_id1"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(cif_info[index].mipi_id1, entity_name, sizeof(cif_info[index].mipi_id1));
        }
    }

    entity = media_get_entity_by_name(device, "stream_cif_mipi_id2", strlen("stream_cif_mipi_id2"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(cif_info[index].mipi_id2, entity_name, sizeof(cif_info[index].mipi_id2));
        }
    }

    entity = media_get_entity_by_name(device, "stream_cif_mipi_id3", strlen("stream_cif_mipi_id3"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(cif_info[index].mipi_id3, entity_name, sizeof(cif_info[index].mipi_id3));
        }
    }

    entity = media_get_entity_by_name(device, "rkcif-mipi-luma", strlen("rkisp-mipi-luma"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(cif_info[index].mipi_luma_path, entity_name, sizeof(cif_info[index].mipi_luma_path));
        }
    }

    entity = media_get_entity_by_name(device, "rockchip-mipi-csi2", strlen("rockchip-mipi-csi2"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(cif_info[index].mipi_csi2_sd_path, entity_name, sizeof(cif_info[index].mipi_csi2_sd_path));
        }
    }

    entity = media_get_entity_by_name(device, "rkcif-lvds-subdev", strlen("rkcif-lvds-subdev"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(cif_info[index].lvds_sd_path, entity_name, sizeof(cif_info[index].lvds_sd_path));
        }
    }

    entity = media_get_entity_by_name(device, "rkcif-lite-lvds-subdev", strlen("rkcif-lite-lvds-subdev"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(cif_info[index].lvds_sd_path, entity_name, sizeof(cif_info[index].lvds_sd_path));
        }
    }

    entity = media_get_entity_by_name(device, "rockchip-mipi-dphy-rx", strlen("rockchip-mipi-dphy-rx"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(cif_info[index].mipi_dphy_rx_path, entity_name, sizeof(cif_info[index].mipi_csi2_sd_path));
        }
    }

    entity = media_get_entity_by_name(device, "stream_cif", strlen("stream_cif"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(cif_info[index].stream_cif_path, entity_name, sizeof(cif_info[index].stream_cif_path));
        }
    }

    entity = media_get_entity_by_name(device, "rkcif-dvp-sof", strlen("rkcif-dvp-sof"));
    if(entity) {
        entity_name = media_entity_get_devname (entity);
        if(entity_name) {
            strncpy(cif_info[index].dvp_sof_sd_path, entity_name, sizeof(cif_info[index].dvp_sof_sd_path));
        }
    }

    return &cif_info[index];
}


static
XCamReturn
SensorInfoCopy(rk_sensor_full_info_t *finfo, rk_aiq_static_info_t *info) {
    int fs_num, i = 0;
    rk_aiq_sensor_info_t *sinfo = NULL;

    //info->media_node_index = finfo->media_node_index;
    strncpy(info->lens_info.len_name, finfo->len_name.c_str(), sizeof(info->lens_info.len_name));
    sinfo = &info->sensor_info;
    strncpy(sinfo->sensor_name, finfo->sensor_name.c_str(), sizeof(sinfo->sensor_name));
    fs_num = finfo->frame_size.size();
    if (fs_num) {
        for (auto iter = finfo->frame_size.begin(); iter != finfo->frame_size.end() && i < 10; ++iter, i++) {
            sinfo->support_fmt[i].width = (*iter).width;
            sinfo->support_fmt[i].height = (*iter).height;
            sinfo->support_fmt[i].format = (*iter).format;
            sinfo->support_fmt[i].fps = (*iter).fps;
            sinfo->support_fmt[i].hdr_mode = (*iter).hdr_mode;
        }
        sinfo->num = i;
    }

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
CamHwIsp20::selectIqFile(const char* sns_ent_name, char* iqfile_name)
{
    if (!sns_ent_name || !iqfile_name)
        return XCAM_RETURN_ERROR_SENSOR;
    std::map<std::string, SmartPtr<rk_sensor_full_info_t>>::iterator it;
    const struct rkmodule_base_inf* base_inf = NULL;
    const char *sensor_name, *module_name, *lens_name;
    char sensor_name_full[32];
    std::string str(sns_ent_name);

    if ((it = CamHwIsp20::mSensorHwInfos.find(str)) == CamHwIsp20::mSensorHwInfos.end()) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "can't find sensor %s", sns_ent_name);
        return XCAM_RETURN_ERROR_SENSOR;
    }
    base_inf = &(it->second.ptr()->mod_info.base);
    if (!strlen(base_inf->module) || !strlen(base_inf->sensor) ||
            !strlen(base_inf->lens)) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "no camera module info, check the drv !");
        return XCAM_RETURN_ERROR_SENSOR;
    }
    sensor_name = base_inf->sensor;
    strncpy(sensor_name_full, sensor_name, 32);

    module_name = base_inf->module;
    lens_name = base_inf->lens;
    if (strlen(module_name) && strlen(lens_name)) {
        sprintf(iqfile_name, "%s_%s_%s.xml",
                sensor_name_full, module_name, lens_name);
    } else {
        sprintf(iqfile_name, "%s.xml", sensor_name_full);
    }

    return XCAM_RETURN_NO_ERROR;
}

rk_aiq_static_info_t*
CamHwIsp20::getStaticCamHwInfo(const char* sns_ent_name, uint16_t index)
{
    std::map<std::string, SmartPtr<rk_aiq_static_info_t>>::iterator it;

    if (sns_ent_name) {
        std::string str(sns_ent_name);

        it = mCamHwInfos.find(str);
        if (it != mCamHwInfos.end()) {
            LOGD_CAMHW_SUBM(ISP20HW_SUBM, "find camerainfo of %s!", sns_ent_name);
            return it->second.ptr();
        } else {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "camerainfo of %s not fount!", sns_ent_name);
        }
    } else {
        if (index >= 0 && index < mCamHwInfos.size()) {
            int i = 0;
            for (it = mCamHwInfos.begin(); it != mCamHwInfos.end(); it++, i++) {
                if (it->first == "FakeCamera")
                    index++;
                else if (i == index)
                    return it->second.ptr();
            }
        }
    }

    return NULL;
}

XCamReturn
CamHwIsp20::clearStaticCamHwInfo()
{
    /* std::map<std::string, SmartPtr<rk_aiq_static_info_t>>::iterator it1; */
    /* std::map<std::string, SmartPtr<rk_sensor_full_info_t>>::iterator it2; */

    /* for (it1 = mCamHwInfos.begin(); it1 != mCamHwInfos.end(); it1++) { */
    /*     rk_aiq_static_info_t *ptr = it1->second.ptr(); */
    /*     delete ptr; */
    /* } */
    /* for (it2 = mSensorHwInfos.begin(); it2 != mSensorHwInfos.end(); it2++) { */
    /*     rk_sensor_full_info_t *ptr = it2->second.ptr(); */
    /*     delete ptr; */
    /* } */
    mCamHwInfos.clear();
    mSensorHwInfos.clear();

    return XCAM_RETURN_NO_ERROR;
}

void
CamHwIsp20::findAttachedSubdevs(struct media_device *device, uint32_t count, rk_sensor_full_info_t *s_info)
{
    const struct media_entity_desc *entity_info = NULL;
    struct media_entity *entity = NULL;
    uint32_t k;

    for (k = 0; k < count; ++k) {
        entity = media_get_entity (device, k);
        entity_info = media_entity_get_info(entity);
        if ((NULL != entity_info) && (entity_info->type == MEDIA_ENT_T_V4L2_SUBDEV_LENS)) {
            if ((entity_info->name[0] == 'm') &&
                    (strncmp(entity_info->name, s_info->module_index_str.c_str(), 3) == 0)) {
                if (entity_info->flags == 1)
                    s_info->module_ircut_dev_name = std::string(media_entity_get_devname(entity));
                else//vcm
                    s_info->module_lens_dev_name = std::string(media_entity_get_devname(entity));
            }
        } else if ((NULL != entity_info) && (entity_info->type == MEDIA_ENT_T_V4L2_SUBDEV_FLASH)) {
            if ((entity_info->name[0] == 'm') &&
                    (strncmp(entity_info->name, s_info->module_index_str.c_str(), 3) == 0)) {

                /* check if entity name has the format string mxx_x_xxx-irxxx */
                if (strstr(entity_info->name, "-ir") != NULL) {
                    s_info->module_flash_ir_dev_name[s_info->flash_ir_num++] =
                        std::string(media_entity_get_devname(entity));
                } else
                    s_info->module_flash_dev_name[s_info->flash_num++] =
                        std::string(media_entity_get_devname(entity));
            }
        }
    }
    // query flash infos
    if (s_info->flash_num) {
        SmartPtr<FlashLightHw> fl = new FlashLightHw(s_info->module_flash_dev_name, s_info->flash_num);
        fl->init(1);
        s_info->fl_strth_adj_sup = fl->isStrengthAdj();
        fl->deinit();
    }
    if (s_info->flash_ir_num) {
        SmartPtr<FlashLightHw> fl_ir = new FlashLightHw(s_info->module_flash_ir_dev_name, s_info->flash_ir_num);
        fl_ir->init(1);
        s_info->fl_ir_strth_adj_sup = fl_ir->isStrengthAdj();
        fl_ir->deinit();
    }
}

XCamReturn
CamHwIsp20::initCamHwInfos()
{
    char sys_path[64], devpath[32];
    FILE *fp = NULL;
    struct media_device *device = NULL;
    uint32_t nents, j = 0, i = 0, node_index = 0;
    const struct media_entity_desc *entity_info = NULL;
    struct media_entity *entity = NULL;

    xcam_mem_clear (CamHwIsp20::mIspHwInfos);
    xcam_mem_clear (CamHwIsp20::mCifHwInfos);

    while (i < MAX_MEDIA_INDEX) {
        node_index = i;
        snprintf (sys_path, 64, "/dev/media%d", i++);
        fp = fopen (sys_path, "r");
        if (!fp)
            continue;
        fclose (fp);
        device = media_device_new (sys_path);

        /* Enumerate entities, pads and links. */
        media_device_enumerate (device);

        rk_aiq_isp_t* isp_info = NULL;
        rk_aiq_cif_info_t* cif_info = NULL;
        bool dvp_itf = false;
        if (strcmp(device->info.model, "rkispp0") == 0 ||
                strcmp(device->info.model, "rkispp1") == 0 ||
                strcmp(device->info.model, "rkispp2") == 0 ||
                strcmp(device->info.model, "rkispp3") == 0 ||
                strcmp(device->info.model, "rkispp") == 0) {
            get_ispp_subdevs(device, sys_path, CamHwIsp20::mIspHwInfos.ispp_info);
            goto media_unref;
        } else if (strcmp(device->info.model, "rkisp0") == 0 ||
                   strcmp(device->info.model, "rkisp1") == 0 ||
                   strcmp(device->info.model, "rkisp2") == 0 ||
                   strcmp(device->info.model, "rkisp3") == 0 ||
                   strcmp(device->info.model, "rkisp") == 0) {
            isp_info = get_isp_subdevs(device, sys_path, CamHwIsp20::mIspHwInfos.isp_info);
            isp_info->valid = true;
        } else if (strcmp(device->info.model, "rkcif") == 0 ||
                   strcmp(device->info.model, "rkcif_dvp") == 0 ||
                   strcmp(device->info.model, "rkcif_mipi_lvds") == 0 ||
                   strcmp(device->info.model, "rkcif_lite_mipi_lvds") == 0) {
            cif_info = get_cif_subdevs(device, sys_path, CamHwIsp20::mCifHwInfos.cif_info);
            strncpy(cif_info->model_str, device->info.model, sizeof(cif_info->model_str));

            if (strcmp(device->info.model, "rkcif_dvp") == 0)
                dvp_itf = true;
        } else {
            goto media_unref;
        }

        nents = media_get_entities_count (device);
        for (j = 0; j < nents; ++j) {
            entity = media_get_entity (device, j);
            entity_info = media_entity_get_info(entity);
            if ((NULL != entity_info) && (entity_info->type == MEDIA_ENT_T_V4L2_SUBDEV_SENSOR)) {
                rk_aiq_static_info_t *info = new rk_aiq_static_info_t();
                rk_sensor_full_info_t *s_full_info = new rk_sensor_full_info_t();
                s_full_info->media_node_index = node_index;
                strncpy(devpath, media_entity_get_devname(entity), sizeof(devpath));
                s_full_info->device_name = std::string(devpath);
                s_full_info->sensor_name = std::string(entity_info->name);
                s_full_info->parent_media_dev = std::string(sys_path);
                parse_module_info(s_full_info);
                get_sensor_caps(s_full_info);

                if (cif_info) {
                    s_full_info->linked_to_isp = false;
                    s_full_info->cif_info = cif_info;
                    s_full_info->isp_info = NULL;
                    s_full_info->dvp_itf = dvp_itf;
                } else if (isp_info) {
                    s_full_info->linked_to_isp = true;
                    isp_info->linked_sensor = true;
                    s_full_info->isp_info = isp_info;
                } else {
                    LOGE_CAMHW_SUBM(ISP20HW_SUBM, "sensor device mount error!\n");
                }

                findAttachedSubdevs(device, nents, s_full_info);
                SensorInfoCopy(s_full_info, info);
                info->has_lens_vcm = s_full_info->module_lens_dev_name.empty() ? false : true;
                info->has_fl = s_full_info->flash_num > 0 ? true : false;
                info->has_irc = s_full_info->module_ircut_dev_name.empty() ? false : true;
                info->fl_strth_adj_sup = s_full_info->fl_ir_strth_adj_sup;
                info->fl_ir_strth_adj_sup = s_full_info->fl_ir_strth_adj_sup;
                CamHwIsp20::mSensorHwInfos[s_full_info->sensor_name] = s_full_info;
                CamHwIsp20::mCamHwInfos[s_full_info->sensor_name] = info;
            }
        }

media_unref:
        media_device_unref (device);
    }

    std::map<std::string, SmartPtr<rk_sensor_full_info_t>>::iterator iter;
    for(iter = CamHwIsp20::mSensorHwInfos.begin(); \
            iter != CamHwIsp20::mSensorHwInfos.end(); iter++) {
        LOGI_CAMHW_SUBM(ISP20HW_SUBM, "match the sensor_name(%s) media link\n", (iter->first).c_str());
        SmartPtr<rk_sensor_full_info_t> s_full_info = iter->second;

        /*
         * The ISP and ISPP match links through the media device model
         */
        if (s_full_info->linked_to_isp) {
            for (i = 0; i < MAX_CAM_NUM; i++) {
                LOGI_CAMHW_SUBM(ISP20HW_SUBM, "isp model_idx: %d, ispp(%d) model_idx: %d\n",
                                s_full_info->isp_info->model_idx,
                                i,
                                CamHwIsp20::mIspHwInfos.ispp_info[i].model_idx);

                if (s_full_info->isp_info->model_idx == CamHwIsp20::mIspHwInfos.ispp_info[i].model_idx) {
                    s_full_info->ispp_info = &CamHwIsp20::mIspHwInfos.ispp_info[i];
                    LOGI_CAMHW_SUBM(ISP20HW_SUBM, "isp(%d) link to ispp(%d)\n",
                                    s_full_info->isp_info->model_idx,
                                    CamHwIsp20::mIspHwInfos.ispp_info[i].model_idx);
                    CamHwIsp20::mCamHwInfos[s_full_info->sensor_name]->sensor_info.binded_strm_media_idx =
                        atoi(s_full_info->ispp_info->media_dev_path + strlen("/dev/media"));
                    LOGI_CAMHW_SUBM(ISP20HW_SUBM, "sensor %s adapted to pp media %d:%s\n",
                                    s_full_info->sensor_name.c_str(),
                                    CamHwIsp20::mCamHwInfos[s_full_info->sensor_name]->sensor_info.binded_strm_media_idx,
                                    s_full_info->ispp_info->media_dev_path);
                    break;
                }
            }
        } else {
            /*
             * Determine which isp that vipCap is linked
             */
            for (i = 0; i < MAX_CAM_NUM; i++) {
                rk_aiq_isp_t* isp_info = &CamHwIsp20::mIspHwInfos.isp_info[i];

                LOGI_CAMHW_SUBM(ISP20HW_SUBM, "vicap %s, linked_vicap %s",
                                s_full_info->cif_info->model_str, isp_info->linked_vicap);
                if (strcmp(s_full_info->cif_info->model_str, isp_info->linked_vicap) == 0) {
                    s_full_info->isp_info = &CamHwIsp20::mIspHwInfos.isp_info[i];
                    s_full_info->ispp_info = &CamHwIsp20::mIspHwInfos.ispp_info[i];
                    LOGI_CAMHW_SUBM(ISP20HW_SUBM, "vicap link to isp(%d) to ispp(%d)\n",
                                    s_full_info->isp_info->model_idx,
                                    s_full_info->ispp_info->model_idx);
                    CamHwIsp20::mCamHwInfos[s_full_info->sensor_name]->sensor_info.binded_strm_media_idx =
                        atoi(s_full_info->ispp_info->media_dev_path + strlen("/dev/media"));
                    LOGI_CAMHW_SUBM(ISP20HW_SUBM, "sensor %s adapted to pp media %d:%s\n",
                                    s_full_info->sensor_name.c_str(),
                                    CamHwIsp20::mCamHwInfos[s_full_info->sensor_name]->sensor_info.binded_strm_media_idx,
                                    s_full_info->ispp_info->media_dev_path);
                    CamHwIsp20::mIspHwInfos.isp_info[i].linked_sensor = true;
                    break;
                }
            }
        }

        if (!s_full_info->isp_info || !s_full_info->ispp_info) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "get isp or ispp info fail, something gos wrong!");
        } else {
            //CamHwIsp20::mCamHwInfos[s_full_info->sensor_name]->linked_isp_info = *s_full_info->isp_info;
            //CamHwIsp20::mCamHwInfos[s_full_info->sensor_name]->linked_ispp_info = *s_full_info->ispp_info;
        }
    }

    /* Look for free isp&ispp link to fake camera */
    for (i = 0; i < 2; i++) {
        if (CamHwIsp20::mIspHwInfos.isp_info[i].valid &&
                !CamHwIsp20::mIspHwInfos.isp_info[i].linked_sensor) {
            rk_aiq_static_info_t *hwinfo = new rk_aiq_static_info_t();
            rk_sensor_full_info_t *fullinfo = new rk_sensor_full_info_t();

            fullinfo->isp_info = &CamHwIsp20::mIspHwInfos.isp_info[i];
            fullinfo->ispp_info = &CamHwIsp20::mIspHwInfos.ispp_info[i];
            fullinfo->media_node_index = -1;
            fullinfo->device_name = std::string("/dev/null");
            fullinfo->sensor_name = std::string("FakeCamera");
            fullinfo->parent_media_dev = std::string("/dev/null");
            fullinfo->linked_to_isp = true;

            hwinfo->sensor_info.binded_strm_media_idx =
                atoi(fullinfo->ispp_info->media_dev_path + strlen("/dev/media"));
            hwinfo->sensor_info.support_fmt[0].hdr_mode = NO_HDR;
            hwinfo->sensor_info.support_fmt[1].hdr_mode = HDR_X2;
            hwinfo->sensor_info.support_fmt[2].hdr_mode = HDR_X3;
            hwinfo->sensor_info.num = 3;
            CamHwIsp20::mIspHwInfos.isp_info[i].linked_sensor = true;

            SensorInfoCopy(fullinfo, hwinfo);
            hwinfo->has_lens_vcm = false;
            hwinfo->has_fl = false;
            hwinfo->has_irc = false;
            hwinfo->fl_strth_adj_sup = 0;
            hwinfo->fl_ir_strth_adj_sup = 0;
            CamHwIsp20::mSensorHwInfos[fullinfo->sensor_name] = fullinfo;
            CamHwIsp20::mCamHwInfos[fullinfo->sensor_name] = hwinfo;
            LOGI_CAMHW_SUBM(ISP20HW_SUBM, "fake camera link to isp(%d) to ispp(%d)\n",
                            fullinfo->isp_info->model_idx,
                            fullinfo->ispp_info->model_idx);
            LOGI_CAMHW_SUBM(ISP20HW_SUBM, "sensor %s adapted to pp media %d:%s\n",
                            fullinfo->sensor_name.c_str(),
                            CamHwIsp20::mCamHwInfos[fullinfo->sensor_name]->sensor_info.binded_strm_media_idx,
                            fullinfo->ispp_info->media_dev_path);
            break;
        }
    }
    if (i == 2) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "No free isp&ispp needed by fake camera!");
    }

    get_isp_ver(&CamHwIsp20::mIspHwInfos);
    return XCAM_RETURN_NO_ERROR;
}

const char*
CamHwIsp20::getBindedSnsEntNmByVd(const char* vd)
{
    if (!vd)
        return NULL;

    std::map<std::string, SmartPtr<rk_sensor_full_info_t>>::iterator iter;
    for(iter = CamHwIsp20::mSensorHwInfos.begin(); \
            iter != CamHwIsp20::mSensorHwInfos.end(); iter++) {
        SmartPtr<rk_sensor_full_info_t> s_full_info = iter->second;
        if (strstr(s_full_info->ispp_info->pp_m_bypass_path, vd) ||
                strstr(s_full_info->ispp_info->pp_scale0_path, vd) ||
                strstr(s_full_info->ispp_info->pp_scale1_path, vd) ||
                strstr(s_full_info->ispp_info->pp_scale2_path, vd))
            return  s_full_info->sensor_name.c_str();
    }

    return NULL;
}

XCamReturn
CamHwIsp20::init(const char* sns_ent_name)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    SmartPtr<Isp20PollThread> isp20Pollthread;
    SmartPtr<PollThread> isp20LumaPollthread;
    SmartPtr<PollThread> isp20IsppPollthread;
    SmartPtr<BaseSensorHw> sensorHw;
    SmartPtr<LensHw> lensHw;
    SmartPtr<V4l2Device> mipi_tx_devs[3];
    SmartPtr<V4l2Device> mipi_rx_devs[3];
    std::string sensor_name(sns_ent_name);

    ENTER_CAMHW_FUNCTION();


    std::map<std::string, SmartPtr<rk_sensor_full_info_t>>::iterator it;
    if ((it = mSensorHwInfos.find(sensor_name)) == mSensorHwInfos.end()) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "can't find sensor %s", sns_ent_name);
        return XCAM_RETURN_ERROR_SENSOR;
    }
    rk_sensor_full_info_t *s_info = it->second.ptr();
    sensorHw = new SensorHw(s_info->device_name.c_str());
    mSensorDev = sensorHw;
    mSensorDev->open();

    strncpy(sns_name, sns_ent_name, sizeof(sns_name));

    if (s_info->linked_to_isp)
        _linked_to_isp = true;

    mIspCoreDev = new V4l2SubDevice(s_info->isp_info->isp_dev_path);
    mIspCoreDev->open();
    if (s_info->linked_to_isp)
        mIspCoreDev->subscribe_event(V4L2_EVENT_FRAME_SYNC);

    if (strlen(s_info->isp_info->mipi_luma_path)) {
        if (_linked_to_isp) {
            mIspLumaDev = new V4l2Device(s_info->isp_info->mipi_luma_path);
        } else
            mIspLumaDev = new V4l2Device(s_info->cif_info->mipi_luma_path);
        mIspLumaDev->open();
    }
    _ispp_sd = new V4l2SubDevice(s_info->ispp_info->pp_dev_path);
    _ispp_sd ->open();
    LOGI_CAMHW_SUBM(ISP20HW_SUBM, "pp_dev_path: %s\n", s_info->ispp_info->pp_dev_path);

#ifndef DISABLE_PP_STATS
    mIsppStatsDev = new V4l2Device(s_info->ispp_info->pp_stats_path);
    mIsppStatsDev->open();
#endif
    mIsppParamsDev = new V4l2Device (s_info->ispp_info->pp_input_params_path);
    mIsppParamsDev->open();

    mIspStatsDev = new V4l2Device (s_info->isp_info->stats_path);
    mIspStatsDev->open();
    mIspParamsDev = new V4l2Device (s_info->isp_info->input_params_path);
    mIspParamsDev->open();

    lensHw = new LensHw(s_info->module_lens_dev_name.c_str());
    mLensDev = lensHw;
    mLensDev->open();

    if(!s_info->module_ircut_dev_name.empty()) {
        mIrcutDev = new V4l2SubDevice(s_info->module_ircut_dev_name.c_str());
        mIrcutDev->open();
    }
    /*
     * for _mipi_tx_devs, index 0 refer to short frame always, inedex 1 refer
     * to middle frame always, index 2 refert to long frame always.
     * for CIF usecase, because mipi_id0 refert to long frame always, so we
     * should know the HDR mode firstly befor building the relationship between
     * _mipi_tx_devs array and mipi_idx. here we just set the mipi_idx to
     * _mipi_tx_devs, we will build the real relation in start.
     * for CIF usecase, rawwr2_path is always connected to _mipi_tx_devs[0],
     * rawwr0_path is always connected to _mipi_tx_devs[1], and rawwr1_path is always
     * connected to _mipi_tx_devs[0]
     */
    //short frame
    if (strlen(s_info->isp_info->rawrd2_s_path)) {
        if (_linked_to_isp)
            _mipi_tx_devs[0] = new V4l2Device (s_info->isp_info->rawwr2_path);//rkisp_rawwr2
        else {
            if (s_info->dvp_itf)
                _mipi_tx_devs[0] = new V4l2Device (s_info->cif_info->stream_cif_path);
            else
                _mipi_tx_devs[0] = new V4l2Device (s_info->cif_info->mipi_id0);
        }
        _mipi_tx_devs[0]->open();

        _mipi_rx_devs[0] = new V4l2Device (s_info->isp_info->rawrd2_s_path);//rkisp_rawrd2_s
        _mipi_rx_devs[0]->open();
        _mipi_rx_devs[0]->set_mem_type(V4L2_MEMORY_DMABUF);
    }
    //mid frame
    if (strlen(s_info->isp_info->rawrd0_m_path)) {
        if (_linked_to_isp)
            _mipi_tx_devs[1] = new V4l2Device (s_info->isp_info->rawwr0_path);//rkisp_rawwr0
        else {
            if (!s_info->dvp_itf)
                _mipi_tx_devs[1] = new V4l2Device (s_info->cif_info->mipi_id1);
        }

        if (_mipi_tx_devs[1].ptr())
            _mipi_tx_devs[1]->open();
        _mipi_rx_devs[1] = new V4l2Device (s_info->isp_info->rawrd0_m_path);//rkisp_rawrd0_m
        _mipi_rx_devs[1]->open();
        _mipi_rx_devs[1]->set_mem_type(V4L2_MEMORY_DMABUF);
    }
    //long frame
    if (strlen(s_info->isp_info->rawrd1_l_path)) {
        if (_linked_to_isp)
            _mipi_tx_devs[2] = new V4l2Device (s_info->isp_info->rawwr1_path);//rkisp_rawwr1
        else {
            if (!s_info->dvp_itf)
                _mipi_tx_devs[2] = new V4l2Device (s_info->cif_info->mipi_id2);//rkisp_rawwr1
        }
        if (_mipi_tx_devs[2].ptr())
            _mipi_tx_devs[2]->open();
        _mipi_rx_devs[2] = new V4l2Device (s_info->isp_info->rawrd1_l_path);//rkisp_rawrd1_l
        _mipi_rx_devs[2]->open();

        _mipi_rx_devs[2]->set_mem_type(V4L2_MEMORY_DMABUF);
    }
    for (int i = 0; i < 3; i++) {
        if (_linked_to_isp) {
            if (_mipi_tx_devs[i].ptr())
                _mipi_tx_devs[i]->set_buffer_count(ISP_TX_BUF_NUM);
            if (_mipi_rx_devs[i].ptr())
                _mipi_rx_devs[i]->set_buffer_count(ISP_TX_BUF_NUM);
        } else {
            if (_mipi_tx_devs[i].ptr())
                _mipi_tx_devs[i]->set_buffer_count(VIPCAP_TX_BUF_NUM);
            if (_mipi_rx_devs[i].ptr())
                _mipi_rx_devs[i]->set_buffer_count(VIPCAP_TX_BUF_NUM);
        }
        if (_mipi_tx_devs[i].ptr())
            _mipi_tx_devs[i]->set_buf_sync (true);
        if (_mipi_rx_devs[i].ptr())
            _mipi_rx_devs[i]->set_buf_sync (true);
    }

    if (!_linked_to_isp) {
        if (strlen(s_info->cif_info->mipi_csi2_sd_path) > 0)
            _cif_csi2_sd = new V4l2SubDevice (s_info->cif_info->mipi_csi2_sd_path);
        else if (strlen(s_info->cif_info->lvds_sd_path) > 0)
            _cif_csi2_sd = new V4l2SubDevice (s_info->cif_info->lvds_sd_path);
        else if (strlen(s_info->cif_info->dvp_sof_sd_path) > 0)
            _cif_csi2_sd = new V4l2SubDevice (s_info->cif_info->dvp_sof_sd_path);
        else
            LOGW_CAMHW_SUBM(ISP20HW_SUBM, "_cif_csi2_sd is null! \n");
        _cif_csi2_sd->open();
        _cif_csi2_sd->subscribe_event(V4L2_EVENT_FRAME_SYNC);
    }

    mIspSpDev = new V4l2Device (s_info->isp_info->self_path);//rkisp_selfpath
    mIspSpDev->open();
    mIspSpThread = new Isp20SpThread();

    isp20Pollthread = new Isp20PollThread();
    isp20Pollthread->set_mipi_devs(_mipi_tx_devs, _mipi_rx_devs, mIspCoreDev);
    isp20Pollthread->set_event_handle_dev(sensorHw);
    if(lensHw.ptr()) {
        isp20Pollthread->set_focus_handle_dev(lensHw);
        //isp20Pollthread->set_iris_handle_dev(lensHw);
    }
    isp20Pollthread->setCamHw(this);
    //isp20Pollthread->set_mipi_devs(_mipi_tx_devs, _mipi_rx_devs, mIspCoreDev);

    mPollthread = isp20Pollthread;
    if (_linked_to_isp)
        mPollthread->set_event_device(mIspCoreDev);
    else
        mPollthread->set_event_device(_cif_csi2_sd);
    mPollthread->set_isp_stats_device(mIspStatsDev);
    mPollthread->set_isp_params_devices(mIspParamsDev, mIsppParamsDev);
    mPollthread->set_poll_callback (this);

    if (mIspLumaDev.ptr()) {
        isp20LumaPollthread = new PollThread();
        mPollLumathread = isp20LumaPollthread;
        mPollLumathread->set_isp_luma_device(mIspLumaDev);
        mPollLumathread->set_poll_callback (this);
    }
#ifndef DISABLE_PP
#ifndef DISABLE_PP_STATS
    isp20IsppPollthread = new PollThread();
    mPollIsppthread = isp20IsppPollthread;
    mPollIsppthread->set_ispp_stats_device(mIsppStatsDev);
    mPollIsppthread->set_poll_callback (this);
#endif
#endif

    if (s_info->flash_num) {
        mFlashLight = new FlashLightHw(s_info->module_flash_dev_name, s_info->flash_num);
        mFlashLight->init(s_info->flash_num);
    }
    if (s_info->flash_ir_num) {
        mFlashLightIr = new FlashLightHw(s_info->module_flash_ir_dev_name, s_info->flash_ir_num);
        mFlashLightIr->init(s_info->flash_ir_num);
    }

    xcam_mem_clear (_full_active_isp_params);
    xcam_mem_clear (_full_active_ispp_params);

    _state = CAM_HW_STATE_INITED;

    EXIT_CAMHW_FUNCTION();

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
CamHwIsp20::deInit()
{
    if (mFlashLight.ptr())
        mFlashLight->deinit();
    if (mFlashLightIr.ptr())
        mFlashLightIr->deinit();

    std::map<std::string, SmartPtr<rk_sensor_full_info_t>>::iterator it;
    if ((it = mSensorHwInfos.find(sns_name)) == mSensorHwInfos.end()) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "can't find sensor %s", sns_name);
        return XCAM_RETURN_ERROR_SENSOR;
    }

    rk_sensor_full_info_t *s_info = it->second.ptr();
    int isp_index = s_info->isp_info->model_idx;
    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "sensor_name(%s) is linked to isp_index(%d)",
                    sns_name, isp_index);
    if (_hdr_mode != RK_AIQ_WORKING_MODE_NORMAL && \
            !mNormalNoReadBack) {
        setupHdrLink(RK_AIQ_WORKING_MODE_ISP_HDR3, isp_index, false);
    } else {
        if (!_linked_to_isp) {
            setupHdrLink(RK_AIQ_WORKING_MODE_ISP_HDR3, isp_index, false);
            setupHdrLink_vidcap(_hdr_mode, isp_index, false);
        }
    }

    _state = CAM_HW_STATE_INVALID;
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
CamHwIsp20::setupPipelineFmtCif(struct v4l2_subdev_selection& sns_sd_sel,
                                struct v4l2_subdev_format& sns_sd_fmt,
                                __u32 sns_v4l_pix_fmt)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    // TODO: set cif crop according to sensor crop bounds

    // set mipi tx,rx fmt
    // for isp: same as sensor crop bounds
    struct v4l2_format format;
    memset(&format, 0, sizeof(format));

    for (int i = 0; i < 3; i++) {

        if (_mipi_tx_devs[i].ptr())
            _mipi_tx_devs[i]->get_format (format);
        if (format.fmt.pix.width != sns_sd_sel.r.width ||
                format.fmt.pix.height != sns_sd_sel.r.height ||
                format.fmt.pix.pixelformat != sns_v4l_pix_fmt) {

            if (_mipi_tx_devs[i].ptr())
                _mipi_tx_devs[i]->set_format(sns_sd_sel.r.width,
                                             sns_sd_sel.r.height,
                                             sns_v4l_pix_fmt,
                                             V4L2_FIELD_NONE,
                                             0);
        }

        if (_mipi_rx_devs[i].ptr())
            _mipi_rx_devs[i]->get_format (format);
        if (format.fmt.pix.width != sns_sd_sel.r.width ||
                format.fmt.pix.height != sns_sd_sel.r.height ||
                format.fmt.pix.pixelformat != sns_v4l_pix_fmt) {
            if (_mipi_rx_devs[i].ptr())
                _mipi_rx_devs[i]->set_format(sns_sd_sel.r.width,
                                             sns_sd_sel.r.height,
                                             sns_v4l_pix_fmt,
                                             V4L2_FIELD_NONE,
                                             0);
        }
    }

    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "mipi tx/rx fmt info: fmt 0x%x, %dx%d !",
                    sns_v4l_pix_fmt, sns_sd_sel.r.width, sns_sd_sel.r.height);

    // set isp sink fmt, same as sensor bounds - crop
    struct v4l2_subdev_format isp_sink_fmt;

    memset(&isp_sink_fmt, 0, sizeof(isp_sink_fmt));
    isp_sink_fmt.pad = 0;
    isp_sink_fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    ret = mIspCoreDev->getFormat(isp_sink_fmt);
    if (ret) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "set mIspCoreDev fmt failed !\n");
        return ret;
    }
    isp_sink_fmt.format.width = sns_sd_sel.r.width;
    isp_sink_fmt.format.height = sns_sd_sel.r.height;
    isp_sink_fmt.format.code = sns_sd_fmt.format.code;

    ret = mIspCoreDev->setFormat(isp_sink_fmt);
    if (ret) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "set mIspCoreDev fmt failed !\n");
        return ret;
    }

    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "isp sink fmt info: fmt 0x%x, %dx%d !",
                    isp_sink_fmt.format.code, isp_sink_fmt.format.width, isp_sink_fmt.format.height);

    // set selection, isp needn't do the crop
    struct v4l2_subdev_selection aSelection;
    memset(&aSelection, 0, sizeof(aSelection));

    aSelection.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    aSelection.pad = 0;
    aSelection.flags = 0;
    aSelection.target = V4L2_SEL_TGT_CROP;
    aSelection.r.width = sns_sd_sel.r.width;
    aSelection.r.height = sns_sd_sel.r.height;
    aSelection.r.left = 0;
    aSelection.r.top = 0;
    ret = mIspCoreDev->set_selection (aSelection);
    if (ret) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "set mIspCoreDev crop failed !\n");
        return ret;
    }

    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "isp sink crop info: %dx%d@%d,%d !",
                    aSelection.r.width, aSelection.r.height,
                    aSelection.r.left, aSelection.r.top);

    // set isp rkisp-isp-subdev src crop
    aSelection.pad = 2;
#if 1 // isp src has no crop
    ret = mIspCoreDev->set_selection (aSelection);
    if (ret) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "set mIspCoreDev source crop failed !\n");
        return ret;
    }
#endif
    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "isp src crop info: %dx%d@%d,%d !",
                    aSelection.r.width, aSelection.r.height,
                    aSelection.r.left, aSelection.r.top);

    // set isp rkisp-isp-subdev src pad fmt
    struct v4l2_subdev_format isp_src_fmt;

    isp_src_fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    isp_src_fmt.pad = 2;
    ret = mIspCoreDev->getFormat(isp_src_fmt);
    if (ret) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "get mIspCoreDev src fmt failed !\n");
        return ret;
    }

    isp_src_fmt.format.width = aSelection.r.width;
    isp_src_fmt.format.height = aSelection.r.height;
    ret = mIspCoreDev->setFormat(isp_src_fmt);
    if (ret) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "set mIspCoreDev src fmt failed !\n");
        return ret;
    }

    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "isp src fmt info: fmt 0x%x, %dx%d !",
                    isp_src_fmt.format.code, isp_src_fmt.format.width, isp_src_fmt.format.height);

    return ret;

}

XCamReturn
CamHwIsp20::setupPipelineFmtIsp(struct v4l2_subdev_selection& sns_sd_sel,
                                struct v4l2_subdev_format& sns_sd_fmt,
                                __u32 sns_v4l_pix_fmt)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    // set mipi tx,rx fmt
    // for cif: same as sensor fmt

    struct v4l2_format format;
    memset(&format, 0, sizeof(format));

    for (int i = 0; i < 3; i++) {
        if (_mipi_tx_devs[i].ptr())
            _mipi_tx_devs[i]->get_format (format);
        if (format.fmt.pix.width != sns_sd_fmt.format.width ||
                format.fmt.pix.height != sns_sd_fmt.format.height ||
                format.fmt.pix.pixelformat != sns_v4l_pix_fmt) {
            if (_mipi_tx_devs[i].ptr())
                _mipi_tx_devs[i]->set_format(sns_sd_fmt.format.width,
                                             sns_sd_fmt.format.height,
                                             sns_v4l_pix_fmt,
                                             V4L2_FIELD_NONE,
                                             0);
        }
        if (_mipi_rx_devs[i].ptr())
            _mipi_rx_devs[i]->get_format (format);
        if (format.fmt.pix.width != sns_sd_fmt.format.width ||
                format.fmt.pix.height != sns_sd_fmt.format.height ||
                format.fmt.pix.pixelformat != sns_v4l_pix_fmt) {
            if (_mipi_rx_devs[i].ptr())
                _mipi_rx_devs[i]->set_format(sns_sd_fmt.format.width,
                                             sns_sd_fmt.format.height,
                                             sns_v4l_pix_fmt,
                                             V4L2_FIELD_NONE,
                                             0);
        }
    }

    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "mipi tx/rx fmt info: fmt 0x%x, %dx%d !",
                    sns_v4l_pix_fmt, sns_sd_fmt.format.width, sns_sd_fmt.format.height);

    // set isp sink fmt, same as sensor fmt
    struct v4l2_subdev_format isp_sink_fmt;

    memset(&isp_sink_fmt, 0, sizeof(isp_sink_fmt));
    isp_sink_fmt.pad = 0;
    isp_sink_fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    ret = mIspCoreDev->getFormat(isp_sink_fmt);
    if (ret) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "set mIspCoreDev fmt failed !\n");
        return ret;
    }

    isp_sink_fmt.format.width = sns_sd_fmt.format.width;
    isp_sink_fmt.format.height = sns_sd_fmt.format.height;
    isp_sink_fmt.format.code = sns_sd_fmt.format.code;

    ret = mIspCoreDev->setFormat(isp_sink_fmt);
    if (ret) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "set mIspCoreDev fmt failed !\n");
        return ret;
    }

    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "isp sink fmt info: fmt 0x%x, %dx%d !",
                    isp_sink_fmt.format.code, isp_sink_fmt.format.width, isp_sink_fmt.format.height);

    // set selection, isp do the crop
    struct v4l2_subdev_selection aSelection;
    memset(&aSelection, 0, sizeof(aSelection));

    aSelection.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    aSelection.pad = 0;
    aSelection.flags = 0;
    aSelection.target = V4L2_SEL_TGT_CROP;
    aSelection.r.width = sns_sd_sel.r.width;
    aSelection.r.height = sns_sd_sel.r.height;
    aSelection.r.left = sns_sd_sel.r.left;
    aSelection.r.top = sns_sd_sel.r.top;
    ret = mIspCoreDev->set_selection (aSelection);
    if (ret) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "set mIspCoreDev crop failed !\n");
        return ret;
    }

    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "isp sink crop info: %dx%d@%d,%d !",
                    aSelection.r.width, aSelection.r.height,
                    aSelection.r.left, aSelection.r.top);

    // set isp rkisp-isp-subdev src crop
    aSelection.pad = 2;
    aSelection.target = V4L2_SEL_TGT_CROP;
    aSelection.r.left = 0;
    aSelection.r.top = 0;
    aSelection.r.width = sns_sd_sel.r.width;
    aSelection.r.height = sns_sd_sel.r.height;
#if 1 // isp src has no crop
    ret = mIspCoreDev->set_selection (aSelection);
    if (ret) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "set mIspCoreDev source crop failed !\n");
        return ret;
    }
#endif
    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "isp src crop info: %dx%d@%d,%d !",
                    aSelection.r.width, aSelection.r.height,
                    aSelection.r.left, aSelection.r.top);

    // set isp rkisp-isp-subdev src pad fmt
    struct v4l2_subdev_format isp_src_fmt;

    isp_src_fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    isp_src_fmt.pad = 2;
    ret = mIspCoreDev->getFormat(isp_src_fmt);
    if (ret) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "get mIspCoreDev src fmt failed !\n");
        return ret;
    }

    isp_src_fmt.format.width = aSelection.r.width;
    isp_src_fmt.format.height = aSelection.r.height;
    ret = mIspCoreDev->setFormat(isp_src_fmt);
    if (ret) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "set mIspCoreDev src fmt failed !\n");
        return ret;
    }

    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "isp src fmt info: fmt 0x%x, %dx%d !",
                    isp_src_fmt.format.code, isp_src_fmt.format.width, isp_src_fmt.format.height);

    return ret;
}

XCamReturn
CamHwIsp20::setupPipelineFmt()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    // get sensor v4l2 pixfmt
    SmartPtr<BaseSensorHw> mSensorSubdev = mSensorDev.dynamic_cast_ptr<BaseSensorHw>();
    rk_aiq_exposure_sensor_descriptor sns_des;
    if (mSensorSubdev->get_format(&sns_des)) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "getSensorModeData failed \n");
        return XCAM_RETURN_ERROR_UNKNOWN;
    }
    __u32 sns_v4l_pix_fmt = sns_des.sensor_pixelformat;

    struct v4l2_subdev_format sns_sd_fmt;

    // get sensor real outupt size
    sns_sd_fmt.pad = 0;
    sns_sd_fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    ret = mSensorDev->getFormat(sns_sd_fmt);
    if (ret) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "get sensor fmt failed !\n");
        return ret;
    }

    // get sensor crop bounds
    struct v4l2_subdev_selection sns_sd_sel;
    memset(&sns_sd_sel, 0, sizeof(sns_sd_sel));

    ret = mSensorDev->get_selection(0, V4L2_SEL_TGT_CROP_BOUNDS, sns_sd_sel);
    if (ret) {
        LOGW_CAMHW_SUBM(ISP20HW_SUBM, "get_selection failed !\n");
        // TODO, some sensor driver has not implemented this
        // ioctl now
        sns_sd_sel.r.width = sns_sd_fmt.format.width;
        sns_sd_sel.r.height = sns_sd_fmt.format.height;
        ret = XCAM_RETURN_NO_ERROR;
    }

    if (!_linked_to_isp && _crop_rect.width && _crop_rect.height) {
        struct v4l2_format mipi_tx_fmt;
        memset(&mipi_tx_fmt, 0, sizeof(mipi_tx_fmt));
        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "vicap get_crop %dx%d@%d,%d\n",
                        _crop_rect.width, _crop_rect.height, _crop_rect.left, _crop_rect.top);
        ret = _mipi_tx_devs[0]->get_format(mipi_tx_fmt);
        mipi_tx_fmt.fmt.pix.width = _crop_rect.width;
        mipi_tx_fmt.fmt.pix.height = _crop_rect.height;
        ret = _mipi_tx_devs[0]->set_format(mipi_tx_fmt);
        sns_sd_sel.r.width = _crop_rect.width;
        sns_sd_sel.r.height = _crop_rect.height;
        sns_sd_fmt.format.width = _crop_rect.width;
        sns_sd_fmt.format.height = _crop_rect.height;
        ret = XCAM_RETURN_NO_ERROR;
    }

    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "sensor fmt info: bounds %dx%d, crop %dx%d@%d,%d !",
                    sns_sd_sel.r.width, sns_sd_sel.r.height,
                    sns_sd_fmt.format.width, sns_sd_fmt.format.height,
                    sns_sd_sel.r.left, sns_sd_sel.r.top);

    if (_linked_to_isp)
        ret = setupPipelineFmtIsp(sns_sd_sel, sns_sd_fmt, sns_v4l_pix_fmt);
    else
        ret = setupPipelineFmtCif(sns_sd_sel, sns_sd_fmt, sns_v4l_pix_fmt);

    if (ret) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "set ispcore fmt failed !\n");
        return ret;
    }

    struct v4l2_subdev_format isp_src_fmt;

    isp_src_fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    isp_src_fmt.pad = 2;
    ret = mIspCoreDev->getFormat(isp_src_fmt);

    // set ispp format, same as isp_src_fmt
    isp_src_fmt.pad = 0;
    ret = _ispp_sd->setFormat(isp_src_fmt);
    if (ret) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "set _ispp_sd sink fmt failed !\n");
        return ret;
    }

    struct v4l2_subdev_selection aSelection;
    memset(&aSelection, 0, sizeof(aSelection));

    aSelection.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    aSelection.pad = 0;
    aSelection.target = V4L2_SEL_TGT_CROP_BOUNDS;
    aSelection.flags = 0;
    aSelection.r.left = 0;
    aSelection.r.top = 0;
    aSelection.r.width = isp_src_fmt.format.width;
    aSelection.r.height = isp_src_fmt.format.height;
#if 0
    ret = _ispp_sd->set_selection (aSelection);
    if (ret) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "set _ispp_sd crop bound failed !\n");
        return ret;
    }
#endif
    aSelection.target = V4L2_SEL_TGT_CROP;
    ret = _ispp_sd->set_selection (aSelection);
    if (ret) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "set _ispp_sd crop failed !\n");
        return ret;
    }

    // set sp format to NV12 as default

    if (mIspSpDev.ptr()) {
        struct v4l2_format fmt;
        ret = mIspSpDev->get_format (fmt);
        if (ret) {
            LOGW_CAMHW_SUBM(ISP20HW_SUBM, "get mIspSpDev fmt failed !\n");
            //return;
        }
        if (V4L2_PIX_FMT_FBCG == fmt.fmt.pix.pixelformat) {
            mIspSpDev->set_format(isp_src_fmt.format.width,
                                  isp_src_fmt.format.height,
                                  V4L2_PIX_FMT_NV12,
                                  V4L2_FIELD_NONE, 0);
        }
    }

    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "ispp sd fmt info: %dx%d",
                    isp_src_fmt.format.width, isp_src_fmt.format.height);

    return ret;
}

XCamReturn
CamHwIsp20::setupHdrLink_vidcap(int hdr_mode, int cif_index, bool enable)
{
    media_device *device = NULL;
    media_entity *entity = NULL;
    media_pad *src_pad_s = NULL, *src_pad_m = NULL, *src_pad_l = NULL, *sink_pad = NULL;

    // TODO: have some bugs now
    return XCAM_RETURN_NO_ERROR;

    if (_linked_to_isp)
        return XCAM_RETURN_NO_ERROR;

    // TODO: normal mode
    device = media_device_new (mCifHwInfos.cif_info[cif_index].media_dev_path);

    /* Enumerate entities, pads and links. */
    media_device_enumerate (device);
    entity = media_get_entity_by_name(device, "rockchip-mipi-csi2", strlen("rockchip-mipi-csi2"));
    if(entity) {
        src_pad_s = (media_pad *)media_entity_get_pad(entity, 1);
        if (!src_pad_s) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "get rockchip-mipi-csi2 source pad0 failed !\n");
            goto FAIL;
        }
    } else {
        entity = media_get_entity_by_name(device, "rkcif-lvds-subdev", strlen("rkcif-lvds-subdev"));
        if(entity) {
            src_pad_s = (media_pad *)media_entity_get_pad(entity, 1);
            if (!src_pad_s) {
                LOGE_CAMHW_SUBM(ISP20HW_SUBM, "get rkcif-lvds-subdev source pad0 failed !\n");
                goto FAIL;
            }
        } else {
            entity = media_get_entity_by_name(device, "rkcif-lite-lvds-subdev", strlen("rkcif-lite-lvds-subdev"));
            if(entity) {
                src_pad_s = (media_pad *)media_entity_get_pad(entity, 1);
                if (!src_pad_s) {
                    LOGE_CAMHW_SUBM(ISP20HW_SUBM, "get rkcif-lite-lvds-subdev source pad0 failed !\n");
                    goto FAIL;
                }
            }
        }
    }

    entity = media_get_entity_by_name(device, "stream_cif_mipi_id0", strlen("stream_cif_mipi_id0"));
    if(entity) {
        sink_pad = (media_pad *)media_entity_get_pad(entity, 0);
        if (!sink_pad) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "get HDR pad s failed!\n");
            goto FAIL;
        }
    }
    if (enable)
        media_setup_link(device, src_pad_s, sink_pad, MEDIA_LNK_FL_ENABLED);
    else
        media_setup_link(device, src_pad_s, sink_pad, 0);

    entity = media_get_entity_by_name(device, "rockchip-mipi-csi2", strlen("rockchip-mipi-csi2"));
    if(entity) {
        src_pad_m = (media_pad *)media_entity_get_pad(entity, 2);
        if (!src_pad_m) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "get rockchip-mipi-csi2 source pad0 failed !\n");
            goto FAIL;
        }
    } else {
        entity = media_get_entity_by_name(device, "rkcif-lvds-subdev", strlen("rkcif-lvds-subdev"));
        if(entity) {
            src_pad_m = (media_pad *)media_entity_get_pad(entity, 2);
            if (!src_pad_m) {
                LOGE_CAMHW_SUBM(ISP20HW_SUBM, "get rkcif-lvds-subdev source pad0 failed !\n");
                goto FAIL;
            }
        } else {
            entity = media_get_entity_by_name(device, "rkcif-lite-lvds-subdev", strlen("rkcif-lite-lvds-subdev"));
            if(entity) {
                src_pad_m = (media_pad *)media_entity_get_pad(entity, 2);
                if (!src_pad_m) {
                    LOGE_CAMHW_SUBM(ISP20HW_SUBM, "get rkcif-lite-lvds-subdev source pad0 failed !\n");
                    goto FAIL;
                }
            }
        }
    }

    entity = media_get_entity_by_name(device, "stream_cif_mipi_id1", strlen("stream_cif_mipi_id1"));
    if(entity) {
        sink_pad = (media_pad *)media_entity_get_pad(entity, 0);
        if (!sink_pad) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "get HDR pad s failed!\n");
            goto FAIL;
        }
    }
    if (enable)
        media_setup_link(device, src_pad_m, sink_pad, MEDIA_LNK_FL_ENABLED);
    else
        media_setup_link(device, src_pad_m, sink_pad, 0);

#if 0
    entity = media_get_entity_by_name(device, "rockchip-mipi-csi2", strlen("rockchip-mipi-csi2"));
    if(entity) {
        src_pad_l = (media_pad *)media_entity_get_pad(entity, 3);
        if (!src_pad_l) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "get rockchip-mipi-csi2 source pad0 failed !\n");
            goto FAIL;
        }
    }

    entity = media_get_entity_by_name(device, "stream_cif_mipi_id2", strlen("stream_cif_mipi_id2"));
    if(entity) {
        sink_pad = (media_pad *)media_entity_get_pad(entity, 0);
        if (!sink_pad) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "get HDR pad s failed!\n");
            goto FAIL;
        }
    }

    if (RK_AIQ_HDR_GET_WORKING_MODE(hdr_mode) == RK_AIQ_WORKING_MODE_ISP_HDR3) {
        if (enable)
            media_setup_link(device, src_pad_l, sink_pad, MEDIA_LNK_FL_ENABLED);
        else
            media_setup_link(device, src_pad_l, sink_pad, 0);
    } else
        media_setup_link(device, src_pad_l, sink_pad, 0);
#endif
    media_device_unref (device);
    return XCAM_RETURN_NO_ERROR;
FAIL:
    media_device_unref (device);
    return XCAM_RETURN_ERROR_FAILED;
}

XCamReturn
CamHwIsp20::setupHdrLink(int hdr_mode, int isp_index, bool enable)
{
    media_device *device = NULL;
    media_entity *entity = NULL;
    media_pad *src_pad_s = NULL, *src_pad_m = NULL, *src_pad_l = NULL, *sink_pad = NULL;

    device = media_device_new (mIspHwInfos.isp_info[isp_index].media_dev_path);

    /* Enumerate entities, pads and links. */
    media_device_enumerate (device);
    entity = media_get_entity_by_name(device, "rkisp-isp-subdev", strlen("rkisp-isp-subdev"));
    if(entity) {
        sink_pad = (media_pad *)media_entity_get_pad(entity, 0);
        if (!sink_pad) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "get HDR sink pad failed!\n");
            goto FAIL;
        }
    }

    entity = media_get_entity_by_name(device, "rkisp_rawrd2_s", strlen("rkisp_rawrd2_s"));
    if(entity) {
        src_pad_s = (media_pad *)media_entity_get_pad(entity, 0);
        if (!src_pad_s) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "get HDR source pad s failed!\n");
            goto FAIL;
        }
    }
    if (enable)
        media_setup_link(device, src_pad_s, sink_pad, MEDIA_LNK_FL_ENABLED);
    else
        media_setup_link(device, src_pad_s, sink_pad, 0);

    entity = media_get_entity_by_name(device, "rkisp_rawrd0_m", strlen("rkisp_rawrd0_m"));
    if(entity) {
        src_pad_m = (media_pad *)media_entity_get_pad(entity, 0);
        if (!src_pad_m) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "get HDR source pad m failed!\n");
            goto FAIL;
        }
    }

    if (RK_AIQ_HDR_GET_WORKING_MODE(hdr_mode) >= RK_AIQ_WORKING_MODE_ISP_HDR2 && enable) {
        media_setup_link(device, src_pad_m, sink_pad, MEDIA_LNK_FL_ENABLED);
    } else
        media_setup_link(device, src_pad_m, sink_pad, 0);

    entity = media_get_entity_by_name(device, "rkisp_rawrd1_l", strlen("rkisp_rawrd1_l"));
    if(entity) {
        src_pad_l = (media_pad *)media_entity_get_pad(entity, 0);
        if (!src_pad_l) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "get HDR source pad l failed!\n");
            goto FAIL;
        }
    }

    if (RK_AIQ_HDR_GET_WORKING_MODE(hdr_mode) == RK_AIQ_WORKING_MODE_ISP_HDR3 && enable) {
        media_setup_link(device, src_pad_l, sink_pad, MEDIA_LNK_FL_ENABLED);
    } else
        media_setup_link(device, src_pad_l, sink_pad, 0);
    media_device_unref (device);
    return XCAM_RETURN_NO_ERROR;
FAIL:
    media_device_unref (device);
    return XCAM_RETURN_ERROR_FAILED;
}

XCamReturn
CamHwIsp20::setExpDelayInfo(int mode)
{
    ENTER_CAMHW_FUNCTION();
    SmartPtr<BaseSensorHw> sensorHw;
    sensorHw = mSensorDev.dynamic_cast_ptr<BaseSensorHw>();

    if(mode != RK_AIQ_WORKING_MODE_NORMAL) {
        sensorHw->set_exp_delay_info(mCalibDb->sysContrl.exp_delay.Hdr.time_delay,
                                     mCalibDb->sysContrl.exp_delay.Hdr.gain_delay,
                                     mCalibDb->sysContrl.dcg.Hdr.support_en ? \
                                     mCalibDb->sysContrl.exp_delay.Hdr.dcg_delay : -1);

        sint32_t timeDelay = mCalibDb->sysContrl.exp_delay.Hdr.time_delay;
        sint32_t gainDelay = mCalibDb->sysContrl.exp_delay.Hdr.gain_delay;
        _exp_delay = timeDelay > gainDelay ? timeDelay : gainDelay;
    } else {
        sensorHw->set_exp_delay_info(mCalibDb->sysContrl.exp_delay.Normal.time_delay,
                                     mCalibDb->sysContrl.exp_delay.Normal.gain_delay,
                                     mCalibDb->sysContrl.dcg.Normal.support_en ? \
                                     mCalibDb->sysContrl.exp_delay.Normal.dcg_delay : -1);

        sint32_t timeDelay = mCalibDb->sysContrl.exp_delay.Normal.time_delay;
        sint32_t gainDelay = mCalibDb->sysContrl.exp_delay.Normal.gain_delay;
        _exp_delay = timeDelay > gainDelay ? timeDelay : gainDelay;
    }

    EXIT_CAMHW_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
CamHwIsp20::setLensVcmCfg()
{
    ENTER_CAMHW_FUNCTION();
    SmartPtr<LensHw> lensHw = mLensDev.dynamic_cast_ptr<LensHw>();
    rk_aiq_lens_vcmcfg old_cfg, new_cfg;
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    if (lensHw.ptr()) {
        ret = lensHw->getLensVcmCfg(old_cfg);
        if (ret != XCAM_RETURN_NO_ERROR)
            return ret;

        new_cfg = old_cfg;
        if (mCalibDb->af.vcmcfg.start_current != -1)
            new_cfg.start_ma = mCalibDb->af.vcmcfg.start_current;
        if (mCalibDb->af.vcmcfg.rated_current != -1)
            new_cfg.rated_ma = mCalibDb->af.vcmcfg.rated_current;
        if (mCalibDb->af.vcmcfg.step_mode != -1)
            new_cfg.step_mode = mCalibDb->af.vcmcfg.step_mode;

        if (memcmp(&new_cfg, &old_cfg, sizeof(new_cfg)) != 0)
            ret = lensHw->setLensVcmCfg(new_cfg);
    }
    EXIT_CAMHW_FUNCTION();
    return ret;
}

void
CamHwIsp20::prepare_cif_mipi()
{
    SmartPtr<Isp20PollThread> isp20Pollthread = mPollthread.dynamic_cast_ptr<Isp20PollThread>();
    SmartPtr<V4l2Device> mipi_tx_devs_tmp[3] =
    {
        _mipi_tx_devs[0],
        _mipi_tx_devs[1],
        _mipi_tx_devs[2],
    };
    // _mipi_tx_devs
    if (_hdr_mode == RK_AIQ_WORKING_MODE_NORMAL) {
        // use _mipi_tx_devs[0] only
        // id0 as normal
        // do nothing
        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "CIF tx: %s -> normal",
                        _mipi_tx_devs[0]->get_device_name());
    } else if (RK_AIQ_HDR_GET_WORKING_MODE(_hdr_mode) == RK_AIQ_WORKING_MODE_ISP_HDR2) {
        // use _mipi_tx_devs[0] and _mipi_tx_devs[1]
        // id0 as l, id1 as s
        SmartPtr<V4l2Device> tmp = mipi_tx_devs_tmp[1];
        mipi_tx_devs_tmp[1] = mipi_tx_devs_tmp[0];
        mipi_tx_devs_tmp[0] = tmp;
        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "CIF tx: %s -> long",
                        _mipi_tx_devs[1]->get_device_name());
        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "CIF tx: %s -> short",
                        _mipi_tx_devs[0]->get_device_name());
    } else if (RK_AIQ_HDR_GET_WORKING_MODE(_hdr_mode) == RK_AIQ_WORKING_MODE_ISP_HDR3) {
        // use _mipi_tx_devs[0] and _mipi_tx_devs[1]
        // id0 as l, id1 as m, id2 as s
        SmartPtr<V4l2Device> tmp = mipi_tx_devs_tmp[2];
        mipi_tx_devs_tmp[2] = mipi_tx_devs_tmp[0];
        mipi_tx_devs_tmp[0] = tmp;
        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "CIF tx: %s -> long",
                        _mipi_tx_devs[2]->get_device_name());
        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "CIF tx: %s -> middle",
                        _mipi_tx_devs[1]->get_device_name());
        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "CIF tx: %s -> short",
                        _mipi_tx_devs[0]->get_device_name());
    } else {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "wrong hdr mode: %d\n", _hdr_mode);
    }
    isp20Pollthread->set_mipi_devs(mipi_tx_devs_tmp, _mipi_rx_devs, mIspCoreDev);
    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "CIF mipi tx: %d\n", _hdr_mode);
}

XCamReturn
CamHwIsp20::prepare(uint32_t width, uint32_t height, int mode, int t_delay, int g_delay)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    SmartPtr<Isp20PollThread> isp20Pollthread;
    SmartPtr<BaseSensorHw> sensorHw;
    SmartPtr<LensHw> lensHw = mLensDev.dynamic_cast_ptr<LensHw>();

    ENTER_CAMHW_FUNCTION();

    XCAM_ASSERT (mCalibDb);

    _hdr_mode = mode;

    Isp20Params::set_working_mode(_hdr_mode);

    std::map<std::string, SmartPtr<rk_sensor_full_info_t>>::iterator it;
    if ((it = mSensorHwInfos.find(sns_name)) == mSensorHwInfos.end()) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "can't find sensor %s", sns_name);
        return XCAM_RETURN_ERROR_SENSOR;
    }

    rk_sensor_full_info_t *s_info = it->second.ptr();
    int isp_index = s_info->isp_info->model_idx;
    LOGI_CAMHW_SUBM(ISP20HW_SUBM, "sensor_name(%s) is linked to isp_index(%d)",
                    sns_name, isp_index);
    if (_linked_to_isp) {
        if (_hdr_mode != RK_AIQ_WORKING_MODE_NORMAL) {
            setupHdrLink(RK_AIQ_WORKING_MODE_ISP_HDR3, isp_index, true);
        } else {
            if (mNormalNoReadBack)
                setupHdrLink(RK_AIQ_WORKING_MODE_ISP_HDR3, isp_index, false);
            else
                setupHdrLink(RK_AIQ_WORKING_MODE_ISP_HDR3, isp_index, true);
        }
    } else {
        setupHdrLink(RK_AIQ_WORKING_MODE_ISP_HDR3, isp_index, true);
        int cif_index = s_info->cif_info->model_idx;
        setupHdrLink_vidcap(_hdr_mode, cif_index, true);
    }

    sensorHw = mSensorDev.dynamic_cast_ptr<BaseSensorHw>();
    ret = sensorHw->set_working_mode(mode);
    if (ret) {
        LOGW_CAMHW_SUBM(ISP20HW_SUBM, "set sensor mode error !");
        return ret;
    }

    setExpDelayInfo(mode);
    setLensVcmCfg();
    xcam_mem_clear(_lens_des);
    if (lensHw.ptr())
        lensHw->getLensModeData(_lens_des);

    isp20Pollthread = mPollthread.dynamic_cast_ptr<Isp20PollThread>();
    isp20Pollthread->set_working_mode(mode, _linked_to_isp);

    _ispp_module_init_ens = 0;

    ret = setupPipelineFmt();
    if (ret < 0) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "setupPipelineFmt err: %d\n", ret);
    }

    if (!_linked_to_isp)
        prepare_cif_mipi();

    isp20Pollthread->set_event_handle_dev(sensorHw);
    if ((mCalibDb->mfnr.enable && mCalibDb->mfnr.motion_detect_en) || mCalibDb->af.ldg_param.enable) {
        prepare_motion_detection(mode);
    }

    _state = CAM_HW_STATE_PREPARED;
    EXIT_CAMHW_FUNCTION();
    return ret;
}

XCamReturn
CamHwIsp20::start()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    SmartPtr<BaseSensorHw> sensorHw;
    SmartPtr<LensHw> lensHw;
    SmartPtr<Isp20PollThread> isp20Pollthread;
    SmartPtr<Isp20SpThread> isp20Spthread;

    ENTER_CAMHW_FUNCTION();

    isp20Pollthread = mPollthread.dynamic_cast_ptr<Isp20PollThread>();
    sensorHw = mSensorDev.dynamic_cast_ptr<BaseSensorHw>();
    lensHw = mLensDev.dynamic_cast_ptr<LensHw>();

    if (_state != CAM_HW_STATE_PREPARED &&
            _state != CAM_HW_STATE_STOPPED) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "camhw state err: %d\n", ret);
        return XCAM_RETURN_ERROR_FAILED;
    }
    if ((mCalibDb->mfnr.enable && mCalibDb->mfnr.motion_detect_en) || mCalibDb->af.ldg_param.enable) {
        ret = mIspSpDev->start(true);
        if (ret < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "start isp selfpath dev err: %d\n", ret);
            return ret;
        }
        mIspSpThread->start();
    }
    ret = hdr_mipi_start_mode(_hdr_mode);
    if (ret < 0) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "hdr mipi start err: %d\n", ret);
    }

    if (mIspLumaDev.ptr()) {
        ret = mIspLumaDev->start();
        if (ret < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "start isp luma dev err: %d\n", ret);
        }
    }

    ret = mIspCoreDev->start();
    if (ret < 0) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "start isp core dev err: %d\n", ret);
    }

    ret = mIspStatsDev->start();
    if (ret < 0) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "start isp stats dev err: %d\n", ret);
    }

#ifndef DISABLE_PP
#ifndef DISABLE_PP_STATS
    ret = mIsppStatsDev->start();
    if (ret < 0) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "start ispp stats dev err: %d\n", ret);
    }
#endif
#endif

    if (mFlashLight.ptr()) {
        ret = mFlashLight->start();
        if (ret < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "start flashlight err: %d\n", ret);
        }
    }
    if (mFlashLightIr.ptr()) {
        ret = mFlashLightIr->start();
        if (ret < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "start flashlight ir err: %d\n", ret);
        }
    }

    mPollthread->start();
    if (mPollLumathread.ptr())
        mPollLumathread->start();
    if (mPollIsppthread.ptr())
        mPollIsppthread->start();
    sensorHw->start();
    if (lensHw.ptr())
        lensHw->start();
    _is_exit = false;
    _state = CAM_HW_STATE_STARTED;

    EXIT_CAMHW_FUNCTION();
    return ret;
}

XCamReturn
CamHwIsp20::hdr_mipi_prepare(int idx)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    // mipi rx/tx format should match to sensor.
    for (int i = 0; i < 3; i++) {
        if (!(idx & (1 << i)))
            continue;
        if (_mipi_rx_devs[i].ptr())
            ret = _mipi_tx_devs[i]->prepare();
        if (ret < 0)
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "mipi tx:%d prepare err: %d\n", ret);
        if (_mipi_rx_devs[i].ptr())
            ret = _mipi_rx_devs[i]->prepare();
        if (ret < 0)
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "mipi rx:%d prepare err: %d\n", ret);
    }
    return ret;
}

XCamReturn
CamHwIsp20::hdr_mipi_prepare_mode(int mode)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    int new_mode = RK_AIQ_HDR_GET_WORKING_MODE(mode);

    if (!mNormalNoReadBack) {
        if (new_mode == RK_AIQ_WORKING_MODE_NORMAL)
            ret = hdr_mipi_prepare(MIPI_STREAM_IDX_0);
        else if (new_mode == RK_AIQ_WORKING_MODE_ISP_HDR2)
            ret = hdr_mipi_prepare(MIPI_STREAM_IDX_0 | MIPI_STREAM_IDX_1);
        else
            ret = hdr_mipi_prepare(MIPI_STREAM_IDX_ALL);
        if (ret < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "hdr mipi start err: %d\n", ret);
        }
    }

    return ret;
}

XCamReturn
CamHwIsp20::hdr_mipi_start(int idx)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    // mipi rx/tx format should match to sensor.
    for (int i = 0; i < 3; i++) {
        if (!(idx & (1 << i)))
            continue;
        if (_mipi_tx_devs[i].ptr())
            ret = _mipi_tx_devs[i]->start(true);
        if (ret < 0)
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "mipi tx:%d start err: %d\n", ret);
        /* ret = _mipi_rx_devs[i]->start(true); */
        /* if (ret < 0) */
        /*     LOGE_CAMHW_SUBM(ISP20HW_SUBM,"mipi rx:%d start err: %d\n", ret); */
    }

    for (int i = 0; i < 3; i++) {
        if (!(idx & (1 << i)))
            continue;
        /* ret = _mipi_tx_devs[i]->start(true); */
        /* if (ret < 0) */
        /*     LOGE_CAMHW_SUBM(ISP20HW_SUBM,"mipi tx:%d start err: %d\n", ret); */
        if (_mipi_rx_devs[i].ptr())
            ret = _mipi_rx_devs[i]->start(true);
        if (ret < 0)
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "mipi rx:%d start err: %d\n", ret);
    }

    return ret;
}

XCamReturn
CamHwIsp20::hdr_mipi_start_mode(int mode)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    int new_mode = RK_AIQ_HDR_GET_WORKING_MODE(mode);

    if (!mNormalNoReadBack) {
        if (new_mode == RK_AIQ_WORKING_MODE_NORMAL)
            ret = hdr_mipi_start(MIPI_STREAM_IDX_0);
        else if (new_mode == RK_AIQ_WORKING_MODE_ISP_HDR2)
            ret = hdr_mipi_start(MIPI_STREAM_IDX_0 | MIPI_STREAM_IDX_1);
        else
            ret = hdr_mipi_start(MIPI_STREAM_IDX_ALL);
        if (ret < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "hdr mipi start err: %d\n", ret);
        }
    }

    return ret;
}

XCamReturn
CamHwIsp20::hdr_mipi_stop(int idx)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    for (int i = 0; i < 3; i++) {
        if (!(idx & (1 << i)))
            continue;
        if (_mipi_rx_devs[i].ptr())
            ret = _mipi_rx_devs[i]->stop();
        if (ret < 0)
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "mipi rx:%d stop err: %d\n", ret);
    }
    for (int i = 0; i < 3; i++) {
        if (!(idx & (1 << i)))
            continue;
        if (_mipi_tx_devs[i].ptr())
            ret = _mipi_tx_devs[i]->stop();
        if (ret < 0)
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "mipi tx:%d stop err: %d\n", ret);
    }

    return ret;
}

XCamReturn CamHwIsp20::stop()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    SmartPtr<Isp20PollThread> isp20Pollthread;
    SmartPtr<Isp20SpThread> isp20Spthread;
    SmartPtr<BaseSensorHw> sensorHw;
    SmartPtr<LensHw> lensHw;

    ENTER_CAMHW_FUNCTION();
    isp20Pollthread = mPollthread.dynamic_cast_ptr<Isp20PollThread>();
    mPollthread->stop();
    if ((mCalibDb->mfnr.enable && mCalibDb->mfnr.motion_detect_en) || mCalibDb->af.ldg_param.enable)
        mIspSpThread->stop();
    if (mPollLumathread.ptr())
        mPollLumathread->stop();
    if (mPollIsppthread.ptr()) {
        mPollIsppthread->stop();
    }
    // stop after pollthread, ensure that no new events
    // come into snesorHw
    sensorHw = mSensorDev.dynamic_cast_ptr<BaseSensorHw>();
    sensorHw->stop();

    lensHw = mLensDev.dynamic_cast_ptr<LensHw>();
    if (lensHw.ptr())
        lensHw->stop();
    if ((mCalibDb->mfnr.enable && mCalibDb->mfnr.motion_detect_en) || mCalibDb->af.ldg_param.enable) {
        ret = mIspSpDev->stop();
        if (ret < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "stop isp selfpath dev err: %d\n", ret);
        }
    }

    if (mIspLumaDev.ptr())
        ret = mIspLumaDev->stop();
    if (ret < 0) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "stop isp luma dev err: %d\n", ret);
    }

    ret = mIspCoreDev->stop();
    if (ret < 0) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "stop isp core dev err: %d\n", ret);
    }

    ret = mIspStatsDev->stop();
    if (ret < 0) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "stop isp stats dev err: %d\n", ret);
    }

    ret = mIspParamsDev->stop();
    if (ret < 0) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "stop isp params dev err: %d\n", ret);
    }

    if ((_hdr_mode != RK_AIQ_WORKING_MODE_NORMAL) ||
            (_hdr_mode == RK_AIQ_WORKING_MODE_NORMAL && !mNormalNoReadBack)) {
        ret = hdr_mipi_stop();
        if (ret < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "hdr mipi stop err: %d\n", ret);
        }
    }

    if (mIsppStatsDev.ptr()) {
        ret = mIsppStatsDev->stop();
        if (ret < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "stop ispp stats dev err: %d\n", ret);
        }
        ret = mIsppParamsDev->stop();
        if (ret < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "stop ispp params dev err: %d\n", ret);
        }
    }
    if (mFlashLight.ptr()) {
        ret = mFlashLight->stop();
        if (ret < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "stop flashlight err: %d\n", ret);
        }
    }
    if (mFlashLightIr.ptr()) {
        mFlashLightIr->keep_status(mKpHwSt);
        ret = mFlashLightIr->stop();
        if (ret < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "stop flashlight ir err: %d\n", ret);
        }
    }

    if (!mKpHwSt)
        setIrcutParams(false);

    SmartLock locker (_mutex);
    _pending_isp_meas_params_queue.clear();
    _pending_isp_other_params_queue.clear();
    _pending_ispp_meas_params_queue.clear();
    _pending_ispp_other_params_queue.clear();
    _effectingIspMeasParmMap.clear();

    _state = CAM_HW_STATE_STOPPED;
    EXIT_CAMHW_FUNCTION();
    return ret;
}

XCamReturn CamHwIsp20::pause()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    SmartPtr<Isp20PollThread> isp20Pollthread;
    SmartPtr<BaseSensorHw> sensorHw;

    isp20Pollthread = mPollthread.dynamic_cast_ptr<Isp20PollThread>();
    mPollthread->stop();
    if (mPollLumathread.ptr())
        mPollLumathread->stop();
    if (mPollIsppthread.ptr()) {
        mPollIsppthread->stop();
    }

    hdr_mipi_stop();

    sensorHw = mSensorDev.dynamic_cast_ptr<BaseSensorHw>();
    sensorHw->stop();

    mIspStatsDev->stop();
    if (ret < 0) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "stop isp stats dev err: %d\n", ret);
    }

    mIspParamsDev->stop();
    if (ret < 0) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "stop isp params dev err: %d\n", ret);
    }

    if (mIsppStatsDev.ptr()) {
        ret = mIsppStatsDev->stop();
        if (ret < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "stop ispp stats dev err: %d\n", ret);
        }
        ret = mIsppParamsDev->stop();
        if (ret < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "stop ispp params dev err: %d\n", ret);
        }
    }

    if (mIspSpThread.ptr())
        mIspSpThread->pause();

    SmartLock locker (_mutex);
    _pending_isp_meas_params_queue.clear();
    _pending_isp_other_params_queue.clear();
    _pending_ispp_meas_params_queue.clear();
    _pending_ispp_other_params_queue.clear();
    _effectingIspMeasParmMap.clear();

    _state = CAM_HW_STATE_PAUSED;
    return ret;
}

XCamReturn CamHwIsp20::swWorkingModeDyn(int mode)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    SmartPtr<BaseSensorHw> sensorHw;
    SmartPtr<Isp20PollThread> isp20Pollthread;

    if (_linked_to_isp) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "sensor linked to isp, not supported now!");
        return XCAM_RETURN_ERROR_FAILED;
    }

    sensorHw = mSensorDev.dynamic_cast_ptr<BaseSensorHw>();

    ret = sensorHw->set_working_mode(mode);
    if (ret) {
        LOGW_CAMHW_SUBM(ISP20HW_SUBM, "set sensor mode error !");
        return ret;
    }

    setExpDelayInfo(mode);

    Isp20Params::set_working_mode(mode);
    isp20Pollthread = mPollthread.dynamic_cast_ptr<Isp20PollThread>();
    isp20Pollthread->set_working_mode(mode, _linked_to_isp);
    if (mIspSpThread.ptr())
        mIspSpThread->set_working_mode(mode);
#if 0 // for quick switch, not used now
    int old_mode = RK_AIQ_HDR_GET_WORKING_MODE(_hdr_mode);
    int new_mode = RK_AIQ_HDR_GET_WORKING_MODE(mode);
    //set hdr mode to drv
    if (mIspCoreDev.ptr()) {
        int drv_mode = ISP2X_HDR_MODE_NOMAL;
        if (new_mode == RK_AIQ_WORKING_MODE_ISP_HDR3)
            drv_mode = ISP2X_HDR_MODE_X3;
        else if (new_mode == RK_AIQ_WORKING_MODE_ISP_HDR2)
            drv_mode = ISP2X_HDR_MODE_X2;

        if (mIspCoreDev->io_control (RKISP_CMD_SW_HDR_MODE_QUICK, &drv_mode) < 0) {
            ret = XCAM_RETURN_ERROR_FAILED;
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "set RKISP_CMD_SW_HDR_MODE_QUICK failed");
            return ret;
        }
    }
    // reconfig tx,rx stream
    if (!old_mode && new_mode) {
        // normal to hdr
        if (new_mode == RK_AIQ_WORKING_MODE_ISP_HDR3)
            hdr_mipi_start(MIPI_STREAM_IDX_1 | MIPI_STREAM_IDX_2);
        else
            hdr_mipi_start(MIPI_STREAM_IDX_1);
    } else if (old_mode && !new_mode) {
        // hdr to normal
        if (new_mode == RK_AIQ_WORKING_MODE_ISP_HDR3)
            hdr_mipi_stop(MIPI_STREAM_IDX_1 | MIPI_STREAM_IDX_2);
        else
            hdr_mipi_stop(MIPI_STREAM_IDX_1);
    } else if (old_mode == RK_AIQ_WORKING_MODE_ISP_HDR3 &&
               new_mode == RK_AIQ_WORKING_MODE_ISP_HDR2) {
        // hdr3 to hdr2
        hdr_mipi_stop(MIPI_STREAM_IDX_1);
    } else if (old_mode == RK_AIQ_WORKING_MODE_ISP_HDR2 &&
               new_mode == RK_AIQ_WORKING_MODE_ISP_HDR3) {
        // hdr3 to hdr2
        hdr_mipi_start(MIPI_STREAM_IDX_2);
    } else {
        // do nothing
        LOGW_CAMHW_SUBM(ISP20HW_SUBM, "do nothing, old mode %d, new mode %d\n",
                        _hdr_mode, mode);
    }
#endif
    _hdr_mode = mode;
    // remap _mipi_tx_devs for cif
    if (!_linked_to_isp)
        prepare_cif_mipi();

    return ret;
}

XCamReturn CamHwIsp20::resume()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    SmartPtr<Isp20PollThread> isp20Pollthread;
    SmartPtr<BaseSensorHw> sensorHw = mSensorDev.dynamic_cast_ptr<BaseSensorHw>();

    isp20Pollthread = mPollthread.dynamic_cast_ptr<Isp20PollThread>();

    ret = hdr_mipi_start_mode(_hdr_mode);
    if (ret < 0) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "hdr mipi start err: %d\n", ret);
    }
    if (mIspSpThread.ptr())
        mIspSpThread->resume();
    sensorHw->start();

    mPollthread->start();
    if (mPollLumathread.ptr())
        mPollLumathread->start();
    if (mPollIsppthread.ptr()) {
        mPollIsppthread->start();
    }
    ret = mIspStatsDev->start();
    if (ret < 0) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "start isp stats dev err: %d\n", ret);
    }

    if (mIsppStatsDev.ptr()) {
        ret = mIsppStatsDev->start();
        if (ret < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "start ispp stats dev err: %d\n", ret);
        }
    }

    _state = CAM_HW_STATE_STARTED;
    return ret;
}

/*
 * some module(HDR/TNR) parameters are related to the next frame exposure
 * and can only be easily obtained at the hwi layer,
 * so these parameters are calculated at hwi and the result is overwritten.
 */
XCamReturn
CamHwIsp20::overrideExpRatioToAiqResults(const sint32_t frameId,
        int module_id,
        SmartPtr<RkAiqIspMeasParamsProxy>& aiq_results,
        SmartPtr<RkAiqIspOtherParamsProxy>& aiq_other_results)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    SmartPtr<RkAiqExpParamsProxy> curFrameExpParam;
    SmartPtr<RkAiqExpParamsProxy> nextFrameExpParam;
    SmartPtr<BaseSensorHw> mSensorSubdev = mSensorDev.dynamic_cast_ptr<BaseSensorHw>();

    if (mSensorSubdev.ptr()) {
        if (mSensorSubdev->getEffectiveExpParams(curFrameExpParam, frameId) < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "exp-sync: module_id: 0x%x, rx id: %d, ispparams id: %d\n",
                            module_id,
                            frameId,
                            aiq_results->data()->frame_id);
            return ret;
        }

        if (mSensorSubdev->getEffectiveExpParams(nextFrameExpParam, frameId + 1) < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "exp-sync: module_id: 0x%x, rx id: %d, ispparams id: %d\n",
                            module_id,
                            frameId + 1,
                            aiq_results->data()->frame_id);
            return ret;
        }
    }

    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "exp-sync: module_id: 0x%x, rx id: %d, ispparams id: %d\n"
                    "curFrame(%d): lexp: %f-%f, mexp: %f-%f, sexp: %f-%f\n"
                    "nextFrame(%d): lexp: %f-%f, mexp: %f-%f, sexp: %f-%f\n",
                    module_id,
                    frameId,
                    aiq_results->data()->frame_id,
                    frameId,
                    curFrameExpParam->data()->aecExpInfo.HdrExp[2].exp_real_params.analog_gain,
                    curFrameExpParam->data()->aecExpInfo.HdrExp[2].exp_real_params.integration_time,
                    curFrameExpParam->data()->aecExpInfo.HdrExp[1].exp_real_params.analog_gain,
                    curFrameExpParam->data()->aecExpInfo.HdrExp[1].exp_real_params.integration_time,
                    curFrameExpParam->data()->aecExpInfo.HdrExp[0].exp_real_params.analog_gain,
                    curFrameExpParam->data()->aecExpInfo.HdrExp[0].exp_real_params.integration_time,
                    frameId + 1,
                    nextFrameExpParam->data()->aecExpInfo.HdrExp[2].exp_real_params.analog_gain,
                    nextFrameExpParam->data()->aecExpInfo.HdrExp[2].exp_real_params.integration_time,
                    nextFrameExpParam->data()->aecExpInfo.HdrExp[1].exp_real_params.analog_gain,
                    nextFrameExpParam->data()->aecExpInfo.HdrExp[1].exp_real_params.integration_time,
                    nextFrameExpParam->data()->aecExpInfo.HdrExp[0].exp_real_params.analog_gain,
                    nextFrameExpParam->data()->aecExpInfo.HdrExp[0].exp_real_params.integration_time);

    rk_aiq_luma_params_t curFrameLumaParam, nextFrameLumaParam;
    SmartPtr<Isp20PollThread> isp20Pollthread = mPollthread.dynamic_cast_ptr<Isp20PollThread>();
    if (isp20Pollthread.ptr()) {
        if (isp20Pollthread->getEffectiveLumaParams(frameId, curFrameLumaParam) < 0)
            LOGW_CAMHW_SUBM(ISP20HW_SUBM, "module_id: 0x%x, rx id: %d, lumaParams id: %d\n",
                            module_id,
                            frameId,
                            curFrameLumaParam.frame_id);

        if (isp20Pollthread->getEffectiveLumaParams(frameId + 1, nextFrameLumaParam) < 0)
            LOGW_CAMHW_SUBM(ISP20HW_SUBM, "module_id: 0x%x, rx id: %d, lumaParams id: %d\n",
                            module_id,
                            frameId + 1,
                            nextFrameLumaParam.frame_id);

#if 0
        unsigned int cur_mean_luma = 0, next_mean_luma = 0;
        for (int i = 0; i < ISP2X_MIPI_LUMA_MEAN_MAX; i++) {
            cur_mean_luma += curFrameLumaParam.luma[0][i];
            next_mean_luma += nextFrameLumaParam.luma[0][i];
        }
        cur_mean_luma = cur_mean_luma / ISP2X_MIPI_LUMA_MEAN_MAX;
        next_mean_luma = next_mean_luma / ISP2X_MIPI_LUMA_MEAN_MAX;
        printf("%s, cur_mean_luma(%d), next_mean_luma(%d)\n",
               __FUNCTION__, cur_mean_luma, next_mean_luma);
#endif

        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "module_id: 0x%x, rx id: %d\n",
                        module_id,
                        frameId,
                        curFrameLumaParam.frame_id,
                        nextFrameLumaParam.frame_id);
    }

    switch (module_id) {
    case RK_ISP2X_HDRTMO_ID:
    {
        float nextSLuma[16] ;
        float curSLuma[16] ;
        float nextMLuma[16] ;
        float curMLuma[16] ;
        float nextLLuma[16];
        float curLLuma[16];
        float blc = (aiq_other_results->data()->blc.stResult.blc_r + aiq_other_results->data()->blc.stResult.blc_gr
                     + aiq_other_results->data()->blc.stResult.blc_gb + aiq_other_results->data()->blc.stResult.blc_b) / (16.0 * 4.0);
        int cols = aiq_results->data()->ahdr_proc_res.TmoFlicker.width;
        int rows = aiq_results->data()->ahdr_proc_res.TmoFlicker.height;
        int PixelNum = cols * rows;
        int PixelNumBlock = PixelNum / ISP2X_MIPI_LUMA_MEAN_MAX;

        //get luma info
        float luma[96];
        hdrtmoGetLumaInfo(&nextFrameLumaParam, &curFrameLumaParam, aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_mode + 1,
                          PixelNumBlock, blc, luma);

        //get expo info
        float expo[6];
        hdrtmoGetAeInfo(&nextFrameExpParam->data()->aecExpInfo,
                        &curFrameExpParam->data()->aecExpInfo, aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_mode + 1, expo);

        float curSExpo = expo[0];
        float curMExpo = expo[1];
        float curLExpo = expo[2];
        float nextSExpo = expo[3];
        float nextMExpo = expo[4];
        float nextLExpo = expo[5];
        float nextRatioLS = 0;
        float nextRatioLM = 0;
        float curRatioLS = 0;

        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "LongFrameMode:%d \n", aiq_results->data()->ahdr_proc_res.LongFrameMode);

        if(aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_mode == 0 || aiq_results->data()->ahdr_proc_res.LongFrameMode)
        {
            nextRatioLS = 1;
            nextRatioLM = 1;
            curRatioLS = 1;
        }
        else if(aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_mode == 1 && !( aiq_results->data()->ahdr_proc_res.LongFrameMode))
        {
            nextRatioLS = nextLExpo / nextSExpo;
            nextRatioLM = 1;
            curRatioLS = curLExpo / curSExpo;
        }
        else if(aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_mode == 2 && !( aiq_results->data()->ahdr_proc_res.LongFrameMode))
        {
            nextRatioLS = nextLExpo / nextSExpo;
            nextRatioLM = nextLExpo / nextMExpo;
            curRatioLS = curLExpo / curSExpo;
        }

        float nextLgmax = 12 + log(nextRatioLS) / log(2);
        float curLgmax = 12 + log(curRatioLS) / log(2);
        float lgmin = 0;

        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "nextRatioLS:%f nextRatioLM:%f curRatioLS:%f\n", nextRatioLS, nextRatioLM, curRatioLS);
        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "nextLgmax:%f curLgmax:%f \n", nextLgmax, curLgmax);

        //tmo
        // shadow resgister,needs to set a frame before, for ctrl_cfg/lg_scl reg
        aiq_results->data()->ahdr_proc_res.TmoProcRes.sw_hdrtmo_expl_lgratio = \
                (int)(2048 * (log(curLExpo / nextLExpo) / log(2)));
        if( aiq_results->data()->ahdr_proc_res.LongFrameMode || aiq_results->data()->ahdr_proc_res.isLinearTmo)
            aiq_results->data()->ahdr_proc_res.TmoProcRes.sw_hdrtmo_lgscl_ratio = 128;
        else
            aiq_results->data()->ahdr_proc_res.TmoProcRes.sw_hdrtmo_lgscl_ratio = \
                    (int)(128 * (log(nextRatioLS) / log(curRatioLS)));
        aiq_results->data()->ahdr_proc_res.TmoProcRes.sw_hdrtmo_lgscl = (int)(4096 * 16 / nextLgmax);
        aiq_results->data()->ahdr_proc_res.TmoProcRes.sw_hdrtmo_lgscl_inv = (int)(4096 * nextLgmax / 16);

        // not shadow resgister
        aiq_results->data()->ahdr_proc_res.TmoProcRes.sw_hdrtmo_lgmax = (int)(2048 * nextLgmax);
        aiq_results->data()->ahdr_proc_res.TmoProcRes.sw_hdrtmo_set_lgmax = \
                aiq_results->data()->ahdr_proc_res.TmoProcRes.sw_hdrtmo_lgmax;

        //sw_hdrtmo_set_lgrange0
        float value = 0.0;
        float clipratio0 = aiq_results->data()->ahdr_proc_res.TmoProcRes.sw_hdrtmo_clipratio0 / 256.0;
        float clipgap0 = aiq_results->data()->ahdr_proc_res.TmoProcRes.sw_hdrtmo_clipgap0 / 4.0;
        float Lgmax = aiq_results->data()->ahdr_proc_res.TmoProcRes.sw_hdrtmo_set_lgmax / 2048.0;
        value = lgmin * (1 - clipratio0) + Lgmax * clipratio0;
        value = MIN(value, (lgmin + clipgap0));
        aiq_results->data()->ahdr_proc_res.TmoProcRes.sw_hdrtmo_set_lgrange0 = (int)(2048 * value);

        //sw_hdrtmo_set_lgrange1
        value = 0.0;
        float clipratio1 = aiq_results->data()->ahdr_proc_res.TmoProcRes.sw_hdrtmo_clipratio1 / 256.0;
        float clipgap1 = aiq_results->data()->ahdr_proc_res.TmoProcRes.sw_hdrtmo_clipgap1 / 4.0;
        value = lgmin * (1 - clipratio1) + Lgmax * clipratio1;
        value = MAX(value, (Lgmax - clipgap1));
        aiq_results->data()->ahdr_proc_res.TmoProcRes.sw_hdrtmo_set_lgrange1 = (int)(2048 * value);

        //merge
        // shadow resgister,needs to set a frame before, for gain0/1/2 reg
        if(aiq_results->data()->ahdr_proc_res.isLinearTmo == false) {
            aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_gain0 = (int)(64 * nextRatioLS);
            if(nextRatioLS == 1)
                aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_gain0_inv = (int)(4096 * (1 / nextRatioLS) - 1);
            else
                aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_gain0_inv = (int)(4096 * (1 / nextRatioLS));
            aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_gain1 = (int)(64 * nextRatioLM);
            if(nextRatioLM == 1)
                aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_gain1_inv = (int)(4096 * (1 / nextRatioLM) - 1);
            else
                aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_gain1_inv = (int)(4096 * (1 / nextRatioLM));
        }
        else
        {
            aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_gain0 = 0x40;
            aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_gain0_inv = 0xfff;
            aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_gain1 = 0x40;
            aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_gain1_inv = 0xfff;
            aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_gain2 = 0x40;
            aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_lm_dif_0p9 = 230;
            aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_ms_dif_0p8 = 205;
            aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_lm_dif_0p15 = 38;
            aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_ms_dif_0p15 = 38;

            int a[17] = {0, 0, 0, 0, 0x1, 0x4, 0xd, 0x2b, 0x89, 0x168, 0x29e, 0x379, 0x3d6, 0x3f3, 0x3fc, 0x3ff, 0x3ff};
            int b[17] = {0, 0, 0, 0, 0, 0, 0, 0, 0x4, 0x2a, 0x162, 0x376, 0x3f3, 0x3ff, 0x3ff, 0x3ff, 0x3ff};
            for(int i = 0; i < 17; i++)
            {
                aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_l0_y[i] = a[i];
                aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_l1_y[i] = a[i];
                aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_e_y[i] = b[i];
            }

        }

        //get predict para
        bool SceneStable = hdrtmoSceneStable(frameId, aiq_results->data()->ahdr_proc_res.TmoFlicker.iirmax,
                                             aiq_results->data()->ahdr_proc_res.TmoFlicker.iir,
                                             aiq_results->data()->ahdr_proc_res.TmoProcRes.sw_hdrtmo_set_weightkey,
                                             aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_mode + 1,
                                             aiq_results->data()->ahdr_proc_res.TmoFlicker.LumaDeviation,
                                             aiq_results->data()->ahdr_proc_res.TmoFlicker.StableThr);
        int PredicPara = 0;//hdrtmoPredictK(luma, expo,
        // aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_mode + 1,
        //&aiq_results->data()->ahdr_proc_res.TmoFlicker.PredictK);
        float GlobalTmoStrength = aiq_results->data()->ahdr_proc_res.TmoFlicker.GlobalTmoStrength;
        aiq_results->data()->ahdr_proc_res.Predict.Scenestable = SceneStable;
        aiq_results->data()->ahdr_proc_res.Predict.K_Rolgmean = PredicPara;
        aiq_results->data()->ahdr_proc_res.Predict.cnt_mode = aiq_results->data()->ahdr_proc_res.TmoFlicker.cnt_mode;
        aiq_results->data()->ahdr_proc_res.Predict.cnt_vsize = aiq_results->data()->ahdr_proc_res.TmoFlicker.cnt_vsize;
        aiq_results->data()->ahdr_proc_res.Predict.iir_max = aiq_results->data()->ahdr_proc_res.TmoFlicker.iirmax;
        aiq_results->data()->ahdr_proc_res.Predict.iir = aiq_results->data()->ahdr_proc_res.TmoFlicker.iir;
        aiq_results->data()->ahdr_proc_res.Predict.global_tmo_strength = 2048 * log(GlobalTmoStrength) / log(2);
        if(aiq_results->data()->ahdr_proc_res.TmoFlicker.GlobalTmoStrengthDown)
            aiq_results->data()->ahdr_proc_res.Predict.global_tmo_strength *= -1;

        LOGD_CAMHW_SUBM(ISP20HW_SUBM,
                        "SceneStable:%d K_Rolgmean:%d iir:%d iir_max:%d global_tmo_strength:%d\n",
                        aiq_results->data()->ahdr_proc_res.Predict.Scenestable,
                        aiq_results->data()->ahdr_proc_res.Predict.K_Rolgmean, aiq_results->data()->ahdr_proc_res.Predict.iir,
                        aiq_results->data()->ahdr_proc_res.Predict.iir_max, aiq_results->data()->ahdr_proc_res.Predict.global_tmo_strength);
        LOGD_CAMHW_SUBM(ISP20HW_SUBM,
                        "sw_hdrtmo_expl_lgratio:%d sw_hdrtmo_lgscl_ratio:%d "
                        "sw_hdrtmo_lgmax:%d sw_hdrtmo_set_lgmax:%d sw_hdrtmo_lgscl:%d sw_hdrtmo_lgscl_inv:%d\n",
                        aiq_results->data()->ahdr_proc_res.TmoProcRes.sw_hdrtmo_expl_lgratio,
                        aiq_results->data()->ahdr_proc_res.TmoProcRes.sw_hdrtmo_lgscl_ratio,
                        aiq_results->data()->ahdr_proc_res.TmoProcRes.sw_hdrtmo_lgmax,
                        aiq_results->data()->ahdr_proc_res.TmoProcRes.sw_hdrtmo_set_lgmax,
                        aiq_results->data()->ahdr_proc_res.TmoProcRes.sw_hdrtmo_lgscl,
                        aiq_results->data()->ahdr_proc_res.TmoProcRes.sw_hdrtmo_lgscl_inv);
        LOGD_CAMHW_SUBM(ISP20HW_SUBM,
                        "sw_hdrtmo_set_lgrange0:%d sw_hdrtmo_set_lgrange1:%d\n",
                        aiq_results->data()->ahdr_proc_res.TmoProcRes.sw_hdrtmo_set_lgrange0,
                        aiq_results->data()->ahdr_proc_res.TmoProcRes.sw_hdrtmo_set_lgrange1);
        LOGD_CAMHW_SUBM(ISP20HW_SUBM,
                        "sw_hdrmge_gain0:%d sw_hdrmge_gain0_inv:%d sw_hdrmge_gain1:%d sw_hdrmge_gain1_inv:%d\n",
                        aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_gain0,
                        aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_gain0_inv,
                        aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_gain1,
                        aiq_results->data()->ahdr_proc_res.MgeProcRes.sw_hdrmge_gain1_inv);
        break;
    }
    case RK_ISP2X_PP_TNR_ID:
        break;
    default:
        LOGW_CAMHW_SUBM(ISP20HW_SUBM, "unkown module id: 0x%x!\n", module_id);
        break;
    }

    return ret;
}

void
CamHwIsp20::gen_full_ispp_params(const struct rkispp_params_cfg* update_params,
                                 struct rkispp_params_cfg* full_params)
{
    XCAM_ASSERT (update_params);
    XCAM_ASSERT (full_params);

    int end = RK_ISP2X_PP_MAX_ID - RK_ISP2X_PP_TNR_ID;

    ENTER_CAMHW_FUNCTION();
    for (int i = 0; i < end; i++)
        if (update_params->module_en_update & (1 << i)) {
            full_params->module_en_update |= 1 << i;
            // clear old bit value
            full_params->module_ens &= ~(1 << i);
            // set new bit value
            full_params->module_ens |= update_params->module_ens & (1LL << i);
        }

    for (int i = 0; i < end; i++)
        if (update_params->module_cfg_update & (1 << i)) {
            full_params->module_cfg_update |= 1 << i;
        }

    EXIT_CAMHW_FUNCTION();
}

void
CamHwIsp20::gen_full_isp_params(const struct isp2x_isp_params_cfg* update_params,
                                struct isp2x_isp_params_cfg* full_params)
{
    XCAM_ASSERT (update_params);
    XCAM_ASSERT (full_params);
    int i = 0;

    ENTER_CAMHW_FUNCTION();
    for (; i <= RK_ISP2X_MAX_ID; i++)
        if (update_params->module_en_update & (1LL << i)) {
            full_params->module_en_update |= 1LL << i;
            // clear old bit value
            full_params->module_ens &= ~(1LL << i);
            // set new bit value
            full_params->module_ens |= update_params->module_ens & (1LL << i);
        }

    for (i = 0; i <= RK_ISP2X_MAX_ID; i++) {
        if (update_params->module_cfg_update & (1LL << i)) {
            full_params->module_cfg_update |= 1LL << i;
            switch (i) {
            case RK_ISP2X_RAWAE_BIG1_ID:
                full_params->meas.rawae3 = update_params->meas.rawae3;
                break;
            case RK_ISP2X_RAWAE_BIG2_ID:
                full_params->meas.rawae1 = update_params->meas.rawae1;
                break;
            case RK_ISP2X_RAWAE_BIG3_ID:
                full_params->meas.rawae2 = update_params->meas.rawae2;
                break;
            case RK_ISP2X_RAWAE_LITE_ID:
                full_params->meas.rawae0 = update_params->meas.rawae0;
                break;
            case RK_ISP2X_RAWHIST_BIG1_ID:
                full_params->meas.rawhist3 = update_params->meas.rawhist3;
                break;
            case RK_ISP2X_RAWHIST_BIG2_ID:
                full_params->meas.rawhist1 = update_params->meas.rawhist1;
                break;
            case RK_ISP2X_RAWHIST_BIG3_ID:
                full_params->meas.rawhist2 = update_params->meas.rawhist2;
                break;
            case RK_ISP2X_RAWHIST_LITE_ID:
                full_params->meas.rawhist0 = update_params->meas.rawhist0;
                break;
            case RK_ISP2X_YUVAE_ID:
                full_params->meas.yuvae = update_params->meas.yuvae;
                break;
            case RK_ISP2X_SIHST_ID:
                full_params->meas.sihst = update_params->meas.sihst;
                break;
            case RK_ISP2X_SIAWB_ID:
                full_params->meas.siawb = update_params->meas.siawb;
                break;
            case RK_ISP2X_RAWAWB_ID:
                full_params->meas.rawawb = update_params->meas.rawawb;
                break;
            case RK_ISP2X_AWB_GAIN_ID:
                full_params->others.awb_gain_cfg = update_params->others.awb_gain_cfg;
                break;
            case RK_ISP2X_RAWAF_ID:
                full_params->meas.rawaf = update_params->meas.rawaf;
                break;
            case RK_ISP2X_HDRMGE_ID:
                full_params->others.hdrmge_cfg = update_params->others.hdrmge_cfg;
                break;
            case RK_ISP2X_HDRTMO_ID:
                full_params->others.hdrtmo_cfg = update_params->others.hdrtmo_cfg;
                break;
            case RK_ISP2X_CTK_ID:
                full_params->others.ccm_cfg = update_params->others.ccm_cfg;
                break;
            case RK_ISP2X_LSC_ID:
                full_params->others.lsc_cfg = update_params->others.lsc_cfg;
                break;
            case RK_ISP2X_GOC_ID:
                full_params->others.gammaout_cfg = update_params->others.gammaout_cfg;
                break;
            case RK_ISP2X_3DLUT_ID:
                full_params->others.isp3dlut_cfg = update_params->others.isp3dlut_cfg;
                break;
            case RK_ISP2X_DPCC_ID:
                full_params->others.dpcc_cfg = update_params->others.dpcc_cfg;
                break;
            case RK_ISP2X_BLS_ID:
                full_params->others.bls_cfg = update_params->others.bls_cfg;
                break;
            case RK_ISP2X_DEBAYER_ID:
                full_params->others.debayer_cfg = update_params->others.debayer_cfg;
                break;
            case RK_ISP2X_DHAZ_ID:
                full_params->others.dhaz_cfg = update_params->others.dhaz_cfg;
                break;
            case RK_ISP2X_RAWNR_ID:
                full_params->others.rawnr_cfg = update_params->others.rawnr_cfg;
                break;
            case RK_ISP2X_GAIN_ID:
                full_params->others.gain_cfg = update_params->others.gain_cfg;
                break;
            case RK_ISP2X_LDCH_ID:
                full_params->others.ldch_cfg = update_params->others.ldch_cfg;
                break;
            case RK_ISP2X_GIC_ID:
                full_params->others.gic_cfg = update_params->others.gic_cfg;
                break;
            case RK_ISP2X_CPROC_ID:
                full_params->others.cproc_cfg = update_params->others.cproc_cfg;
                break;
            case RK_ISP2X_SDG_ID:
                full_params->others.sdg_cfg = update_params->others.sdg_cfg;
                break;
            case RK_ISP2X_WDR_ID:
                full_params->others.wdr_cfg = update_params->others.wdr_cfg;
                break;
            default:
                break;
            }
        }
    }
    EXIT_CAMHW_FUNCTION();
}

void CamHwIsp20::dump_isp_config(struct isp2x_isp_params_cfg* isp_params,
                                 SmartPtr<RkAiqIspMeasParamsProxy> aiq_results,
                                 SmartPtr<RkAiqIspOtherParamsProxy> aiq_other_results)
{
    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "isp_params mask: 0x%llx-0x%llx-0x%llx\n",
                    isp_params->module_en_update,
                    isp_params->module_ens,
                    isp_params->module_cfg_update);

    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "aiq_results: ae-lite.winnum=%d; ae-big2.winnum=%d, sub[1].size: [%dx%d]\n",
                    aiq_results->data()->aec_meas.rawae0.wnd_num,
                    aiq_results->data()->aec_meas.rawae1.wnd_num,
                    aiq_results->data()->aec_meas.rawae1.subwin[1].h_size,
                    aiq_results->data()->aec_meas.rawae1.subwin[1].v_size);

    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "isp_params: ae-lite.winnum=%d; ae-big2.winnum=%d, sub[1].size: [%dx%d]\n",
                    isp_params->meas.rawae0.wnd_num,
                    isp_params->meas.rawae1.wnd_num,
                    isp_params->meas.rawae1.subwin[1].h_size,
                    isp_params->meas.rawae1.subwin[1].v_size);

    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "isp_params: win size: [%dx%d]-[%dx%d]-[%dx%d]-[%dx%d]\n",
                    isp_params->meas.rawae0.win.h_size,
                    isp_params->meas.rawae0.win.v_size,
                    isp_params->meas.rawae3.win.h_size,
                    isp_params->meas.rawae3.win.v_size,
                    isp_params->meas.rawae1.win.h_size,
                    isp_params->meas.rawae1.win.v_size,
                    isp_params->meas.rawae2.win.h_size,
                    isp_params->meas.rawae2.win.v_size);

    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "isp_params: debayer:");
    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "clip_en:%d, dist_scale:%d, filter_c_en:%d, filter_g_en:%d",
                    isp_params->others.debayer_cfg.clip_en,
                    isp_params->others.debayer_cfg.dist_scale,
                    isp_params->others.debayer_cfg.filter_c_en,
                    isp_params->others.debayer_cfg.filter_g_en);
    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "gain_offset:%d,hf_offset:%d,max_ratio:%d,offset:%d,order_max:%d",
                    isp_params->others.debayer_cfg.gain_offset,
                    isp_params->others.debayer_cfg.hf_offset,
                    isp_params->others.debayer_cfg.max_ratio,
                    isp_params->others.debayer_cfg.offset,
                    isp_params->others.debayer_cfg.order_max);
    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "order_min:%d, shift_num:%d, thed0:%d, thed1:%d",
                    isp_params->others.debayer_cfg.order_min,
                    isp_params->others.debayer_cfg.shift_num,
                    isp_params->others.debayer_cfg.thed0,
                    isp_params->others.debayer_cfg.thed1);
    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "filter1:[%d, %d, %d, %d, %d]",
                    isp_params->others.debayer_cfg.filter1_coe1,
                    isp_params->others.debayer_cfg.filter1_coe2,
                    isp_params->others.debayer_cfg.filter1_coe3,
                    isp_params->others.debayer_cfg.filter1_coe4,
                    isp_params->others.debayer_cfg.filter1_coe5);
    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "filter2:[%d, %d, %d, %d, %d]",
                    isp_params->others.debayer_cfg.filter2_coe1,
                    isp_params->others.debayer_cfg.filter2_coe2,
                    isp_params->others.debayer_cfg.filter2_coe3,
                    isp_params->others.debayer_cfg.filter2_coe4,
                    isp_params->others.debayer_cfg.filter2_coe5);

    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "isp_params: gic: \n"
                    "edge_open:%d, regmingradthrdark2:%d, regmingradthrdark1:%d, regminbusythre:%d\n"
                    "regdarkthre:%d,regmaxcorvboth:%d,regdarktthrehi:%d,regkgrad2dark:%d,regkgrad1dark:%d\n"
                    "regstrengthglobal_fix:%d, regdarkthrestep:%d, regkgrad2:%d, regkgrad1:%d\n"
                    "reggbthre:%d, regmaxcorv:%d, regmingradthr2:%d, regmingradthr1:%d, gr_ratio:%d\n"
                    "dnloscale:%d, dnhiscale:%d, reglumapointsstep:%d, gvaluelimitlo:%d, gvaluelimithi:%d\n"
                    "fratiohilimt1:%d, strength_fix:%d, noise_cuten:%d, noise_coe_a:%d, noise_coe_b:%d, diff_clip:%d\n",
                    isp_params->others.gic_cfg.edge_open,
                    isp_params->others.gic_cfg.regmingradthrdark2,
                    isp_params->others.gic_cfg.regmingradthrdark1,
                    isp_params->others.gic_cfg.regminbusythre,
                    isp_params->others.gic_cfg.regdarkthre,
                    isp_params->others.gic_cfg.regmaxcorvboth,
                    isp_params->others.gic_cfg.regdarktthrehi,
                    isp_params->others.gic_cfg.regkgrad2dark,
                    isp_params->others.gic_cfg.regkgrad1dark,
                    isp_params->others.gic_cfg.regstrengthglobal_fix,
                    isp_params->others.gic_cfg.regdarkthrestep,
                    isp_params->others.gic_cfg.regkgrad2,
                    isp_params->others.gic_cfg.regkgrad1,
                    isp_params->others.gic_cfg.reggbthre,
                    isp_params->others.gic_cfg.regmaxcorv,
                    isp_params->others.gic_cfg.regmingradthr2,
                    isp_params->others.gic_cfg.regmingradthr1,
                    isp_params->others.gic_cfg.gr_ratio,
                    isp_params->others.gic_cfg.dnloscale,
                    isp_params->others.gic_cfg.dnhiscale,
                    isp_params->others.gic_cfg.reglumapointsstep,
                    isp_params->others.gic_cfg.gvaluelimitlo,
                    isp_params->others.gic_cfg.gvaluelimithi,
                    isp_params->others.gic_cfg.fusionratiohilimt1,
                    isp_params->others.gic_cfg.regstrengthglobal_fix,
                    isp_params->others.gic_cfg.noise_cut_en,
                    isp_params->others.gic_cfg.noise_coe_a,
                    isp_params->others.gic_cfg.noise_coe_b,
                    isp_params->others.gic_cfg.diff_clip);
    for(int i = 0; i < ISP2X_GIC_SIGMA_Y_NUM; i++) {
        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "sigma_y[%d]=%d\n", i, isp_params->others.gic_cfg.sigma_y[i]);
    }
    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "aiq_results: gic: dnloscale=%f, dnhiscale=%f,gvaluelimitlo=%f,gvaluelimithi=%f,fusionratiohilimt1=%f"
                    "textureStrength=%f,globalStrength=%f,noiseCurve_0=%f,noiseCurve_1=%f",
                    aiq_other_results->data()->gic.dnloscale, aiq_other_results->data()->gic.dnhiscale,
                    aiq_other_results->data()->gic.gvaluelimitlo, aiq_other_results->data()->gic.gvaluelimithi,
                    aiq_other_results->data()->gic.fusionratiohilimt1, aiq_other_results->data()->gic.textureStrength,
                    aiq_other_results->data()->gic.globalStrength, aiq_other_results->data()->gic.noiseCurve_0,
                    aiq_other_results->data()->gic.noiseCurve_1);
    for(int i = 0; i < ISP2X_GIC_SIGMA_Y_NUM; i++) {
        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "sigma[%d]=%f\n", i, aiq_other_results->data()->gic.sigma_y[i]);
    }

}

XCamReturn
CamHwIsp20::setIspParamsSync(int frameId)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ENTER_CAMHW_FUNCTION();
    _mutex.lock();
    while (_effectingIspMeasParmMap.size() > 4)
        _effectingIspMeasParmMap.erase(_effectingIspMeasParmMap.begin());

    if (_pending_isp_meas_params_queue.empty() && \
            _pending_isp_other_params_queue.empty()) {
        LOGW_CAMHW_SUBM(ISP20HW_SUBM,
                        "no new isp %s params for frame %d !",
                        _pending_isp_meas_params_queue.empty() ? "meas" : "other",
                        frameId);
        _mutex.unlock();
        return ret;
    }
    _mutex.unlock();

    // merge all pending params
    struct isp2x_isp_params_cfg update_params;
    SmartPtr<RkAiqIspMeasParamsProxy> aiqIspMeasResult = nullptr;
    SmartPtr<RkAiqIspOtherParamsProxy> aiqIspOtherResult = nullptr;

    /* xcam_mem_clear (update_params); */
    /* xcam_mem_clear (_full_active_isp_params); */
    update_params.module_en_update = 0;
    update_params.module_ens = 0;
    update_params.module_cfg_update = 0;
    if (_state == CAM_HW_STATE_STOPPED || _state == CAM_HW_STATE_PREPARED || _state == CAM_HW_STATE_PAUSED) {
        // update all ens
        _full_active_isp_params.module_en_update = ~0;
        // just re-config the enabled moddules
        _full_active_isp_params.module_cfg_update = _full_active_isp_params.module_ens;

        // don't update ldch params if restoring params from pause state
        if (_state == CAM_HW_STATE_PAUSED) {
            _full_active_isp_params.module_en_update &= ~(1ULL << RK_ISP2X_LDCH_ID);
            _full_active_isp_params.module_cfg_update &= ~(1ULL << RK_ISP2X_LDCH_ID);
            LOGW_CAMHW_SUBM(ISP20HW_SUBM, "don't update ldch params if restoring params from pause state\n");
        }
    } else {
        _full_active_isp_params.module_en_update = 0;
        // use module_ens to store module status, so we can use it to restore
        // the init params for re-start and re-prepare
        /* _full_active_isp_params.module_ens = 0; */
        _full_active_isp_params.module_cfg_update = 0;
    }
    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "merge isp params num %d\n", _pending_isp_meas_params_queue.size());

    _mutex.lock();
    // merge all isp other params
    while (!_pending_isp_other_params_queue.empty()) {
        aiqIspOtherResult = _pending_isp_other_params_queue.front();
        _pending_isp_other_params_queue.pop_front();

        ret = convertAiqOtherResultsToIsp20Params(update_params, aiqIspOtherResult,
                _lastAiqIspOtherResult);
        if (ret != XCAM_RETURN_NO_ERROR) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "rkisp1_convert_results error\n");
        }
    }

    if (!aiqIspOtherResult.ptr()) {
        LOGW_CAMHW_SUBM(ISP20HW_SUBM, "frame(%d) aiq isp other result params is null\n",
                        frameId);
        aiqIspOtherResult = _lastAiqIspOtherResult;
    }

    // merge all isp meas params
    while (!_pending_isp_meas_params_queue.empty()) {
        aiqIspMeasResult = _pending_isp_meas_params_queue.front();
        _pending_isp_meas_params_queue.pop_front();

        if(aiqIspMeasResult->data()->update_mask & RKAIQ_ISP_AHDRTMO_ID) {
            ret = overrideExpRatioToAiqResults(frameId, RK_ISP2X_HDRTMO_ID,
                                               aiqIspMeasResult, aiqIspOtherResult);
            if (ret < 0) {
                LOGE_CAMHW_SUBM(ISP20HW_SUBM, "convertExpRatioToAiqResults error!\n");
            }
        }

        ret = convertAiqMeasResultsToIsp20Params(update_params, aiqIspMeasResult,
                aiqIspOtherResult, _lastAiqIspMeasResult);
        if (ret != XCAM_RETURN_NO_ERROR) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "rkisp1_convert_results error\n");
        }
    }
    _mutex.unlock();

    if (!aiqIspMeasResult.ptr()) {
        LOGW_CAMHW_SUBM(ISP20HW_SUBM, "frame(%d) aiq isp meas result params is null\n",
                        frameId);
        aiqIspMeasResult = _lastAiqIspMeasResult;
    }

    if (frameId > aiqIspMeasResult->data()->frame_id ||
            frameId > aiqIspOtherResult->data()->frame_id)
        LOGW_CAMHW_SUBM(ISP20HW_SUBM, "isp stream sequence(%d) != aiq result params id(%d)\n",
                        frameId,  aiqIspMeasResult->data()->frame_id);

    if (mIspSpThread.ptr()) {
        mIspSpThread->update_motion_detection_params(&aiqIspOtherResult->data()->motion_param);
        mIspSpThread->update_af_meas_params(&aiqIspMeasResult->data()->af_meas);
    }

    forceOverwriteAiqIspCfg(update_params, aiqIspMeasResult, aiqIspOtherResult);
    gen_full_isp_params(&update_params, &_full_active_isp_params);

    if (_state == CAM_HW_STATE_STOPPED)
        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "ispparam ens 0x%llx, en_up 0x%llx, cfg_up 0x%llx",
                        _full_active_isp_params.module_ens,
                        _full_active_isp_params.module_en_update,
                        _full_active_isp_params.module_cfg_update);

#ifdef RUNTIME_MODULE_DEBUG
    _full_active_isp_params.module_en_update &= ~g_disable_isp_modules_en;
    _full_active_isp_params.module_ens |= g_disable_isp_modules_en;
    _full_active_isp_params.module_cfg_update &= ~g_disable_isp_modules_cfg_update;
#endif
    dump_isp_config(&_full_active_isp_params, aiqIspMeasResult, aiqIspOtherResult);

    if (mIspParamsDev.ptr()) {
        struct isp2x_isp_params_cfg* isp_params;
        SmartPtr<V4l2Buffer> v4l2buf;

        ret = mIspParamsDev->get_buffer(v4l2buf);
        if (!ret) {
            int buf_index = v4l2buf->get_buf().index;

            isp_params = (struct isp2x_isp_params_cfg*)v4l2buf->get_buf().m.userptr;
            *isp_params = _full_active_isp_params;
            isp_params->frame_id = frameId;

            SmartPtr<SensorHw> mSensorSubdev = mSensorDev.dynamic_cast_ptr<SensorHw>();
            if (mSensorSubdev.ptr()) {
                memset(&isp_params->exposure, 0, sizeof(isp_params->exposure));
                SmartPtr<RkAiqExpParamsProxy> expParam;

                if (mSensorSubdev->getEffectiveExpParams(expParam, frameId) < 0) {
                    LOGE_CAMHW_SUBM(ISP20HW_SUBM, "frame_id(%d), get exposure failed!!!\n", frameId);
                } else {
                    if (RK_AIQ_HDR_GET_WORKING_MODE(_hdr_mode) == RK_AIQ_WORKING_MODE_NORMAL) {
                        isp_params->exposure.linear_exp.analog_gain_code_global = \
                                expParam->data()->aecExpInfo.LinearExp.exp_sensor_params.analog_gain_code_global;
                        isp_params->exposure.linear_exp.coarse_integration_time = \
                                expParam->data()->aecExpInfo.LinearExp.exp_sensor_params.coarse_integration_time;
                    } else {
                        isp_params->exposure.hdr_exp[0].analog_gain_code_global = \
                                expParam->data()->aecExpInfo.HdrExp[0].exp_sensor_params.analog_gain_code_global;
                        isp_params->exposure.hdr_exp[0].coarse_integration_time = \
                                expParam->data()->aecExpInfo.HdrExp[0].exp_sensor_params.coarse_integration_time;
                        isp_params->exposure.hdr_exp[1].analog_gain_code_global = \
                                expParam->data()->aecExpInfo.HdrExp[1].exp_sensor_params.analog_gain_code_global;
                        isp_params->exposure.hdr_exp[1].coarse_integration_time = \
                                expParam->data()->aecExpInfo.HdrExp[1].exp_sensor_params.coarse_integration_time;
                        isp_params->exposure.hdr_exp[2].analog_gain_code_global = \
                                expParam->data()->aecExpInfo.HdrExp[2].exp_sensor_params.analog_gain_code_global;
                        isp_params->exposure.hdr_exp[2].coarse_integration_time = \
                                expParam->data()->aecExpInfo.HdrExp[2].exp_sensor_params.coarse_integration_time;
                    }
                }
            }

            if (mIspParamsDev->queue_buffer (v4l2buf) != 0) {
                LOGE_CAMHW_SUBM(ISP20HW_SUBM, "RKISP1: failed to ioctl VIDIOC_QBUF for index %d, %d %s.\n",
                                buf_index, errno, strerror(errno));
                return ret;
            }

            ispModuleEns = _full_active_isp_params.module_ens;
            _mutex.lock();
            _effectingIspMeasParmMap[frameId] = aiqIspMeasResult;
            _mutex.unlock();
            LOGD_CAMHW_SUBM(ISP20HW_SUBM, "ispparam ens 0x%llx, en_up 0x%llx, cfg_up 0x%llx",
                            _full_active_isp_params.module_ens,
                            _full_active_isp_params.module_en_update,
                            _full_active_isp_params.module_cfg_update);

            LOGD_CAMHW_SUBM(ISP20HW_SUBM, "device(%s) queue buffer index %d, queue cnt %d, check exit status again[exit: %d]",
                            XCAM_STR (mIspParamsDev->get_device_name()),
                            buf_index, mIspParamsDev->get_queued_bufcnt(), _is_exit);
        } else {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "Can not get isp params buffer for frame %d \n", frameId);
        }

        if (_is_exit)
            return XCAM_RETURN_BYPASS;
    }

    EXIT_CAMHW_FUNCTION();
    return ret;
}

XCamReturn
CamHwIsp20::setIspMeasParams(SmartPtr<RkAiqIspMeasParamsProxy>& ispMeasParams)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ENTER_CAMHW_FUNCTION();
    if (_is_exit) {
        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "set 3a config bypass since ia engine has stop");
        return XCAM_RETURN_BYPASS;
    }

    _mutex.lock();
    if (_pending_isp_meas_params_queue.size() > 6) {
        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "too many pending isp params:%d !", _pending_isp_meas_params_queue.size());
        _pending_isp_meas_params_queue.erase(_pending_isp_meas_params_queue.begin());
    }
    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "isp push id(%d) meas params\n", ispMeasParams->data()->frame_id);
    _pending_isp_meas_params_queue.push_back(ispMeasParams);
    _mutex.unlock();

    if (_state == CAM_HW_STATE_PREPARED || _state == CAM_HW_STATE_STOPPED ||
            _state == CAM_HW_STATE_PAUSED) {
        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "hdr-debug: %s: first set ispparams id[%d]\n",
                        __func__, ispMeasParams->data()->frame_id);
        if (!mIspParamsDev->is_activated()) {
            ret = mIspParamsDev->start();
            if (ret < 0) {
                LOGE_CAMHW_SUBM(ISP20HW_SUBM, "prepare isp params dev err: %d\n", ret);
            }
        }

        setIspParamsSync(ispMeasParams->data()->frame_id);

        if ((mCalibDb->mfnr.enable && mCalibDb->mfnr.motion_detect_en) || mCalibDb->af.ldg_param.enable) {
            if (!mIspSpDev->is_activated()) {
                ret = mIspSpDev->prepare();
                if (ret < 0) {
                    LOGE_CAMHW_SUBM(ISP20HW_SUBM, "mIspSpDev prepare failed !\n");
                }
            }
        }
        ret = hdr_mipi_prepare_mode(_hdr_mode);
        if (ret < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "hdr mipi start err: %d\n", ret);
        }
    }

    if (RK_AIQ_HDR_GET_WORKING_MODE(_hdr_mode) == RK_AIQ_WORKING_MODE_NORMAL &&
            mNormalNoReadBack)
        setIspParamsSync(ispMeasParams->data()->frame_id);

    EXIT_CAMHW_FUNCTION();
    return ret;
}

XCamReturn
CamHwIsp20::setIspOtherParams(SmartPtr<RkAiqIspOtherParamsProxy>& ispOtherParams)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ENTER_CAMHW_FUNCTION();
    if (_is_exit) {
        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "set 3a config bypass since ia engine has stop");
        return XCAM_RETURN_BYPASS;
    }

    _mutex.lock();
    if (_pending_isp_other_params_queue.size() > 6) {
        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "too many pending isp params:%d !", _pending_isp_other_params_queue.size());
        _pending_isp_other_params_queue.erase(_pending_isp_other_params_queue.begin());
    }
    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "isp push id(%d) other params\n", ispOtherParams->data()->frame_id);
    _pending_isp_other_params_queue.push_back(ispOtherParams);
    _mutex.unlock();

    EXIT_CAMHW_FUNCTION();
    return ret;
}

XCamReturn
CamHwIsp20::setIsppMeasParams(SmartPtr<RkAiqIsppMeasParamsProxy>& isppParams)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ENTER_CAMHW_FUNCTION();
    if (_is_exit) {
        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "set 3a config bypass since ia engine has stop");
        return XCAM_RETURN_BYPASS;
    }

    _mutex.lock();
    if (_pending_ispp_meas_params_queue.size() > 8) {
        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "too many pending ispp params:%d !", _pending_ispp_meas_params_queue.size());
        _pending_ispp_meas_params_queue.erase(_pending_ispp_meas_params_queue.begin());
    }
    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "ispp push id(%d) meas params\n", isppParams->data()->frame_id);
    _pending_ispp_meas_params_queue.push_back(isppParams);
    _mutex.unlock();

    if (_state == CAM_HW_STATE_PREPARED || _state == CAM_HW_STATE_STOPPED ||
            _state == CAM_HW_STATE_PAUSED) {
        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "hdr-debug: %s: first set isppParams id[%d]\n",
                        __func__, isppParams->data()->frame_id);
        if (!mIsppParamsDev->is_activated()) {
            ret = mIsppParamsDev->start();
            if (ret < 0) {
                LOGE_CAMHW_SUBM(ISP20HW_SUBM, "prepare ispp params dev err: %d\n", ret);
            }
        }
        setIsppParamsSync(isppParams->data()->frame_id);
    }

    if (RK_AIQ_HDR_GET_WORKING_MODE(_hdr_mode) == RK_AIQ_WORKING_MODE_NORMAL &&
            mNormalNoReadBack)
        setIsppParamsSync(isppParams->data()->frame_id);

    EXIT_CAMHW_FUNCTION();
    return ret;
}

XCamReturn
CamHwIsp20::setIsppOtherParams(SmartPtr<RkAiqIsppOtherParamsProxy>& isppOtherParams)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ENTER_CAMHW_FUNCTION();
    if (_is_exit) {
        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "set 3a config bypass since ia engine has stop");
        return XCAM_RETURN_BYPASS;
    }

    _mutex.lock();
    if (_pending_ispp_other_params_queue.size() > 8) {
        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "too many pending ispp params:%d !", _pending_ispp_other_params_queue.size());
        _pending_ispp_other_params_queue.erase(_pending_ispp_other_params_queue.begin());
    }
    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "ispp push id(%d) other params\n", isppOtherParams->data()->frame_id);
    _pending_ispp_other_params_queue.push_back(isppOtherParams);
    _mutex.unlock();

    if (_state == CAM_HW_STATE_PREPARED || _state == CAM_HW_STATE_STOPPED ||
            _state == CAM_HW_STATE_PAUSED) {
        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "hdr-debug: %s: first set isppParams id[%d]\n",
                        __func__, isppOtherParams->data()->frame_id);
        if (!mIsppParamsDev->is_activated()) {
            ret = mIsppParamsDev->start();
            if (ret < 0) {
                LOGE_CAMHW_SUBM(ISP20HW_SUBM, "prepare ispp params dev err: %d\n", ret);
            }
        }
        setIsppParamsSync(isppOtherParams->data()->frame_id);
    }

    if (RK_AIQ_HDR_GET_WORKING_MODE(_hdr_mode) == RK_AIQ_WORKING_MODE_NORMAL &&
            mNormalNoReadBack)
        setIsppParamsSync(isppOtherParams->data()->frame_id);

    EXIT_CAMHW_FUNCTION();
    return ret;
}

XCamReturn
CamHwIsp20::setIsppSharpFbcRot(struct rkispp_sharp_config* shp_cfg)
{
    // check if sharp enable
    // check if fec disable
    if ((_ispp_module_init_ens & ISPP_MODULE_SHP) &&
            !(_ispp_module_init_ens & ISPP_MODULE_FEC)) {
        switch (_sharp_fbc_rotation) {
        case RK_AIQ_ROTATION_0 :
            shp_cfg->rotation = 0;
            break;
        case RK_AIQ_ROTATION_90 :
            shp_cfg->rotation = 1;
            break;
        case RK_AIQ_ROTATION_270 :
            shp_cfg->rotation = 3;
            break;
        default:
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "wrong rotation %d\n", _sharp_fbc_rotation);
            return XCAM_RETURN_ERROR_PARAM;
        }
    } else {
        if (_sharp_fbc_rotation != RK_AIQ_ROTATION_0) {
            shp_cfg->rotation = 0;
            _sharp_fbc_rotation = RK_AIQ_ROTATION_0;
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "can't set sharp config, check fec & sharp config\n");
            return XCAM_RETURN_ERROR_PARAM;
        }
    }

    LOGD("sharp rotation %d", _sharp_fbc_rotation);

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
CamHwIsp20::setIsppParamsSync(int frameId)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ENTER_CAMHW_FUNCTION();


    _mutex.lock();
    if (_is_exit) {
        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "check if ia engine has stop");
        _mutex.unlock();
        return XCAM_RETURN_BYPASS;
    }

    if (_pending_ispp_meas_params_queue.empty() && \
            _pending_ispp_other_params_queue.empty()) {
        LOGW_CAMHW_SUBM(ISP20HW_SUBM,
                        "no new ispp %s params for frame %d !",
                        _pending_ispp_meas_params_queue.empty() ? "meas" : "other",
                        frameId);
        _mutex.unlock();
        return ret;
    }

    _mutex.unlock();

    if (mIsppParamsDev.ptr()) {
        struct rkispp_params_cfg* ispp_params;
        SmartPtr<V4l2Buffer> v4l2buf;
        SmartPtr<RkAiqIsppMeasParamsProxy> isppMeasParams = nullptr;
        SmartPtr<RkAiqIsppOtherParamsProxy> isppOtherParams = nullptr;

        ret = mIsppParamsDev->get_buffer(v4l2buf);
        if (!ret) {
            int buf_index = v4l2buf->get_buf().index;

            ispp_params = (struct rkispp_params_cfg*)v4l2buf->get_buf().m.userptr;
            ispp_params->module_en_update = 0;
            ispp_params->module_ens = 0;
            ispp_params->module_cfg_update = 0;
            ispp_params->frame_id = frameId;

            bool update_full = false;
            // restore params for re-start and re-prepare
            if (_state == CAM_HW_STATE_STOPPED || _state == CAM_HW_STATE_PREPARED || _state == CAM_HW_STATE_PAUSED) {
                // TODO: don't know why updating all ens will lead to no frame output
                _full_active_ispp_params.module_en_update = _full_active_ispp_params.module_ens;
                // just re-config the enabled moddules
                _full_active_ispp_params.module_cfg_update = _full_active_ispp_params.module_ens;

                // don't update fec params if restoring params from pause state
                if (_state == CAM_HW_STATE_PAUSED) {
                    _full_active_ispp_params.module_en_update &= ~(1 << (RK_ISP2X_PP_TFEC_ID - RK_ISP2X_PP_TNR_ID));
                    _full_active_ispp_params.module_cfg_update &= ~(1 << (RK_ISP2X_PP_TFEC_ID - RK_ISP2X_PP_TNR_ID));
                }
                update_full = true;
            } else {
                _full_active_ispp_params.module_en_update = 0;
                _full_active_ispp_params.module_cfg_update = 0;
            }

            // merge all ispp params
            _mutex.lock();
            while (!_pending_ispp_meas_params_queue.empty()) {
                isppMeasParams = _pending_ispp_meas_params_queue.front();
                _pending_ispp_meas_params_queue.pop_front();
                convertAiqMeasResultsToIsp20PpParams(*ispp_params, isppMeasParams, _lastAiqIsppMeasResult);
            }

            if (!isppMeasParams.ptr()) {
                LOGW_CAMHW_SUBM(ISP20HW_SUBM, "frame(%d) aiq ispp meas result params is null\n",
                                frameId);
                isppMeasParams = _lastAiqIsppMeasResult;
            }

            while (!_pending_ispp_other_params_queue.empty()) {
                isppOtherParams = _pending_ispp_other_params_queue.front();
                _pending_ispp_other_params_queue.pop_front();
                convertAiqOtherResultsToIsp20PpParams(*ispp_params, isppOtherParams, _lastAiqIsppOtherResult);
                forceOverwriteAiqIsppCfg(*ispp_params, nullptr, isppOtherParams);
            }

            if (!isppOtherParams.ptr()) {
                LOGW_CAMHW_SUBM(ISP20HW_SUBM, "frame(%d) aiq ispp other result params is null\n",
                                frameId);
                isppOtherParams = _lastAiqIsppOtherResult;
            }
            _mutex.unlock();

            forceOverwriteAiqIsppCfg(*ispp_params, isppMeasParams, isppOtherParams);

            u64 last_module_ens = _full_active_ispp_params.module_ens;
            gen_full_ispp_params(ispp_params, &_full_active_ispp_params);

            LOGD_CAMHW_SUBM(ISP20HW_SUBM, "ispp full en update 0x%x, ens 0x%x, cfg update 0x%x",
                            _full_active_ispp_params.module_en_update,
                            _full_active_ispp_params.module_ens,
                            _full_active_ispp_params.module_cfg_update);

            if (update_full) {
                // replace ens,update with _full_active_ispp_params
                ispp_params->module_en_update = _full_active_ispp_params.module_en_update;
                ispp_params->module_ens = _full_active_ispp_params.module_ens;
                ispp_params->module_cfg_update = _full_active_ispp_params.module_cfg_update;
            } else {
                int end = RK_ISP2X_PP_MAX_ID - RK_ISP2X_PP_TNR_ID;
                u64 new_module_ens_update = 0;

                for (int i = 0; i < end; i++)
                    if ((last_module_ens & (1 << i)) != (_full_active_ispp_params.module_ens & (1 << i))) {
                        new_module_ens_update |= 1 << i;
                    }

                ispp_params->module_en_update = new_module_ens_update;
                ispp_params->module_ens = _full_active_ispp_params.module_ens;
                ispp_params->module_cfg_update = _full_active_ispp_params.module_cfg_update;
                LOGD_CAMHW_SUBM(ISP20HW_SUBM, "ispp new en update 0x%x, ens 0x%x, cfg update 0x%x",
                                ispp_params->module_en_update, ispp_params->module_ens,
                                ispp_params->module_cfg_update);
            }

            LOGD_CAMHW_SUBM(ISP20HW_SUBM, "module_init_ens frome drv 0x%x\n", ispp_params->module_init_ens);

#ifdef RUNTIME_MODULE_DEBUG
            ispp_params->module_en_update &= ~g_disable_ispp_modules_en;
            ispp_params->module_ens |= g_disable_ispp_modules_en;
            ispp_params->module_cfg_update &= ~g_disable_ispp_modules_cfg_update;
#endif
            if (_state == CAM_HW_STATE_PREPARED)
                _ispp_module_init_ens = ispp_params->module_init_ens;
            else {
                if (_ispp_module_init_ens != ispp_params->module_init_ens) {
                    LOGE_CAMHW_SUBM(ISP20HW_SUBM, "ispp working mode changed from 0x%x to 0x%x\n",
                                    _ispp_module_init_ens, ispp_params->module_init_ens);
                    ispp_params->module_init_ens = _ispp_module_init_ens;
                }
            }

            setIsppSharpFbcRot(&ispp_params->shp_cfg);

            //TODO set update bits
            if (mIsppParamsDev->queue_buffer (v4l2buf) != 0) {
                LOGE_CAMHW_SUBM(ISP20HW_SUBM, "RKISP1: failed to ioctl VIDIOC_QBUF for index %d, %d %s.\n",
                                buf_index, errno, strerror(errno));
                return ret;
            }

            LOGD_CAMHW_SUBM(ISP20HW_SUBM, "device(%s) queue buffer index %d, queue cnt %d, check exit status again[exit: %d]",
                            XCAM_STR (mIsppParamsDev->get_device_name()),
                            buf_index, mIsppParamsDev->get_queued_bufcnt(), _is_exit);
        } else {
            LOGD_CAMHW_SUBM(ISP20HW_SUBM, "Can not get ispp params buffer for frame %d .\n", frameId);
        }

        if (_is_exit)
            return XCAM_RETURN_BYPASS;
    }

    EXIT_CAMHW_FUNCTION();
    return ret;
}

XCamReturn
CamHwIsp20::getSensorModeData(const char* sns_ent_name,
                              rk_aiq_exposure_sensor_descriptor& sns_des)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    const SmartPtr<BaseSensorHw> mSensorSubdev = mSensorDev.dynamic_cast_ptr<BaseSensorHw>();
    const SmartPtr<LensHw> mLensSubdev = mLensDev.dynamic_cast_ptr<LensHw>();
    struct v4l2_subdev_selection select;

    ret = mSensorSubdev->getSensorModeData(sns_ent_name, sns_des);
    if (ret) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "getSensorModeData failed \n");
        return ret;
    }

    xcam_mem_clear (select);
    ret = mIspCoreDev->get_selection(0, V4L2_SEL_TGT_CROP, select);
    if (ret == XCAM_RETURN_NO_ERROR) {
        sns_des.isp_acq_width = select.r.width;
        sns_des.isp_acq_height = select.r.height;
        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "get isp acq,w: %d, h: %d\n",
                        sns_des.isp_acq_width,
                        sns_des.isp_acq_height);
    } else {
        LOGW_CAMHW_SUBM(ISP20HW_SUBM, "get selecttion error \n");
        sns_des.isp_acq_width = sns_des.sensor_output_width;
        sns_des.isp_acq_height = sns_des.sensor_output_height;
        ret = XCAM_RETURN_NO_ERROR;
    }

    xcam_mem_clear (sns_des.lens_des);
    if (mLensSubdev.ptr())
        mLensSubdev->getLensModeData(sns_des.lens_des);

    return ret;
}

XCamReturn
CamHwIsp20::setExposureParams(SmartPtr<RkAiqExpParamsProxy>& expPar)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ENTER_CAMHW_FUNCTION();
    SmartPtr<BaseSensorHw> mSensorSubdev = mSensorDev.dynamic_cast_ptr<BaseSensorHw>();
    SmartPtr<LensHw> mLensSubdev = mLensDev.dynamic_cast_ptr<LensHw>();

    //exp
    ret = mSensorSubdev->setExposureParams(expPar);
#if 0
    //P-iris
    int step = expPar->data()->aecExpInfo.Iris.PIris.step;
    bool update = expPar->data()->aecExpInfo.Iris.PIris.update;

    if (mLensSubdev.ptr() && update) {
        LOGE("|||set P-Iris step: %d", step);
        if (mLensSubdev->setPIrisParams(step) < 0) {
            LOGE("set P-Iris step failed to device");
            return XCAM_RETURN_ERROR_IOCTL;
        }
    }

    //DC-iris
    int PwmDuty = expPar->data()->aecExpInfo.Iris.DCIris.pwmDuty;
    bool update = expPar->data()->aecExpInfo.Iris.DCIris.update;

    if (mLensSubdev.ptr() && update) {
        LOGE("|||set DC-Iris step: %d", PwmDuty);
        if (mLensSubdev->setPIrisParams(PwmDuty) < 0) {
            LOGE("set DC-Iris step failed to device");
            return XCAM_RETURN_ERROR_IOCTL;
        }
    }

#endif
    EXIT_CAMHW_FUNCTION();
    return ret;
}

XCamReturn
CamHwIsp20::setIrisParams(SmartPtr<RkAiqIrisParamsProxy>& irisPar, CalibDb_IrisType_t irisType)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ENTER_CAMHW_FUNCTION();
    SmartPtr<LensHw> mLensSubdev = mLensDev.dynamic_cast_ptr<LensHw>();

    if(irisType == IRIS_P_TYPE) {   //P-iris
        int step = irisPar->data()->PIris.step;
        bool update = irisPar->data()->PIris.update;

        if (mLensSubdev.ptr() && update) {
            LOGE("|||set P-Iris step: %d", step);
            if (mLensSubdev->setPIrisParams(step) < 0) {
                LOGE("set P-Iris step failed to device");
                return XCAM_RETURN_ERROR_IOCTL;
            }
        }
    } else if(irisType == IRIS_DC_TYPE) {
        //DC-iris
        int PwmDuty = irisPar->data()->DCIris.pwmDuty;
        bool update = irisPar->data()->DCIris.update;

        if (mLensSubdev.ptr() && update) {
            LOGE("|||set DC-Iris PwmDuty: %d", PwmDuty);
            if (mLensSubdev->setDCIrisParams(PwmDuty) < 0) {
                LOGE("set DC-Iris PwmDuty failed to device");
                return XCAM_RETURN_ERROR_IOCTL;
            }
        }
    }
    EXIT_CAMHW_FUNCTION();
    return ret;
}


XCamReturn
CamHwIsp20::setFocusParams(SmartPtr<RkAiqFocusParamsProxy>& focus_params)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ENTER_CAMHW_FUNCTION();
    SmartPtr<LensHw> mLensSubdev = mLensDev.dynamic_cast_ptr<LensHw>();
    bool focus_valid = focus_params->data()->lens_pos_valid;
    bool zoom_valid = focus_params->data()->zoom_pos_valid;
    bool focus_correction = focus_params->data()->focus_correction;
    bool zoom_correction = focus_params->data()->zoom_correction;
    bool zoomfocus_modifypos = focus_params->data()->zoomfocus_modifypos;
    bool end_zoom_chg = focus_params->data()->end_zoom_chg;

    if (!mLensSubdev.ptr())
        goto OUT;

    if (zoomfocus_modifypos)
        mLensSubdev->ZoomFocusModifyPosition(focus_params);
    if (focus_correction)
        mLensSubdev->FocusCorrection();
    if (zoom_correction)
        mLensSubdev->ZoomCorrection();

    if (focus_valid && !zoom_valid) {
        if (mLensSubdev->setFocusParams(focus_params) < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "set focus result failed to device");
            return XCAM_RETURN_ERROR_IOCTL;
        }
    } else if ((focus_valid && zoom_valid) || end_zoom_chg) {
        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "|||setZoomFocusParams");
        if (mLensSubdev->setZoomFocusParams(focus_params) < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "set setZoomFocusParams failed to device");
            return XCAM_RETURN_ERROR_IOCTL;
        }
    }

OUT:
    EXIT_CAMHW_FUNCTION();
    return ret;
}

XCamReturn
CamHwIsp20::getZoomPosition(int& position)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ENTER_CAMHW_FUNCTION();
    SmartPtr<LensHw> mLensSubdev = mLensDev.dynamic_cast_ptr<LensHw>();

    if (mLensSubdev.ptr()) {
        if (mLensSubdev->getZoomParams(&position) < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "get zoom result failed to device");
            return XCAM_RETURN_ERROR_IOCTL;
        }
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "|||get zoom result: %d", position);
    }

    EXIT_CAMHW_FUNCTION();
    return ret;
}

XCamReturn
CamHwIsp20::setLensVcmCfg(rk_aiq_lens_vcmcfg& lens_cfg)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ENTER_CAMHW_FUNCTION();
    SmartPtr<LensHw> mLensSubdev = mLensDev.dynamic_cast_ptr<LensHw>();

    if (mLensSubdev.ptr()) {
        if (mLensSubdev->setLensVcmCfg(lens_cfg) < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "set vcm config failed");
            return XCAM_RETURN_ERROR_IOCTL;
        }
    }

    EXIT_CAMHW_FUNCTION();
    return ret;
}

XCamReturn
CamHwIsp20::FocusCorrection()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ENTER_CAMHW_FUNCTION();
    SmartPtr<LensHw> mLensSubdev = mLensDev.dynamic_cast_ptr<LensHw>();

    if (mLensSubdev.ptr()) {
        if (mLensSubdev->FocusCorrection() < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "focus correction failed");
            return XCAM_RETURN_ERROR_IOCTL;
        }
    }

    EXIT_CAMHW_FUNCTION();
    return ret;
}

XCamReturn
CamHwIsp20::ZoomCorrection()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ENTER_CAMHW_FUNCTION();
    SmartPtr<LensHw> mLensSubdev = mLensDev.dynamic_cast_ptr<LensHw>();

    if (mLensSubdev.ptr()) {
        if (mLensSubdev->ZoomCorrection() < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "zoom correction failed");
            return XCAM_RETURN_ERROR_IOCTL;
        }
    }

    EXIT_CAMHW_FUNCTION();
    return ret;
}

XCamReturn
CamHwIsp20::getLensVcmCfg(rk_aiq_lens_vcmcfg& lens_cfg)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ENTER_CAMHW_FUNCTION();
    SmartPtr<LensHw> mLensSubdev = mLensDev.dynamic_cast_ptr<LensHw>();

    if (mLensSubdev.ptr()) {
        if (mLensSubdev->getLensVcmCfg(lens_cfg) < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "get vcm config failed");
            return XCAM_RETURN_ERROR_IOCTL;
        }
    }

    EXIT_CAMHW_FUNCTION();
    return ret;
}

XCamReturn
CamHwIsp20::setCpslParams(SmartPtr<RkAiqCpslParamsProxy>& cpsl_params)
{
    ENTER_CAMHW_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RKAiqCpslInfoWrapper_t* cpsl_setting = cpsl_params->data().ptr();

    if (cpsl_setting->update_fl) {
        rk_aiq_flash_setting_t* fl_setting = &cpsl_setting->fl;
        if (mFlashLight.ptr()) {
            ret = mFlashLight->set_params(*fl_setting);
            if (ret < 0) {
                LOGE_CAMHW_SUBM(ISP20HW_SUBM, "set flashlight params err: %d\n", ret);
            }
        }
    }

    if (cpsl_setting->update_ir) {
        rk_aiq_ir_setting_t* ir_setting = &cpsl_setting->ir;
        ret = setIrcutParams(ir_setting->irc_on);
        if (ret < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "set ir params err: %d\n", ret);
        }

        rk_aiq_flash_setting_t* fl_ir_setting = &cpsl_setting->fl_ir;

        if (mFlashLightIr.ptr()) {
            ret = mFlashLightIr->set_params(*fl_ir_setting);
            if (ret < 0) {
                LOGE_CAMHW_SUBM(ISP20HW_SUBM, "set flashlight ir params err: %d\n", ret);
            }
        }
    }

    EXIT_CAMHW_FUNCTION();
    return ret;
}

XCamReturn
CamHwIsp20::setHdrProcessCount(rk_aiq_luma_params_t luma_params)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    SmartPtr<Isp20PollThread> isp20Pollthread;

    ENTER_CAMHW_FUNCTION();

    isp20Pollthread = mPollthread.dynamic_cast_ptr<Isp20PollThread>();
    isp20Pollthread->set_hdr_frame_readback_infos(luma_params);

    EXIT_CAMHW_FUNCTION();
    return ret;
}


XCamReturn
CamHwIsp20::getEffectiveIspParams(SmartPtr<RkAiqIspMeasParamsProxy>& ispParams, int frame_id)
{
    ENTER_CAMHW_FUNCTION();
    std::map<int, SmartPtr<RkAiqIspMeasParamsProxy>>::iterator it;
    int search_id = frame_id < 0 ? 0 : frame_id;
    SmartLock locker (_mutex);

    if (_effectingIspMeasParmMap.size() == 0) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "can't search id %d,  _effecting_exp_mapsize is %d\n",
                        frame_id, _effectingIspMeasParmMap.size());
        return  XCAM_RETURN_ERROR_PARAM;
    }

    it = _effectingIspMeasParmMap.find(search_id);

    // havn't found
    if (it == _effectingIspMeasParmMap.end()) {
        std::map<int, SmartPtr<RkAiqIspMeasParamsProxy>>::reverse_iterator rit;

        rit = _effectingIspMeasParmMap.rbegin();
        do {
            LOGD_CAMHW_SUBM(ISP20HW_SUBM, "traverse _effectingIspMeasParmMap to find id %d, current id is [%d]\n",
                            search_id, rit->first);
            if (search_id >= rit->first ) {
                LOGD_CAMHW_SUBM(ISP20HW_SUBM, "exp-sync: can't find id %d, get latest id %d in _effectingIspMeasParmMap\n",
                                search_id, rit->first);
                break;
            }
        } while (++rit != _effectingIspMeasParmMap.rend());

        if (rit == _effectingIspMeasParmMap.rend()) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "can't find the latest effecting exposure for id %d, impossible case !", frame_id);
            return XCAM_RETURN_ERROR_PARAM;
        }

        ispParams = rit->second;
    } else {
        ispParams = it->second;
    }

    while (_effectingIspMeasParmMap.size() > 4)
        _effectingIspMeasParmMap.erase(_effectingIspMeasParmMap.begin());

    EXIT_CAMHW_FUNCTION();

    return XCAM_RETURN_NO_ERROR;
}

void CamHwIsp20::dumpRawnrFixValue(struct isp2x_rawnr_cfg * pRawnrCfg )
{
    printf("%s:(%d)  enter \n", __FUNCTION__, __LINE__);
    int i = 0;

    //(0x0004)
    printf("(0x0004) gauss_en:%d log_bypass:%d \n",
           pRawnrCfg->gauss_en,
           pRawnrCfg->log_bypass);

    //(0x0008 - 0x0010)
    printf("(0x0008 - 0x0010) filtpar0-2:%d %d %d \n",
           pRawnrCfg->filtpar0,
           pRawnrCfg->filtpar1,
           pRawnrCfg->filtpar2);

    //(0x0014 - 0x001c)
    printf("(0x0014 - 0x001c) dgain0-2:%d %d %d \n",
           pRawnrCfg->dgain0,
           pRawnrCfg->dgain1,
           pRawnrCfg->dgain2);

    //(0x0020 - 0x002c)
    for(int i = 0; i < ISP2X_RAWNR_LUMA_RATION_NUM; i++) {
        printf("(0x0020 - 0x002c) luration[%d]:%d \n",
               i, pRawnrCfg->luration[i]);
    }

    //(0x0030 - 0x003c)
    for(int i = 0; i < ISP2X_RAWNR_LUMA_RATION_NUM; i++) {
        printf("(0x0030 - 0x003c) lulevel[%d]:%d \n",
               i, pRawnrCfg->lulevel[i]);
    }

    //(0x0040)
    printf("(0x0040) gauss:%d \n",
           pRawnrCfg->gauss);

    //(0x0044)
    printf("(0x0044) sigma:%d \n",
           pRawnrCfg->sigma);

    //(0x0048)
    printf("(0x0048) pix_diff:%d \n",
           pRawnrCfg->pix_diff);

    //(0x004c)
    printf("(0x004c) thld_diff:%d \n",
           pRawnrCfg->thld_diff);

    //(0x0050)
    printf("(0x0050) gas_weig_scl1:%d  gas_weig_scl2:%d  thld_chanelw:%d \n",
           pRawnrCfg->gas_weig_scl1,
           pRawnrCfg->gas_weig_scl2,
           pRawnrCfg->thld_chanelw);

    //(0x0054)
    printf("(0x0054) lamda:%d \n",
           pRawnrCfg->lamda);

    //(0x0058 - 0x005c)
    printf("(0x0058 - 0x005c) fixw0-3:%d %d %d %d\n",
           pRawnrCfg->fixw0,
           pRawnrCfg->fixw1,
           pRawnrCfg->fixw2,
           pRawnrCfg->fixw3);

    //(0x0060 - 0x0068)
    printf("(0x0060 - 0x0068) wlamda0-2:%d %d %d\n",
           pRawnrCfg->wlamda0,
           pRawnrCfg->wlamda1,
           pRawnrCfg->wlamda2);


    //(0x006c)
    printf("(0x006c) rgain_filp-2:%d bgain_filp:%d\n",
           pRawnrCfg->rgain_filp,
           pRawnrCfg->bgain_filp);


    printf("%s:(%d)  exit \n", __FUNCTION__, __LINE__);
}



void CamHwIsp20::dumpTnrFixValue(struct rkispp_tnr_config  * pTnrCfg)
{
    int i = 0;
    printf("%s:(%d) enter \n", __FUNCTION__, __LINE__);
    //0x0080
    printf("(0x0080) opty_en:%d optc_en:%d gain_en:%d\n",
           pTnrCfg->opty_en,
           pTnrCfg->optc_en,
           pTnrCfg->gain_en);

    //0x0088
    printf("(0x0088) pk0_y:%d pk1_y:%d pk0_c:%d pk1_c:%d \n",
           pTnrCfg->pk0_y,
           pTnrCfg->pk1_y,
           pTnrCfg->pk0_c,
           pTnrCfg->pk1_c);

    //0x008c
    printf("(0x008c) glb_gain_cur:%d glb_gain_nxt:%d \n",
           pTnrCfg->glb_gain_cur,
           pTnrCfg->glb_gain_nxt);

    //0x0090
    printf("(0x0090) glb_gain_cur_div:%d gain_glb_filt_sqrt:%d \n",
           pTnrCfg->glb_gain_cur_div,
           pTnrCfg->glb_gain_cur_sqrt);

    //0x0094 - 0x0098
    for(i = 0; i < TNR_SIGMA_CURVE_SIZE - 1; i++) {
        printf("(0x0094 - 0x0098) sigma_x[%d]:%d \n",
               i, pTnrCfg->sigma_x[i]);
    }

    //0x009c - 0x00bc
    for(i = 0; i < TNR_SIGMA_CURVE_SIZE; i++) {
        printf("(0x009c - 0x00bc) sigma_y[%d]:%d \n",
               i, pTnrCfg->sigma_y[i]);
    }

    //0x00c4 - 0x00cc
    //dir_idx = 0;
    for(i = 0; i < TNR_LUMA_CURVE_SIZE; i++) {
        printf("(0x00c4 - 0x00cc) luma_curve[%d]:%d \n",
               i, pTnrCfg->luma_curve[i]);
    }

    //0x00d0
    printf("(0x00d0) txt_th0_y:%d txt_th1_y:%d \n",
           pTnrCfg->txt_th0_y,
           pTnrCfg->txt_th1_y);

    //0x00d4
    printf("(0x00d0) txt_th0_c:%d txt_th1_c:%d \n",
           pTnrCfg->txt_th0_c,
           pTnrCfg->txt_th1_c);

    //0x00d8
    printf("(0x00d8) txt_thy_dlt:%d txt_thc_dlt:%d \n",
           pTnrCfg->txt_thy_dlt,
           pTnrCfg->txt_thc_dlt);

    //0x00dc - 0x00ec
    for(i = 0; i < TNR_GFCOEF6_SIZE; i++) {
        printf("(0x00dc - 0x00ec) gfcoef_y0[%d]:%d \n",
               i, pTnrCfg->gfcoef_y0[i]);
    }
    for(i = 0; i < TNR_GFCOEF3_SIZE; i++) {
        printf("(0x00dc - 0x00ec) gfcoef_y1[%d]:%d \n",
               i, pTnrCfg->gfcoef_y1[i]);
    }
    for(i = 0; i < TNR_GFCOEF3_SIZE; i++) {
        printf("(0x00dc - 0x00ec) gfcoef_y2[%d]:%d \n",
               i, pTnrCfg->gfcoef_y2[i]);
    }
    for(i = 0; i < TNR_GFCOEF3_SIZE; i++) {
        printf("(0x00dc - 0x00ec) gfcoef_y3[%d]:%d \n",
               i, pTnrCfg->gfcoef_y3[i]);
    }

    //0x00f0 - 0x0100
    for(i = 0; i < TNR_GFCOEF6_SIZE; i++) {
        printf("(0x00f0 - 0x0100) gfcoef_yg0[%d]:%d \n",
               i, pTnrCfg->gfcoef_yg0[i]);
    }
    for(i = 0; i < TNR_GFCOEF3_SIZE; i++) {
        printf("(0x00f0 - 0x0100) gfcoef_yg1[%d]:%d \n",
               i, pTnrCfg->gfcoef_yg1[i]);
    }
    for(i = 0; i < TNR_GFCOEF3_SIZE; i++) {
        printf("(0x00f0 - 0x0100) gfcoef_yg2[%d]:%d \n",
               i, pTnrCfg->gfcoef_yg2[i]);
    }
    for(i = 0; i < TNR_GFCOEF3_SIZE; i++) {
        printf("(0x00f0 - 0x0100) gfcoef_yg3[%d]:%d \n",
               i, pTnrCfg->gfcoef_yg3[i]);
    }


    //0x0104 - 0x0110
    for(i = 0; i < TNR_GFCOEF6_SIZE; i++) {
        printf("(0x0104 - 0x0110) gfcoef_yl0[%d]:%d \n",
               i, pTnrCfg->gfcoef_yl0[i]);
    }
    for(i = 0; i < TNR_GFCOEF3_SIZE; i++) {
        printf("(0x0104 - 0x0110) gfcoef_yl1[%d]:%d \n",
               i, pTnrCfg->gfcoef_yl1[i]);
    }
    for(i = 0; i < TNR_GFCOEF3_SIZE; i++) {
        printf("(0x0104 - 0x0110) gfcoef_yl2[%d]:%d \n",
               i, pTnrCfg->gfcoef_yl2[i]);
    }

    //0x0114 - 0x0120
    for(i = 0; i < TNR_GFCOEF6_SIZE; i++) {
        printf("(0x0114 - 0x0120) gfcoef_cg0[%d]:%d \n",
               i, pTnrCfg->gfcoef_cg0[i]);
    }
    for(i = 0; i < TNR_GFCOEF3_SIZE; i++) {
        printf("(0x0114 - 0x0120) gfcoef_cg1[%d]:%d \n",
               i, pTnrCfg->gfcoef_cg1[i]);
    }
    for(i = 0; i < TNR_GFCOEF3_SIZE; i++) {
        printf("(0x0114 - 0x0120) gfcoef_cg2[%d]:%d \n",
               i, pTnrCfg->gfcoef_cg2[i]);
    }


    //0x0124 - 0x012c
    for(i = 0; i < TNR_GFCOEF6_SIZE; i++) {
        printf("(0x0124 - 0x012c) gfcoef_cl0[%d]:%d \n",
               i, pTnrCfg->gfcoef_cl0[i]);
    }
    for(i = 0; i < TNR_GFCOEF3_SIZE; i++) {
        printf("(0x0124 - 0x012c) gfcoef_cl1[%d]:%d \n",
               i, pTnrCfg->gfcoef_cl1[i]);
    }


    //0x0130 - 0x0134
    //dir_idx = 0;  i = lvl;
    for(i = 0; i < TNR_SCALE_YG_SIZE; i++) {
        printf("(0x0130 - 0x0134) scale_yg[%d]:%d \n",
               i, pTnrCfg->scale_yg[i]);
    }

    //0x0138 - 0x013c
    //dir_idx = 1;  i = lvl;
    for(i = 0; i < TNR_SCALE_YL_SIZE; i++) {
        printf("(0x0138 - 0x013c) scale_yl[%d]:%d \n",
               i, pTnrCfg->scale_yl[i]);
    }

    //0x0140 - 0x0148
    //dir_idx = 0;  i = lvl;
    for(i = 0; i < TNR_SCALE_CG_SIZE; i++) {
        printf("(0x0140 - 0x0148) scale_cg[%d]:%d \n",
               i, pTnrCfg->scale_cg[i]);
        printf("(0x0140 - 0x0148) scale_y2cg[%d]:%d \n",
               i, pTnrCfg->scale_y2cg[i]);
    }

    //0x014c - 0x0154
    //dir_idx = 1;  i = lvl;
    for(i = 0; i < TNR_SCALE_CL_SIZE; i++) {
        printf("(0x014c - 0x0154) scale_cl[%d]:%d \n",
               i, pTnrCfg->scale_cl[i]);
    }
    for(i = 0; i < TNR_SCALE_Y2CL_SIZE; i++) {
        printf("(0x014c - 0x0154) scale_y2cl[%d]:%d \n",
               i, pTnrCfg->scale_y2cl[i]);
    }

    //0x0158
    for(i = 0; i < TNR_WEIGHT_Y_SIZE; i++) {
        printf("(0x0158) weight_y[%d]:%d \n",
               i, pTnrCfg->weight_y[i]);
    }

    printf("%s:(%d) exit \n", __FUNCTION__, __LINE__);
}


void CamHwIsp20::dumpUvnrFixValue(struct rkispp_nr_config  * pNrCfg)
{
    int i = 0;
    printf("%s:(%d) exit \n", __FUNCTION__, __LINE__);
    //0x0080
    printf("(0x0088) uvnr_step1_en:%d uvnr_step2_en:%d nr_gain_en:%d uvnr_nobig_en:%d uvnr_big_en:%d\n",
           pNrCfg->uvnr_step1_en,
           pNrCfg->uvnr_step2_en,
           pNrCfg->nr_gain_en,
           pNrCfg->uvnr_nobig_en,
           pNrCfg->uvnr_big_en);

    //0x0084
    printf("(0x0084) uvnr_gain_1sigma:%d \n",
           pNrCfg->uvnr_gain_1sigma);

    //0x0088
    printf("(0x0088) uvnr_gain_offset:%d \n",
           pNrCfg->uvnr_gain_offset);

    //0x008c
    printf("(0x008c) uvnr_gain_uvgain:%d uvnr_step2_en:%d uvnr_gain_t2gen:%d uvnr_gain_iso:%d\n",
           pNrCfg->uvnr_gain_uvgain[0],
           pNrCfg->uvnr_gain_uvgain[1],
           pNrCfg->uvnr_gain_t2gen,
           pNrCfg->uvnr_gain_iso);


    //0x0090
    printf("(0x0090) uvnr_t1gen_m3alpha:%d \n",
           pNrCfg->uvnr_t1gen_m3alpha);

    //0x0094
    printf("(0x0094) uvnr_t1flt_mode:%d \n",
           pNrCfg->uvnr_t1flt_mode);

    //0x0098
    printf("(0x0098) uvnr_t1flt_msigma:%d \n",
           pNrCfg->uvnr_t1flt_msigma);

    //0x009c
    printf("(0x009c) uvnr_t1flt_wtp:%d \n",
           pNrCfg->uvnr_t1flt_wtp);

    //0x00a0-0x00a4
    for(i = 0; i < NR_UVNR_T1FLT_WTQ_SIZE; i++) {
        printf("(0x00a0-0x00a4) uvnr_t1flt_wtq[%d]:%d \n",
               i, pNrCfg->uvnr_t1flt_wtq[i]);
    }

    //0x00a8
    printf("(0x00a8) uvnr_t2gen_m3alpha:%d \n",
           pNrCfg->uvnr_t2gen_m3alpha);

    //0x00ac
    printf("(0x00ac) uvnr_t2gen_msigma:%d \n",
           pNrCfg->uvnr_t2gen_msigma);

    //0x00b0
    printf("(0x00b0) uvnr_t2gen_wtp:%d \n",
           pNrCfg->uvnr_t2gen_wtp);

    //0x00b4
    for(i = 0; i < NR_UVNR_T2GEN_WTQ_SIZE; i++) {
        printf("(0x00b4) uvnr_t2gen_wtq[%d]:%d \n",
               i, pNrCfg->uvnr_t2gen_wtq[i]);
    }

    //0x00b8
    printf("(0x00b8) uvnr_t2flt_msigma:%d \n",
           pNrCfg->uvnr_t2flt_msigma);

    //0x00bc
    printf("(0x00bc) uvnr_t2flt_wtp:%d \n",
           pNrCfg->uvnr_t2flt_wtp);
    for(i = 0; i < NR_UVNR_T2FLT_WT_SIZE; i++) {
        printf("(0x00bc) uvnr_t2flt_wt[%d]:%d \n",
               i, pNrCfg->uvnr_t2flt_wt[i]);
    }


    printf("%s:(%d) entor \n", __FUNCTION__, __LINE__);
}


void CamHwIsp20::dumpYnrFixValue(struct rkispp_nr_config  * pNrCfg)
{
    printf("%s:(%d) enter \n", __FUNCTION__, __LINE__);

    int i = 0;

    //0x0104 - 0x0108
    for(i = 0; i < NR_YNR_SGM_DX_SIZE; i++) {
        printf("(0x0104 - 0x0108) ynr_sgm_dx[%d]:%d \n",
               i, pNrCfg->ynr_sgm_dx[i]);
    }

    //0x010c - 0x012c
    for(i = 0; i < NR_YNR_SGM_Y_SIZE; i++) {
        printf("(0x010c - 0x012c) ynr_lsgm_y[%d]:%d \n",
               i, pNrCfg->ynr_lsgm_y[i]);
    }

    //0x0130
    for(i = 0; i < NR_YNR_CI_SIZE; i++) {
        printf("(0x0130) ynr_lci[%d]:%d \n",
               i, pNrCfg->ynr_lci[i]);
    }

    //0x0134
    for(i = 0; i < NR_YNR_LGAIN_MIN_SIZE; i++) {
        printf("(0x0134) ynr_lgain_min[%d]:%d \n",
               i, pNrCfg->ynr_lgain_min[i]);
    }

    //0x0138
    printf("(0x0138) ynr_lgain_max:%d \n",
           pNrCfg->ynr_lgain_max);


    //0x013c
    printf("(0x013c) ynr_lmerge_bound:%d ynr_lmerge_ratio:%d\n",
           pNrCfg->ynr_lmerge_bound,
           pNrCfg->ynr_lmerge_ratio);

    //0x0140
    for(i = 0; i < NR_YNR_LWEIT_FLT_SIZE; i++) {
        printf("(0x0140) ynr_lweit_flt[%d]:%d \n",
               i, pNrCfg->ynr_lweit_flt[i]);
    }

    //0x0144 - 0x0164
    for(i = 0; i < NR_YNR_SGM_Y_SIZE; i++) {
        printf("(0x0144 - 0x0164) ynr_hsgm_y[%d]:%d \n",
               i, pNrCfg->ynr_hsgm_y[i]);
    }

    //0x0168
    for(i = 0; i < NR_YNR_CI_SIZE; i++) {
        printf("(0x0168) ynr_hlci[%d]:%d \n",
               i, pNrCfg->ynr_hlci[i]);
    }

    //0x016c
    for(i = 0; i < NR_YNR_CI_SIZE; i++) {
        printf("(0x016c) ynr_lhci[%d]:%d \n",
               i, pNrCfg->ynr_lhci[i]);
    }

    //0x0170
    for(i = 0; i < NR_YNR_CI_SIZE; i++) {
        printf("(0x0170) ynr_hhci[%d]:%d \n",
               i, pNrCfg->ynr_hhci[i]);
    }

    //0x0174
    for(i = 0; i < NR_YNR_HGAIN_SGM_SIZE; i++) {
        printf("(0x0174) ynr_hgain_sgm[%d]:%d \n",
               i, pNrCfg->ynr_hgain_sgm[i]);
    }

    //0x0178 - 0x0188
    for(i = 0; i < 5; i++) {
        printf("(0x0178 - 0x0188) ynr_hweit_d[%d - %d]:%d %d %d %d \n",
               i * 4 + 0, i * 4 + 3,
               pNrCfg->ynr_hweit_d[i * 4 + 0],
               pNrCfg->ynr_hweit_d[i * 4 + 1],
               pNrCfg->ynr_hweit_d[i * 4 + 2],
               pNrCfg->ynr_hweit_d[i * 4 + 3]);
    }


    //0x018c - 0x01a0
    for(i = 0; i < 6; i++) {
        printf("(0x018c - 0x01a0) ynr_hgrad_y[%d - %d]:%d %d %d %d \n",
               i * 4 + 0, i * 4 + 3,
               pNrCfg->ynr_hgrad_y[i * 4 + 0],
               pNrCfg->ynr_hgrad_y[i * 4 + 1],
               pNrCfg->ynr_hgrad_y[i * 4 + 2],
               pNrCfg->ynr_hgrad_y[i * 4 + 3]);
    }

    //0x01a4 -0x01a8
    for(i = 0; i < NR_YNR_HWEIT_SIZE; i++) {
        printf("(0x01a4 -0x01a8) ynr_hweit[%d]:%d \n",
               i, pNrCfg->ynr_hweit[i]);
    }

    //0x01b0
    printf("(0x01b0) ynr_hmax_adjust:%d \n",
           pNrCfg->ynr_hmax_adjust);

    //0x01b4
    printf("(0x01b4) ynr_hstrength:%d \n",
           pNrCfg->ynr_hstrength);

    //0x01b8
    printf("(0x01b8) ynr_lweit_cmp0-1:%d %d\n",
           pNrCfg->ynr_lweit_cmp[0],
           pNrCfg->ynr_lweit_cmp[1]);

    //0x01bc
    printf("(0x01bc) ynr_lmaxgain_lv4:%d \n",
           pNrCfg->ynr_lmaxgain_lv4);

    //0x01c0 - 0x01e0
    for(i = 0; i < NR_YNR_HSTV_Y_SIZE; i++) {
        printf("(0x01c0 - 0x01e0 ) ynr_hstv_y[%d]:%d \n",
               i, pNrCfg->ynr_hstv_y[i]);
    }

    //0x01e4  - 0x01e8
    for(i = 0; i < NR_YNR_ST_SCALE_SIZE; i++) {
        printf("(0x01e4  - 0x01e8 ) ynr_st_scale[%d]:%d \n",
               i, pNrCfg->ynr_st_scale[i]);
    }

    printf("%s:(%d) exit \n", __FUNCTION__, __LINE__);

}


void CamHwIsp20::dumpSharpFixValue(struct rkispp_sharp_config  * pSharpCfg)
{
    printf("%s:(%d) enter \n", __FUNCTION__, __LINE__);
    int i = 0;

    //0x0080
    printf("(0x0080) alpha_adp_en:%d yin_flt_en:%d edge_avg_en:%d\n",
           pSharpCfg->alpha_adp_en,
           pSharpCfg->yin_flt_en,
           pSharpCfg->edge_avg_en);


    //0x0084
    printf("(0x0084) hbf_ratio:%d ehf_th:%d pbf_ratio:%d\n",
           pSharpCfg->hbf_ratio,
           pSharpCfg->ehf_th,
           pSharpCfg->pbf_ratio);

    //0x0088
    printf("(0x0088) edge_thed:%d dir_min:%d smoth_th4:%d\n",
           pSharpCfg->edge_thed,
           pSharpCfg->dir_min,
           pSharpCfg->smoth_th4);

    //0x008c
    printf("(0x008c) l_alpha:%d g_alpha:%d \n",
           pSharpCfg->l_alpha,
           pSharpCfg->g_alpha);


    //0x0090
    for(i = 0; i < 3; i++) {
        printf("(0x0090) pbf_k[%d]:%d  \n",
               i, pSharpCfg->pbf_k[i]);
    }



    //0x0094 - 0x0098
    for(i = 0; i < 6; i++) {
        printf("(0x0094 - 0x0098) mrf_k[%d]:%d  \n",
               i, pSharpCfg->mrf_k[i]);
    }


    //0x009c -0x00a4
    for(i = 0; i < 12; i++) {
        printf("(0x009c -0x00a4) mbf_k[%d]:%d  \n",
               i, pSharpCfg->mbf_k[i]);
    }


    //0x00a8 -0x00ac
    for(i = 0; i < 6; i++) {
        printf("(0x00a8 -0x00ac) hrf_k[%d]:%d  \n",
               i, pSharpCfg->hrf_k[i]);
    }


    //0x00b0
    for(i = 0; i < 3; i++) {
        printf("(0x00b0) hbf_k[%d]:%d  \n",
               i, pSharpCfg->hbf_k[i]);
    }


    //0x00b4
    for(i = 0; i < 3; i++) {
        printf("(0x00b4) eg_coef[%d]:%d  \n",
               i, pSharpCfg->eg_coef[i]);
    }

    //0x00b8
    for(i = 0; i < 3; i++) {
        printf("(0x00b8) eg_smoth[%d]:%d  \n",
               i, pSharpCfg->eg_smoth[i]);
    }


    //0x00bc - 0x00c0
    for(i = 0; i < 6; i++) {
        printf("(0x00bc - 0x00c0) eg_gaus[%d]:%d  \n",
               i, pSharpCfg->eg_gaus[i]);
    }


    //0x00c4 - 0x00c8
    for(i = 0; i < 6; i++) {
        printf("(0x00c4 - 0x00c8) dog_k[%d]:%d  \n",
               i, pSharpCfg->dog_k[i]);
    }


    //0x00cc - 0x00d0
    for(i = 0; i < SHP_LUM_POINT_SIZE; i++) {
        printf("(0x00cc - 0x00d0) lum_point[%d]:%d  \n",
               i, pSharpCfg->lum_point[i]);
    }

    //0x00d4
    printf("(0x00d4) pbf_shf_bits:%d  mbf_shf_bits:%d hbf_shf_bits:%d\n",
           pSharpCfg->pbf_shf_bits,
           pSharpCfg->mbf_shf_bits,
           pSharpCfg->hbf_shf_bits);


    //0x00d8 - 0x00dc
    for(i = 0; i < SHP_SIGMA_SIZE; i++) {
        printf("(0x00d8 - 0x00dc) pbf_sigma[%d]:%d  \n",
               i, pSharpCfg->pbf_sigma[i]);
    }

    //0x00e0 - 0x00e4
    for(i = 0; i < SHP_LUM_CLP_SIZE; i++) {
        printf("(0x00e0 - 0x00e4) lum_clp_m[%d]:%d  \n",
               i, pSharpCfg->lum_clp_m[i]);
    }

    //0x00e8 - 0x00ec
    for(i = 0; i < SHP_LUM_MIN_SIZE; i++) {
        printf("(0x00e8 - 0x00ec) lum_min_m[%d]:%d  \n",
               i, pSharpCfg->lum_min_m[i]);
    }

    //0x00f0 - 0x00f4
    for(i = 0; i < SHP_SIGMA_SIZE; i++) {
        printf("(0x00f0 - 0x00f4) mbf_sigma[%d]:%d  \n",
               i, pSharpCfg->mbf_sigma[i]);
    }

    //0x00f8 - 0x00fc
    for(i = 0; i < SHP_LUM_CLP_SIZE; i++) {
        printf("(0x00f8 - 0x00fc) lum_clp_h[%d]:%d  \n",
               i, pSharpCfg->lum_clp_h[i]);
    }

    //0x0100 - 0x0104
    for(i = 0; i < SHP_SIGMA_SIZE; i++) {
        printf("(0x0100 - 0x0104) hbf_sigma[%d]:%d  \n",
               i, pSharpCfg->hbf_sigma[i]);
    }

    //0x0108 - 0x010c
    for(i = 0; i < SHP_EDGE_LUM_THED_SIZE; i++) {
        printf("(0x0108 - 0x010c) edge_lum_thed[%d]:%d  \n",
               i, pSharpCfg->edge_lum_thed[i]);
    }

    //0x0110 - 0x0114
    for(i = 0; i < SHP_CLAMP_SIZE; i++) {
        printf("(0x0110 - 0x0114) clamp_pos[%d]:%d  \n",
               i, pSharpCfg->clamp_pos[i]);
    }

    //0x0118 - 0x011c
    for(i = 0; i < SHP_CLAMP_SIZE; i++) {
        printf("(0x0118 - 0x011c) clamp_neg[%d]:%d  \n",
               i, pSharpCfg->clamp_neg[i]);
    }

    //0x0120 - 0x0124
    for(i = 0; i < SHP_DETAIL_ALPHA_SIZE; i++) {
        printf("(0x0120 - 0x0124) detail_alpha[%d]:%d  \n",
               i, pSharpCfg->detail_alpha[i]);
    }

    //0x0128
    printf("(0x0128) rfl_ratio:%d  rfh_ratio:%d\n",
           pSharpCfg->rfl_ratio, pSharpCfg->rfh_ratio);

    // mf/hf ratio

    //0x012C
    printf("(0x012C) m_ratio:%d  h_ratio:%d\n",
           pSharpCfg->m_ratio, pSharpCfg->h_ratio);

    printf("%s:(%d) exit \n", __FUNCTION__, __LINE__);
}

XCamReturn
CamHwIsp20::setModuleCtl(rk_aiq_module_id_t moduleId, bool en)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (mCalibDb->mfnr.enable && mCalibDb->mfnr.motion_detect_en) {
        if ((moduleId == RK_MODULE_TNR) && (en == false)) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "motion detect is running, operate not permit!");
            return XCAM_RETURN_ERROR_FAILED;
        }
    }
    setModuleStatus(moduleId, en);
    return ret;
}

XCamReturn
CamHwIsp20::getModuleCtl(rk_aiq_module_id_t moduleId, bool &en)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    getModuleStatus(moduleId, en);
    return ret;
}

XCamReturn CamHwIsp20::notify_capture_raw()
{
    SmartPtr<Isp20PollThread> isp20Pollthread = mPollthread.dynamic_cast_ptr<Isp20PollThread>();
    if (isp20Pollthread.ptr())
        return isp20Pollthread->notify_capture_raw();

    return XCAM_RETURN_ERROR_FAILED;
}

XCamReturn CamHwIsp20::capture_raw_ctl(capture_raw_t type, int count, const char* capture_dir, char* output_dir)
{
    SmartPtr<Isp20PollThread> isp20Pollthread = mPollthread.dynamic_cast_ptr<Isp20PollThread>();
    if (isp20Pollthread.ptr()) {
        if (type == CAPTURE_RAW_AND_YUV_SYNC)
            return isp20Pollthread->capture_raw_ctl(type);
        else if (type == CAPTURE_RAW_SYNC)
            return isp20Pollthread->capture_raw_ctl(type, count, capture_dir, output_dir);
    }

    return XCAM_RETURN_ERROR_FAILED;
}

XCamReturn
CamHwIsp20::setIrcutParams(bool on)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ENTER_CAMHW_FUNCTION();

    struct v4l2_control control;

    xcam_mem_clear (control);
    control.id = V4L2_CID_BAND_STOP_FILTER;
    if(on)
        control.value = IRCUT_STATE_CLOSED;
    else
        control.value = IRCUT_STATE_OPENED;
    if (mIrcutDev.ptr()) {
        LOGD_CAMHW_SUBM(ISP20HW_SUBM, "set ircut value: %d", control.value);
        if (mIrcutDev->io_control (VIDIOC_S_CTRL, &control) < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "set ircut value failed to device!");
            ret = XCAM_RETURN_ERROR_IOCTL;
        }
    }

    EXIT_CAMHW_FUNCTION();
    return ret;
}

uint64_t CamHwIsp20::getIspModuleEnState()
{
    return ispModuleEns;
}

XCamReturn CamHwIsp20::setSensorFlip(bool mirror, bool flip, int skip_frm_cnt)
{
    SmartPtr<BaseSensorHw> mSensorSubdev = mSensorDev.dynamic_cast_ptr<BaseSensorHw>();
    SmartPtr<Isp20PollThread> isp20Pollthread = mPollthread.dynamic_cast_ptr<Isp20PollThread>();
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    int32_t skip_frame_sequence = 0;
    ret = mSensorSubdev->set_mirror_flip(mirror, flip, skip_frame_sequence);

    /* struct timespec tp; */
    /* clock_gettime(CLOCK_MONOTONIC, &tp); */
    /* int64_t skip_ts = (int64_t)(tp.tv_sec) * 1000 * 1000 * 1000 + (int64_t)(tp.tv_nsec); */

    if (_state == CAM_HW_STATE_STARTED && skip_frame_sequence != -1)
        isp20Pollthread->skip_frames(skip_frm_cnt, skip_frame_sequence);

    return ret;
}

XCamReturn CamHwIsp20::getSensorFlip(bool& mirror, bool& flip)
{
    SmartPtr<BaseSensorHw> mSensorSubdev = mSensorDev.dynamic_cast_ptr<BaseSensorHw>();

    return mSensorSubdev->get_mirror_flip(mirror, flip);
}

XCamReturn CamHwIsp20::setSensorCrop(rk_aiq_rect_t& rect)
{
    XCamReturn ret;
    struct v4l2_crop crop;
    for (int i = 0; i < 3; i++) {
        SmartPtr<V4l2Device> mipi_tx = _mipi_tx_devs[i].dynamic_cast_ptr<V4l2Device>();
        memset(&crop, 0, sizeof(crop));
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ret = mipi_tx->get_crop(crop);
        crop.c.left = rect.left;
        crop.c.top = rect.top;
        crop.c.width = rect.width;
        crop.c.height = rect.height;
        ret = mipi_tx->set_crop(crop);
    }
    _crop_rect = rect;
    return ret;
}

XCamReturn CamHwIsp20::getSensorCrop(rk_aiq_rect_t& rect)
{
    XCamReturn ret;
    struct v4l2_crop crop;
    SmartPtr<V4l2Device> mipi_tx = _mipi_tx_devs[0].dynamic_cast_ptr<V4l2Device>();
    memset(&crop, 0, sizeof(crop));
    ret = mipi_tx->get_crop(crop);
    rect.left = crop.c.left;
    rect.top = crop.c.top;
    rect.width = crop.c.width;
    rect.height = crop.c.height;
    return ret;
}

void CamHwIsp20::setHdrGlobalTmoMode(int frame_id, bool mode)
{
    if (mNormalNoReadBack)
        return;

    SmartPtr<Isp20PollThread> isp20Pollthread = mPollthread.dynamic_cast_ptr<Isp20PollThread>();

    if (isp20Pollthread.ptr())
        isp20Pollthread->set_hdr_global_tmo_mode(frame_id, mode);
}

void CamHwIsp20::setMulCamConc(bool cc)
{
    SmartPtr<Isp20PollThread> isp20Pollthread = mPollthread.dynamic_cast_ptr<Isp20PollThread>();

    if (isp20Pollthread.ptr())
        isp20Pollthread->setMulCamConc(cc);
}

void CamHwIsp20::getShareMemOps(isp_drv_share_mem_ops_t** mem_ops)
{
    this->alloc_mem = (alloc_mem_t)&CamHwIsp20::allocMemResource;
    this->release_mem = (release_mem_t)&CamHwIsp20::releaseMemResource;
    this->get_free_item = (get_free_item_t)&CamHwIsp20::getFreeItem;
    *mem_ops = this;
}

void CamHwIsp20::allocMemResource(void *ops_ctx, void *config, void **mem_ctx)
{
    int ret = -1;
    struct rkisp_ldchbuf_info ldchbuf_info;
    struct rkispp_fecbuf_info  fecbuf_info;
    struct rkisp_ldchbuf_size ldchbuf_size;
    struct rkispp_fecbuf_size fecbuf_size;

    CamHwIsp20 *isp20 = static_cast<CamHwIsp20*>((isp_drv_share_mem_ops_t*)ops_ctx);
    rk_aiq_share_mem_config_t* share_mem_cfg = (rk_aiq_share_mem_config_t *)config;

    SmartLock locker (isp20->_mem_mutex);
    if (share_mem_cfg->mem_type == MEM_TYPE_LDCH) {
        ldchbuf_size.meas_width = share_mem_cfg->alloc_param.width;
        ldchbuf_size.meas_height = share_mem_cfg->alloc_param.height;
        ret = isp20->mIspCoreDev->io_control(RKISP_CMD_SET_LDCHBUF_SIZE, &ldchbuf_size);
        if (ret < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "alloc ldch buf failed!");
            return;
        }
        xcam_mem_clear(ldchbuf_info);
        ret = isp20->mIspCoreDev->io_control(RKISP_CMD_GET_LDCHBUF_INFO, &ldchbuf_info);
        if (ret < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "failed to get ldch buf info!!");
            return;
        }
        rk_aiq_ldch_share_mem_info_t* mem_info_array =
            (rk_aiq_ldch_share_mem_info_t*)(isp20->_ldch_drv_mem_ctx.mem_info);
        for (int i = 0; i < ISP2X_LDCH_BUF_NUM; i++) {
            mem_info_array[i].map_addr =
                mmap(NULL, ldchbuf_info.buf_size[i], PROT_READ | PROT_WRITE, MAP_SHARED, ldchbuf_info.buf_fd[i], 0);
            if (MAP_FAILED == mem_info_array[i].map_addr)
                LOGE_CAMHW_SUBM(ISP20HW_SUBM, "failed to map ldch buf!!");

            mem_info_array[i].fd = ldchbuf_info.buf_fd[i];
            mem_info_array[i].size = ldchbuf_info.buf_size[i];
            struct isp2x_ldch_head *head = (struct isp2x_ldch_head*)mem_info_array[i].map_addr;
            mem_info_array[i].addr = (void*)((char*)mem_info_array[i].map_addr + head->data_oft);
            mem_info_array[i].state = (char*)&head->stat;
        }
        *mem_ctx = (void*)(&isp20->_ldch_drv_mem_ctx);
    } else if (share_mem_cfg->mem_type == MEM_TYPE_FEC) {
        fecbuf_size.meas_width = share_mem_cfg->alloc_param.width;
        fecbuf_size.meas_height = share_mem_cfg->alloc_param.height;
        fecbuf_size.meas_mode = share_mem_cfg->alloc_param.reserved[0];
        ret = isp20->_ispp_sd->io_control(RKISPP_CMD_SET_FECBUF_SIZE, &fecbuf_size);
        if (ret < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "alloc fec buf failed!");
            return;
        }
        xcam_mem_clear(fecbuf_info);
        ret = isp20->_ispp_sd->io_control(RKISPP_CMD_GET_FECBUF_INFO, &fecbuf_info);
        if (ret < 0) {
            LOGE_CAMHW_SUBM(ISP20HW_SUBM, "failed to get fec buf info!!");
            return;
        }
        rk_aiq_fec_share_mem_info_t* mem_info_array =
            (rk_aiq_fec_share_mem_info_t*)(isp20->_fec_drv_mem_ctx.mem_info);
        for (int i = 0; i < FEC_MESH_BUF_NUM; i++) {
            mem_info_array[i].map_addr =
                mmap(NULL, fecbuf_info.buf_size[i], PROT_READ | PROT_WRITE, MAP_SHARED, fecbuf_info.buf_fd[i], 0);
            if (MAP_FAILED == mem_info_array[i].map_addr)
                LOGE_CAMHW_SUBM(ISP20HW_SUBM, "failed to map fec buf!!");

            mem_info_array[i].fd = fecbuf_info.buf_fd[i];
            mem_info_array[i].size = fecbuf_info.buf_size[i];
            struct rkispp_fec_head *head = (struct rkispp_fec_head*)mem_info_array[i].map_addr;
            mem_info_array[i].meshxf =
                (unsigned char*)mem_info_array[i].map_addr + head->meshxf_oft;
            mem_info_array[i].meshyf =
                (unsigned char*)mem_info_array[i].map_addr + head->meshyf_oft;
            mem_info_array[i].meshxi =
                (unsigned short*)((char*)mem_info_array[i].map_addr + head->meshxi_oft);
            mem_info_array[i].meshyi =
                (unsigned short*)((char*)mem_info_array[i].map_addr + head->meshyi_oft);
            mem_info_array[i].state = (char*)&head->stat;
        }
        *mem_ctx = (void*)(&isp20->_fec_drv_mem_ctx);
    }
}

void CamHwIsp20::releaseMemResource(void *mem_ctx)
{
    int ret = -1;
    drv_share_mem_ctx_t* drv_mem_ctx = (drv_share_mem_ctx_t*)mem_ctx;
    CamHwIsp20 *isp20 = (CamHwIsp20*)(drv_mem_ctx->ops_ctx);

    SmartLock locker (isp20->_mem_mutex);
    if (drv_mem_ctx->type == MEM_TYPE_LDCH) {
        rk_aiq_ldch_share_mem_info_t* mem_info_array =
            (rk_aiq_ldch_share_mem_info_t*)(drv_mem_ctx->mem_info);
        for (int i = 0; i < ISP2X_LDCH_BUF_NUM; i++) {
            if (mem_info_array[i].map_addr) {
                ret = munmap(mem_info_array[i].map_addr, mem_info_array[i].size);
                if (ret < 0)
                    LOGE_CAMHW_SUBM(ISP20HW_SUBM, "munmap ldch buf info!!");
                mem_info_array[i].map_addr = NULL;
            }
            ::close(mem_info_array[i].fd);
        }
    } else if (drv_mem_ctx->type == MEM_TYPE_FEC) {
        rk_aiq_fec_share_mem_info_t* mem_info_array =
            (rk_aiq_fec_share_mem_info_t*)(drv_mem_ctx->mem_info);
        for (int i = 0; i < FEC_MESH_BUF_NUM; i++) {
            if (mem_info_array[i].map_addr) {
                ret = munmap(mem_info_array[i].map_addr, mem_info_array[i].size);
                if (ret < 0)
                    LOGE_CAMHW_SUBM(ISP20HW_SUBM, "munmap fec buf info!!");
                mem_info_array[i].map_addr = NULL;
            }
            ::close(mem_info_array[i].fd);
        }
    }
}

void*
CamHwIsp20::getFreeItem(void *mem_ctx)
{
    unsigned int idx;
    int retry_cnt = 3;
    drv_share_mem_ctx_t* drv_mem_ctx = (drv_share_mem_ctx_t*)mem_ctx;
    CamHwIsp20 *isp20 = (CamHwIsp20*)(drv_mem_ctx->ops_ctx);

    SmartLock locker (isp20->_mem_mutex);
    if (drv_mem_ctx->type == MEM_TYPE_LDCH) {
        rk_aiq_ldch_share_mem_info_t* mem_info_array =
            (rk_aiq_ldch_share_mem_info_t*)(drv_mem_ctx->mem_info);
        do {
            for (idx = 0; idx < ISP2X_LDCH_BUF_NUM; idx++) {
                if (mem_info_array[idx].state &&
                        (0 == mem_info_array[idx].state[0])) {
                    return (void*)&mem_info_array[idx];
                }
            }
        } while(retry_cnt--);
    } else if (drv_mem_ctx->type == MEM_TYPE_FEC) {
        rk_aiq_fec_share_mem_info_t* mem_info_array =
            (rk_aiq_fec_share_mem_info_t*)(drv_mem_ctx->mem_info);
        do {
            for (idx = 0; idx < FEC_MESH_BUF_NUM; idx++) {
                if (mem_info_array[idx].state &&
                        (0 == mem_info_array[idx].state[0])) {
                    return (void*)&mem_info_array[idx];
                }
            }
        } while(retry_cnt--);
    }
    return NULL;
}

void
CamHwIsp20::prepare_motion_detection(int mode)
{
#define __ALIGN_MASK(x,mask)    (((x)+(mask))&~(mask))
#define ALIGN(x,a)               __ALIGN_MASK(x, a-1)

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    struct v4l2_subdev_format isp_src_fmt;
    isp_src_fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    isp_src_fmt.pad = 2;
    ret = mIspCoreDev->getFormat(isp_src_fmt);
    if (ret) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "get mIspCoreDev src fmt failed !\n");
        return;
    }
    _ds_width            = (isp_src_fmt.format.width + 3) / 4;
    _ds_heigth           = (isp_src_fmt.format.height + 7) / 8;
    _ds_width_align      = ALIGN(_ds_width, 16);
    _ds_heigth_align     = (_ds_heigth + 1)   & (~1);
    LOGD_CAMHW_SUBM(ISP20HW_SUBM, "set sp format: width %d %d height %d %d\n",
                    _ds_width, _ds_width_align, _ds_heigth, _ds_heigth_align);
    ret = mIspSpDev->set_format(_ds_width_align, _ds_heigth_align, V4L2_PIX_FMT_FBCG, V4L2_FIELD_NONE, 0);//w && h should be aligned by 8
    if (ret) {
        LOGE_CAMHW_SUBM(ISP20HW_SUBM, "set mIspSpDev fmt failed !\n");
        return;
    }
    mIspSpDev->set_mem_type(V4L2_MEMORY_MMAP);
    mIspSpDev->set_buf_type(V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
    mIspSpDev->set_buffer_count(6);
    mIspSpDev->set_mplanes_count(2);
    if (mIspSpThread.ptr()) {
        mIspSpThread->set_sp_dev(mIspCoreDev, mIspSpDev, _ispp_sd, mSensorDev, mLensDev);
        mIspSpThread->set_sp_img_size(_ds_width, _ds_heigth, _ds_width_align, _ds_heigth_align);
        mIspSpThread->set_calibDb(mCalibDb);
        mIspSpThread->set_working_mode(mode);
    }
}

XCamReturn
CamHwIsp20::poll_event_ready (uint32_t sequence, int type)
{
    if (type == V4L2_EVENT_FRAME_SYNC && mIspEvtsListener) {
        SmartPtr<SensorHw> mSensor = mSensorDev.dynamic_cast_ptr<SensorHw>();
        SmartPtr<Isp20Evt> evtInfo = new Isp20Evt(this, mSensor);
        evtInfo->evt_code = type;
        evtInfo->sequence = sequence;
        evtInfo->expDelay = _exp_delay;

        return  mIspEvtsListener->ispEvtsCb(evtInfo);
    }

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
CamHwIsp20::poll_event_failed (int64_t timestamp, const char *msg)
{
    return XCAM_RETURN_ERROR_FAILED;
}

XCamReturn
CamHwIsp20::getEffectiveExpParams(uint32_t id, SmartPtr<RkAiqExpParamsProxy>& expParams)
{
    XCAM_ASSERT (mSensorDev.ptr ());
    SmartPtr<SensorHw> mSensor = mSensorDev.dynamic_cast_ptr<SensorHw>();
    return mSensor->getEffectiveExpParams(expParams, id);
}

}; //namspace RkCam
