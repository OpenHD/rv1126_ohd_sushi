// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <inttypes.h>
#include <sys/time.h>

#include "buffer.h"
#include "codec.h"
#include "flow.h"
#include "muxer.h"
#include "muxer_flow.h"
#include "utils.h"

#include "fcntl.h"
#include "stdint.h"
#include "stdio.h"
#include "unistd.h"

#include <sstream>

namespace easymedia {

#if DEBUG_MUXER_OUTPUT_BUFFER
static unsigned sg_buffer_size = 0;
static int64_t sg_last_time = 0;
static unsigned sg_buffer_count = 0;
#endif

static int muxer_buffer_callback(void *handler, uint8_t *buf, int buf_size) {
  MuxerFlow *f = (MuxerFlow *)handler;
  auto media_buffer = MediaBuffer::Alloc(buf_size);
  memcpy(media_buffer->GetPtr(), buf, buf_size);
  f->GetInputSize();
  media_buffer->SetValidSize(buf_size);
  media_buffer->SetUSTimeStamp(easymedia::gettimeofday());
  f->SetOutput(media_buffer, 0);
#if DEBUG_MUXER_OUTPUT_BUFFER
  int64_t cur_time = easymedia::gettimeofday();
  sg_buffer_size += buf_size;
  sg_buffer_count++;
  if ((cur_time - sg_last_time) / 1000 > 1000) {
    RKMEDIA_LOGI(
        "MUXER:: one second output buffer size = %u, count = %u, last_size = "
        "%u, \n",
        sg_buffer_size, sg_buffer_count, buf_size);
    sg_buffer_size = 0;
    sg_last_time = cur_time;
    sg_buffer_count = 0;
  }
#endif
  return buf_size;
}

MuxerFlow::MuxerFlow(const char *param)
    : video_recorder(nullptr), video_in(false), audio_in(false),
      file_duration(-1), real_file_duration(-1), file_index(-1), last_ts(0),
      file_time_en(false), enable_streaming(true), request_stop_stream(false),
      file_name_cb(nullptr), file_name_handle(nullptr), muxer_id(0),
      manual_split(false), manual_split_record(false),
      manual_split_file_duration(0), is_first_file(true),
      pre_record_mode(MUXER_PRE_RECORD_NONE), pre_record_time(0),
      pre_record_cache_time(0), vid_buffer_size(0), aud_buffer_size(0),
      is_lapse_record(false), lapse_time_stamp(0) {
  std::list<std::string> separate_list;
  std::map<std::string, std::string> params;

  if (!ParseWrapFlowParams(param, params, separate_list)) {
    SetError(-EINVAL);
    return;
  }

  std::string &muxer_name = params[KEY_NAME];
  if (muxer_name.empty()) {
    RKMEDIA_LOGE("%s: missing muxer name\n", __func__);
    SetError(-EINVAL);
    return;
  }

  file_path = params[KEY_PATH];
  if (!file_path.empty()) {
    RKMEDIA_LOGI("Muxer will use internal path\n");
    is_use_customio = false;
  } else {
    is_use_customio = true;
    RKMEDIA_LOGI("Muxer:: file_path is null, will use CustomeIO.\n");
  }

  file_prefix = params[KEY_FILE_PREFIX];
  if (file_prefix.empty()) {
    RKMEDIA_LOGI("Muxer will use default prefix\n");
  }

  std::string time_str = params[KEY_FILE_TIME];
  if (!time_str.empty()) {
    file_time_en = !!std::stoi(time_str);
    RKMEDIA_LOGI("Muxer will record video end with time\n");
  }

  std::string index_str = params[KEY_FILE_INDEX];
  if (!index_str.empty()) {
    file_index = std::stoi(index_str);
    RKMEDIA_LOGI("Muxer will record video start with index %" PRId64 "\n",
                 file_index);
  }

  std::string &duration_str = params[KEY_FILE_DURATION];
  if (!duration_str.empty()) {
    file_duration = std::stoi(duration_str);
    RKMEDIA_LOGI("Muxer will save video file per %" PRId64 "sec\n",
                 file_duration);
  }

  std::string pre_record_mode_str = params[KEY_PRE_RECORD_MODE];
  if (!pre_record_mode_str.empty()) {
    pre_record_mode = (PRE_RECORD_MODE_E)std::stoi(pre_record_mode_str);
    RKMEDIA_LOGI("Muxer: pre_record_mode %d\n", pre_record_mode);
  }

  std::string &pre_record_time_str = params[KEY_PRE_RECORD_TIME];
  if (!pre_record_time_str.empty()) {
    pre_record_time = std::stoi(pre_record_time_str);
    RKMEDIA_LOGI("Muxer: pre-record time %d(s)\n", pre_record_time);
  }

  std::string &pre_record_cache_time_str = params[KEY_PRE_RECORD_CACHE_TIME];
  if (!pre_record_cache_time_str.empty()) {
    pre_record_cache_time = std::stoi(pre_record_cache_time_str);
    if (pre_record_cache_time < pre_record_time)
      pre_record_cache_time = pre_record_time;
    RKMEDIA_LOGI("Muxer: pre-record cache time %d(s)\n", pre_record_cache_time);
  }

  std::string &muxer_id_str = params[KEY_MUXER_ID];
  if (!muxer_id_str.empty()) {
    muxer_id = std::stoi(muxer_id_str);
    RKMEDIA_LOGI("Muxer: muxer_id %d\n", muxer_id);
  }

  std::string lapse_record_str = params[KEY_LAPSE_RECORD];
  if (!lapse_record_str.empty()) {
    is_lapse_record = !!std::stoi(lapse_record_str);
    RKMEDIA_LOGI("Muxer: is_lapse_record %d\n", is_lapse_record);
  }

  output_format = params[KEY_OUTPUTDATATYPE];
  if (output_format.empty() && is_use_customio) {
    RKMEDIA_LOGI("Muxer: output_data_type is null, no use customio.\n");
    is_use_customio = false;
  }

  std::string enable_streaming_s = params[KEY_ENABLE_STREAMING];
  if (!enable_streaming_s.empty()) {
    if (!enable_streaming_s.compare("false"))
      enable_streaming = false;
    else
      enable_streaming = true;
  }
  RKMEDIA_LOGI("Muxer:: enable_streaming is %d\n", enable_streaming);

  rkaudio_avdictionary = params[KEY_MUXER_RKAUDIO_AVDICTIONARY];

  for (auto param_str : separate_list) {
    MediaConfig enc_config;
    std::map<std::string, std::string> enc_params;
    if (!parse_media_param_map(param_str.c_str(), enc_params)) {
      continue;
    }

    if (!ParseMediaConfigFromMap(enc_params, enc_config)) {
      continue;
    }

    if (enc_config.type == Type::Video) {
      vid_enc_config = enc_config;
      video_in = true;
      RKMEDIA_LOGI("Found video encode config!\n");
    } else if (enc_config.type == Type::Audio) {
      aud_enc_config = enc_config;
      audio_in = true;
      RKMEDIA_LOGI("Found audio encode config!\n");
    }
  }

  std::string token;
  std::istringstream tokenStream(param);
  std::getline(tokenStream, token, FLOW_PARAM_SEPARATE_CHAR);
  muxer_param = token;

  SlotMap sm;
  sm.input_slots.push_back(0);
  sm.input_slots.push_back(1);
  if (is_use_customio)
    sm.output_slots.push_back(0);
  sm.thread_model = Model::ASYNCCOMMON;
  sm.mode_when_full = InputMode::BLOCKING;
  sm.input_maxcachenum.push_back(20);
  sm.input_maxcachenum.push_back(40);
  sm.fetch_block.push_back(false);
  sm.fetch_block.push_back(false);
  sm.process = save_buffer;

  if (!InstallSlotMap(sm, "MuxerFlow", 0)) {
    RKMEDIA_LOGI("Fail to InstallSlotMap for MuxerFlow\n");
    return;
  }
  SetFlowTag("MuxerFlow");
}

MuxerFlow::~MuxerFlow() {
  StopAllThread();
  video_recorder = nullptr;
  video_extra = nullptr;
  vid_cached_buffers.clear();
  aud_cached_buffers.clear();
}

std::shared_ptr<VideoRecorder> MuxerFlow::NewRecorder(const char *path) {
  std::string param = std::string(muxer_param);
  std::shared_ptr<VideoRecorder> vrecorder = nullptr;
  PARAM_STRING_APPEND(param, KEY_OUTPUTDATATYPE, output_format.c_str());
  PARAM_STRING_APPEND(param, KEY_PATH, path);
  PARAM_STRING_APPEND(param, KEY_MUXER_RKAUDIO_AVDICTIONARY,
                      rkaudio_avdictionary);

  if (is_use_customio) {
    vrecorder = std::make_shared<VideoRecorder>(param.c_str(), this, path, 1);
    RKMEDIA_LOGI("use customio, output foramt is %s.\n", output_format.c_str());
  } else {
    vrecorder = std::make_shared<VideoRecorder>(param.c_str(), this, path, 0);
  }

  if (!vrecorder) {
    RKMEDIA_LOGI("Create video recoder failed, path:[%s]\n", path);
    return nullptr;
  } else {
    RKMEDIA_LOGI("Ready to recod new video file path:[%s]\n", path);
  }

  return vrecorder;
}

std::string MuxerFlow::GenFilePath() {
  std::ostringstream ostr;

  if (file_name_cb) {
    char new_name[MUXER_FLOW_FILE_NAME_LEN] = {0};
    int ret = (*file_name_cb)(file_name_handle, new_name, muxer_id);
    if (!ret) {
      // callback sucess!
      return std::string(new_name);
    } else {
      RKMEDIA_LOGE("%s: file name callback error!\n", __func__);
    }
  }

  // if user special a file path then use it.
  if (!file_path.empty() && file_prefix.empty()) {
    return file_path;
  }

  if (!file_path.empty()) {
    ostr << file_path;
    ostr << "/";
  }

  if (!file_prefix.empty()) {
    ostr << file_prefix;
  }

  if (file_time_en) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char time_str[128] = {0};

    snprintf(time_str, 128, "_%d%02d%02d%02d%02d%02d", tm.tm_year + 1900,
             tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    ostr << time_str;
  }

  if (file_index > 0) {
    ostr << "_" << file_index;
    file_index++;
  }

  ostr << ".mp4";

  return ostr.str();
}

int MuxerFlow::GetVideoExtradata(std::shared_ptr<MediaBuffer> vid_buffer) {
  CodecType c_type = vid_enc_config.vid_cfg.image_cfg.codec_type;
  int extra_size = 0;
  void *extra_ptr = NULL;

  if (c_type == CODEC_TYPE_H264)
    extra_ptr = GetSpsPpsFromBuffer(vid_buffer, extra_size, c_type);
  else if (c_type == CODEC_TYPE_H265)
    extra_ptr = GetVpsSpsPpsFromBuffer(vid_buffer, extra_size, c_type);

  if (extra_ptr && (extra_size > 0)) {
    video_extra = MediaBuffer::Alloc(extra_size);
    if (!video_extra) {
      LOG_NO_MEMORY();
      return -1;
    }
    memcpy(video_extra->GetPtr(), extra_ptr, extra_size);
    video_extra->SetValidSize(extra_size);
  } else {
    RKMEDIA_LOGE("Muxer Flow: Intra Frame without sps pps\n");
  }

  return 0;
}

int MuxerFlow::Control(unsigned long int request, ...) {
  int ret = 0;
  va_list vl;
  va_start(vl, request);

  switch (request) {
  case S_START_SRTEAM: {
    StartStream();
  } break;
  case S_STOP_SRTEAM: {
    request_stop_stream = true;
  } break;
  case S_MANUAL_SPLIT_STREAM: {
    int duration = va_arg(vl, int);
    if (duration > 0 && enable_streaming) {
      RKMEDIA_LOGI("Muxer: manual split file duration = %d\n", duration);
      manual_split_file_duration = duration;
      manual_split = true;
    }
  } break;
  case G_MUXER_GET_STATUS: {
    int *value = va_arg(vl, int *);
    if (value)
      *value = enable_streaming ? 1 : 0;
  } break;
  case S_MUXER_FILE_DURATION: {
    int duration = va_arg(vl, int);
    RKMEDIA_LOGI("Muxer:: file_duration is %d\n", duration);
    if (duration)
      file_duration = duration;
  } break;
  case S_MUXER_FILE_PATH: {
    std::string path = va_arg(vl, std::string);
    RKMEDIA_LOGI("Muxer:: file_path is %s\n", path.c_str());
    if (!path.empty())
      file_path = path;
  } break;
  case S_MUXER_FILE_PREFIX: {
    std::string prefix = va_arg(vl, std::string);
    RKMEDIA_LOGI("Muxer:: file_prefix is %s\n", prefix.c_str());
    if (!prefix.empty())
      file_prefix = prefix;
  } break;
  case S_MUXER_FILE_NAME_CB: {
    file_name_cb = va_arg(vl, GET_FILE_NAMES_CB);
    file_name_handle = va_arg(vl, void *);
    RKMEDIA_LOGI("Muxer:: file_name_cb is %p, file_name_handle is %p\n",
                 file_name_cb, file_name_handle);
  } break;
  default:
    ret = -1;
    break;
  }

  va_end(vl);
  return ret;
}

void MuxerFlow::StartStream() {
  if (event_callback_ && !enable_streaming) {
    MuxerEvent muxer_event;
    memset(&muxer_event, 0, sizeof(MuxerEvent));
    muxer_event.type = MUX_EVENT_STREAM_START;
    event_callback_(event_handler2_, (void *)&muxer_event);
    is_first_file = true;
  }
  enable_streaming = true;
}

void MuxerFlow::StopStream() {
  if (video_recorder) {
    video_recorder.reset();
    video_recorder = nullptr;
    video_extra = nullptr;
  }

  manual_split = false;
  manual_split_record = false;
  vid_cached_buffers.clear();
  aud_cached_buffers.clear();

  if (enable_streaming) {
    // Send Stream Stop event
    if (event_callback_) {
      MuxerEvent muxer_event;
      memset(&muxer_event, 0, sizeof(MuxerEvent));
      muxer_event.type = MUX_EVENT_STREAM_STOP;
      event_callback_(event_handler2_, (void *)&muxer_event);
    }
  }
}

void MuxerFlow::CheckRecordEnd(int duration_s,
                               std::shared_ptr<MediaBuffer> vid_buffer) {
  if (duration_s <= 0 || last_ts == 0)
    return;

  if (!video_in || video_recorder == nullptr || vid_buffer == nullptr)
    return;

  if (!(vid_buffer->GetUserFlag() & MediaBuffer::kIntra))
    return;

  int frame_interval = 1000000 / vid_enc_config.vid_cfg.frame_rate; // us
  if ((vid_buffer->GetUSTimeStamp() - last_ts) >=
      (duration_s * 1000000 - frame_interval)) {
    real_file_duration = (vid_buffer->GetUSTimeStamp() - last_ts) / 1000;
    video_recorder.reset();
    video_recorder = nullptr;
    video_extra = nullptr;
    manual_split = false;
    manual_split_record = false;
    is_first_file = false;
  }
}

void MuxerFlow::ManualSplit(std::shared_ptr<MediaBuffer> vid_buffer) {
  if (!last_ts || !video_in || !video_recorder || !vid_buffer)
    return;

  if (!(vid_buffer->GetUserFlag() & MediaBuffer::kIntra))
    return;

  video_recorder.reset();
  video_recorder = nullptr;
  video_extra = nullptr;
  is_first_file = false;
  manual_split = false;
  manual_split_record = true;
}

void MuxerFlow::DequePushBack(std::deque<std::shared_ptr<MediaBuffer>> *deque,
                              std::shared_ptr<MediaBuffer> buffer,
                              bool is_video) {
  if (buffer == nullptr) {
    RKMEDIA_LOGE("clone mb failed");
    return;
  }

  if (deque->size() > 2) {
    auto front = deque->front();
    auto back = deque->back();
    if (back->GetUSTimeStamp() - front->GetUSTimeStamp() >=
        pre_record_cache_time * 1000000) {
      deque->pop_front();
      deque->push_back(buffer);

      if (is_video) {
#if PRE_RECORD_DEBUG
        vid_buffer_size += buffer->GetValidSize() - front->GetValidSize();
        printf("video0: buffer cnt: %d, vid_buffer_size: %d\n", deque->size(),
               aud_buffer_size);
#endif
      } else {
#if PRE_RECORD_DEBUG
        aud_buffer_size += buffer->GetValidSize() - front->GetValidSize();
        printf("audio0: buffer cnt: %d, aud_buffer_size: %d\n", deque->size(),
               aud_buffer_size);
#endif
      }

      return;
    }
  }

  deque->push_back(buffer);

  if (is_video) {
#if PRE_RECORD_DEBUG
    vid_buffer_size += buffer->GetValidSize();
    printf("video1: buffer cnt: %d, vid_buffer_size: %d\n", deque->size(),
           vid_buffer_size);
#endif
  } else {
#if PRE_RECORD_DEBUG
    aud_buffer_size += buffer->GetValidSize();
    printf("audio1: buffer cnt: %d, aud_buffer_size: %d\n", deque->size(),
           aud_buffer_size);
#endif
  }
}

int MuxerFlow::PreRecordWrite(std::shared_ptr<MediaBuffer> vid_buffer,
                              std::shared_ptr<MediaBuffer> aud_buffer) {
  int64_t seek_time = 0;
  int size = 0, seek_frames = 0, pre_record_frames = 0;
  bool is_pre_record = false;

  if (video_recorder == nullptr || is_lapse_record)
    return 0;

  switch (pre_record_mode) {
  case MUXER_PRE_RECORD_MANUAL_SPLIT:
    is_pre_record = manual_split_record && (pre_record_time > 0);
    break;
  case MUXER_PRE_RECORD_SINGLE:
    is_pre_record = is_first_file && (pre_record_time > 0);
    break;
  case MUXER_PRE_RECORD_NORMAL:
    is_pre_record = pre_record_time > 0 ? true : false;
    break;
  default:
    return 0;
  }

  if (!is_pre_record)
    return 0;

  // write pre-record video buffers
  size = vid_cached_buffers.size();
  if (video_in && vid_buffer)
    size -= 1;

  if (size <= 0)
    return 0;

  pre_record_frames = pre_record_time * vid_enc_config.vid_cfg.frame_rate;
  if (size < pre_record_frames) {
    RKMEDIA_LOGI(
        "%s: video cached buffer not enough(%d), pre_record_frames = %d\n",
        __func__, size, pre_record_frames);
    pre_record_frames = size;
  }

  int first_frame_index = size - pre_record_frames;
  int i = first_frame_index;
  auto first_frame = vid_cached_buffers.at(i);
  if (!(first_frame->GetUserFlag() & MediaBuffer::kIntra)) {
    // look forward for find I frame
    for (int j = (i - 1); j >= 0; j--) {
      auto tmp_frame = vid_cached_buffers.at(j);
      if ((tmp_frame->GetUserFlag() & MediaBuffer::kIntra)) {
        i = j;
        seek_time = (j - first_frame_index) * 1000000 /
                    vid_enc_config.vid_cfg.frame_rate;
        break;
      }
    }

    if (i == first_frame_index) {
      // look back for find I frame
      for (int j = (i + 1); j < size; j++) {
        auto tmp_frame = vid_cached_buffers.at(j);
        if ((tmp_frame->GetUserFlag() & MediaBuffer::kIntra)) {
          i = j;
          seek_time = (j - first_frame_index) * 1000000 /
                      vid_enc_config.vid_cfg.frame_rate;
          break;
        }
      }

      // not find I frame
      if (i == first_frame_index) {
        RKMEDIA_LOGI("%s: not find I frame\n", __func__);
        return 0;
      }
    }
  }

  RKMEDIA_LOGD("%s:\n", __func__);
  RKMEDIA_LOGD("\t video pre_record_frames: %d\n", pre_record_frames);
  RKMEDIA_LOGD("\t video_cached_buffers.size(): %d\n", size);
  RKMEDIA_LOGD("\t first_frame_index = %d\n", first_frame_index);
  RKMEDIA_LOGD("\t I frame index = %d\n", i);
  RKMEDIA_LOGD("\t seek_time = %lld us\n", seek_time);

  if (!video_extra) {
    if (GetVideoExtradata(vid_cached_buffers.at(i))) {
      RKMEDIA_LOGI("%s: Intra Frame without sps pps\n", __func__);
      return 0;
    }
  }

  last_ts = vid_cached_buffers.at(i)->GetUSTimeStamp();
  for (; i < size; i++) {
    if (!video_recorder->Write(this, vid_cached_buffers.at(i))) {
      RKMEDIA_LOGW("%s: write video pre-record buffers failed\n", __func__);
      video_extra = nullptr;
      last_ts = 0;
      return -1;
    }
  }

  // write pre-record audio buffers
  size = aud_cached_buffers.size();
  if (audio_in && aud_buffer)
    size -= 1;

  if (size <= 0)
    return 0;

  SampleInfo *sample_info = &(aud_enc_config.aud_cfg.sample_info);
  pre_record_frames =
      (pre_record_time * sample_info->sample_rate) / sample_info->nb_samples;
  seek_frames = (seek_time * sample_info->sample_rate) /
                sample_info->nb_samples / 1000000;

  if (size < pre_record_frames) {
    RKMEDIA_LOGI(
        "%s: audio cached buffer not enough(%d), pre_record_frames = %d\n",
        __func__, size, pre_record_frames);
    pre_record_frames = size;
  }
  i = size - pre_record_frames + seek_frames;

  RKMEDIA_LOGD("%s:\n", __func__);
  RKMEDIA_LOGD("\t audio pre_record_frames: %d\n", pre_record_frames);
  RKMEDIA_LOGD("\t audio_cached_buffers.size(): %d\n", size);
  RKMEDIA_LOGD("\t audio seek_frames: %d\n", seek_frames);
  RKMEDIA_LOGD("\t first audio frame index = %d\n", i);

  for (; i < size; i++) {
    if (!video_recorder->Write(this, aud_cached_buffers.at(i))) {
      RKMEDIA_LOGI("%s: write audio pre-record buffers failed\n", __func__);
      video_extra = nullptr;
      last_ts = 0;
      return -1;
    }
  }

  return 0;
}

bool save_buffer(Flow *f, MediaBufferVector &input_vector) {
  MuxerFlow *flow = static_cast<MuxerFlow *>(f);
  auto &&recorder = flow->video_recorder;
  int duration_s;
  auto &vid_buffer = input_vector[0];
  auto &aud_buffer = input_vector[1];

  if (flow->last_ts != 0 && flow->video_in && vid_buffer &&
      flow->is_lapse_record) {
    int lapse_frame_interval =
        1000000 / flow->vid_enc_config.vid_cfg.frame_rate; // us
    vid_buffer->SetUSTimeStamp(flow->lapse_time_stamp + lapse_frame_interval);
    flow->lapse_time_stamp += lapse_frame_interval;
  }

  if (!flow->is_lapse_record &&
      (flow->pre_record_mode != MUXER_PRE_RECORD_NONE) &&
      (flow->pre_record_time > 0)) {
    if (flow->audio_in && aud_buffer != nullptr)
      flow->DequePushBack(&(flow->aud_cached_buffers), aud_buffer, false);

    if (flow->video_in && vid_buffer != nullptr)
      flow->DequePushBack(&(flow->vid_cached_buffers), vid_buffer, true);
  }

  if (flow->request_stop_stream) {
    flow->StopStream();
    flow->enable_streaming = false;
    flow->request_stop_stream = false;
    return true;
  }

  if (!flow->enable_streaming)
    return true;

  if (flow->manual_split_record)
    duration_s = flow->manual_split_file_duration;
  else
    duration_s = flow->file_duration;

  if (flow->manual_split)
    flow->ManualSplit(vid_buffer);

  flow->CheckRecordEnd(duration_s, vid_buffer);

  if (recorder == nullptr) {
    recorder = flow->NewRecorder(flow->GenFilePath().c_str());
    flow->last_ts = 0;
    flow->real_file_duration = 0;
    if (recorder == nullptr) {
      flow->enable_streaming = false;
      flow->manual_split = false;
      flow->manual_split_record = false;
    } else {
      if (flow->PreRecordWrite(vid_buffer, aud_buffer)) {
        recorder.reset();
        flow->enable_streaming = false;
        flow->manual_split_record = false;
        return true;
      }
    }
  }

  // process audio stream here
  do {
    if (!flow->audio_in || aud_buffer == nullptr)
      break;

    if (!recorder->Write(flow, aud_buffer)) {
      recorder.reset();
      flow->enable_streaming = false;
      flow->manual_split_record = false;
      return true;
    }
  } while (0);

  // process video stream here
  do {
    if (!flow->video_in || vid_buffer == nullptr)
      break;

    if (!flow->video_extra) {
      if ((vid_buffer->GetUserFlag() & MediaBuffer::kIntra)) {
        if (flow->GetVideoExtradata(vid_buffer))
          break;
      } else {
        RKMEDIA_LOGW("Muxer: wait I frame\n");
        break;
      }
    }

    if (!recorder->Write(flow, vid_buffer)) {
      recorder.reset();
      flow->enable_streaming = false;
      flow->manual_split_record = false;
      return true;
    }

    if (flow->last_ts == 0 || vid_buffer->GetUSTimeStamp() < flow->last_ts) {
      flow->last_ts = vid_buffer->GetUSTimeStamp();
      flow->lapse_time_stamp = flow->last_ts;
    }

    flow->real_file_duration =
        (vid_buffer->GetUSTimeStamp() - flow->last_ts) / 1000;
  } while (0);
  return true;
}

DEFINE_FLOW_FACTORY(MuxerFlow, Flow)
const char *FACTORY(MuxerFlow)::ExpectedInputDataType() { return nullptr; }
const char *FACTORY(MuxerFlow)::OutPutDataType() { return ""; }

VideoRecorder::VideoRecorder(const char *param, Flow *f, const char *rpath,
                             int customio)
    : vid_stream_id(-1), aud_stream_id(-1), muxer_flow(f), record_path(rpath) {
  muxer =
      easymedia::REFLECTOR(Muxer)::Create<easymedia::Muxer>("rkaudio", param);
  if (!muxer) {
    RKMEDIA_LOGI("Create muxer rkaudio failed\n");
    exit(EXIT_FAILURE);
  }
  if (muxer_flow && customio)
    muxer->SetWriteCallback(muxer_flow, &muxer_buffer_callback);

  MuxerFlow *muxer_flow_ptr = static_cast<MuxerFlow *>(muxer_flow);
  if (muxer_flow_ptr->manual_split_record)
    ProcessEvent(MUX_EVENT_FILE_BEGIN,
                 (int)muxer_flow_ptr->manual_split_file_duration);
  else
    ProcessEvent(MUX_EVENT_FILE_BEGIN, (int)muxer_flow_ptr->file_duration);
}

VideoRecorder::~VideoRecorder() {
  if (vid_stream_id != -1) {
    auto buffer = easymedia::MediaBuffer::Alloc(1);
    buffer->SetEOF(true);
    buffer->SetValidSize(0);
    muxer->Write(buffer, vid_stream_id);
  }

  MuxerFlow *muxer_flow_ptr = static_cast<MuxerFlow *>(muxer_flow);
  if (muxer_flow_ptr->manual_split_record) {
    ProcessEvent(MUX_EVENT_MANUAL_SPLIT_END,
                 (int)muxer_flow_ptr->real_file_duration);
  } else {
    ProcessEvent(MUX_EVENT_FILE_END, (int)muxer_flow_ptr->real_file_duration);
  }

  if (muxer) {
    muxer.reset();
  }
}

void VideoRecorder::ProcessEvent(MuxerEventType event_type, int value) {
  if (muxer_flow) {
    MuxerEvent muxer_event;
    memset(&muxer_event, 0, sizeof(MuxerEvent));
    muxer_event.type = event_type;
    if (strlen(record_path.c_str())) {
      memcpy(muxer_event.file_name, record_path.c_str(),
             strlen(record_path.c_str()));
      muxer_event.value = value;
    }
    if (muxer_flow->event_callback_)
      muxer_flow->event_callback_(muxer_flow->event_handler2_,
                                  (void *)&muxer_event);
  }
}

void VideoRecorder::ClearStream() {
  vid_stream_id = -1;
  aud_stream_id = -1;
}

bool VideoRecorder::Write(MuxerFlow *f, std::shared_ptr<MediaBuffer> buffer) {
  MuxerFlow *flow = static_cast<MuxerFlow *>(f);
  if (flow->video_in && flow->video_extra && vid_stream_id == -1) {
    if (!muxer->NewMuxerStream(flow->vid_enc_config, flow->video_extra,
                               vid_stream_id)) {
      RKMEDIA_LOGE("NewMuxerStream failed for video\n");
      ProcessEvent(MUX_EVENT_ERR_CREATE_FILE_FAIL, -1);
    } else {
      RKMEDIA_LOGI("Video: create video stream finished!\n");
    }

    if (flow->audio_in) {
      if (!muxer->NewMuxerStream(flow->aud_enc_config, nullptr,
                                 aud_stream_id)) {
        RKMEDIA_LOGE("NewMuxerStream failed for audio\n");
        ProcessEvent(MUX_EVENT_ERR_CREATE_FILE_FAIL, -2);
      } else {
        RKMEDIA_LOGI("Audio: create audio stream finished!\n");
      }
    }

    auto header = muxer->WriteHeader(vid_stream_id);
    if (!header) {
      RKMEDIA_LOGI("WriteHeader on video stream return nullptr\n");
      ClearStream();
      ProcessEvent(MUX_EVENT_ERR_WRITE_FILE_FAIL, 0);
      return false;
    }
  }

  if (buffer->GetType() == Type::Video && vid_stream_id != -1) {
    if (nullptr == muxer->Write(buffer, vid_stream_id)) {
      RKMEDIA_LOGE("Write on video stream return nullptr\n");
      ClearStream();
      ProcessEvent(MUX_EVENT_ERR_WRITE_FILE_FAIL, -1);
      return false;
    }
  } else if (buffer->GetType() == Type::Audio && aud_stream_id != -1) {
    if (nullptr == muxer->Write(buffer, aud_stream_id)) {
      RKMEDIA_LOGE("Write on audio stream return nullptr\n");
      ClearStream();
      ProcessEvent(MUX_EVENT_ERR_WRITE_FILE_FAIL, -2);
      return false;
    }
  }

  return true;
}

} // namespace easymedia
