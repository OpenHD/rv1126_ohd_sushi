// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mpp_encoder.h"

#include <assert.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#include "buffer.h"
#include "mpp_jpeg_packet.h"
#include "utils.h"
#include <memory>
#include <rga/im2d.h>
#include <rga/rga.h>

#include "mpp_jpeg_fd_info.h"

#ifdef MOD_TAG
#undef MOD_TAG
#endif
#define MOD_TAG RK_ID_VENC

namespace easymedia {

MPPEncoder::MPPEncoder()
    : coding_type(MPP_VIDEO_CodingAutoDetect), output_mb_flags(0),
      encoder_sta_en(false), encoded_bps(0), encoded_fps(0), stream_size_1s(0),
      frame_cnt_1s(0), last_ts(0), cur_ts(0), userdata_len(0),
      userdata_frame_id(0), userdata_all_frame_en(0), app2_len(0) {
  mpp_ctx = NULL;
#ifdef MPP_SUPPORT_HW_OSD
  // reset osd data.
  memset(&osd_data, 0, sizeof(osd_data));
  memset(&osd_data2, 0, sizeof(osd_data2));
  osd_buf_grp = NULL;
#endif

#ifdef RGA_OSD_ENABLE
  // reset jpeg osd data.
  for (int i = 0; i < OSD_REGIONS_CNT; i++) {
    rga_osd_data[i].data.reset();
    rga_osd_data[i].pos_x = 0;
    rga_osd_data[i].pos_y = 0;
    rga_osd_data[i].width = 0;
    rga_osd_data[i].height = 0;
    rga_osd_data[i].enable = 0;
    rga_osd_data[i].inverse = 0;
    rga_osd_data[i].region_type = REGION_TYPE_OVERLAY_EX;
    rga_osd_data[i].cover_color = 0;
  }
  rga_osd_cnt = 0;
#endif
  memset(&ud_set, 0, sizeof(MppEncUserDataSet));
  memset(ud_datas, 0, sizeof(ud_datas) * 2);
  memset(&roi_cfg, 0, sizeof(roi_cfg));
  rc_api_brief_name = "default";
  memset(thumbnail_type, 0, sizeof(thumbnail_type));
  memset(thumbnail_width, 0, sizeof(thumbnail_width));
  memset(thumbnail_height, 0, sizeof(thumbnail_height));
}

MPPEncoder::~MPPEncoder() {
#ifdef MPP_SUPPORT_HW_OSD
  if (osd_data.buf) {
    RKMEDIA_LOGD("MPP Encoder: free osd buff\n");
    mpp_buffer_put(osd_data.buf);
    osd_data.buf = NULL;
  }
  for (int i = 0; i < OSD_REGIONS_CNT; i++) {
    RKMEDIA_LOGD("MPP Encoder: free osd2 buff\n");
    if (osd_data2.region[i].buf) {
      mpp_buffer_put(osd_data2.region[i].buf);
      osd_data2.region[i].buf = NULL;
    }
  }
  if (osd_buf_grp) {
    RKMEDIA_LOGD("MPP Encoder: free osd buff group\n");
    mpp_buffer_group_put(osd_buf_grp);
    osd_buf_grp = NULL;
  }
#endif

#ifdef RGA_OSD_ENABLE
  // reset jpeg osd data.
  for (int i = 0; i < OSD_REGIONS_CNT; i++)
    rga_osd_data[i].data.reset();
  rga_osd_cnt = 0;
#endif

  if (roi_cfg.regions) {
    RKMEDIA_LOGD("MPP Encoder: free enc roi region buff\n");
    free(roi_cfg.regions);
    roi_cfg.regions = NULL;
  }
}

void MPPEncoder::SetMppCodeingType(MppCodingType type) {
  coding_type = type;
  if (type == MPP_VIDEO_CodingMJPEG)
    codec_type = CODEC_TYPE_JPEG;
  else if (type == MPP_VIDEO_CodingAVC)
    codec_type = CODEC_TYPE_H264;
  else if (type == MPP_VIDEO_CodingHEVC)
    codec_type = CODEC_TYPE_H265;
  // mpp always return a single nal
  if (type == MPP_VIDEO_CodingAVC || type == MPP_VIDEO_CodingHEVC)
    output_mb_flags |= MediaBuffer::kSingleNalUnit;
}

bool MPPEncoder::Init() {
  int ret = 0;
  if (coding_type == MPP_VIDEO_CodingUnused)
    return false;
#ifdef MPP_SUPPORT_HW_OSD
  ret = mpp_buffer_group_get_internal(&osd_buf_grp, MPP_BUFFER_TYPE_DRM);
  if (ret) {
    RKMEDIA_LOGE("MPP Encoder: failed to get mpp buffer group! ret=%d\n", ret);
    return false;
  }
  ret = mpp_buffer_group_limit_config(osd_buf_grp, 0, OSD_REGIONS_CNT + 1);
  if (ret) {
    RKMEDIA_LOGE("MPP Encoder: failed to limit buffer group! ret=%d\n", ret);
    return false;
  }
#endif
  mpp_ctx = std::make_shared<MPPContext>();
  if (!mpp_ctx)
    return false;
  MppCtx ctx = NULL;
  MppApi *mpi = NULL;
  ret = mpp_create(&ctx, &mpi);
  if (ret) {
    RKMEDIA_LOGI("mpp_create failed\n");
    return false;
  }
  mpp_ctx->ctx = ctx;
  mpp_ctx->mpi = mpi;
  ret = mpp_init(ctx, MPP_CTX_ENC, coding_type);
  if (ret != MPP_OK) {
    RKMEDIA_LOGI("mpp_init failed with type %d\n", coding_type);
    mpp_destroy(ctx);
    ctx = NULL;
    mpi = NULL;
    return false;
  }

  return true;
}

int MPPEncoder::PrepareMppFrame(const std::shared_ptr<MediaBuffer> &input,
                                std::shared_ptr<MediaBuffer> &mdinfo,
                                MppFrame &frame) {
  MppBuffer pic_buf = nullptr;
  if (input->GetType() != Type::Image) {
    RKMEDIA_LOGI("mpp encoder input source only support image buffer\n");
    return -EINVAL;
  }
  PixelFormat fmt = input->GetPixelFormat();
  if (fmt == PIX_FMT_NONE) {
    RKMEDIA_LOGI("mpp encoder input source invalid pixel format\n");
    return -EINVAL;
  }
  ImageBuffer *hw_buffer = static_cast<ImageBuffer *>(input.get());

  assert(input->GetValidSize() > 0);
  mpp_frame_set_pts(frame, hw_buffer->GetUSTimeStamp());
  mpp_frame_set_dts(frame, hw_buffer->GetUSTimeStamp());
  mpp_frame_set_width(frame, hw_buffer->GetWidth());
  mpp_frame_set_height(frame, hw_buffer->GetHeight());
  mpp_frame_set_offset_x(frame, this->GetConfig().img_cfg.rect_info.x);
  mpp_frame_set_offset_y(frame, this->GetConfig().img_cfg.rect_info.y);
  mpp_frame_set_fmt(frame, ConvertToMppPixFmt(fmt));

  if (fmt == PIX_FMT_YUYV422 || fmt == PIX_FMT_UYVY422)
    mpp_frame_set_hor_stride(frame, hw_buffer->GetVirWidth() * 2);
  else
    mpp_frame_set_hor_stride(frame, hw_buffer->GetVirWidth());
  mpp_frame_set_ver_stride(frame, hw_buffer->GetVirHeight());

  MppMeta meta = mpp_frame_get_meta(frame);
  auto &related_vec = input->GetRelatedSPtrs();
  if (!related_vec.empty()) {
    mdinfo = std::static_pointer_cast<MediaBuffer>(related_vec[0]);
    RKMEDIA_LOGD("MPP Encoder: set mdinfo(%p, %zuBytes) to frame\n",
                 mdinfo->GetPtr(), mdinfo->GetValidSize());
    mpp_meta_set_ptr(meta, KEY_MV_LIST, mdinfo->GetPtr());
  }

  if (roi_cfg.number && roi_cfg.regions) {
    RKMEDIA_LOGD("MPP Encoder: set roi cfg(cnt:%d,%p) to frame\n",
                 roi_cfg.number, roi_cfg.regions);
    mpp_meta_set_ptr(meta, KEY_ROI_DATA, &roi_cfg);
  }

#ifdef MPP_SUPPORT_HW_OSD
  if (osd_data.num_region && osd_data.buf) {
    RKMEDIA_LOGD("MPP Encoder: set osd data(%d regions) to frame\n",
                 osd_data.num_region);
    mpp_meta_set_ptr(meta, KEY_OSD_DATA, (void *)&osd_data);
  }
  if (osd_data2.num_region) {
    RKMEDIA_LOGD("MPP Encoder: set osd data2(%d regions) to frame\n",
                 osd_data2.num_region);
    mpp_meta_set_ptr(meta, KEY_OSD_DATA2, (void *)&osd_data2);
  }
#endif // MPP_SUPPORT_HW_OSD

  int ud_idx = 0;
  if (userdata_len) {
    RKMEDIA_LOGD("MPP Encoder: set userdata(%dBytes) to frame\n", userdata_len);
    bool skip_frame = false;
    if (!userdata_all_frame_en) {
      MediaConfig &cfg = GetConfig();
      // userdata_frame_id = 0 : first gop frame.
      if (userdata_frame_id)
        skip_frame = true;

      userdata_frame_id++;
      if (userdata_frame_id == cfg.vid_cfg.gop_size)
        userdata_frame_id = 0;
    }

    if (!skip_frame) {
      ud_datas[ud_idx].pdata = userdata;
      ud_datas[ud_idx].len = userdata_len;
      ud_datas[ud_idx].uuid = NULL;
      ud_idx++;
    }
  }

  if (input->GetDbgInfo() && input->GetDbgInfoSize()) {
    ud_datas[ud_idx].pdata = input->GetDbgInfo();
    ud_datas[ud_idx].len = input->GetDbgInfoSize();
    ud_datas[ud_idx].uuid = NULL;
    ud_idx++;
  }

  if (app2_len) {
    ud_datas[ud_idx].pdata = app2;
    ud_datas[ud_idx].len = app2_len;
    ud_datas[ud_idx].uuid = NULL;
    ud_idx++;
  }

  if (ud_idx) {
    ud_set.count = ud_idx;
    ud_set.datas = ud_datas;
    RKMEDIA_LOGD("MPP Encoder: userdata: ud_set.count:%d, ud_set.datas:%p...\n",
                 ud_set.count, ud_set.datas);
    mpp_meta_set_ptr(meta, KEY_USER_DATAS, &ud_set);
  }

  MPP_RET ret = init_mpp_buffer_with_content(pic_buf, input);
  if (ret) {
    RKMEDIA_LOGI("prepare picture buffer failed\n");
    return ret;
  }

  mpp_frame_set_buffer(frame, pic_buf);
  if (input->IsEOF())
    mpp_frame_set_eos(frame, 1);

  mpp_buffer_put(pic_buf);

  return 0;
}

int MPPEncoder::PrepareMppPacket(std::shared_ptr<MediaBuffer> &output,
                                 MppPacket &packet) {
  MppBuffer mpp_buf = nullptr;
  if (!output->IsHwBuffer())
    return 0;
  MPP_RET ret = init_mpp_buffer(mpp_buf, output, 0);
  if (ret) {
    RKMEDIA_LOGI("import output stream buffer failed\n");
    return ret;
  }

  if (mpp_buf) {
    mpp_packet_init_with_buffer(&packet, mpp_buf);
    mpp_buffer_put(mpp_buf);
  }

  return 0;
}

int MPPEncoder::PrepareThumbnail(const std::shared_ptr<MediaBuffer> &input,
                                 int thumbnail_num, char *buf, int *len) {
  int ret = 0;
  MppBuffer frame_buf = nullptr;
  MppFrame frame = nullptr;
  MppPacket packet = nullptr;
  MppBuffer mv_buf = nullptr;
  MppEncCfg enc_cfg;
  int packet_len = 0;
  assert(thumbnail_num <= THUMBNAIL_MAX_CNT);
  assert(thumbnail_type[thumbnail_num] != 0);

  ImageBuffer *hw_buffer = static_cast<ImageBuffer *>(input.get());
  if (thumbnail_type[thumbnail_num] == THUMBNAIL_TYPE_APP2) {
    assert(thumbnail_width[thumbnail_num] <
           this->GetConfig().img_cfg.rect_info.w);
    assert(thumbnail_width[thumbnail_num] > 160);
    assert(thumbnail_height[thumbnail_num] <
           this->GetConfig().img_cfg.rect_info.h);
    assert(thumbnail_height[thumbnail_num] < 120);
  }

  if (thumbnail_type[thumbnail_num] == THUMBNAIL_TYPE_APP1) {
    assert(thumbnail_width[thumbnail_num] == 160);
    assert(thumbnail_height[thumbnail_num] == 120);
  }

  ret = mpp_frame_init(&frame);
  if (MPP_OK != ret) {
    RKMEDIA_LOGI("mpp_frame_init failed, ret = %d\n", ret);
    return ret;
  }
  extern int get_rga_format(PixelFormat f);
  PixelFormat src_pixfmt = input->GetPixelFormat();
  PixelFormat trg_pixfmt = PIX_FMT_NV12;
  RK_U32 u32PicSize = CalPixFmtSize(trg_pixfmt, thumbnail_width[thumbnail_num],
                                    thumbnail_height[thumbnail_num], 8);
  RK_VOID *pvPicPtr = malloc(u32PicSize);
  if (pvPicPtr == NULL) {
    ret = -1;
    goto THUMB_END;
  }
  rga_buffer_t src_info, dst_info;
  memset(&src_info, 0, sizeof(rga_buffer_t));
  memset(&dst_info, 0, sizeof(rga_buffer_t));
  src_info = wrapbuffer_virtualaddr(
      input->GetPtr(), hw_buffer->GetWidth(), hw_buffer->GetHeight(),
      get_rga_format(src_pixfmt), hw_buffer->GetVirWidth(),
      hw_buffer->GetVirHeight());
  im_rect rect_info;
  rect_info.x = this->GetConfig().img_cfg.rect_info.x;
  rect_info.y = this->GetConfig().img_cfg.rect_info.y;
  rect_info.width = this->GetConfig().img_cfg.rect_info.w;
  rect_info.height = this->GetConfig().img_cfg.rect_info.h;

  dst_info = wrapbuffer_virtualaddr(
      pvPicPtr, thumbnail_width[thumbnail_num], thumbnail_height[thumbnail_num],
      get_rga_format(trg_pixfmt), UPALIGNTO(thumbnail_width[thumbnail_num], 8),
      UPALIGNTO(thumbnail_height[thumbnail_num], 8));
  ret = imcrop(src_info, dst_info, rect_info);
  if (ret <= 0) {
    RKMEDIA_LOGE("imcrop failed, ret = %d\n", ret);
    RKMEDIA_LOGE("%s: rect_info(%d, %d, %d, %d)", __func__, rect_info.x,
                 rect_info.y, rect_info.width, rect_info.height);
    RKMEDIA_LOGE("%s: src_info(%d, %d)\n", __func__, src_info.width,
                 src_info.height);
    RKMEDIA_LOGE("%s: dst_info(%d, %d)\n", __func__, dst_info.width,
                 dst_info.height);
    ret = -1;
    goto THUMB_END;
  }

  ret = mpp_enc_cfg_init(&enc_cfg);
  if (MPP_OK != ret) {
    RKMEDIA_LOGE("MPP Encoder: MPPConfig: cfg init failed, ret = %d\n", ret);
    goto THUMB_END;
  }

  ret = mpp_enc_cfg_set_s32(enc_cfg, "prep:width",
                            thumbnail_width[thumbnail_num]);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:height",
                             thumbnail_height[thumbnail_num]);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:hor_stride",
                             UPALIGNTO(thumbnail_width[thumbnail_num], 8));
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:ver_stride",
                             UPALIGNTO(thumbnail_height[thumbnail_num], 8));
  ret = EncodeControl(MPP_ENC_SET_CFG, enc_cfg);
  if (ret) {
    RKMEDIA_LOGE("MPP Encoder[JPEG]: set resolution cfg failed ret %d\n", ret);
    goto THUMB_END;
  }
  mpp_frame_set_pts(frame, hw_buffer->GetUSTimeStamp());
  mpp_frame_set_dts(frame, hw_buffer->GetUSTimeStamp());
  mpp_frame_set_width(frame, thumbnail_width[thumbnail_num]);
  mpp_frame_set_height(frame, thumbnail_height[thumbnail_num]);
  mpp_frame_set_offset_x(frame, 0);
  mpp_frame_set_offset_y(frame, 0);
  mpp_frame_set_fmt(frame, ConvertToMppPixFmt(PIX_FMT_NV12));
  mpp_frame_set_hor_stride(frame, UPALIGNTO(thumbnail_width[thumbnail_num], 8));
  mpp_frame_set_ver_stride(frame,
                           UPALIGNTO(thumbnail_height[thumbnail_num], 8));
  ret = init_mpp_buffer(frame_buf, NULL, u32PicSize);
  if (ret) {
    RKMEDIA_LOGE("init frame buffer failed, ret = %d\n", ret);
    goto THUMB_END;
  }

  if (frame_buf) {
    memcpy(mpp_buffer_get_ptr(frame_buf), pvPicPtr, u32PicSize);
    mpp_frame_set_buffer(frame, frame_buf);
    mpp_buffer_put(frame_buf);
  }
  mpp_frame_set_eos(frame, 1);
  ret = Process(frame, packet, mv_buf);
  if (ret) {
    RKMEDIA_LOGE("Process failed, ret = %d\n", ret);
    goto THUMB_END;
  }

  packet_len = mpp_packet_get_length(packet);
  if (packet_len < *len) {
    memcpy(buf, mpp_packet_get_data(packet), packet_len);
    *len = packet_len;
  }
  ret = mpp_enc_cfg_set_s32(enc_cfg, "prep:width",
                            this->GetConfig().img_cfg.rect_info.w);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:height",
                             this->GetConfig().img_cfg.rect_info.h);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:hor_stride",
                             this->GetConfig().img_cfg.image_info.vir_width);
  ret |= mpp_enc_cfg_set_s32(enc_cfg, "prep:ver_stride",
                             this->GetConfig().img_cfg.image_info.vir_height);
  ret = EncodeControl(MPP_ENC_SET_CFG, enc_cfg);
  if (ret) {
    RKMEDIA_LOGE("MPP Encoder[JPEG]: set resolution cfg failed ret %d\n", ret);
    goto THUMB_END;
  }
  if (enc_cfg) {
    mpp_enc_cfg_deinit(enc_cfg);
    RKMEDIA_LOGI("MPP Encoder: MPPConfig: cfg deinit done!\n");
  }

THUMB_END:
  if (pvPicPtr)
    free(pvPicPtr);
  if (frame)
    mpp_frame_deinit(&frame);
  if (packet)
    mpp_packet_deinit(&packet);

  return ret;
}

int MPPEncoder::PrepareMppExtraBuffer(std::shared_ptr<MediaBuffer> extra_output,
                                      MppBuffer &buffer) {
  MppBuffer mpp_buf = nullptr;
  if (!extra_output || !extra_output->IsValid())
    return 0;
  MPP_RET ret =
      init_mpp_buffer(mpp_buf, extra_output, extra_output->GetValidSize());
  if (ret) {
    RKMEDIA_LOGI("import extra stream buffer failed\n");
    return ret;
  }
  buffer = mpp_buf;
  return 0;
}

class MPPPacketContext {
public:
  MPPPacketContext(std::shared_ptr<MPPContext> ctx, MppPacket p)
      : mctx(ctx), packet(p) {}
  ~MPPPacketContext() {
    if (packet)
      mpp_packet_deinit(&packet);
  }

private:
  std::shared_ptr<MPPContext> mctx;
  MppPacket packet;
};

static int __free_mpppacketcontext(void *p) {
  assert(p);
  delete (MPPPacketContext *)p;
  return 0;
}

int MPPEncoder::Process(const std::shared_ptr<MediaBuffer> &input,
                        std::shared_ptr<MediaBuffer> &output,
                        std::shared_ptr<MediaBuffer> extra_output) {
  MppFrame frame = nullptr;
  MppPacket packet = nullptr;
  MppPacket import_packet = nullptr;
  MppBuffer mv_buf = nullptr;
  size_t packet_len = 0;
  size_t packet_size = 0;
  RK_U32 packet_flag = 0;
  RK_U32 out_eof = 0;
  RK_S64 pts = 0;
  std::shared_ptr<MediaBuffer> mdinfo;
  RK_S32 temporal_id = -1;
  Type out_type;
  char *mpf[3] = {NULL, NULL, NULL};
  int mpf_len[3] = {0, 0, 0};
  int image_cnt = 0;

  if (!input)
    return 0;
  if (!output)
    return -EINVAL;

  // all changes must set before encode and among the same thread
  while (HasChangeReq()) {
    auto change = PeekChange();
    if (change.first && !CheckConfigChange(change))
      return -1;
  }

  // check input buffer.
  MediaConfig &cfg_check = GetConfig();
  ImageBuffer *hw_buffer = static_cast<ImageBuffer *>(input.get());
  if (cfg_check.img_cfg.image_info.width != hw_buffer->GetWidth() ||
      cfg_check.img_cfg.image_info.height != hw_buffer->GetHeight() ||
      cfg_check.img_cfg.image_info.vir_width != hw_buffer->GetVirWidth() ||
      cfg_check.img_cfg.image_info.vir_height != hw_buffer->GetVirHeight()) {
    RKMEDIA_LOGE("The resolution of the input buffer is wrong. check[%d, %d, "
                 "%d, %d], buffer[%d, %d, %d, %d]\n",
                 cfg_check.img_cfg.image_info.width,
                 cfg_check.img_cfg.image_info.height,
                 cfg_check.img_cfg.image_info.vir_width,
                 cfg_check.img_cfg.image_info.vir_height, hw_buffer->GetWidth(),
                 hw_buffer->GetHeight(), hw_buffer->GetVirWidth(),
                 hw_buffer->GetVirHeight());
    return 0;
  }

  int ret = mpp_frame_init(&frame);
  if (MPP_OK != ret) {
    RKMEDIA_LOGI("mpp_frame_init failed\n");
    goto ENCODE_OUT;
  }

  if (thumbnail_type[0] == THUMBNAIL_TYPE_APP1) {
    int len = 32 * 1024;
    char *buf = (char *)malloc(len);
    if (buf) {
      if (!PrepareThumbnail(input, 0, buf, &len)) {
        memset(userdata, 0, MPP_ENCODER_USERDATA_MAX_SIZE);
        userdata_len =
            PackageApp1(stIfd0, stIfd1, sizeof(stIfd0) / sizeof(IFD_S),
                        sizeof(stIfd1) / sizeof(IFD_S), userdata, buf, len);
        userdata_all_frame_en = 1;
        RKMEDIA_LOGD("app1 len = %d, %d", userdata_len, len);
      }
      free(buf);
    }
  }

  if (thumbnail_type[1] == THUMBNAIL_TYPE_APP2) {
    mpf_len[1] = thumbnail_width[1] * thumbnail_height[1] * 2;
    mpf[1] = (char *)malloc(mpf_len[1]);
    if (mpf[1] && (!PrepareThumbnail(input, 1, mpf[1], &mpf_len[1])))
      image_cnt++;
  }

#ifdef RGA_OSD_ENABLE
  if (rga_osd_cnt > 0)
    RgaOsdRegionProcess(hw_buffer);
#endif

  if ((thumbnail_type[2] == THUMBNAIL_TYPE_APP2) && (image_cnt == 1)) {
    mpf_len[2] = thumbnail_width[2] * thumbnail_height[2] * 2;
    mpf[2] = (char *)malloc(mpf_len[2]);
    if (mpf[2] && (!PrepareThumbnail(input, 2, mpf[2], &mpf_len[2])))
      image_cnt++;
  }
  if (image_cnt) {
    image_cnt++;
    app2_len = PackageApp2(stMpfd, sizeof(stMpfd) / sizeof(IFD_S), image_cnt,
                           app2, mpf_len);
  } else {
    app2_len = 0;
  }

  ret = PrepareMppFrame(input, mdinfo, frame);
  if (ret) {
    RKMEDIA_LOGI("PrepareMppFrame failed\n");
    goto ENCODE_OUT;
  }

  if (output->IsValid()) {
    ret = PrepareMppPacket(output, packet);
    if (ret) {
      RKMEDIA_LOGI("PrepareMppPacket failed\n");
      goto ENCODE_OUT;
    }
    import_packet = packet;
  }

  ret = PrepareMppExtraBuffer(extra_output, mv_buf);
  if (ret) {
    RKMEDIA_LOGI("PrepareMppExtraBuffer failed\n");
    goto ENCODE_OUT;
  }

  ret = Process(frame, packet, mv_buf);
  if (ret)
    goto ENCODE_OUT;

  if (!packet) {
    RKMEDIA_LOGE("MPP Encoder: input frame:%p, %zuBytes; output null packet!\n",
                 frame, mpp_buffer_get_size(mpp_frame_get_buffer(frame)));
    goto ENCODE_OUT;
  }

  packet_len = mpp_packet_get_length(packet);
  packet_size = mpp_packet_get_size(packet);
  if (image_cnt) {
    char *ptr = (char *)mpp_packet_get_data(packet);
    if (image_cnt > 1) {
      if (mpf_len[1] > (int)(packet_size - packet_len)) {
        RKMEDIA_LOGE("MPP Encoder: mpf[1] = %d > %d - %d\n", mpf_len[1],
                     packet_size, packet_len);
        mpf_len[1] = packet_size - packet_len;
      }
      if (ptr)
        memcpy(ptr + packet_len, mpf[1], mpf_len[1]);
    }

    if (image_cnt > 2) {
      if (mpf_len[1] > (int)(packet_size - packet_len - mpf_len[1])) {
        RKMEDIA_LOGE("MPP Encoder: mpf[2] = %d > %d - %d - %d\n", mpf_len[1],
                     packet_size, packet_len, mpf_len[1]);
        mpf_len[2] = packet_size - packet_len - mpf_len[1];
      }
      if (ptr)
        memcpy(ptr + packet_len + mpf_len[1], mpf[2], mpf_len[2]);
    }
    mpf_len[0] = packet_len - (0x14 + userdata_len);
    if (ptr)
      PackageApp2(stMpfd, 5, image_cnt, ptr + 0x14 + userdata_len, mpf_len);
    packet_len += mpf_len[1];
    packet_len += mpf_len[2];
  }

  {
    MppMeta packet_meta = mpp_packet_get_meta(packet);
    RK_S32 is_intra = 0;
    mpp_meta_get_s32(packet_meta, KEY_OUTPUT_INTRA, &is_intra);
    packet_flag = (is_intra) ? MediaBuffer::kIntra : MediaBuffer::kPredicted;
    mpp_meta_get_s32(packet_meta, KEY_TEMPORAL_ID, &temporal_id);
  }
  out_eof = mpp_packet_get_eos(packet);
  pts = mpp_packet_get_pts(packet);
  if (pts <= 0)
    pts = mpp_packet_get_dts(packet);

  // out fps < in fps ?
  if (packet_len == 0) {
    output->SetValidSize(0);
    if (extra_output)
      extra_output->SetValidSize(0);
    goto ENCODE_OUT;
  }

  // Calculate bit rate statistics.
  if (encoder_sta_en) {
    MediaConfig &cfg = GetConfig();
    int target_fps = cfg.vid_cfg.frame_rate;
    int target_bpsmax = cfg.vid_cfg.bit_rate_max;
    int enable_bps = 1;
    frame_cnt_1s += 1;
    stream_size_1s += packet_len;
    if (target_fps <= 0) {
      target_fps = 30;
      enable_bps = 0;
    }
    // Refresh every second
    if ((frame_cnt_1s % target_fps) == 0) {
      // Calculate the frame rate based on the system time.
      cur_ts = gettimeofday();
      if (last_ts)
        encoded_fps = ((float)target_fps / (cur_ts - last_ts)) * 1000000;
      else
        encoded_fps = 0;

      last_ts = cur_ts;
      if (enable_bps) {
        // convert bytes to bits
        encoded_bps = stream_size_1s * 8;
        RKMEDIA_LOGI(
            "MPP ENCODER: bps:%d, actual_bps:%d, fps:%d, actual_fps:%f\n",
            target_bpsmax, encoded_bps, target_fps, encoded_fps);
      } else {
        RKMEDIA_LOGI("MPP ENCODER: fps statistical period:%d, actual_fps:%f\n",
                     target_fps, encoded_fps);
      }

      // reset 1s variable
      stream_size_1s = 0;
      frame_cnt_1s = 0;
    }
  } else if (cur_ts) {
    // clear tmp statistics variable.
    stream_size_1s = 0;
    frame_cnt_1s = 0;
    cur_ts = 0;
    last_ts = 0;
  }

  if (output->IsValid()) {
    if (!import_packet) {
      // !!time-consuming operation
      void *ptr = output->GetPtr();
      assert(ptr);
      RKMEDIA_LOGD("extra time-consuming memcpy to cpu!\n");
      memcpy(ptr, mpp_packet_get_data(packet), packet_len);
      // sync to cpu?
    }
  } else {
    MPPPacketContext *ctx = new MPPPacketContext(mpp_ctx, packet);
    if (!ctx) {
      LOG_NO_MEMORY();
      ret = -ENOMEM;
      goto ENCODE_OUT;
    }
    output->SetFD(mpp_buffer_get_fd(mpp_packet_get_buffer(packet)));
    output->SetPtr(mpp_packet_get_data(packet));
    output->SetSize(mpp_packet_get_size(packet));
    output->SetUserData(ctx, __free_mpppacketcontext);
    packet = nullptr;
  }
  output->SetValidSize(packet_len);
  output->SetUserFlag(packet_flag | output_mb_flags);
  output->SetTsvcLevel(temporal_id);
  output->SetUSTimeStamp(pts);
  output->SetEOF(out_eof ? true : false);
  out_type = output->GetType();
  if (out_type == Type::Image) {
    auto out_img = std::static_pointer_cast<ImageBuffer>(output);
    auto &info = out_img->GetImageInfo();
    const auto &in_cfg = GetConfig();
    info = (coding_type == MPP_VIDEO_CodingMJPEG)
               ? in_cfg.img_cfg.image_info
               : in_cfg.vid_cfg.image_cfg.image_info;
    // info.pix_fmt = codec_type;
  } else {
    output->SetType(Type::Video);
  }

  if (mv_buf) {
    if (extra_output->GetFD() < 0) {
      void *ptr = extra_output->GetPtr();
      assert(ptr);
      memcpy(ptr, mpp_buffer_get_ptr(mv_buf), mpp_buffer_get_size(mv_buf));
    }
    extra_output->SetValidSize(mpp_buffer_get_size(mv_buf));
    extra_output->SetUserFlag(packet_flag);
    extra_output->SetUSTimeStamp(pts);
  }

ENCODE_OUT:
  if (frame)
    mpp_frame_deinit(&frame);
  if (packet)
    mpp_packet_deinit(&packet);
  if (mv_buf)
    mpp_buffer_put(mv_buf);
  if (mpf[1])
    free(mpf[1]);
  if (mpf[2])
    free(mpf[2]);
  return ret;
}

int MPPEncoder::Process(MppFrame frame, MppPacket &packet, MppBuffer &mv_buf) {
  MppCtx ctx = mpp_ctx->ctx;
  MppApi *mpi = mpp_ctx->mpi;

  if (mv_buf)
    RKMEDIA_LOGI("TODO move detection frome mpp encoder...\n");

  int ret = mpi->encode_put_frame(ctx, frame);
  if (ret) {
    RKMEDIA_LOGI("mpp encode put frame failed\n");
    return -1;
  }

  ret = mpi->encode_get_packet(ctx, &packet);
  if (ret) {
    RKMEDIA_LOGI("mpp encode get packet failed\n");
    return -1;
  }

  return 0;
}

int MPPEncoder::SendInput(const std::shared_ptr<MediaBuffer> &) {
  errno = ENOSYS;
  return -1;
}
std::shared_ptr<MediaBuffer> MPPEncoder::FetchOutput() {
  errno = ENOSYS;
  return nullptr;
}

int MPPEncoder::EncodeControl(int cmd, void *param) {
  MpiCmd mpi_cmd = (MpiCmd)cmd;
  int ret = mpp_ctx->mpi->control(mpp_ctx->ctx, mpi_cmd, (MppParam)param);

  if (ret) {
    RKMEDIA_LOGI("mpp control cmd 0x%08x param %p failed\n", cmd, param);
    return ret;
  }

  return 0;
}

void MPPEncoder::QueryChange(uint32_t change, void *value, int32_t size) {
  if (!value || !size) {
    RKMEDIA_LOGE("MPP ENCODER: %s invalid argument!\n", __func__);
    return;
  }
  switch (change) {
  case VideoEncoder::kMoveDetectionFlow:
    if (size < (int)sizeof(int32_t)) {
      RKMEDIA_LOGE("MPP ENCODER: %s change:[%d], size invalid!\n", __func__,
                   VideoEncoder::kMoveDetectionFlow);
      return;
    }
    if (rc_api_brief_name == "smart")
      *((int32_t *)value) = 1;
    else
      *((int32_t *)value) = 0;
    break;
  default:
    RKMEDIA_LOGW("MPP ENCODER: %s change:[%d] not support!\n", __func__,
                 change);
  }
}

void MPPEncoder::set_statistics_switch(bool value) {
  RKMEDIA_LOGI("[INFO] MPP ENCODER %s statistics\n",
               value ? "enable" : "disable");
  encoder_sta_en = value;
}

int MPPEncoder::get_statistics_bps() {
  if (!encoder_sta_en)
    RKMEDIA_LOGI("[WARN] MPP ENCODER statistics should enable first!\n");
  return encoded_bps;
}

int MPPEncoder::get_statistics_fps() {
  if (!encoder_sta_en)
    RKMEDIA_LOGI("[WARN] MPP ENCODER statistics should enable first!\n");
  return encoded_fps;
}

#ifdef MPP_SUPPORT_HW_OSD

#define OSD_PTL_SIZE 1024 // Bytes.

#ifndef NDEBUG
static void OsdDummpRegions(OsdRegionData *rdata) {
  if (!rdata)
    return;

  RKMEDIA_LOGD("#RegionData:%p:\n", rdata->buffer);
  RKMEDIA_LOGI("\t enable:%u\n", rdata->enable);
  RKMEDIA_LOGI("\t region_id:%u\n", rdata->region_id);
  RKMEDIA_LOGI("\t inverse:%u\n", rdata->inverse);
  RKMEDIA_LOGI("\t pos_x:%u\n", rdata->pos_x);
  RKMEDIA_LOGI("\t pos_y:%u\n", rdata->pos_y);
  RKMEDIA_LOGI("\t width:%u\n", rdata->width);
  RKMEDIA_LOGI("\t height:%u\n", rdata->height);
  RKMEDIA_LOGI("\t region_type:%u\n", rdata->region_type);
  RKMEDIA_LOGI("\t cover_color:%u\n", rdata->cover_color);
}

static void OsdDummpMppOsd(MppEncOSDData *osd) {
  RKMEDIA_LOGD("#MPP OsdData: cnt:%d buff:%p, bufSize:%zu\n", osd->num_region,
               osd->buf, mpp_buffer_get_size(osd->buf));
  for (int i = 0; i < OSD_REGIONS_CNT; i++) {
    RKMEDIA_LOGD("#MPP OsdData[%d]:\n", i);
    RKMEDIA_LOGI("\t enable:%u\n", osd->region[i].enable);
    RKMEDIA_LOGI("\t inverse:%u\n", osd->region[i].inverse);
    RKMEDIA_LOGI("\t pos_x:%u\n", osd->region[i].start_mb_x * 16);
    RKMEDIA_LOGI("\t pos_y:%u\n", osd->region[i].start_mb_y * 16);
    RKMEDIA_LOGI("\t width:%u\n", osd->region[i].num_mb_x * 16);
    RKMEDIA_LOGI("\t height:%u\n", osd->region[i].num_mb_y * 16);
    RKMEDIA_LOGI("\t buf_offset:%u\n", osd->region[i].buf_offset);
  }
}

static void SaveOsdImg(MppEncOSDData *_data, int index) {
  if (!_data->buf)
    return;

  char _path[64] = {0};
  sprintf(_path, "/tmp/osd_img%d", index);
  RKMEDIA_LOGD("MPP Encoder: save osd img to %s\n", _path);
  int fd = open(_path, O_WRONLY | O_CREAT);
  if (fd <= 0)
    return;

  int size = _data->region[index].num_mb_x * 16;
  size *= _data->region[index].num_mb_y * 16;
  uint8_t *ptr = (uint8_t *)mpp_buffer_get_ptr(_data->buf);
  ptr += _data->region[index].buf_offset;
  if (ptr && size)
    write(fd, ptr, size);
  close(fd);
}
#endif // NDEBUG

int MPPEncoder::OsdPaletteSet(uint32_t *ptl_data) {
  if (!ptl_data)
    return -1;

  RKMEDIA_LOGD("MPP Encoder: setting yuva palette...\n");
  MppCtx ctx = mpp_ctx->ctx;
  MppApi *mpi = mpp_ctx->mpi;
  MppEncOSDPltCfg osd_plt_cfg;
  MppEncOSDPlt osd_plt;

  // TODO rgba plt to yuva plt.
  for (int k = 0; k < 256; k++)
    osd_plt.data[k].val = *(ptl_data + k);

  osd_plt_cfg.change = MPP_ENC_OSD_PLT_CFG_CHANGE_ALL;
  osd_plt_cfg.type = MPP_ENC_OSD_PLT_TYPE_USERDEF;
  osd_plt_cfg.plt = &osd_plt;

  int ret = mpi->control(ctx, MPP_ENC_SET_OSD_PLT_CFG, &osd_plt_cfg);
  if (ret)
    RKMEDIA_LOGE("MPP Encoder: set osd plt failed ret %d\n", ret);

  return ret;
}

static int OsdUpdateRegionInfo2(MppEncOSDData2 *osd, OsdRegionData *region_data,
                                MppBufferGroup buf_grp) {
  uint32_t new_size = 0;
  uint32_t old_size = 0;
  uint8_t rid = region_data->region_id;
  uint8_t *region_src = NULL;
  uint8_t *region_dst = NULL;
  if (!region_data->enable) {
    osd->region[rid].enable = 0;
    osd->num_region = 0;
    for (int i = 0; i < OSD_REGIONS_CNT; i++) {
      if (osd->region[i].enable) {
        osd->num_region = i + 1;
      }
    }
    assert(osd->num_region <= 8);
    return 0;
  }

  // get buffer size to compare.
  new_size = region_data->width * region_data->height;
  // If there is enough space, reuse the previous buffer.
  // However, the current area must be active, so as to
  // avoid opening up too large a buffer at the beginning,
  // and it will not be reduced later.
  if (osd->region[rid].enable)
    old_size = osd->region[rid].num_mb_x * osd->region[rid].num_mb_y * 256;

  // update region info.
  osd->region[rid].enable = 1;
  osd->region[rid].inverse = region_data->inverse;
  osd->region[rid].start_mb_x = region_data->pos_x / 16;
  osd->region[rid].start_mb_y = region_data->pos_y / 16;
  osd->region[rid].num_mb_x = region_data->width / 16;
  osd->region[rid].num_mb_y = region_data->height / 16;
  osd->num_region = 0;
  for (int i = 0; i < OSD_REGIONS_CNT; i++) {
    if (osd->region[i].enable) {
      osd->num_region = i + 1;
    }
  }
  assert(osd->num_region <= 8);

  // 256 * 16 => 4096 is enough for osd.
  assert(osd->region[rid].start_mb_x <= 256);
  assert(osd->region[rid].start_mb_y <= 256);
  assert(osd->region[rid].num_mb_x <= 256);
  assert(osd->region[rid].num_mb_y <= 256);

  // region[rid] buffer size is enough, copy data directly.
  if (old_size >= new_size) {
    RKMEDIA_LOGD("MPP Encoder: Region[%d] reuse old buff:%u, new_size:%u\n",
                 rid, old_size, new_size);
    region_src = region_data->buffer;
    region_dst = (uint8_t *)mpp_buffer_get_ptr(osd->region[rid].buf);
    memcpy(region_dst, region_src, new_size);
  } else {
    // region[rid] buffer size too small, resize buffer.
    MppBuffer new_buff = NULL;
    MppBuffer old_buff = NULL;
    old_buff = osd->region[rid].buf;
    int ret = mpp_buffer_get(buf_grp, &new_buff, new_size);
    if (ret) {
      RKMEDIA_LOGE("MPP Encoder: get osd %dBytes buffer failed(%d)\n", new_size,
                   ret);
      // reset target region.
      osd->region[rid].enable = 0;
      osd->region[rid].start_mb_x = 0;
      osd->region[rid].start_mb_y = 0;
      osd->region[rid].buf_offset = 0;
      return -1;
    }
    region_src = region_data->buffer;
    region_dst = (uint8_t *)mpp_buffer_get_ptr(new_buff);
    memcpy(region_dst, region_src, new_size);
    osd->region[rid].buf = new_buff;
    if (old_buff) {
      mpp_buffer_put(old_buff);
      old_buff = NULL;
    }
  }
  return 0;
}

static int OsdUpdateRegionInfo(MppEncOSDData *osd, OsdRegionData *region_data,
                               MppBufferGroup buf_grp) {
  uint32_t new_size = 0;
  uint32_t old_size = 0;
  uint8_t rid = region_data->region_id;
  uint8_t *region_src = NULL;
  uint8_t *region_dst = NULL;

  if (!region_data->enable) {
    osd->region[rid].enable = 0;
    osd->num_region = 0;
    for (int i = 0; i < OSD_REGIONS_CNT; i++) {
      if (osd->region[i].enable)
        osd->num_region = i + 1;
    }
    assert(osd->num_region <= 8);
    return 0;
  }

  // get buffer size to compare.
  new_size = region_data->width * region_data->height;
  // If there is enough space, reuse the previous buffer.
  // However, the current area must be active, so as to
  // avoid opening up too large a buffer at the beginning,
  // and it will not be reduced later.
  if (osd->region[rid].enable)
    old_size = osd->region[rid].num_mb_x * osd->region[rid].num_mb_y * 256;

  // update region info.
  osd->region[rid].enable = 1;
  osd->region[rid].inverse = region_data->inverse;
  osd->region[rid].start_mb_x = region_data->pos_x / 16;
  osd->region[rid].start_mb_y = region_data->pos_y / 16;
  osd->region[rid].num_mb_x = region_data->width / 16;
  osd->region[rid].num_mb_y = region_data->height / 16;

  // 256 * 16 => 4096 is enough for osd.
  assert(osd->region[rid].start_mb_x <= 256);
  assert(osd->region[rid].start_mb_y <= 256);
  assert(osd->region[rid].num_mb_x <= 256);
  assert(osd->region[rid].num_mb_y <= 256);

  // region[rid] buffer size is enough, copy data directly.
  if (old_size >= new_size) {
    RKMEDIA_LOGD("MPP Encoder: Region[%d] reuse old buff:%u, new_size:%u\n",
                 rid, old_size, new_size);
    region_src = region_data->buffer;
    region_dst = (uint8_t *)mpp_buffer_get_ptr(osd->buf);
    region_dst += osd->region[rid].buf_offset;
    memcpy(region_dst, region_src, new_size);
#ifndef NDEBUG
    SaveOsdImg(osd, rid);
#endif
    return 0;
  }

  // region[rid] buffer size too small, resize buffer.
  MppBuffer new_buff = NULL;
  MppBuffer old_buff = NULL;
  uint32_t old_offset[OSD_REGIONS_CNT] = {0};
  uint32_t total_size = 0;
  uint32_t current_size = 0;

  osd->num_region = 0;
  for (int i = 0; i < OSD_REGIONS_CNT; i++) {
    if (osd->region[i].enable) {
      old_offset[i] = osd->region[i].buf_offset;
      osd->region[i].buf_offset = total_size;
      total_size += osd->region[i].num_mb_x * osd->region[i].num_mb_y * 256;
      osd->num_region = i + 1;
    } else {
      osd->region[i].start_mb_x = 0;
      osd->region[i].start_mb_y = 0;
      osd->region[i].buf_offset = 0;
      osd->region[i].num_mb_x = 0;
      osd->region[i].num_mb_y = 0;
    }
  }

  old_buff = osd->buf;
  int ret = mpp_buffer_get(buf_grp, &new_buff, total_size);
  if (ret) {
    RKMEDIA_LOGE("MPP Encoder: get osd %dBytes buffer failed(%d)\n", total_size,
                 ret);
    // reset target region.
    osd->region[rid].enable = 0;
    osd->region[rid].start_mb_x = 0;
    osd->region[rid].start_mb_y = 0;
    osd->region[rid].buf_offset = 0;
    return -1;
  }

  for (int i = 0; i < OSD_REGIONS_CNT; i++) {
    if (!osd->region[i].enable)
      continue;

    if (i != rid) {
      // copy other region data to new buffer.
      region_src = (uint8_t *)mpp_buffer_get_ptr(old_buff);
      region_src += old_offset[i];
      region_dst = (uint8_t *)mpp_buffer_get_ptr(new_buff);
      region_dst += osd->region[i].buf_offset;
      current_size = osd->region[i].num_mb_x * osd->region[i].num_mb_y * 256;
    } else {
      // copy current region data to new buffer.
      region_src = region_data->buffer;
      region_dst = (uint8_t *)mpp_buffer_get_ptr(new_buff);
      region_dst += osd->region[i].buf_offset;
      current_size = new_size;
    }

    assert(region_src);
    assert(region_dst);
    memcpy(region_dst, region_src, current_size);
#ifndef NDEBUG
    SaveOsdImg(osd, i);
#endif
  }

  // replace old buff with new buff.
  osd->buf = new_buff;
  if (old_buff)
    mpp_buffer_put(old_buff);

  return 0;
}

int MPPEncoder::OsdRegionSet(OsdRegionData *rdata) {
  if (!rdata)
    return -EINVAL;

  RKMEDIA_LOGD("MPP Encoder: setting osd regions...\n");
  if ((rdata->region_id >= OSD_REGIONS_CNT)) {
    RKMEDIA_LOGE("MPP Encoder: invalid region id(%d), should be [0, %d).\n",
                 rdata->region_id, OSD_REGIONS_CNT);
    return -EINVAL;
  }

  if (rdata->enable && !rdata->buffer) {
    RKMEDIA_LOGE("MPP Encoder: invalid region data");
    return -EINVAL;
  }

  if ((rdata->width % 16) || (rdata->height % 16) || (rdata->pos_x % 16) ||
      (rdata->pos_y % 16)) {
    RKMEDIA_LOGW("MPP Encoder: osd size must be 16 aligned\n");
    rdata->width = UPALIGNTO16(rdata->width);
    rdata->height = UPALIGNTO16(rdata->height);
    rdata->pos_x = UPALIGNTO16(rdata->pos_x);
    rdata->pos_y = UPALIGNTO16(rdata->pos_y);
  }
#ifndef NDEBUG
  OsdDummpRegions(rdata);
#endif
  int ret;
  if (1) {
    ret = OsdUpdateRegionInfo2(&osd_data2, rdata, osd_buf_grp);
  } else {
    ret = OsdUpdateRegionInfo(&osd_data, rdata, osd_buf_grp);
  }

#ifndef NDEBUG
  OsdDummpMppOsd(&osd_data);
#endif

  return ret;
}

int MPPEncoder::OsdRegionGet(OsdRegionData *rdata) {
  RKMEDIA_LOGI("ToDo...%p\n", rdata);
  return 0;
}
#endif // MPP_SUPPORT_HW_OSD

#ifdef RGA_OSD_ENABLE
#if 0
static void OsdDummpRgaRegions(OsdRegionData *rdata) {
  if (!rdata)
    return;

  RKMEDIA_LOGD("#RegionData:%p:\n", rdata->buffer);
  RKMEDIA_LOGI("\t enable:%u\n", rdata->enable);
  RKMEDIA_LOGI("\t region_id:%u\n", rdata->region_id);
  RKMEDIA_LOGI("\t inverse:%u\n", rdata->inverse);
  RKMEDIA_LOGI("\t pos_x:%u\n", rdata->pos_x);
  RKMEDIA_LOGI("\t pos_y:%u\n", rdata->pos_y);
  RKMEDIA_LOGI("\t width:%u\n", rdata->width);
  RKMEDIA_LOGI("\t height:%u\n", rdata->height);
}

static void OsdDummpRgaOsd(RgaOsdData *osd, int valid_cnt) {
  RKMEDIA_LOGD("#OSD-EX OsdData: cnt:%d\n", valid_cnt);
  for (int i = 0; i < OSD_REGIONS_CNT; i++) {
    printf("#OSD-EX OsdData[%d]:\n", i);
    printf("\t enable:%u\n", osd[i].enable);
    printf("\t inverse:%u\n", osd[i].inverse);
    printf("\t pos_x:%u\n", osd[i].pos_x);
    printf("\t pos_y:%u\n", osd[i].pos_y);
    printf("\t width:%u\n", osd[i].width);
    printf("\t height:%u\n", osd[i].height);
    if (osd[i].data) {
      printf("\t buf ptr:%p\n", osd[i].data->GetPtr());
      printf("\t buf size:%zuBytes\n", osd[i].data->GetValidSize());
    }
  }
}

static void OsdExSaveImg(RgaOsdData *osdRgn, int index) {
  if (!osdRgn || !osdRgn->data)
    return;

  char _path[64] = {0};
  sprintf(_path, "/tmp/osd_img%d", index);
  RKMEDIA_LOGD("MPP Encoder: save osd img to %s\n", _path);
  int fd = open(_path, O_WRONLY | O_CREAT);
  if (fd <= 0)
    return;

  int size = osdRgn->data->GetValidSize();
  uint8_t *ptr = (uint8_t *)osdRgn->data->GetPtr();
  if (ptr && size)
    write(fd, ptr, size);
  close(fd);
}
#endif // #ifndef NDEBUG

int MPPEncoder::RgaOsdRegionSet(OsdRegionData *rdata, VideoConfig &vfg) {
  if (!rdata)
    return -EINVAL;

  RKMEDIA_LOGD("MPP Encoder[OSD EX]: setting osd regions...\n");
  if ((rdata->region_id >= OSD_REGIONS_CNT)) {
    RKMEDIA_LOGE(
        "MPP Encoder[OSD EX]: invalid region id(%d), should be [0, %d).\n",
        rdata->region_id, OSD_REGIONS_CNT);
    return -EINVAL;
  }

  if (rdata->enable && rdata->region_type == REGION_TYPE_OVERLAY_EX &&
      !rdata->buffer) {
    RKMEDIA_LOGE("MPP Encoder[OSD EX]: invalid region data");
    return -EINVAL;
  }

  int rid = rdata->region_id;
  if (!rdata->enable) {
    if (rga_osd_data[rid].enable)
      rga_osd_cnt--;
    if (rga_osd_cnt < 0) {
      RKMEDIA_LOGW("MPP Encoder[OSD EX]: osd cnt incorrect!\n");
      rga_osd_cnt = 0;
    }
    rga_osd_data[rid].enable = 0;
    rga_osd_data[rid].data.reset();
    return 0;
  }

  int img_width = vfg.image_cfg.image_info.width;
  int img_height = vfg.image_cfg.image_info.height;

  if (rdata->region_type == REGION_TYPE_OVERLAY_EX) {
    int total_size = rdata->width * rdata->height * 4; // rgab8888
    rga_osd_data[rid].data =
        MediaBuffer::Alloc(total_size, MediaBuffer::MemType::MEM_HARD_WARE);
    if (!rga_osd_data[rid].data) {
      RKMEDIA_LOGW("MPP Encoder[OSD EX]: no space left!\n");
      return -ENOMEM;
    }

    if (vfg.rotation == 0) {
      rga_osd_data[rid].data->BeginCPUAccess(true);
      memcpy(rga_osd_data[rid].data->GetPtr(), rdata->buffer, total_size);
      rga_osd_data[rid].data->EndCPUAccess(true);
      rga_osd_data[rid].data->SetValidSize(total_size);
    } else {
      IM_STATUS STATUS;
      IM_USAGE usage;
      rga_buffer_t src, dst;
      int src_w, src_h;
      int dst_w, dst_h;
      memset(&src, 0, sizeof(src));
      memset(&dst, 0, sizeof(dst));
      src_w = rdata->width;
      src_h = rdata->height;
      switch (vfg.rotation) {
      case 90:
        dst_w = src_h;
        dst_h = src_w;
        usage = IM_HAL_TRANSFORM_ROT_270;
        break;
      case 180:
        dst_w = src_w;
        dst_h = src_h;
        usage = IM_HAL_TRANSFORM_ROT_180;
        break;
      case 270:
        dst_w = src_h;
        dst_h = src_w;
        usage = IM_HAL_TRANSFORM_ROT_90;
        break;
      default:
        return -EINVAL;
      }
      src = wrapbuffer_virtualaddr(rdata->buffer, src_w, src_h,
                                   RK_FORMAT_BGRA_8888);
      dst = wrapbuffer_fd(rga_osd_data[rid].data->GetFD(), dst_w, dst_h,
                          RK_FORMAT_BGRA_8888);
      STATUS = imrotate(src, dst, usage);
      if (STATUS != IM_STATUS_SUCCESS) {
        RKMEDIA_LOGE("MPP Encoder[OSD EX]: osd rotate failed!\n");
        return -EBUSY;
      }
    }
  }
  if (!rga_osd_data[rid].enable)
    rga_osd_cnt++;
  if (rga_osd_cnt > OSD_REGIONS_CNT) {
    RKMEDIA_LOGW("MPP Encoder[OSD EX]: osd cnt > OSD_REGIONS_CNT\n");
    rga_osd_cnt = OSD_REGIONS_CNT;
  }
  switch (vfg.rotation) {
  case 0:
    rga_osd_data[rid].pos_x = rdata->pos_x;
    rga_osd_data[rid].pos_y = rdata->pos_y;
    rga_osd_data[rid].width = rdata->width;
    rga_osd_data[rid].height = rdata->height;
    break;
  case 90:
    rga_osd_data[rid].pos_x = rdata->pos_y;
    rga_osd_data[rid].pos_y = img_height - rdata->width - rdata->pos_x;
    rga_osd_data[rid].width = rdata->height;
    rga_osd_data[rid].height = rdata->width;
    break;
  case 180:
    rga_osd_data[rid].pos_x = img_width - rdata->width - rdata->pos_x;
    rga_osd_data[rid].pos_y = img_height - rdata->height - rdata->pos_y;
    rga_osd_data[rid].width = rdata->width;
    rga_osd_data[rid].height = rdata->height;
    break;
  case 270:
    rga_osd_data[rid].pos_x = img_width - rdata->height - rdata->pos_y;
    rga_osd_data[rid].pos_y = rdata->pos_x;
    rga_osd_data[rid].width = rdata->height;
    rga_osd_data[rid].height = rdata->width;
    break;
  default:
    return -EINVAL;
  }
  rga_osd_data[rid].enable = rdata->enable;
  rga_osd_data[rid].inverse = rdata->inverse;
  rga_osd_data[rid].region_type = rdata->region_type;
  rga_osd_data[rid].cover_color = rdata->cover_color;

  return 0;
}

static int ARGBTOABGR(int color) {
  uint8_t transBlue = (uint8_t)(color >> 16);
  color &= 0xFF00FFFF;
  color |= (color << 16 & 0x00FF0000);
  color &= 0xFFFFFF00;
  color |= (transBlue & 0x000000FF);
  return color;
}

static int get_rga_format(PixelFormat f) {
  static std::map<PixelFormat, int> rga_format_map = {
      {PIX_FMT_YUV420P, RK_FORMAT_YCbCr_420_P},
      {PIX_FMT_NV12, RK_FORMAT_YCbCr_420_SP},
      {PIX_FMT_NV21, RK_FORMAT_YCrCb_420_SP},
      {PIX_FMT_YUV422P, RK_FORMAT_YCbCr_422_P},
      {PIX_FMT_NV16, RK_FORMAT_YCbCr_422_SP},
      {PIX_FMT_NV61, RK_FORMAT_YCrCb_422_SP},
      {PIX_FMT_YUYV422, RK_FORMAT_YUYV_422},
      {PIX_FMT_UYVY422, RK_FORMAT_UYVY_422},
      {PIX_FMT_RGB565, RK_FORMAT_RGB_565},
      {PIX_FMT_BGR565, -1},
      {PIX_FMT_RGB888, RK_FORMAT_BGR_888},
      {PIX_FMT_BGR888, RK_FORMAT_RGB_888},
      {PIX_FMT_ARGB8888, RK_FORMAT_BGRA_8888},
      {PIX_FMT_ABGR8888, RK_FORMAT_RGBA_8888}};
  auto it = rga_format_map.find(f);
  if (it != rga_format_map.end())
    return it->second;
  return -1;
}

int MPPEncoder::RgaOsdRegionProcess(ImageBuffer *hw_buffer) {
  rga_buffer_t pat;
  rga_buffer_t src;
  IM_STATUS STATUS;

  if (!hw_buffer)
    return 0;

  ImageInfo imgInfo = hw_buffer->GetImageInfo();
  // ToDo: fmt check and convert

  src = wrapbuffer_fd_t(hw_buffer->GetFD(), imgInfo.width, imgInfo.height,
                        imgInfo.vir_width, imgInfo.vir_height,
                        get_rga_format(hw_buffer->GetPixelFormat()));
  for (int i = 0; i < OSD_REGIONS_CNT; i++) {
    if (!rga_osd_data[i].enable)
      continue;
    im_rect src_rect = {(int)rga_osd_data[i].pos_x, (int)rga_osd_data[i].pos_y,
                        (int)rga_osd_data[i].width,
                        (int)rga_osd_data[i].height};

    if (rga_osd_data[i].region_type == REGION_TYPE_OVERLAY_EX) {
      pat = wrapbuffer_fd(rga_osd_data[i].data->GetFD(), rga_osd_data[i].width,
                          rga_osd_data[i].height, RK_FORMAT_BGRA_8888);
      im_rect pat_rect = {0, 0, (int)rga_osd_data[i].width,
                          (int)rga_osd_data[i].height};
      int usgae = IM_ALPHA_BLEND_DST_OVER | IM_ALPHA_BLEND_PRE_MUL;
      src.color_space_mode =
          IM_YUV_TO_RGB_BT601_FULL | IM_RGB_TO_YUV_BT601_FULL;
      STATUS = improcess(src, src, pat, src_rect, src_rect, pat_rect, usgae);
      if (STATUS != IM_STATUS_SUCCESS) {
        RKMEDIA_LOGE("MPP Encoder[OSD EX]: do overlay failed!\n");
        break;
      }
    } else if (rga_osd_data[i].region_type == REGION_TYPE_COVER_EX) {
      STATUS = imfill(src, src_rect, ARGBTOABGR(rga_osd_data[i].cover_color));
      if (STATUS != IM_STATUS_SUCCESS) {
        RKMEDIA_LOGE("MPP Encoder[OSD EX]: do cover failed!\n");
        break;
      }
    }
  }

  return 0;
}
#endif // RGA_OSD_ENABLE

int MPPEncoder::RoiUpdateRegions(EncROIRegion *regions, int region_cnt) {
  if (!regions || region_cnt == 0) {
    roi_cfg.number = 0;
    if (roi_cfg.regions) {
      free(roi_cfg.regions);
      roi_cfg.regions = NULL;
    }
    RKMEDIA_LOGI("MPP Encoder: disable roi function.\n");
    return 0;
  }

  int msize = region_cnt * sizeof(MppEncROIRegion);
  MppEncROIRegion *region = (MppEncROIRegion *)malloc(msize);
  if (!region) {
    LOG_NO_MEMORY();
    return -ENOMEM;
  }

  for (int i = 0; i < region_cnt; i++) {
    if ((regions[i].x % 16) || (regions[i].y % 16) || (regions[i].w % 16) ||
        (regions[i].h % 16)) {
      RKMEDIA_LOGW(
          "MPP Encoder: region parameter should be an integer multiple "
          "of 16\n");
      RKMEDIA_LOGW("MPP Encoder: reset region[%d] frome <%d,%d,%d,%d> to "
                   "<%d,%d,%d,%d>\n",
                   i, regions[i].x, regions[i].y, regions[i].w, regions[i].h,
                   UPALIGNTO16(regions[i].x), UPALIGNTO16(regions[i].y),
                   UPALIGNTO16(regions[i].w), UPALIGNTO16(regions[i].h));
      regions[i].x = UPALIGNTO16(regions[i].x);
      regions[i].y = UPALIGNTO16(regions[i].y);
      regions[i].w = UPALIGNTO16(regions[i].w);
      regions[i].h = UPALIGNTO16(regions[i].h);
    }
    RKMEDIA_LOGD("MPP Encoder: roi region[%d]:<%d,%d,%d,%d>\n", i, regions[i].x,
                 regions[i].y, regions[i].w, regions[i].h);
    RKMEDIA_LOGD("MPP Encoder: roi region[%d].intra=%d,\n", i,
                 regions[i].intra);
    RKMEDIA_LOGD("MPP Encoder: roi region[%d].quality=%d,\n", i,
                 regions[i].quality);
    RKMEDIA_LOGD("MPP Encoder: roi region[%d].abs_qp_en=%d,\n", i,
                 regions[i].abs_qp_en);
    RKMEDIA_LOGD("MPP Encoder: roi region[%d].qp_area_idx=%d,\n", i,
                 regions[i].qp_area_idx);
    RKMEDIA_LOGD("MPP Encoder: roi region[%d].area_map_en=%d,\n", i,
                 regions[i].area_map_en);
    assert(regions[i].x < 8192);
    assert(regions[i].y < 8192);
    assert(regions[i].w < 8192);
    assert(regions[i].h < 8192);
    assert(regions[i].x < 8192);
    assert(regions[i].intra <= 1);
    assert(regions[i].abs_qp_en <= 1);
    assert(regions[i].qp_area_idx <= 7);
    assert(regions[i].area_map_en <= 1);
    VALUE_SCOPE_CHECK(regions[i].quality, -48, 51);

    region[i].x = regions[i].x;
    region[i].y = regions[i].y;
    region[i].w = regions[i].w;
    region[i].h = regions[i].h;
    region[i].intra = regions[i].intra;
    region[i].quality = regions[i].quality;
    region[i].abs_qp_en = regions[i].abs_qp_en;
    region[i].qp_area_idx = regions[i].qp_area_idx;
    region[i].area_map_en = regions[i].area_map_en;
  }
  roi_cfg.number = region_cnt;
  if (roi_cfg.regions)
    free(roi_cfg.regions);
  roi_cfg.regions = region;
  return 0;
}

int MPPEncoder::SetUserData(const char *data, uint16_t len) {
  uint32_t valid_size = len;

  if (!data && len) {
    RKMEDIA_LOGE("Mpp Encoder: invalid userdata!\n");
    return -1;
  }

  if (valid_size > MPP_ENCODER_USERDATA_MAX_SIZE) {
    valid_size = MPP_ENCODER_USERDATA_MAX_SIZE;
    RKMEDIA_LOGW("Mpp Encoder: UserData exceeds maximum length(%d),"
                 "Reset to %d\n",
                 valid_size, valid_size);
  }

  if (valid_size)
    memcpy(userdata, data, valid_size);

  userdata_len = valid_size;
  return 0;
}

void MPPEncoder::ClearUserData() { userdata_len = 0; }

void MPPEncoder::RestartUserData() { userdata_frame_id = 0; }

void MPPEncoder::EnableUserDataAllFrame(bool value) {
  userdata_all_frame_en = value ? 1 : 0;
}

} // namespace easymedia
