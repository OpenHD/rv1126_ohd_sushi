// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <string>

#include "buffer.h"
#include "encoder.h"
#include "flow.h"
#include "key_string.h"
#include "media_config.h"
#include "media_type.h"
#include "message.h"
#include "stream.h"
#include "utils.h"

static bool quit = false;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

namespace easymedia {

class TestReadFlow : public Flow {
public:
  TestReadFlow(const char *param);
  virtual ~TestReadFlow();
  static const char *GetFlowName() { return "read_flow_test"; }

private:
  void ReadThreadRun();

  std::shared_ptr<Stream> fstream;
  std::string path;
  MediaBuffer::MemType mtype;
  size_t read_size;
  ImageInfo info;
  int fps;
  int loop_time;
  bool loop;
  std::thread *read_thread;
};

MediaBuffer::MemType StringToMemType(const char *s) {
  if (s) {
#ifdef LIBION
    if (!strcmp(s, KEY_MEM_ION) || !strcmp(s, KEY_MEM_HARDWARE))
      return MediaBuffer::MemType::MEM_HARD_WARE;
#endif
#ifdef LIBDRM
    if (!strcmp(s, KEY_MEM_DRM) || !strcmp(s, KEY_MEM_HARDWARE))
      return MediaBuffer::MemType::MEM_HARD_WARE;
#endif
    RKMEDIA_LOGI(
        "warning: %s is not supported or not integrated, fallback to common\n",
        s);
  }
  return MediaBuffer::MemType::MEM_COMMON;
}

bool ParseImageInfoFromMap(std::map<std::string, std::string> &params,
                           ImageInfo &info, bool input) {
  std::string value;
  const char *type = input ? KEY_INPUTDATATYPE : KEY_OUTPUTDATATYPE;
  CHECK_EMPTY(value, params, type)
  info.pix_fmt = StringToPixFmt(value.c_str());
  if (info.pix_fmt == PIX_FMT_NONE) {
    RKMEDIA_LOGI("unsupport pix fmt %s\n", value.c_str());
    return false;
  }
  CHECK_EMPTY(value, params, KEY_BUFFER_WIDTH)
  info.width = std::stoi(value);
  CHECK_EMPTY(value, params, KEY_BUFFER_HEIGHT)
  info.height = std::stoi(value);
  CHECK_EMPTY(value, params, KEY_BUFFER_VIR_WIDTH)
  info.vir_width = std::stoi(value);
  CHECK_EMPTY(value, params, KEY_BUFFER_VIR_HEIGHT)
  info.vir_height = std::stoi(value);
  return true;
}

TestReadFlow::TestReadFlow(const char *param)
    : mtype(MediaBuffer::MemType::MEM_COMMON), read_size(0), fps(0),
      loop_time(0), loop(false), read_thread(nullptr) {
  memset(&info, 0, sizeof(info));
  info.pix_fmt = PIX_FMT_NONE;
  std::map<std::string, std::string> params;
  if (!parse_media_param_map(param, params)) {
    SetError(-EINVAL);
    return;
  }
  std::string s;
  std::string value;
  CHECK_EMPTY_SETERRNO(value, params, KEY_PATH, EINVAL)
  path = value;
  CHECK_EMPTY_SETERRNO(value, params, KEY_OPEN_MODE, EINVAL)
  PARAM_STRING_APPEND(s, KEY_PATH, path);
  PARAM_STRING_APPEND(s, KEY_OPEN_MODE, value);
  fstream = REFLECTOR(Stream)::Create<Stream>("file_read_stream", s.c_str());
  if (!fstream) {
    fprintf(stderr, "Create stream file_read_stream failed\n");
    SetError(-EINVAL);
    return;
  }
  value = params[KEY_MEM_TYPE];
  if (!value.empty())
    mtype = StringToMemType(value.c_str());
  value = params[KEY_MEM_SIZE_PERTIME];
  if (value.empty()) {
    if (!ParseImageInfoFromMap(params, info)) {
      SetError(-EINVAL);
      return;
    }
  } else {
    read_size = std::stoul(value);
  }
  value = params[KEY_FPS];
  if (!value.empty())
    fps = std::stoi(value);
  value = params[KEY_LOOP_TIME];
  if (!value.empty())
    loop_time = std::stoi(value);
  if (!SetAsSource(std::vector<int>({0}), void_transaction00, path)) {
    SetError(-EINVAL);
    return;
  }
  loop = true;
  read_thread = new std::thread(&TestReadFlow::ReadThreadRun, this);
  if (!read_thread) {
    loop = false;
    SetError(-EINVAL);
    return;
  }
}

TestReadFlow::~TestReadFlow() {
  StopAllThread();
  if (read_thread) {
    source_start_cond_mtx->lock();
    loop = false;
    source_start_cond_mtx->notify();
    source_start_cond_mtx->unlock();
    read_thread->join();
    delete read_thread;
  }
  fstream.reset();
}

void TestReadFlow::ReadThreadRun() {
  source_start_cond_mtx->lock();
  if (down_flow_num == 0)
    source_start_cond_mtx->wait();
  source_start_cond_mtx->unlock();
  AutoPrintLine apl(__func__);
  size_t alloc_size = read_size;
  bool is_image = (info.pix_fmt != PIX_FMT_NONE);
  if (alloc_size == 0) {
    if (is_image) {
      int num = 0, den = 0;
      GetPixFmtNumDen(info.pix_fmt, num, den);
      alloc_size = info.vir_width * info.vir_height * num / den;
    }
  }
  while (loop) {
    if (fstream->Eof()) {
      if (loop_time-- > 0) {
        fstream->Seek(0, SEEK_SET);
      } else {
        NotifyToEventHandler(MSG_FLOW_EVENT_INFO_EOS, MESSAGE_TYPE_LIFO);
        break;
      }
    }
    auto buffer = MediaBuffer::Alloc(alloc_size, mtype);
    if (!buffer) {
      LOG_NO_MEMORY();
      continue;
    }
#if FLOW_EVENT_TEST
    static int cnt = 0;
    cnt++;
    if ((cnt % 15) == 1) {
      int msg_id = 2;
      static int msg_param = 0;
      msg_param++;
      EventParamPtr param = std::make_shared<EventParam>(msg_id, msg_param);
      char *params = (char *)malloc(24);
      sprintf(params, "%s", "hello rv1109");
      param->SetParams(params, 24);
      NotifyToEventHandler(param, MESSAGE_TYPE_LIFO);
    }
#endif
    if (is_image) {
      auto imagebuffer = std::make_shared<ImageBuffer>(*(buffer.get()), info);
      if (!imagebuffer) {
        LOG_NO_MEMORY();
        continue;
      }
      buffer = imagebuffer;
    }
    size_t size;
    if (read_size) {
      size = fstream->Read(buffer->GetPtr(), 1, read_size);
      if (size != read_size && !fstream->Eof()) {
        RKMEDIA_LOGI("read get %d != expect %d\n", (int)size, (int)read_size);
        SetDisable();
        break;
      }
      buffer->SetValidSize(size);
    }
    if (is_image) {
      if (!fstream->ReadImage(buffer->GetPtr(), info) && !fstream->Eof()) {
        SetDisable();
        break;
      }
      buffer->SetValidSize(buffer->GetSize());
    }
    buffer->SetUSTimeStamp(gettimeofday());
    SendInput(buffer, 0);
    if (fps != 0) {
      static int interval = 1000 / fps;
      msleep(interval);
    }
  }
}

DEFINE_FLOW_FACTORY(TestReadFlow, Flow)
const char *FACTORY(TestReadFlow)::ExpectedInputDataType() { return nullptr; }
const char *FACTORY(TestReadFlow)::OutPutDataType() { return ""; }

static bool save_buffer(Flow *f, MediaBufferVector &input_vector);

class TestWriteFlow : public Flow {
public:
  TestWriteFlow(const char *param);
  virtual ~TestWriteFlow();
  static const char *GetFlowName() { return "write_flow_test"; }

private:
  friend bool save_buffer(Flow *f, MediaBufferVector &input_vector);

private:
  std::shared_ptr<Stream> fstream;
  std::string path;
};

TestWriteFlow::TestWriteFlow(const char *param) {
  std::map<std::string, std::string> params;
  if (!parse_media_param_map(param, params)) {
    SetError(-EINVAL);
    return;
  }
  std::string s;
  std::string value;
  CHECK_EMPTY_SETERRNO(value, params, KEY_PATH, EINVAL)
  path = value;
  CHECK_EMPTY_SETERRNO(value, params, KEY_OPEN_MODE, EINVAL)
  PARAM_STRING_APPEND(s, KEY_PATH, path);
  PARAM_STRING_APPEND(s, KEY_OPEN_MODE, value);
  fstream = REFLECTOR(Stream)::Create<Stream>("file_write_stream", s.c_str());
  if (!fstream) {
    fprintf(stderr, "Create stream file_write_stream failed\n");
    SetError(-EINVAL);
    return;
  }

  SlotMap sm;
  sm.input_slots.push_back(0);
  sm.thread_model = Model::ASYNCCOMMON;
  sm.mode_when_full = InputMode::DROPFRONT;
  sm.input_maxcachenum.push_back(0);
  sm.process = save_buffer;

  std::string &name = params[KEY_NAME];
  if (!InstallSlotMap(sm, name, 0)) {
    RKMEDIA_LOGI("Fail to InstallSlotMap, %s\n", name.c_str());
    return;
  }
}

TestWriteFlow::~TestWriteFlow() {
  StopAllThread();
  fstream.reset();
}

bool save_buffer(Flow *f, MediaBufferVector &input_vector) {
  TestWriteFlow *flow = static_cast<TestWriteFlow *>(f);
  auto &buffer = input_vector[0];
#if FLOW_EVENT_TEST
  int msg_id = 1;
  int msg_param = 2;
  EventParamPtr param = std::make_shared<EventParam>(msg_id, msg_param);
  flow->NotifyToEventHandler(param);
#endif
  if (!buffer)
    return true;
  return flow->fstream->Write(buffer->GetPtr(), 1, buffer->GetValidSize());
}

DEFINE_FLOW_FACTORY(TestWriteFlow, Flow)
const char *FACTORY(TestWriteFlow)::ExpectedInputDataType() { return nullptr; }
const char *FACTORY(TestWriteFlow)::OutPutDataType() { return ""; }

} // namespace easymedia

static char optstr[] = "?i:o:w:h:t:";

int GetRandom() {
  srand((int)time(0));
  return rand() % 33;
}

int InputFlowEventProc(std::shared_ptr<easymedia::Flow> flow, bool &loop) {
  while (loop) {
    flow->EventHookWait();
    auto msg = flow->GetEventMessage();
    auto param = flow->GetEventParam(msg);
    if (param == nullptr)
      continue;
    printf("InputFlowEventProc flow %p msg id %d param %d\n", flow.get(),
           param->GetId(), param->GetParam());
    auto params = param->GetParams();
    if (params)
      printf("params -------------- %s\n", (char *)params);
    // TODO
    switch (param->GetId()) {
    case MSG_FLOW_EVENT_INFO_EOS:
      loop = false;
      quit = true;
      break;
    default:
      std::this_thread::sleep_for(std::chrono::milliseconds(GetRandom()));
      break;
    }
  }
  return 0;
}

int EncFlowEventProc(std::shared_ptr<easymedia::Flow> flow, bool &loop) {
  while (loop) {
    flow->EventHookWait();
    auto msg = flow->GetEventMessage();
    auto param = flow->GetEventParam(msg);
    if (param == nullptr)
      continue;
    printf("InputFlowEventProc flow %p msg id %d param %d\n", flow.get(),
           param->GetId(), param->GetParam());
    // TODO
    std::this_thread::sleep_for(std::chrono::milliseconds(GetRandom()));
  }
  return 0;
}

int OutPutFlowEventProc(std::shared_ptr<easymedia::Flow> flow, bool &loop) {
  while (loop) {
    flow->EventHookWait();
    auto msg = flow->GetEventMessage();
    auto param = flow->GetEventParam(msg);
    if (param == nullptr)
      continue;
    printf("OutPutFlowEventProc flow %p msg id %d param %d\n", flow.get(),
           param->GetId(), param->GetParam());
    // TODO
    std::this_thread::sleep_for(std::chrono::milliseconds(GetRandom()));
  }
  return 0;
}

int main(int argc, char **argv) {
  int c;
  std::string input_path;
  std::string output_path;
  int w = 0, h = 0;
  std::string input_format;
  int loop_times = 0;

  std::string flow_name;
  std::string flow_param;
  std::string stream_name;
  std::string stream_param;
  std::shared_ptr<easymedia::Flow> input_flow_;
  std::shared_ptr<easymedia::Flow> enc_flow_;
  std::shared_ptr<easymedia::Flow> output_flow_;
  std::shared_ptr<easymedia::Flow> cam_flow_;

  opterr = 1;
  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 'i':
      input_path = optarg;
      printf("input file path: %s\n", input_path.c_str());
      break;
    case 'o':
      output_path = optarg;
      printf("output file path: %s\n", output_path.c_str());
      break;
    case 'w':
      w = atoi(optarg);
      break;
    case 'h':
      h = atoi(optarg);
      break;
    case 't':
      loop_times = atoi(optarg);
      printf("loop_times: %d\n", loop_times);
      break;
    case '?':
    default:
      printf("usage example: \n");
      printf(
          "flow_event_test -i input.yuv -o output.h264 -w 320 -h 240 -t 0\n");
      exit(0);
    }
  }
  if (input_path.empty() || output_path.empty())
    exit(EXIT_FAILURE);
  if (!w || !h)
    exit(EXIT_FAILURE);
  printf("width, height: %d, %d\n", w, h);

  signal(SIGINT, sigterm_handler);

  easymedia::REFLECTOR(Flow)::DumpFactories();
  easymedia::REFLECTOR(Stream)::DumpFactories();

  int fps = 30;
  int w_align = UPALIGNTO16(w);
  int h_align = UPALIGNTO16(h);

  ImageInfo info;
  info.pix_fmt = PIX_FMT_NV12;
  info.width = w;
  info.height = h;
  info.vir_width = w_align;
  info.vir_height = h_align;

  flow_name = "read_flow_test";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_PATH, input_path);
  PARAM_STRING_APPEND(flow_param, KEY_OPEN_MODE,
                      "re"); // read and close-on-exec
  PARAM_STRING_APPEND(flow_param, KEY_MEM_TYPE, KEY_MEM_HARDWARE);
  flow_param += easymedia::to_param_string(info);
  PARAM_STRING_APPEND_TO(flow_param, KEY_FPS, fps); // rhythm reading
  PARAM_STRING_APPEND_TO(flow_param, KEY_LOOP_TIME, loop_times);
  printf("\nflow_param 1:\n%s\n", flow_param.c_str());

  input_flow_ = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!input_flow_) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }

  flow_name = "video_enc";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_NAME, "rkmpp");
  PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE,
                      IMAGE_NV12); // IMAGE_NV12 IMAGE_YUYV422
  PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE, VIDEO_H264);
  MediaConfig enc_config;
  VideoConfig &vid_cfg = enc_config.vid_cfg;
  ImageConfig &img_cfg = vid_cfg.image_cfg;
  img_cfg.image_info = info;
  vid_cfg.qp_init = 24;
  vid_cfg.qp_step = 4;
  vid_cfg.qp_min = 12;
  vid_cfg.qp_max = 48;
  vid_cfg.bit_rate = w * h * 7;
  if (vid_cfg.bit_rate > 1000000) {
    vid_cfg.bit_rate /= 1000000;
    vid_cfg.bit_rate *= 1000000;
  }
  vid_cfg.frame_rate = fps;
  vid_cfg.level = 52;
  vid_cfg.gop_size = fps;
  vid_cfg.profile = 100;
  // vid_cfg.rc_quality = "aq_only"; vid_cfg.rc_mode = "vbr";
  vid_cfg.rc_quality = KEY_HIGHEST;
  vid_cfg.rc_mode = KEY_CBR;
  std::string enc_param;
  enc_param.append(easymedia::to_param_string(enc_config, VIDEO_H264));
  enc_param = easymedia::JoinFlowParam(flow_param, 1, enc_param);
  printf("\nparam 2:\n%s\n", enc_param.c_str());

  enc_flow_ = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), enc_param.c_str());
  if (!enc_flow_) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    input_flow_.reset();
    exit(EXIT_FAILURE);
  }

  flow_name = "write_flow_test";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_PATH, output_path);
  PARAM_STRING_APPEND(flow_param, KEY_OPEN_MODE,
                      "w+"); // read and close-on-exec
  printf("\nparam 3:\n%s\n", flow_param.c_str());

  output_flow_ = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!output_flow_) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    input_flow_.reset();
    enc_flow_.reset();
    exit(EXIT_FAILURE);
  }

  enc_flow_->AddDownFlow(output_flow_, 0, 0);
  input_flow_->AddDownFlow(enc_flow_, 0, 0);

  input_flow_->RegisterEventHandler(input_flow_, InputFlowEventProc);
  enc_flow_->RegisterEventHandler(enc_flow_, EncFlowEventProc);
  output_flow_->RegisterEventHandler(output_flow_, OutPutFlowEventProc);

  while (!quit)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

  output_flow_->UnRegisterEventHandler();
  enc_flow_->UnRegisterEventHandler();
  input_flow_->UnRegisterEventHandler();

  enc_flow_->RemoveDownFlow(output_flow_);
  output_flow_.reset();
  input_flow_->RemoveDownFlow(enc_flow_);
  enc_flow_.reset();
  input_flow_.reset();
}
