// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "drm_stream.h"

#include <string.h>

namespace easymedia {

DRMStream::DRMStream(const char *param, bool as)
    : accept_scale(as), fps(0), connector_id(0), crtc_id(0), encoder_id(0),
      plane_id(0), active(false), drm_fmt(0), find_strict_match_wh(false),
      fd(-1), res(nullptr) {
  memset(&img_info, 0, sizeof(img_info));
  img_info.pix_fmt = PIX_FMT_NONE;
  memset(&cur_mode, 0, sizeof(cur_mode));

  std::map<std::string, std::string> params;
  std::list<std::pair<const std::string, std::string &>> req_list;
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_DEVICE, device));
  std::string str_width, str_height;
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_OUTPUTDATATYPE, data_type));
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_BUFFER_WIDTH, str_width));
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_BUFFER_HEIGHT, str_height));
  std::string str_fps;
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_FPS, str_fps));
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_PLANE_TYPE, plane_type));
  std::string str_conn_id, str_crtc_id, str_encoder_id, str_plane_id;
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_CONNECTOR_ID, str_conn_id));
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_CRTC_ID, str_crtc_id));
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_ENCODER_ID, str_encoder_id));
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_PLANE_ID, str_plane_id));
  std::string str_skip_plane_ids;
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_SKIP_PLANE_IDS, str_skip_plane_ids));
  int ret = parse_media_param_match(param, params, req_list);
  if (ret == 0)
    return;
  if (!str_width.empty())
    img_info.width = img_info.vir_width = std::stoi(str_width);
  if (!str_height.empty())
    img_info.height = img_info.vir_height = std::stoi(str_height);
  if (!str_fps.empty())
    fps = std::stoi(str_fps);
  if (!str_conn_id.empty())
    connector_id = std::stoi(str_conn_id);
  if (!str_crtc_id.empty())
    crtc_id = std::stoi(str_crtc_id);
  if (!str_encoder_id.empty())
    encoder_id = std::stoi(str_encoder_id);
  if (!str_plane_id.empty())
    plane_id = std::stoi(str_plane_id);
  if (!str_skip_plane_ids.empty()) {
    std::list<std::string> ids;
    if (!parse_media_param_list(str_skip_plane_ids.c_str(), ids, ','))
      return;
    for (auto &id : ids)
      skip_plane_ids.push_back(std::stoi(id));
  }
}

void DRMStream::SetModeInfo(const drmModeModeInfo &mode) {
  if (img_info.vir_width != mode.hdisplay ||
      img_info.vir_height != mode.vdisplay) {
    img_info.width = img_info.vir_width = mode.hdisplay;
    img_info.height = img_info.vir_height = mode.vdisplay;
  }
  if (fps != (int)mode.vrefresh)
    fps = mode.vrefresh;
}

int DRMStream::Open() {
  if (DRM_ID_ISVALID(plane_id) &&
      std::find(skip_plane_ids.begin(), skip_plane_ids.end(), plane_id) !=
          skip_plane_ids.end()) {
    // duplicated plane id
    return -EINVAL;
  }
  if (device.empty()) {
    RKMEDIA_LOGE("DrmDisp: missing device path\n");
    return -EINVAL;
  }
  dev = std::make_shared<DRMDevice>(device);
  if (!dev)
    return -ENOMEM;
  fd = dev->GetDeviceFd();
  if (fd < 0)
    return errno ? -errno : -1;
  res = dev->get_resources();
  if (!res)
    return -1;
  // user specify connector id
  if (DRM_ID_ISVALID(connector_id)) {
    auto dmc = get_connector_by_id(res, connector_id);
    if (!dmc || dmc->connection != DRM_MODE_CONNECTED ||
        dmc->count_modes <= 0) {
      RKMEDIA_LOGI("connector[%d] is not ready\n", connector_id);
      return -1;
    }
    assert(dmc->count_encoders > 0);
    if (!reserve_ids_by_connector(res, connector_id)) {
      RKMEDIA_LOGI("connector[%d] is unapplicable\n", connector_id);
      return -1;
    }
  } else {
    if (!filter_ids_if_connector_notready(res)) {
      RKMEDIA_LOGI("non connector is ready\n");
      return -1;
    }
  }
  // user specify encoder id
  if (DRM_ID_ISVALID(encoder_id) && !reserve_ids_by_encoder(res, encoder_id)) {
    RKMEDIA_LOGI("encoder[%d] is unapplicable\n", encoder_id);
    return -1;
  }
  // user specify crtc id
  if (DRM_ID_ISVALID(crtc_id) && !reserve_ids_by_crtc(res, crtc_id)) {
    RKMEDIA_LOGI("crtc[%d] is unapplicable\n", crtc_id);
    return -1;
  }
  // user specify plane id
  if (DRM_ID_ISVALID(plane_id) && !reserve_ids_by_plane(res, plane_id)) {
    RKMEDIA_LOGI("plane[%d] is unapplicable\n", plane_id);
    return -1;
  }
  if (!skip_plane_ids.empty() &&
      !filter_ids_by_skip_plane_ids(res, skip_plane_ids)) {
    RKMEDIA_LOGI("got valid ids overlap in skip_plane_ids\n");
    return -1;
  }
  // user specify exact fps
  if (fps != 0 && !filter_ids_by_fps(res, fps)) {
    RKMEDIA_LOGI("specified fps [%d] is unacceptable\n", fps);
    return -1;
  }
  drm_fmt = GetDRMFmtByString(data_type.c_str());
  img_info.pix_fmt = StringToPixFmt(data_type.c_str());
  if (!data_type.empty() && !filter_ids_by_data_type(res, data_type)) {
    RKMEDIA_LOGI("data type [%s] is unacceptable\n", data_type.c_str());
    return -1;
  }
  if (!plane_type.empty() && !filter_ids_by_plane_type(res, plane_type)) {
    RKMEDIA_LOGI("plane type [%s] is unacceptable\n", plane_type.c_str());
    return -1;
  }
  if (img_info.vir_width > 0 && img_info.vir_height > 0) {
#define ivw img_info.vir_width
#define ivh img_info.vir_height
    find_strict_match_wh = find_connector_ids_by_wh(res, ivw, ivh);
    if (find_strict_match_wh) {
      if (!filter_ids_by_wh(res, ivw, ivh)) {
        RKMEDIA_LOGI("strict widthxheight [%dx%d] is unacceptable\n", ivw, ivh);
        assert(0); // should not happen
        return -1;
      }
    } else if (!accept_scale) {
      RKMEDIA_LOGI("strict widthxheight [%dx%d] is unacceptable\n", ivw, ivh);
      return -1;
    }
#undef ivw
#undef ivh
  }
  return 0;
}

int DRMStream::Close() {
  fd = -1;
  if (res) {
    dev->free_resources(res);
    res = nullptr;
  }
  dev = nullptr; // release reference
  return 0;
}

bool DRMStream::GetAgreeableIDSet() {
#ifndef NDEBUG
  dump_suitable_ids(res);
#endif
  // get an agreeable set of ids, but may be not the most suitable one
  // TODO: get the most suitable one
  struct drm_ids &ids = res->ids;
  uint32_t i;
  for (i = 0; i < ids.count_connectors; i++) {
    uint32_t connid = ids.connector_ids[i];
    auto dmc = get_connector_by_id(res, connid);
    assert(dmc);
    if (find_strict_match_wh &&
        !comp_width_height(dmc, img_info.vir_width, img_info.height))
      continue;
    if (dmc->encoder_id <= 0) {
      int j = 0;
      do {
        if ((dmc->encoder_id = dmc->encoders[j]) > 0)
          break;
      } while (++j < dmc->count_encoders);
      if (j >= dmc->count_encoders)
        continue;
      RKMEDIA_LOGI(
          "connector's encoder is not ready, try first possible encoder<%d>\n",
          dmc->encoder_id);
    }
    connector_id = connid;
    encoder_id = dmc->encoder_id;
    auto enc = get_encoder_by_id(res, dmc->encoder_id);
    assert(dmc);
    int idx = enc->crtc_id > 0 ? get_crtc_index(res, enc->crtc_id)
                               : get_crtc_index_by_connector(res, dmc);
    assert(idx >= 0);
    crtc_id = res->crtcs[idx].crtc->crtc_id;
    uint32_t planeid = 0;
    for (uint32_t j = 0; j < ids.count_planes; j++) {
      auto dmp = get_plane_by_id(res, ids.plane_ids[j]);
      assert(dmp);
      if ((dmp->possible_crtcs & (1 << idx)) &&
          ((dmp->crtc_id == 0 || dmp->crtc_id == crtc_id))) {
        planeid = dmp->plane_id;
        break;
      }
    }
    if (planeid == 0)
      continue;
    plane_id = planeid;
    break;
  }
  // get mode info
  if (i < ids.count_connectors) {
    //    auto dme = get_encoder_by_id(res, encoder_id);
    //    active = (dme->crtc_id > 0 && dme->crtc_id == crtc_id);

    auto dmc = get_connector_by_id(res, connector_id);
    int idx = -1;
    int size = dmc->count_modes;
    drmModeModeInfoPtr *modes =
        (drmModeModeInfoPtr *)calloc(size, sizeof(*modes));
    if (!modes)
      return false;
    for (int j = 0; j < size; j++)
      modes[j] = &dmc->modes[j];
    if (find_strict_match_wh) {
      filter_modeinfo_by_wh(img_info.vir_width, img_info.vir_height, size,
                            modes);
      assert(size > 0);
    }
    if (fps > 0)
      idx = filter_modeinfo_by_fps(fps, size, modes);
    if (idx < 0)
      idx = filter_modeinfo_by_type(DRM_MODE_TYPE_PREFERRED, size, modes);
    if (idx < 0)
      idx = 0;
    cur_mode = *(modes[idx]);
    free(modes);

    SetModeInfo(cur_mode);
  }
  RKMEDIA_LOGI(
      "conn id : %d, enc id: %d, crtc id: %d, plane id: %d, w/h: %d,%d, fps: "
      "%d\n",
      connector_id, encoder_id, crtc_id, plane_id, img_info.vir_width,
      img_info.vir_height, fps);
  return i < ids.count_connectors;
}

} // namespace easymedia
