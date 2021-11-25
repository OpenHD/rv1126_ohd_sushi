// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>

#include "drm_utils.h"
#include "key_string.h"
#include "media_type.h"
#include "utils.h"
#include <functional>

namespace easymedia {

struct type_name {
  unsigned int type;
  const char *name;
};

static const struct type_name encoder_type_names[] = {
    {DRM_MODE_ENCODER_NONE, "none"},   {DRM_MODE_ENCODER_DAC, "DAC"},
    {DRM_MODE_ENCODER_TMDS, "TMDS"},   {DRM_MODE_ENCODER_LVDS, "LVDS"},
    {DRM_MODE_ENCODER_TVDAC, "TVDAC"}, {DRM_MODE_ENCODER_VIRTUAL, "Virtual"},
    {DRM_MODE_ENCODER_DSI, "DSI"},     {DRM_MODE_ENCODER_DPMST, "DPMST"},
    {DRM_MODE_ENCODER_DPI, "DPI"},
};

static const struct type_name connector_status_names[] = {
    {DRM_MODE_CONNECTED, "connected"},
    {DRM_MODE_DISCONNECTED, "disconnected"},
    {DRM_MODE_UNKNOWNCONNECTION, "unknown"},
};

static const struct type_name connector_type_names[] = {
    {DRM_MODE_CONNECTOR_Unknown, "unknown"},
    {DRM_MODE_CONNECTOR_VGA, "VGA"},
    {DRM_MODE_CONNECTOR_DVII, "DVI-I"},
    {DRM_MODE_CONNECTOR_DVID, "DVI-D"},
    {DRM_MODE_CONNECTOR_DVIA, "DVI-A"},
    {DRM_MODE_CONNECTOR_Composite, "composite"},
    {DRM_MODE_CONNECTOR_SVIDEO, "s-video"},
    {DRM_MODE_CONNECTOR_LVDS, "LVDS"},
    {DRM_MODE_CONNECTOR_Component, "component"},
    {DRM_MODE_CONNECTOR_9PinDIN, "9-pin DIN"},
    {DRM_MODE_CONNECTOR_DisplayPort, "DP"},
    {DRM_MODE_CONNECTOR_HDMIA, "HDMI-A"},
    {DRM_MODE_CONNECTOR_HDMIB, "HDMI-B"},
    {DRM_MODE_CONNECTOR_TV, "TV"},
    {DRM_MODE_CONNECTOR_eDP, "eDP"},
    {DRM_MODE_CONNECTOR_VIRTUAL, "Virtual"},
    {DRM_MODE_CONNECTOR_DSI, "DSI"},
    {DRM_MODE_CONNECTOR_DPI, "DPI"},
};

static const char *lookup_drm_type_name(unsigned int type,
                                        const struct type_name *table,
                                        unsigned int count) {
  unsigned int i;

  for (i = 0; i < count; i++)
    if (table[i].type == type)
      return table[i].name;

  return NULL;
}

#if 0
static const char *lookup_drm_encoder_type_name(unsigned int type) {
  return lookup_drm_type_name(type, encoder_type_names,
                              ARRAY_ELEMS(encoder_type_names));
}

static const char *lookup_drm_connector_status_name(unsigned int status) {
  return lookup_drm_type_name(status, connector_status_names,
                              ARRAY_ELEMS(connector_status_names));
}
#endif

static const char *lookup_drm_connector_type_name(unsigned int type) {
  return lookup_drm_type_name(type, connector_type_names,
                              ARRAY_ELEMS(connector_type_names));
}

#define DUMP_IDS(type, size, ids)                                              \
  RKMEDIA_LOGI("\t%s:", #type);                                                \
  for (uint32_t i = 0; i < size; i++)                                          \
    RKMEDIA_LOGI("%c%d", i == 0 ? '\t' : ',', ids[i]);                         \
  RKMEDIA_LOGI("\n");

void dump_suitable_ids(struct resources *res) {
#define DUMP_SUITABLE_IDS(type)                                                \
  DUMP_IDS(type, ids.count_##type##s, ids.type##_ids)

  struct drm_ids &ids = res->ids;
  RKMEDIA_LOGI("suitable ids: \n");
  DUMP_SUITABLE_IDS(connector)
  DUMP_SUITABLE_IDS(encoder)
  DUMP_SUITABLE_IDS(crtc)
  DUMP_SUITABLE_IDS(plane)
}

#define DefineGetObject(_res, type, Type)                                      \
  int get_##type##_index(struct resources *res, uint32_t id) {                 \
    if (!res->_res)                                                            \
      return -1;                                                               \
    drmMode##Type *type;                                                       \
    for (int i = 0; i < (int)res->_res->count_##type##s; i++) {                \
      type = res->type##s[i].type;                                             \
      if (type && type->type##_id == id)                                       \
        return i;                                                              \
    }                                                                          \
                                                                               \
    return -1;                                                                 \
  }                                                                            \
                                                                               \
  drmMode##Type *get_##type##_by_id(struct resources *res, uint32_t id) {      \
    int idx = get_##type##_index(res, id);                                     \
    if (idx < 0)                                                               \
      return nullptr;                                                          \
    return res->type##s[idx].type;                                             \
  }

DefineGetObject(res, connector, Connector) DefineGetObject(res, crtc, Crtc)
    DefineGetObject(plane_res, plane, Plane)

    /* Return the first possible and active CRTC if one exists, or the first
     * possible CRTC otherwise.
     */
    int get_crtc_index_by_connector(struct resources *res,
                                    drmModeConnector *conn) {
  int crtc_idx = -1;
  uint32_t possible_crtcs = ~0;
  uint32_t active_crtcs = 0;
  uint32_t crtcs_for_connector = 0;
  int idx;
  for (int j = 0; j < conn->count_encoders; ++j) {
    drmModeEncoder *encoder = get_encoder_by_id(res, conn->encoders[j]);
    if (!encoder)
      continue;

    crtcs_for_connector |= encoder->possible_crtcs;

    idx = get_crtc_index(res, encoder->crtc_id);
    if (idx >= 0)
      active_crtcs |= 1 << idx;
  }
  possible_crtcs &= crtcs_for_connector;
  if (!possible_crtcs)
    return -1;
  if (possible_crtcs & active_crtcs)
    crtc_idx = ffs(possible_crtcs & active_crtcs);
  else
    crtc_idx = ffs(possible_crtcs);
  return crtc_idx - 1;
}

drmModeEncoder *get_encoder_by_id(struct resources *res, uint32_t id) {
  drmModeRes *dmr = res->res;
  drmModeEncoder *encoder;
  int i;

  for (i = 0; i < dmr->count_encoders; i++) {
    encoder = res->encoders[i].encoder;
    if (encoder && encoder->encoder_id == id)
      return encoder;
  }

  return nullptr;
}

const struct PlayTypeMap {
  int type;
  const char *type_str;
} plane_type_string_map[] = {{DRM_PLANE_TYPE_OVERLAY, KEY_OVERLAY},
                             {DRM_PLANE_TYPE_PRIMARY, KEY_PRIMARY},
                             {DRM_PLANE_TYPE_CURSOR, KEY_CURSOR}};

static int GetPlaneTypeByString(const char *type_str) {
  if (!type_str)
    return -1;
  for (size_t i = 0; i < ARRAY_ELEMS(plane_type_string_map) - 1; i++) {
    if (!strcmp(type_str, plane_type_string_map[i].type_str))
      return plane_type_string_map[i].type;
  }
  return -1;
}

// return size-1 if match, else size
template <typename T> static int remove_element(int size, T *ids, T id) {
  if (size == 0)
    return 0;
  int i = 0;
  int ret = size;
  do {
    if (ids[i] == id) {
      ids[i] = 0;
      ret--;
      break;
    }
  } while (++i < size);
  for (; i < size - 1; i++) {
    ids[i] = ids[i + 1];
    ids[i + 1] = 0;
  }
  return ret;
}

static bool contain_element(int size, const uint32_t *ids, uint32_t id) {
  for (int i = 0; i < size; i++) {
    if (ids[i] == id)
      return true;
  }
  return false;
}

// Delete the value in ids array that is not equal to id,
// and reset the length of the ids array to 1.
static int remove_element_except(int size, uint32_t *ids, uint32_t id) {
  int ret = 0;

  for (int i = 0; i < size; i++) {
    if (ids[i] == id) {
      if (i > 0)
        ids[0] = id;
      ret = 1;
      break;
    }
  }

  return ret;
}

// return: the new size of ids after intersecting
static int _intersection(int src_size, const uint32_t *src_ids, int size,
                         uint32_t *ids) {
  int ret = size;
  for (int i = 0; i < ret;) {
    uint32_t id = ids[i];
    if (id == 0)
      break;
    if (!contain_element(src_size, src_ids, id))
      ret = remove_element<uint32_t>(ret, ids, id);
    else
      i++;
  }
  return ret;
}

// return: the new size of ids after uniting
static int _union(int src_size, const uint32_t *src_ids, int size,
                  uint32_t *ids) {
  if (size == 0) {
    memcpy(ids, src_ids, src_size);
    return src_size;
  }
  int idx = size;
  for (int i = 0; i < src_size; i++) {
    uint32_t id = src_ids[i];
    assert(DRM_ID_ISVALID(id));
    if (!contain_element(size, ids, id))
      ids[idx++] = id;
  }
  return idx;
}

static uint32_t get_possible_crtcs_by_encoder_ids(struct resources *res) {
  struct drm_ids &ids = res->ids;
  uint32_t crtcs_for_encoder = 0;
  for (uint32_t i = 0; i < ids.count_encoders; ++i) {
    drmModeEncoder *encoder = get_encoder_by_id(res, ids.encoder_ids[i]);
    assert(encoder);
    crtcs_for_encoder |= encoder->possible_crtcs;
  }
  return crtcs_for_encoder;
}

static uint32_t get_possible_crtcs_by_crtc_ids(struct resources *res) {
  struct drm_ids &ids = res->ids;
  uint32_t crtcs_for_crtc = 0;
  for (uint32_t i = 0; i < ids.count_crtcs; ++i) {
    int index = get_crtc_index(res, ids.crtc_ids[i]);
    assert(index >= 0);
    crtcs_for_crtc |= (1 << index);
  }
  return crtcs_for_crtc;
}

static uint32_t get_possible_crtcs_by_plane_ids(struct resources *res) {
  struct drm_ids &ids = res->ids;
  uint32_t crtcs_for_plane = 0;
  for (uint32_t i = 0; i < ids.count_planes; ++i) {
    auto dmp = get_plane_by_id(res, ids.plane_ids[i]);
    assert(dmp);
    crtcs_for_plane |= dmp->possible_crtcs;
  }
  return crtcs_for_plane;
}

// return: count before filtering
static int filter_connector_ids_by_encoder_ids(struct resources *res) {
  struct drm_ids &ids = res->ids;
  int ret, count;
  ret = count = ids.count_connectors;
  for (int i = 0; i < count;) {
    uint32_t conn_id = ids.connector_ids[i];
    auto dmc = get_connector_by_id(res, conn_id);
    assert(dmc);
    assert(dmc->count_encoders > 0);
    int j = 0;
    for (; j < dmc->count_encoders; j++) {
      uint32_t enc_id = dmc->encoders[j];
      if (contain_element(ids.count_encoders, ids.encoder_ids, enc_id))
        break;
    }
    if (j >= dmc->count_encoders)
      count = remove_element<uint32_t>(ids.count_connectors, ids.connector_ids,
                                       conn_id);
    else
      i++;
  }
  ids.count_connectors = count;
  return ret;
}

// return: count before filtering
static int filter_encoder_ids_by_crtc_ids(struct resources *res) {
  struct drm_ids &ids = res->ids;
  uint32_t possible_crtcs = get_possible_crtcs_by_crtc_ids(res);
  int ret, count;
  ret = count = ids.count_encoders;
  for (int i = 0; i < count;) {
    uint32_t id = ids.encoder_ids[i];
    assert(DRM_ID_ISVALID(id));
    auto dme = get_encoder_by_id(res, id);
    assert(dme);
    if (!(possible_crtcs & dme->possible_crtcs))
      count = remove_element<uint32_t>(count, ids.encoder_ids, id);
    else
      i++;
  }
  ids.count_planes = count;
  return ret;
}

// return: count before filtering
static int filter_crtc_ids_by_possible_crtcs(struct resources *res,
                                             uint32_t possible_crtcs) {
  struct drm_ids &ids = res->ids;
  int ret, count;
  ret = count = ids.count_crtcs;
  for (int i = 0; i < count;) {
    uint32_t id = ids.crtc_ids[i];
    if (id == 0)
      break;
    int index = get_crtc_index(res, id);
    assert(index >= 0);
    if (!(possible_crtcs & (1 << index)))
      count = remove_element<uint32_t>(count, ids.crtc_ids, id);
    else
      i++;
  }
  ids.count_crtcs = count;
  return ret;
}

// return: count before filtering
static int filter_plane_ids_by_crtc_ids(struct resources *res) {
  struct drm_ids &ids = res->ids;
  uint32_t possible_crtcs = get_possible_crtcs_by_crtc_ids(res);
  int ret, count;
  ret = count = ids.count_planes;
  for (int i = 0; i < count;) {
    uint32_t id = ids.plane_ids[i];
    assert(DRM_ID_ISVALID(id));
    auto dmp = get_plane_by_id(res, id);
    assert(dmp);
    if (!(possible_crtcs & dmp->possible_crtcs))
      count = remove_element<uint32_t>(count, ids.plane_ids, id);
    else
      i++;
  }
  ids.count_planes = count;
  return ret;
}

// call when res->ids.encoder_ids is changed
static bool filter_ids_from_encoder_ids(struct resources *res);

// call when res->ids.connector_ids is changed
static bool filter_ids_from_connector_ids(struct resources *res) {
  struct drm_ids &ids = res->ids;
  int union_count = 0;
  assert(res->res->count_encoders > 0);
  uint32_t *union_enc_ids =
      (uint32_t *)calloc(res->res->count_encoders, sizeof(uint32_t));
  if (!union_enc_ids) {
    // no memory
    return false;
  }
  for (uint32_t i = 0; i < ids.count_connectors; i++) {
    uint32_t conn_id = ids.connector_ids[i];
    auto dmc = get_connector_by_id(res, conn_id);
    assert(dmc);
    assert(dmc->count_encoders > 0);
    union_count =
        _union(dmc->count_encoders, dmc->encoders, union_count, union_enc_ids);
    assert(union_count <= res->res->count_encoders);
  }
  assert(union_count > 0);
  // filter encoder ids
  int ret = _intersection(union_count, union_enc_ids, ids.count_encoders,
                          ids.encoder_ids);
  if (ids.count_encoders == 0) {
    ids.count_encoders = ret;
    goto error;
  }
  if (ids.count_encoders != (uint32_t)ret) {
    ids.count_encoders = ret;
    if (!filter_ids_from_encoder_ids(res))
      goto error;
  }

  free(union_enc_ids);
  return true;

error:
  free(union_enc_ids);
  return false;
}

bool filter_ids_from_encoder_ids(struct resources *res) {
  struct drm_ids &ids = res->ids;
  // filter crtc ids
  int ret = filter_crtc_ids_by_possible_crtcs(
      res, get_possible_crtcs_by_encoder_ids(res));
  if (ids.count_crtcs == 0)
    return false;
  if ((uint32_t)ret != ids.count_crtcs) {
    // filter plane ids
    ret = filter_plane_ids_by_crtc_ids(res);
    if (ids.count_planes == 0)
      return false;
  }
  return true;
}

typedef std::function<bool(drmModeConnector *)> ConnectorCondFunction;
static bool filter_ids_by_connector_condition(struct resources *res,
                                              ConnectorCondFunction f) {
  struct drm_ids &ids = res->ids;
  int ret, count;
  ret = count = ids.count_connectors;
  for (int i = 0; i < count;) {
    uint32_t conn_id = ids.connector_ids[i];
    auto dmc = get_connector_by_id(res, conn_id);
    assert(dmc);
    if (!f(dmc)) {
      count = remove_element<uint32_t>(count, ids.connector_ids, conn_id);
      continue;
    }
    i++;
  }
  ids.count_connectors = count;
  if (count == 0)
    return false;
  if (count == ret)
    return true;
  return filter_ids_from_connector_ids(res);
}

bool filter_ids_if_connector_notready(struct resources *res) {
  auto f = [](drmModeConnector *dmc) -> bool {
    assert(!dmc || dmc->count_encoders > 0);
    return dmc && dmc->connection == DRM_MODE_CONNECTED &&
           dmc->count_modes > 0 && dmc->count_encoders > 0;
  };
  return filter_ids_by_connector_condition(res, f);
}

typedef std::function<bool(drmModeModeInfo *dmmi)> ModeInfoCondFunction;
static bool comp_modeinfo(drmModeConnector *dmc, ModeInfoCondFunction f) {
  if (dmc->count_modes <= 0)
    return false;
  int j = 0;
  for (; j < dmc->count_modes; j++) {
    auto dmmi = &dmc->modes[j];
    if (dmmi && f(dmmi))
      return true;
  }
  return false;
}
static bool comp_vrefresh(drmModeConnector *dmc, uint32_t vrefresh) {
  auto f = [vrefresh](drmModeModeInfo *dmmi) -> bool {
    return (dmmi->vrefresh == 0 || dmmi->vrefresh == vrefresh);
  };
  return comp_modeinfo(dmc, f);
}

bool filter_ids_by_fps(struct resources *res, uint32_t vrefresh) {
  return filter_ids_by_connector_condition(
      res, std::bind(&comp_vrefresh, std::placeholders::_1, vrefresh));
}

bool comp_width_height(drmModeConnector *dmc, int w, int h) {
  auto f = [w, h](drmModeModeInfo *dmmi) -> bool {
    return (dmmi->hdisplay == w && dmmi->vdisplay == h);
  };
  return comp_modeinfo(dmc, f);
}

bool find_connector_ids_by_wh(struct resources *res, int w, int h) {
  struct drm_ids &ids = res->ids;
  auto f = std::bind(&comp_width_height, std::placeholders::_1, w, h);
  for (uint32_t i = 0; i < ids.count_connectors; i++) {
    uint32_t conn_id = ids.connector_ids[i];
    auto dmc = get_connector_by_id(res, conn_id);
    assert(dmc);
    if (f(dmc, w, h))
      return true;
  }
  return false;
}

bool filter_ids_by_wh(struct resources *res, int w, int h) {
  auto f = std::bind(&comp_width_height, std::placeholders::_1, w, h);
  return filter_ids_by_connector_condition(res, f);
}

// ret : (>0) means strict match; (=0) means fuzzy match; (<0) means not match
typedef std::function<int(drmModeModeInfoPtr mode)> FilterModeInfoCondFunction;
static int filter_modeinfo(int &size, drmModeModeInfoPtr *modes,
                           FilterModeInfoCondFunction f) {
  if (size <= 0)
    return -1;
  int ret = -1;
  int j = 0;

  for (; j < size;) {
    auto dmmi = modes[j];
    int r = f(dmmi);
    if (r < 0) {
      size = remove_element<drmModeModeInfoPtr>(size, modes, dmmi);
    } else if (r >= 0) {
      ret = 0;
      j++;
    }
  }
  return ret;
}

int filter_modeinfo_by_wh(int w, int h, int &size, drmModeModeInfoPtr *modes) {
  auto f = [w, h](drmModeModeInfoPtr dmmi) -> int {
    return (dmmi->hdisplay == w && dmmi->vdisplay == h) ? 1 : -1;
  };
  return filter_modeinfo(size, modes, f);
}

int filter_modeinfo_by_fps(int fps, int &size, drmModeModeInfoPtr *modes) {
  auto f = [fps](drmModeModeInfo *dmmi) -> int {
    if (dmmi->vrefresh == 0)
      return 0;
    if (dmmi->vrefresh == (uint32_t)fps)
      return 1;
    return -1;
  };
  return filter_modeinfo(size, modes, f);
}

int filter_modeinfo_by_type(uint32_t type, int &size,
                            drmModeModeInfoPtr *modes) {
  auto f = [type](drmModeModeInfo *dmmi) -> int {
    return (dmmi->type & type) ? 1 : 0;
  };
  return filter_modeinfo(size, modes, f);
}

bool filter_ids_up_plane_ids(struct resources *res) {
  struct drm_ids &ids = res->ids;
  int ret = filter_crtc_ids_by_possible_crtcs(
      res, get_possible_crtcs_by_plane_ids(res));
  if (ids.count_crtcs == 0)
    return false;
  if ((uint32_t)ret == ids.count_crtcs)
    return true;
  ret = filter_encoder_ids_by_crtc_ids(res);
  if (ids.count_encoders == 0)
    return false;
  if ((uint32_t)ret == ids.count_encoders)
    return true;
  ret = filter_connector_ids_by_encoder_ids(res);
  if (ids.count_connectors == 0)
    return false;
  return true;
}

typedef std::function<bool(struct plane *)> PlaneCondFunction;
static int filter_plane_ids_by_cond(struct resources *res,
                                    PlaneCondFunction f) {
  struct drm_ids &ids = res->ids;
  int ret, count;
  ret = count = ids.count_planes;
  for (int i = 0; i < count;) {
    uint32_t id = ids.plane_ids[i];
    assert(DRM_ID_ISVALID(id));
    int index = get_plane_index(res, id);
    assert(index >= 0);
    struct plane *plane = &res->planes[index];
    assert(plane->plane);
    if (!f(plane))
      count = remove_element<uint32_t>(count, ids.plane_ids, id);
    else
      i++;
  }
  ids.count_planes = count;
  return ret;
}

bool filter_ids_by_data_type(struct resources *res,
                             const std::string &data_type) {
  uint32_t drm_fmt = GetDRMFmtByString(data_type.c_str());
  if (drm_fmt == 0)
    return false;
  auto f = [drm_fmt](struct plane *plane) -> bool {
    auto dmp = plane->plane;
    if (!dmp)
      return false;
    for (uint32_t i = 0; i < dmp->count_formats; i++) {
      if (drm_fmt == dmp->formats[i])
        return true;
    }
    return false;
  };
  int ret = filter_plane_ids_by_cond(res, f);
  if (ret == 0)
    return false;
  if (res->ids.count_planes == (uint32_t)ret)
    return true;
  return filter_ids_up_plane_ids(res);
}

typedef std::function<bool(struct plane *, int prop_index)>
    PlanePropCondFunction;
static bool comp_plane_prop(struct plane *plane, PlanePropCondFunction f) {
  auto dmp = plane->plane;
  if (!dmp)
    return false;
  for (uint32_t i = 0; i < plane->props->count_props; i++) {
    if (f(plane, i))
      return true;
  }
  return false;
}

bool filter_ids_by_plane_type(struct resources *res,
                              const std::string &plane_type) {
  int expect_type = GetPlaneTypeByString(plane_type.c_str());
  if (expect_type < 0)
    return false;
  auto f = [expect_type](struct plane *plane, int prop_index) -> bool {
    auto prop = plane->props_info[prop_index];
    if (!prop)
      return false;
    return !strcmp(prop->name, "type") &&
           ((uint32_t)expect_type == plane->props->prop_values[prop_index]);
  };
  int ret = filter_plane_ids_by_cond(
      res, std::bind(comp_plane_prop, std::placeholders::_1, f));
  if (ret == 0)
    return false;
  if (res->ids.count_planes == (uint32_t)ret)
    return true;
  return filter_ids_up_plane_ids(res);
}

bool filter_ids_by_skip_plane_ids(struct resources *res,
                                  const std::vector<uint32_t> &s_ids) {
  struct drm_ids &ids = res->ids;
  int ret, count;
  ret = count = ids.count_planes;
  for (int i = 0; i < count;) {
    uint32_t id = ids.plane_ids[i];
    assert(DRM_ID_ISVALID(id));
    if (std::find(s_ids.begin(), s_ids.end(), id) != s_ids.end())
      count = remove_element<uint32_t>(count, ids.plane_ids, id);
    else
      i++;
  }
  ids.count_planes = count;
  if (ret == count)
    return true;
  return filter_ids_up_plane_ids(res);
}

#define PREFIX_RESERVE_IDS_BY(drm_res, type, id)                               \
  struct drm_ids &ids = res->ids;                                              \
  if (!contain_element(drm_res->count_##type##s, drm_res->type##s, id)) {      \
    return false; /* means the input id is not valid */                        \
  }                                                                            \
  if (!contain_element(ids.count_##type##s, ids.type##_ids, id))               \
    return false; /* means the input id has been filtered */                   \
  ids.count_##type##s =                                                        \
      remove_element_except(ids.count_##type##s, ids.type##_ids, id);          \
  assert(ids.count_##type##s == 1);

bool reserve_ids_by_connector(struct resources *res, uint32_t conn_id) {
  PREFIX_RESERVE_IDS_BY(res->res, connector, conn_id)
  return filter_ids_from_connector_ids(res);
}

bool reserve_ids_by_encoder(struct resources *res, uint32_t enc_id) {
  PREFIX_RESERVE_IDS_BY(res->res, encoder, enc_id)
  int ret = filter_connector_ids_by_encoder_ids(res);
  if (ids.count_connectors == 0)
    return false;
  UNUSED(ret);
  return filter_ids_from_encoder_ids(res);
}

bool reserve_ids_by_crtc(struct resources *res, uint32_t crtc_id) {
  PREFIX_RESERVE_IDS_BY(res->res, crtc, crtc_id)
  int ret = filter_encoder_ids_by_crtc_ids(res);
  if (ids.count_encoders == 0)
    return false;
  if ((uint32_t)ret == ids.count_encoders)
    return true;
  ret = filter_connector_ids_by_encoder_ids(res);
  if (ids.count_connectors == 0)
    return false;
  return filter_ids_from_encoder_ids(res);
}

bool reserve_ids_by_plane(struct resources *res, uint32_t plane_id) {
  PREFIX_RESERVE_IDS_BY(res->plane_res, plane, plane_id)
  return filter_ids_up_plane_ids(res);
}

uint32_t get_property_id(struct resources *res, uint32_t object_type,
                         uint32_t obj_id, const char *property_name,
                         uint64_t *value) {
#define GetPropsInfo(type)                                                     \
  index = get_##type##_index(res, obj_id);                                     \
  if (index < 0)                                                               \
    return 0;                                                                  \
  props = res->type##s[index].props;                                           \
  props_info = res->type##s[index].props_info;

  assert(property_name);
  drmModeObjectProperties *props = nullptr;
  drmModePropertyRes **props_info = nullptr;
  int index;
  switch (object_type) {
  case DRM_MODE_OBJECT_CRTC:
    GetPropsInfo(crtc) break;
  case DRM_MODE_OBJECT_CONNECTOR:
    GetPropsInfo(connector) break;
  case DRM_MODE_OBJECT_PLANE:
    GetPropsInfo(plane) break;
  default:
    return 0;
  }

  for (uint32_t i = 0; i < props->count_props; i++) {
    drmModePropertyRes *prop = props_info[i];
    if (!prop)
      continue;
    if (!strcasecmp(prop->name, property_name)) {
      if (value)
        *value = props->prop_values[i];
      return prop->prop_id;
    }
  }
  return 0;
}

int set_property(struct resources *res, uint32_t object_type, uint32_t obj_id,
                 const char *property_name, uint64_t value) {
  uint64_t old_value = 0;
  uint32_t prop_id =
      get_property_id(res, object_type, obj_id, property_name, &old_value);
  if (prop_id <= 0)
    return -1;
  if (old_value == value)
    return 0;
  return drmModeObjectSetProperty(res->drm_fd, obj_id, object_type, prop_id,
                                  value);
}

#if 0
static void store_saved_crtc(struct resources *res) {
  drmModeRes *dmr = res->res;
  for (int i = 0; i < dmr->count_connectors; i++) {
    struct connector *connector = &res->connectors[i];
    drmModeConnector *conn = connector->connector;
    if (!conn)
      continue;
    if (conn->connection != DRM_MODE_CONNECTED)
      continue;
    if (conn->count_modes <= 0)
      continue;
    int crtc_idx = get_crtc_index_by_connector(res, conn);
    if (crtc_idx >= 0)
      connector->saved_crtc = res->crtcs[crtc_idx].crtc;
  }
}

static void restore_saved_crtc(int fd, struct resources *res) {
  drmModeRes *dmr = res->res;
  for (int i = 0; i < dmr->count_connectors; i++) {
    struct connector *connector = &res->connectors[i];
    drmModeCrtc *saved_crtc = connector->saved_crtc;
    if (!saved_crtc)
      continue;
    drmModeConnector *conn = connector->connector;
    drmModeSetCrtc(fd, saved_crtc->crtc_id, saved_crtc->buffer_id,
                   saved_crtc->x, saved_crtc->y, &conn->connector_id, 1,
                   &saved_crtc->mode);
  }
}
#endif

struct DRMFmtStringEntry {
  uint32_t fmt;
  const char *type_str;
};
const struct DRMFmtStringEntry drm_fmt_string_map[] = {
    {DRM_FORMAT_YUV420, IMAGE_YUV420P},   {DRM_FORMAT_NV12, IMAGE_NV12},
    {DRM_FORMAT_NV21, IMAGE_NV21},        {DRM_FORMAT_YUV422, IMAGE_YUV422P},
    {DRM_FORMAT_NV16, IMAGE_NV16},        {DRM_FORMAT_NV61, IMAGE_NV61},
    {DRM_FORMAT_YUYV, IMAGE_YUYV422},     {DRM_FORMAT_UYVY, IMAGE_UYVY422},
    {DRM_FORMAT_RGB332, IMAGE_RGB332},    {DRM_FORMAT_RGB565, IMAGE_RGB565},
    {DRM_FORMAT_BGR565, IMAGE_BGR565},    {DRM_FORMAT_RGB888, IMAGE_RGB888},
    {DRM_FORMAT_BGR888, IMAGE_BGR888},    {DRM_FORMAT_ARGB8888, IMAGE_ARGB8888},
    {DRM_FORMAT_ABGR8888, IMAGE_ABGR8888}};

uint32_t GetDRMFmtByString(const char *type) {
  if (!type)
    return 0;
  for (size_t i = 0; i < ARRAY_ELEMS(drm_fmt_string_map) - 1; i++) {
    if (!strcmp(type, drm_fmt_string_map[i].type_str))
      return drm_fmt_string_map[i].fmt;
  }
  return 0;
}

class _DRM_SUPPORT_TYPES : public SupportMediaTypes {
public:
  _DRM_SUPPORT_TYPES() {
    for (size_t i = 0; i < ARRAY_ELEMS(drm_fmt_string_map) - 1; i++) {
      types.append(drm_fmt_string_map[i].type_str);
    }
  }
};
static _DRM_SUPPORT_TYPES priv_types;

const std::string &GetStringOfDRMFmts() { return priv_types.types; }

DRMDevice::DRMDevice(const std::string &drm_path)
    : fd(-1)
#ifndef NDEBUG
      ,
      path(drm_path)
#endif
{
#if 1
  fd = open(drm_path.c_str(), O_RDWR | O_CLOEXEC);
  RKMEDIA_LOGD("open %s = %d\n", drm_path.c_str(), fd);
#else
  int drm_fd = open(drm_path.c_str(), O_RDWR | O_CLOEXEC);
  if (drm_fd >= 0) {
    drmModeRes *resources = drmModeGetResources(drm_fd);
    if (resources != NULL) {
      fd = drm_fd;
      drmModeFreeResources(resources);
    } else {
      close(drm_fd);
    }
  }
#endif
}

DRMDevice::~DRMDevice() {
  if (fd >= 0) {
    close(fd);
    RKMEDIA_LOGD("close %s = %d\n", path.c_str(), fd);
    fd = -1;
  }
}

struct resources *DRMDevice::get_resources() {
  if (fd < 0)
    return nullptr;
  if (drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1)) {
    RKMEDIA_LOGI("Failed to set universal planes cap %m\n");
    return nullptr;
  }
  // many features need atomic cap
  if (drmSetClientCap(fd, DRM_CLIENT_CAP_ATOMIC, 1)) {
    RKMEDIA_LOGI("Failed set drm atomic cap %m\n");
    return nullptr;
  }
  int i;
  struct resources *res = (struct resources *)calloc(1, sizeof(*res));
  if (!res)
    return nullptr;
  res->drm_fd = -1;
  res->res = drmModeGetResources(fd);
  if (!res->res) {
    RKMEDIA_LOGI("drmModeGetResources failed: %m\n");
    goto error;
  }
  res->crtcs =
      (struct crtc *)calloc(res->res->count_crtcs, sizeof(*res->crtcs));
  res->ids.crtc_ids =
      (uint32_t *)calloc(res->res->count_crtcs, sizeof(*res->ids.crtc_ids));
  res->encoders = (struct encoder *)calloc(res->res->count_encoders,
                                           sizeof(*res->encoders));
  res->ids.encoder_ids = (uint32_t *)calloc(res->res->count_encoders,
                                            sizeof(*res->ids.encoder_ids));
  res->connectors = (struct connector *)calloc(res->res->count_connectors,
                                               sizeof(*res->connectors));
  res->ids.connector_ids = (uint32_t *)calloc(res->res->count_connectors,
                                              sizeof(*res->ids.connector_ids));
  res->fbs = (struct fb *)calloc(res->res->count_fbs, sizeof(*res->fbs));

  if (!res->crtcs || !res->encoders || !res->connectors || !res->fbs)
    goto error;
  if (!res->ids.crtc_ids || !res->ids.encoder_ids || !res->ids.connector_ids)
    goto error;

#define get_resource(_res, __res, type, Type)                                  \
  do {                                                                         \
    for (i = 0; i < (int)(_res)->__res->count_##type##s; ++i) {                \
      (_res)->type##s[i].type =                                                \
          drmModeGet##Type(fd, (_res)->__res->type##s[i]);                     \
      if (!(_res)->type##s[i].type) {                                          \
        RKMEDIA_LOGI("could not get %s %i: %m\n", #type,                       \
                     (_res)->__res->type##s[i]);                               \
      } else if ((_res)->ids.type##_ids) {                                     \
        (_res)->ids.type##_ids[(_res)->ids.count_##type##s++] =                \
            (_res)->__res->type##s[i];                                         \
      }                                                                        \
    }                                                                          \
  } while (0)

  get_resource(res, res, crtc, Crtc);
  get_resource(res, res, encoder, Encoder);
  get_resource(res, res, connector, Connector);
  get_resource(res, res, fb, FB);

  /* Set the name of all connectors based on the type name and the per-type
   * ID.
   */
  for (i = 0; i < res->res->count_connectors; i++) {
    struct connector *sconnector = &res->connectors[i];
    drmModeConnector *conn = sconnector->connector;

    int ret = asprintf(&sconnector->name, "%s-%u",
                       lookup_drm_connector_type_name(conn->connector_type),
                       conn->connector_type_id);
    if (ret < 0) {
      RKMEDIA_LOGI("asprintf failed\n");
    }
  }

#define get_properties(_res, __res, type, Type)                                \
  do {                                                                         \
    for (i = 0; i < (int)(_res)->__res->count_##type##s; ++i) {                \
      struct type *obj = &res->type##s[i];                                     \
      unsigned int j;                                                          \
      obj->props = drmModeObjectGetProperties(fd, obj->type->type##_id,        \
                                              DRM_MODE_OBJECT_##Type);         \
      if (!obj->props) {                                                       \
        RKMEDIA_LOGI("could not get %s %i properties: %m\n", #type,            \
                     obj->type->type##_id);                                    \
        continue;                                                              \
      }                                                                        \
      obj->props_info = (drmModePropertyRes **)calloc(                         \
          obj->props->count_props, sizeof(*obj->props_info));                  \
      if (!obj->props_info)                                                    \
        continue;                                                              \
      for (j = 0; j < obj->props->count_props; ++j)                            \
        obj->props_info[j] = drmModeGetProperty(fd, obj->props->props[j]);     \
    }                                                                          \
  } while (0)

  get_properties(res, res, crtc, CRTC);
  get_properties(res, res, connector, CONNECTOR);

  for (i = 0; i < res->res->count_crtcs; ++i)
    res->crtcs[i].mode = &res->crtcs[i].crtc->mode;

  res->plane_res = drmModeGetPlaneResources(fd);
  if (!res->plane_res) {
    RKMEDIA_LOGI("drmModeGetPlaneResources failed: %m\n");
    goto error;
  }
  res->planes = (struct plane *)calloc(res->plane_res->count_planes,
                                       sizeof(*res->planes));
  if (!res->planes)
    goto error;
  res->ids.plane_ids = (uint32_t *)calloc(res->plane_res->count_planes,
                                          sizeof(*res->ids.plane_ids));
  if (!res->ids.plane_ids)
    goto error;

  get_resource(res, plane_res, plane, Plane);
  get_properties(res, plane_res, plane, PLANE);

  res->drm_fd = fd;
  return res;

error:
  free_resources(res);
  return nullptr;
}

void DRMDevice::free_resources(struct resources *res) {
  int i;
  if (!res)
    return;
  assert(res->drm_fd < 0 || res->drm_fd == fd);
#define free_resource(_res, __res, type, Type)                                 \
  do {                                                                         \
    if (!(_res)->type##s)                                                      \
      break;                                                                   \
    for (i = 0; i < (int)(_res)->__res->count_##type##s; ++i) {                \
      if ((_res)->type##s[i].type)                                             \
        drmModeFree##Type((_res)->type##s[i].type);                            \
    }                                                                          \
    free((_res)->type##s);                                                     \
    if ((_res->ids.type##_ids))                                                \
      free((_res->ids.type##_ids));                                            \
  } while (0)

#define free_properties(_res, __res, type)                                     \
  do {                                                                         \
    for (i = 0; i < (int)(_res)->__res->count_##type##s; ++i) {                \
      struct type *obj = &res->type##s[i];                                     \
      unsigned int j;                                                          \
      for (j = 0; j < obj->props->count_props; ++j) {                          \
        if (obj->props_info[j])                                                \
          drmModeFreeProperty(obj->props_info[j]);                             \
      }                                                                        \
      if (obj->props_info)                                                     \
        free(obj->props_info);                                                 \
      drmModeFreeObjectProperties(obj->props);                                 \
    }                                                                          \
  } while (0)

  if (res->res) {
    free_properties(res, res, crtc);
    free_resource(res, res, crtc, Crtc);
    free_resource(res, res, encoder, Encoder);
    for (i = 0; i < res->res->count_connectors; i++)
      free(res->connectors[i].name);
    free_properties(res, res, connector);
    free_resource(res, res, connector, Connector);
    free_resource(res, res, fb, FB);
    drmModeFreeResources(res->res);
  }

  if (res->plane_res) {
    free_properties(res, plane_res, plane);
    free_resource(res, plane_res, plane, Plane);
    drmModeFreePlaneResources(res->plane_res);
  }
  free(res);
}

} // namespace easymedia
