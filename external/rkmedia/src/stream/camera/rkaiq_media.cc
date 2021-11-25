// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "rkaiq_media.h"
#include "utils.h"

static media_entity *media_get_entity_by_name_rk(struct media_device *device,
                                                 const char *name) {
  return media_get_entity_by_name(device, name, strlen(name));
}

int RKAiqMedia::SetCameraOrder(int idx) {
  if (idx >= MAX_CAM_NUM) {
    RKMEDIA_LOGE("RKAIQ: index(%d) over max(%d)!", idx, MAX_CAM_NUM);
    return -1;
  }
  for (int i = 0; i < MAX_CAM_NUM; i++) {
    if (cam_id2ispp_id[i] == ISP_ORDER_FREE) {
      cam_id2ispp_id[i] = idx;
      return 0;
    }
  }
  RKMEDIA_LOGE("RKAIQ: set %d fail, cam_id2ispp_id is full!", idx);
  return -1;
}

void RKAiqMedia::BindCameraWithIsp() {
  int free_cam_id2ispp_id;
  for (int i = 0; i < MAX_CAM_NUM; i++) {
    free_cam_id2ispp_id = ISP_ORDER_FREE;
    if (media_info[i].isp.model_idx < 0 || media_info[i].ispp.model_idx < 0) {
      break;
    }
    for (int j = 0; j < MAX_CAM_NUM; j++) {
      if (cam_id2ispp_id[j] == ISP_ORDER_OCCUPY &&
          free_cam_id2ispp_id == ISP_ORDER_FREE) {
        free_cam_id2ispp_id = j;
      }
      if (cam_id2ispp_id[j] == media_info[i].isp.model_idx) {
        free_cam_id2ispp_id = ISP_ORDER_FREE;
        break;
      }
    }
    if (free_cam_id2ispp_id != ISP_ORDER_FREE) {
      cam_id2ispp_id[free_cam_id2ispp_id] = media_info[i].isp.model_idx;
    }
  }
}

std::string RKAiqMedia::GetSensorName(struct media_device *device) {
  std::string sensor_name;
  media_entity *entity = NULL;
  const struct media_entity_desc *entity_info;
  uint32_t nents = media_get_entities_count(device);
  for (uint32_t j = 0; j < nents; ++j) {
    entity = media_get_entity(device, j);
    entity_info = media_entity_get_info(entity);
    if ((NULL != entity_info) &&
        (entity_info->type == MEDIA_ENT_T_V4L2_SUBDEV_SENSOR)) {
      sensor_name = entity_info->name;
      break;
    }
  }
  return sensor_name;
}

int RKAiqMedia::IsLinkSensor(struct media_device *device) {
  int link_sensor = 0;
  media_entity *entity = NULL;
  const struct media_entity_desc *entity_info;
  uint32_t nents = media_get_entities_count(device);
  for (uint32_t j = 0; j < nents; ++j) {
    entity = media_get_entity(device, j);
    entity_info = media_entity_get_info(entity);
    if ((NULL != entity_info) &&
        (entity_info->type == MEDIA_ENT_T_V4L2_SUBDEV_SENSOR)) {
      link_sensor = 1;
      break;
    }
  }

  return link_sensor;
}

void RKAiqMedia::GetIsppSubDevs(int id, struct media_device *device,
                                const char *devpath) {
  media_entity *entity = NULL;
  const char *entity_name = NULL;
  ispp_info_t *ispp_info = NULL;

  if (id >= MAX_CAM_NUM || id < 0)
    return;

  for (int index = 0; index < MAX_CAM_NUM; index++) {
    ispp_info = &media_info[index].ispp;
    if (ispp_info->media_dev_path.empty())
      break;
    if (ispp_info->media_dev_path.compare(devpath) == 0) {
      RKMEDIA_LOGE("RKAIQ: isp info of path %s exists!\n", devpath);
      return;
    }
  }

  ispp_info = &media_info[id].ispp;

  ispp_info->model_idx = id;
  ispp_info->media_dev_path = devpath;

  entity = media_get_entity_by_name_rk(device, "rkispp_input_image");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      ispp_info->pp_input_image_path = entity_name;
    }
  }
  entity = media_get_entity_by_name_rk(device, "rkispp_m_bypass");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      ispp_info->pp_m_bypass_path = entity_name;
    }
  }
  entity = media_get_entity_by_name_rk(device, "rkispp_scale0");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      ispp_info->pp_scale0_path = entity_name;
    }
  }
  entity = media_get_entity_by_name_rk(device, "rkispp_scale1");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      ispp_info->pp_scale1_path = entity_name;
    }
  }
  entity = media_get_entity_by_name_rk(device, "rkispp_scale2");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      ispp_info->pp_scale2_path = entity_name;
    }
  }
  entity = media_get_entity_by_name_rk(device, "rkispp_input_params");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      ispp_info->pp_input_params_path = entity_name;
    }
  }
  entity = media_get_entity_by_name_rk(device, "rkispp-stats");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      ispp_info->pp_stats_path = entity_name;
    }
  }
  entity = media_get_entity_by_name_rk(device, "rkispp-subdev");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      ispp_info->pp_dev_path = entity_name;
    }
  }

  RKMEDIA_LOGI("RKAIQ: model(%s): ispp_info(%d): ispp-subdev entity name: %s\n",
               device->info.model, id, ispp_info->pp_dev_path.c_str());
}

void RKAiqMedia::GetIspSubDevs(int id, struct media_device *device,
                               const char *devpath) {
  media_entity *entity = NULL;
  const char *entity_name = NULL;
  isp_info_t *isp_info = NULL;

  if (id >= MAX_CAM_NUM) {
    RKMEDIA_LOGW("RKAIQ: path %s set id(%d) over max(%d)!\n", devpath, id,
                 MAX_CAM_NUM - 1);
    return;
  }

  for (int index = 0; index < MAX_CAM_NUM; index++) {
    isp_info = &media_info[index].isp;
    if (isp_info->media_dev_path.compare(devpath) == 0) {
      RKMEDIA_LOGE("RKAIQ: isp info of path %s exists!\n", devpath);
      return;
    }
  }

  isp_info = &media_info[id].isp;

  isp_info->linked_sensor = IsLinkSensor(device);
  if (isp_info->linked_sensor) {
    isp_info->sensor_name = GetSensorName(device);
    SetCameraOrder(id);
  }
  isp_info->model_idx = id;
  isp_info->media_dev_path = devpath;

  entity = media_get_entity_by_name_rk(device, "rkisp-isp-subdev");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      isp_info->isp_dev_path = entity_name;
    }
  }

  entity = media_get_entity_by_name_rk(device, "rkisp-csi-subdev");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      isp_info->csi_dev_path = entity_name;
    }
  }
  entity = media_get_entity_by_name_rk(device, "rkisp-mpfbc-subdev");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      isp_info->mpfbc_dev_path = entity_name;
    }
  }
  entity = media_get_entity_by_name_rk(device, "rkisp_mainpath");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      isp_info->main_path = entity_name;
    }
  }
  entity = media_get_entity_by_name_rk(device, "rkisp_selfpath");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      isp_info->self_path = entity_name;
    }
  }
  entity = media_get_entity_by_name_rk(device, "rkisp_rawwr0");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      isp_info->rawwr0_path = entity_name;
    }
  }
  entity = media_get_entity_by_name_rk(device, "rkisp_rawwr1");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      isp_info->rawwr1_path = entity_name;
    }
  }
  entity = media_get_entity_by_name_rk(device, "rkisp_rawwr2");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      isp_info->rawwr2_path = entity_name;
    }
  }
  entity = media_get_entity_by_name_rk(device, "rkisp_rawwr3");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      isp_info->rawwr3_path = entity_name;
    }
  }
  entity = media_get_entity_by_name_rk(device, "rkisp_dmapath");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      isp_info->dma_path = entity_name;
    }
  }
  entity = media_get_entity_by_name_rk(device, "rkisp_rawrd0_m");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      isp_info->rawrd0_m_path = entity_name;
    }
  }
  entity = media_get_entity_by_name_rk(device, "rkisp_rawrd1_l");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      isp_info->rawrd1_l_path = entity_name;
    }
  }
  entity = media_get_entity_by_name_rk(device, "rkisp_rawrd2_s");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      isp_info->rawrd2_s_path = entity_name;
    }
  }
  entity = media_get_entity_by_name_rk(device, "rkisp-statistics");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      isp_info->stats_path = entity_name;
    }
  }
  entity = media_get_entity_by_name_rk(device, "rkisp-input-params");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      isp_info->input_params_path = entity_name;
    }
  }
  entity = media_get_entity_by_name_rk(device, "rkisp-mipi-luma");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      isp_info->mipi_luma_path = entity_name;
    }
  }
  entity = media_get_entity_by_name_rk(device, "rockchip-mipi-dphy-rx");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      isp_info->mipi_dphy_rx_path = entity_name;
    }
  }

  RKMEDIA_LOGI("RKAIQ: model(%s): isp_info(%d): isp-subdev entity name: %s\n",
               device->info.model, id, isp_info->isp_dev_path.c_str());
}

void RKAiqMedia::GetCifSubDevs(int id, struct media_device *device,
                               const char *devpath) {
  (void)id;
  media_entity *entity = NULL;
  const char *entity_name = NULL;
  int index = 0;
  cif_info_t *cif_info = nullptr;

  int link_sensor = IsLinkSensor(device);
  if (!link_sensor) {
    return;
  } else {
    SetCameraOrder(ISP_ORDER_OCCUPY);
  }

  for (index = 0; index < MAX_CAM_NUM; index++) {
    cif_info = &media_info[index].cif;
    if (cif_info->media_dev_path.empty())
      break;
    if (cif_info->media_dev_path.compare(devpath) == 0) {
      RKMEDIA_LOGE("RKAIQ: isp info of path %s exists!\n", devpath);
      return;
    }
  }

  if (index >= MAX_CAM_NUM)
    return;

  cif_info->model_idx = index;
  cif_info->linked_sensor = link_sensor;
  if (cif_info->linked_sensor)
    cif_info->sensor_name = GetSensorName(device);
  cif_info->media_dev_path = devpath;

  entity = media_get_entity_by_name_rk(device, "stream_cif_mipi_id0");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      cif_info->mipi_id0 = entity_name;
    }
  }

  entity = media_get_entity_by_name_rk(device, "stream_cif_mipi_id1");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      cif_info->mipi_id1 = entity_name;
    }
  }

  entity = media_get_entity_by_name_rk(device, "stream_cif_mipi_id2");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      cif_info->mipi_id2 = entity_name;
    }
  }

  entity = media_get_entity_by_name_rk(device, "stream_cif_mipi_id3");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      cif_info->mipi_id3 = entity_name;
    }
  }

  entity = media_get_entity_by_name_rk(device, "rkcif-mipi-luma");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      cif_info->mipi_luma_path = entity_name;
    }
  }

  entity = media_get_entity_by_name_rk(device, "rockchip-mipi-csi2");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      cif_info->mipi_csi2_sd_path = entity_name;
    }
  }

  entity = media_get_entity_by_name_rk(device, "rkcif-lvds-subdev");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      cif_info->lvds_sd_path = entity_name;
    }
  }

  entity = media_get_entity_by_name_rk(device, "rkcif-lite-lvds-subdev");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      cif_info->lvds_sd_path = entity_name;
    }
  }

  entity = media_get_entity_by_name_rk(device, "rockchip-mipi-dphy-rx");
  if (entity) {
    entity_name = media_entity_get_devname(entity);
    if (entity_name) {
      cif_info->mipi_dphy_rx_path = entity_name;
    }
  }
}

int RKAiqMedia::GetMediaInfo() {
  struct media_device *device = NULL;
  int ret;
  char sys_path[64];
  unsigned int index = 0;

  while (index < MAX_MEDIA_NUM) {
    snprintf(sys_path, 64, "/dev/media%d", index++);
    if (access(sys_path, F_OK))
      continue;

    RKMEDIA_LOGI("RKAIQ: parsing %s\n", sys_path);

    device = media_device_new(sys_path);
    if (!device)
      return -ENOMEM;

    ret = media_device_enumerate(device);
    if (ret) {
      media_device_unref(device);
      return ret;
    }

    if (strcmp(device->info.model, "rkispp0") == 0 ||
        strcmp(device->info.model, "rkispp") == 0) {
      GetIsppSubDevs(0, device, sys_path);
    } else if (strcmp(device->info.model, "rkispp1") == 0) {
      GetIsppSubDevs(1, device, sys_path);
    } else if (strcmp(device->info.model, "rkisp0") == 0 ||
               strcmp(device->info.model, "rkisp") == 0) {
      GetIspSubDevs(0, device, sys_path);
    } else if (strcmp(device->info.model, "rkisp1") == 0) {
      GetIspSubDevs(1, device, sys_path);
    } else if (strcmp(device->info.model, "rkcif") == 0 ||
               strcmp(device->info.model, "rkcif_mipi_lvds") == 0 ||
               strcmp(device->info.model, "rkcif_lite_mipi_lvds") == 0) {
      GetCifSubDevs(index, device, sys_path);
    }

    media_device_unref(device);
  }
  BindCameraWithIsp();
  return ret;
}

int RKAiqMedia::DumpMediaInfo() {
  RKMEDIA_LOGI("RKAIQ: DumpMediaInfo:\n");

  for (int i = 0; i < MAX_CAM_NUM; i++) {
    cif_info_t *cif = &media_info[i].cif;
    isp_info_t *isp = &media_info[i].isp;
    ispp_info_t *ispp = &media_info[i].ispp;
    RKMEDIA_LOGI("RKAIQ: ##### Camera index: %d\n", i);
    if (isp->linked_sensor)
      RKMEDIA_LOGI("RKAIQ: \t sensor_name :    %s\n", isp->sensor_name.c_str());
    else if (cif->linked_sensor)
      RKMEDIA_LOGI("RKAIQ: \t sensor_name :    %s\n", cif->sensor_name.c_str());
    if (cif->model_idx >= 0) {
      RKMEDIA_LOGI("RKAIQ: #### cif:\n");
      RKMEDIA_LOGI("RKAIQ: \t model_idx :         %d\n", cif->model_idx);
      RKMEDIA_LOGI("RKAIQ: \t linked_sensor :     %d\n", cif->linked_sensor);
      RKMEDIA_LOGI("RKAIQ: \t media_dev_path :    %s\n",
                   cif->media_dev_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t mipi_id0 : 		  %s\n",
                   cif->mipi_id0.c_str());
      RKMEDIA_LOGI("RKAIQ: \t mipi_id1 : 		  %s\n",
                   cif->mipi_id1.c_str());
      RKMEDIA_LOGI("RKAIQ: \t mipi_id2 : 		  %s\n",
                   cif->mipi_id2.c_str());
      RKMEDIA_LOGI("RKAIQ: \t mipi_id3 : 		  %s\n",
                   cif->mipi_id3.c_str());
      RKMEDIA_LOGI("RKAIQ: \t mipi_dphy_rx_path : %s\n",
                   cif->mipi_dphy_rx_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t mipi_csi2_sd_path : %s\n",
                   cif->mipi_csi2_sd_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t lvds_sd_path :      %s\n",
                   cif->lvds_sd_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t mipi_luma_path :    %s\n",
                   cif->mipi_luma_path.c_str());
    }
    if (isp->model_idx >= 0) {
      RKMEDIA_LOGI("RKAIQ: #### isp:\n");
      RKMEDIA_LOGI("RKAIQ: \t model_idx :         %d\n", isp->model_idx);
      RKMEDIA_LOGI("RKAIQ: \t linked_sensor :     %d\n", isp->linked_sensor);
      RKMEDIA_LOGI("RKAIQ: \t media_dev_path :    %s\n",
                   isp->media_dev_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t isp_dev_path :      %s\n",
                   isp->isp_dev_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t csi_dev_path :      %s\n",
                   isp->csi_dev_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t mpfbc_dev_path :    %s\n",
                   isp->mpfbc_dev_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t main_path :         %s\n",
                   isp->main_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t self_path :         %s\n",
                   isp->self_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t rawwr0_path :       %s\n",
                   isp->rawwr0_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t rawwr1_path :       %s\n",
                   isp->rawwr1_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t rawwr2_path :       %s\n",
                   isp->rawwr2_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t rawwr3_path :       %s\n",
                   isp->rawwr3_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t dma_path :          %s\n", isp->dma_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t rawrd0_m_path :     %s\n",
                   isp->rawrd0_m_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t rawrd1_l_path :     %s\n",
                   isp->rawrd1_l_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t rawrd2_s_path :     %s\n",
                   isp->rawrd2_s_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t stats_path :        %s\n",
                   isp->stats_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t input_params_path : %s\n",
                   isp->input_params_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t mipi_luma_path :    %s\n",
                   isp->mipi_luma_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t mipi_dphy_rx_path : %s\n",
                   isp->mipi_dphy_rx_path.c_str());
    }
    if (ispp->model_idx >= 0) {
      RKMEDIA_LOGI("RKAIQ: #### ispp:\n");
      RKMEDIA_LOGI("RKAIQ: \t model_idx :            %d\n", ispp->model_idx);
      RKMEDIA_LOGI("RKAIQ: \t media_dev_path :       %s\n",
                   ispp->media_dev_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t pp_input_image_path :  %s\n",
                   ispp->pp_input_image_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t pp_m_bypass_path :     %s\n",
                   ispp->pp_m_bypass_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t pp_scale0_path :       %s\n",
                   ispp->pp_scale0_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t pp_scale1_path :       %s\n",
                   ispp->pp_scale1_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t pp_scale2_path :       %s\n",
                   ispp->pp_scale2_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t pp_input_params_path : %s\n",
                   ispp->pp_input_params_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t pp_stats_path :        %s\n",
                   ispp->pp_stats_path.c_str());
      RKMEDIA_LOGI("RKAIQ: \t pp_dev_path :          %s\n",
                   ispp->pp_dev_path.c_str());
    }
  }
  return 0;
}

std::string RKAiqMedia::GetVideoNode(int camera_id, const char *chn_name) {
  RKMEDIA_LOGW("camera_id: %d, chn: %s\n", camera_id, chn_name);
  if (camera_id >= MAX_CAM_NUM || camera_id < 0)
    return "";

  if (camera_id >= 2) {
    RKMEDIA_LOGW("video node may only support 2 sensor!\n");
  }

  int idx = cam_id2ispp_id[camera_id];
  if (idx >= MAX_CAM_NUM || idx < 0)
    return "";

  RKMEDIA_LOGW("camera_id: %d, chn: %s, idx: %d\n", camera_id, chn_name, idx);
  if (strstr(chn_name, "bypass"))
    return media_info[idx].ispp.pp_m_bypass_path;
  else if (strstr(chn_name, "scale0"))
    return media_info[idx].ispp.pp_scale0_path;
  else if (strstr(chn_name, "scale1"))
    return media_info[idx].ispp.pp_scale1_path;
  else if (strstr(chn_name, "scale2"))
    return media_info[idx].ispp.pp_scale2_path;

  return "";
}

RKAiqMedia::RKAiqMedia() {
  for (int i = 0; i < MAX_CAM_NUM; i++) {
    media_info[i].cif.model_idx = -1;
    media_info[i].isp.model_idx = -1;
    media_info[i].ispp.model_idx = -1;
    media_info[i].cif.linked_sensor = 0;
    media_info[i].isp.linked_sensor = 0;
    cam_id2ispp_id[i] = ISP_ORDER_FREE;
  }

  GetMediaInfo();
}
