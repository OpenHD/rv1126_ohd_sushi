// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __MUXER_FLOW_H__
#define __MUXER_FLOW_H__

#include <sys/time.h>

#include "buffer.h"
#include "flow.h"
#include "muxer.h"
#include "utils.h"

#include "fcntl.h"
#include "stdint.h"
#include "stdio.h"
#include "unistd.h"

namespace easymedia {
#define PRE_RECORD_DEBUG 0
#define MUXER_FLOW_FILE_NAME_LEN 256

typedef int (*GET_FILE_NAMES_CB)(void *handle, char *file_name,
                                 unsigned int muxer_id);

typedef enum {
  MUXER_PRE_RECORD_NONE = 0,
  MUXER_PRE_RECORD_MANUAL_SPLIT,
  MUXER_PRE_RECORD_SINGLE,
  MUXER_PRE_RECORD_NORMAL
} PRE_RECORD_MODE_E;

class VideoRecorder;

static bool save_buffer(Flow *f, MediaBufferVector &input_vector);
static int muxer_buffer_callback(void *handler, uint8_t *buf, int buf_size);

class MuxerFlow : public Flow {
  friend VideoRecorder;

public:
  MuxerFlow(const char *param);
  virtual ~MuxerFlow();
  static const char *GetFlowName() { return "muxer_flow"; }
  virtual int Control(unsigned long int request, ...) final;

private:
  void StartStream();
  void StopStream();
  void ManualSplit(std::shared_ptr<MediaBuffer> vid_buffer);
  std::shared_ptr<VideoRecorder> NewRecorder(const char *path);
  std::string GenFilePath();
  int GetVideoExtradata(std::shared_ptr<MediaBuffer> vid_buffer);
  void CheckRecordEnd(int duration_s, std::shared_ptr<MediaBuffer> vid_buffer);
  void DequePushBack(std::deque<std::shared_ptr<MediaBuffer>> *deque,
                     std::shared_ptr<MediaBuffer> buffer, bool is_video);
  int PreRecordWrite(std::shared_ptr<MediaBuffer> vid_buffer,
                     std::shared_ptr<MediaBuffer> aud_buffer);
  friend bool save_buffer(Flow *f, MediaBufferVector &input_vector);
  friend int muxer_buffer_callback(void *handler, uint8_t *buf, int buf_size);

private:
  std::shared_ptr<MediaBuffer> video_extra;
  std::string muxer_param;
  std::string file_prefix;
  std::string file_path;
  std::string output_format; // rkaudio customio output format.
  std::string
      rkaudio_avdictionary; // examples: key1-value,key2-value,key3-value
  std::shared_ptr<VideoRecorder> video_recorder;
  MediaConfig vid_enc_config;
  MediaConfig aud_enc_config;
  bool video_in;
  bool audio_in;
  int64_t file_duration;      // s
  int64_t real_file_duration; // ms
  int64_t file_index;
  int64_t last_ts;
  bool file_time_en;
  bool is_use_customio;
  bool enable_streaming;
  bool request_stop_stream;
  // get file name frome callback
  GET_FILE_NAMES_CB file_name_cb;
  void *file_name_handle;
  unsigned int muxer_id;
  bool manual_split;
  bool manual_split_record;
  int64_t manual_split_file_duration;
  bool is_first_file;
  PRE_RECORD_MODE_E pre_record_mode;
  unsigned int pre_record_time;
  unsigned int pre_record_cache_time;
  std::deque<std::shared_ptr<MediaBuffer>> vid_cached_buffers;
  std::deque<std::shared_ptr<MediaBuffer>> aud_cached_buffers;
  int vid_buffer_size;
  int aud_buffer_size;
  bool is_lapse_record;
  int64_t lapse_time_stamp;
};

class VideoRecorder {
public:
  VideoRecorder(const char *param, Flow *f, const char *rpath, int customio);
  ~VideoRecorder();

  bool Write(MuxerFlow *f, std::shared_ptr<MediaBuffer> buffer);
  void ProcessEvent(MuxerEventType type, int value);

private:
  std::shared_ptr<Muxer> muxer;
  int vid_stream_id;
  int aud_stream_id;
  void ClearStream();
  Flow *muxer_flow;
  std::string record_path;
};
} // namespace easymedia

#endif
