// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EASYMEDIA_DRM_UTILS_H_
#define EASYMEDIA_DRM_UTILS_H_

#include <drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <string>
#include <vector>

namespace easymedia {

#if 1
// move to drm_control.h

// "CSI-TX-PATH"
enum csi_path_mode { VOP_PATH = 0, BYPASS_PATH };

// "PDAF_TYPE"
enum vop_pdaf_mode {
  VOP_HOLD_MODE = 0,
  VOP_NORMAL_MODE,
  VOP_PINGPONG_MODE,
  VOP_BYPASS_MODE,
  VOP_BACKGROUND_MODE,
  VOP_ONEFRAME_MODE,
  VOP_ONEFRAME_NOSEND_MODE
};

// "WORK_MODE"
enum vop_pdaf_type {
  VOP_PDAF_TYPE_DEFAULT = 0,
  VOP_PDAF_TYPE_HBLANK,
  VOP_PDAF_TYPE_VBLANK,
};
#endif

// come from modetest.c
struct crtc {
  drmModeCrtc *crtc;
  drmModeObjectProperties *props;
  drmModePropertyRes **props_info;
  drmModeModeInfo *mode;
};

struct encoder {
  drmModeEncoder *encoder;
};

struct connector {
  drmModeConnector *connector;
  drmModeObjectProperties *props;
  drmModePropertyRes **props_info;
  char *name;

  drmModeCrtc *saved_crtc;
};

struct fb {
  drmModeFB *fb;
};

struct plane {
  drmModePlane *plane;
  drmModeObjectProperties *props;
  drmModePropertyRes **props_info;
};

struct drm_ids {
  uint32_t count_fbs; // unneed
  uint32_t *fb_ids;   // unneed

  uint32_t count_connectors;
  uint32_t *connector_ids;

  uint32_t count_encoders;
  uint32_t *encoder_ids;

  uint32_t count_crtcs;
  uint32_t *crtc_ids;

  uint32_t count_planes;
  uint32_t *plane_ids;
};

struct resources {
  int drm_fd;
  drmModeRes *res;
  drmModePlaneRes *plane_res;

  struct crtc *crtcs;
  struct encoder *encoders;
  struct connector *connectors;
  struct fb *fbs;
  struct plane *planes;
  struct drm_ids ids;
};

#define DRM_ID_ISVALID(id) (id != 0 && id != ((uint32_t)-1))

void dump_suitable_ids(struct resources *res);

int get_connector_index(struct resources *res, uint32_t id);
drmModeConnector *get_connector_by_id(struct resources *res, uint32_t id);
drmModeEncoder *get_encoder_by_id(struct resources *res, uint32_t id);
int get_crtc_index(struct resources *res, uint32_t id);
int get_crtc_index_by_connector(struct resources *res, drmModeConnector *conn);
drmModeCrtc *get_crtc_by_id(struct resources *res, uint32_t id);
int get_plane_index(struct resources *res, uint32_t id);
drmModePlane *get_plane_by_id(struct resources *res, uint32_t id);
const char *GetPlaneTypeStrByPlaneID(struct resources *res, uint32_t id);

// reserve: filter the unmatch elements
// Return false if there has any empty type_ids which means we can not get any
// mating set of connector/encoder/crtc/plane id. If return false, we may not
// have filtered ids of other types.
bool reserve_ids_by_connector(struct resources *res, uint32_t conn_id);
bool reserve_ids_by_encoder(struct resources *res, uint32_t enc_id);
bool reserve_ids_by_crtc(struct resources *res, uint32_t crtc_id);
bool reserve_ids_by_plane(struct resources *res, uint32_t plane_id);

bool filter_ids_if_connector_notready(struct resources *res);
bool filter_ids_by_fps(struct resources *res, uint32_t vrefresh);
bool comp_width_height(drmModeConnector *dmc, int w, int h);
bool find_connector_ids_by_wh(struct resources *res, int w, int h);
bool filter_ids_by_wh(struct resources *res, int w, int h);
bool filter_ids_by_data_type(struct resources *res,
                             const std::string &data_type);
bool filter_ids_by_plane_type(struct resources *res,
                              const std::string &plane_type);
bool filter_ids_by_skip_plane_ids(struct resources *res,
                                  const std::vector<uint32_t> &skip_plane_ids);

int filter_modeinfo_by_wh(int w, int h, int &size, drmModeModeInfoPtr *modes);
int filter_modeinfo_by_fps(int fps, int &size, drmModeModeInfoPtr *modes);
int filter_modeinfo_by_type(uint32_t type, int &size,
                            drmModeModeInfoPtr *modes);

uint32_t get_property_id(struct resources *res, uint32_t object_type,
                         uint32_t obj_id, const char *property_name,
                         uint64_t *value = nullptr);
int set_property(struct resources *res, uint32_t object_type, uint32_t obj_id,
                 const char *property_name, uint64_t value);

uint32_t GetDRMFmtByString(const char *type);
const std::string &GetStringOfDRMFmts();

#if 1
class DRMDevice {
public:
  DRMDevice(const std::string &drm_path);
  ~DRMDevice();
  int GetDeviceFd() { return fd; }
  struct resources *get_resources();
  void free_resources(struct resources *res);

private:
  int fd;
  //#ifndef NDEBUG
  std::string path;
  //#endif
};
#else
class DRMDevice {
public:
  static std::shared_ptr<DRMDevice> GetDrmDevice(std::string path);
  void free_resources(struct resources *res);
  struct resources *get_resources();

  int fd;

private:
  DRMDevice(std::string path);
  ~DRMDevice();
  static std::map<std::string, std::shared_ptr<DRMDevice>> gDrmDevices;
  static std::map<std::string, std::mutex> gDrmDeviceMutexs;
};
#endif

} // namespace easymedia

#endif // #ifndef EASYMEDIA_DRM_UTILS_H_