// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <assert.h>
#include <string>
#include <thread>

#include "buffer.h"
#include "flow.h"
#include "media_reflector.h"
#include "utils.h"

#define DEBUG_IO_PROCESS 0

// RANSPORT_DELAY is used to count the time spent passing buffers
// between flows.
// Calculation method: A time stamp t0 is recorded after the upper-level
// Flow Process is completed, and a time stamp t1 is recorded when the
// lower-level FLow starts processing the Buffer.
// T1-t0 is the delay time passed between flows.
#define TRANSPORT_DELAY_DEBUG 0
#define TRANSPORT_DELAY_AVERAGE_DEBUG 0

// Count the average number of buffers
#define AVERAGE_CNT 300

#if TRANSPORT_DELAY_DEBUG || TRANSPORT_DELAY_AVERAGE_DEBUG
#define TRANSPORT_DELAY_CODE_PROCESS(NAME, ID, BUFFER)                         \
  if (BUFFER) {                                                                \
    static int64_t average_delta_sum##ID = 0;                                  \
    static int average_delta_cnt##ID = 0;                                      \
    int average_delta##ID =                                                    \
        (int)(easymedia::gettimeofday() - BUFFER->GetAtomicClock());           \
    if (TRANSPORT_DELAY_DEBUG) {                                               \
      RKMEDIA_LOGI("TRANS:[%s] Input[%d] Delay:%0.2fms\n", NAME, ID,           \
                   average_delta##ID / 1000.0);                                \
    }                                                                          \
    average_delta_sum##ID += average_delta##ID;                                \
    average_delta_cnt##ID++;                                                   \
    if (average_delta_cnt##ID >= AVERAGE_CNT) {                                \
      RKMEDIA_LOGI("TRANS:[%s] Input[%d] Average Delay:%0.2fms\n", NAME, ID,   \
                   average_delta_sum##ID / 1000.0 / average_delta_cnt##ID);    \
      average_delta_cnt##ID = 0;                                               \
      average_delta_sum##ID = 0;                                               \
    }                                                                          \
  }
#define TRANSPORT_DELAY_CODE_RESET_TS(BUFFER)                                  \
  if (BUFFER)                                                                  \
    BUFFER->SetAtomicClock(easymedia::gettimeofday());
#else
#define TRANSPORT_DELAY_CODE_PROCESS(NAME, ID, BUFFER)
#define TRANSPORT_DELAY_CODE_RESET_TS(BUFFER)
#endif

int g_need_sleep = 1;

namespace easymedia {

#define KEY_IN_CNT "in_cnt"
#define KEY_OUT_CNT "out_cnt"

#if DEBUG_IO_PROCESS
static void dump_buffer(std::shared_ptr<MediaBuffer> mb, const char *tag) {
  if (!mb) {
    RKMEDIA_LOGI("DUMP BUFF:: [%s] size:0\n", tag);
    return;
  }

  RKMEDIA_LOGI("DUMP BUFF:: [%s] size:%zu\n", tag, mb->GetValidSize());
  for (size_t i = 0; i < mb->GetValidSize(); i++) {
    RKMEDIA_LOGI("%02x ", *((char *)mb->GetPtr() + i));
    if (i && !((i + 1) % 8))
      RKMEDIA_LOGI("");
  }
  RKMEDIA_LOGI("");
}
#endif

// static void check_specific_buff(std::shared_ptr<MediaBuffer> mb, std::string
// name) {
//  char *start = (char *)mb->GetPtr();
//  // check for start buff or eos buff.
//}

static bool do_src(Flow *f, MediaBufferVector &input_vector);
static bool is_src_eos(std::shared_ptr<easymedia::Flow> &f);
class MockSourceFlow : public Flow {
public:
  MockSourceFlow(const char *param);
  virtual ~MockSourceFlow();
  static const char *GetFlowName() { return "mock_src_flow"; }
  std::string GetName() const { return name_; }
  FILE *file;
  int interval_ts; // ms
  int eos_flag;

private:
  void ReadThreadRun();
  bool loop_;
  std::thread *read_thread_;
  std::string name_;

  friend bool do_src(Flow *f, MediaBufferVector &input_vector) {
    MockSourceFlow *flow = static_cast<MockSourceFlow *>(f);
    auto in = input_vector[0];
    TRANSPORT_DELAY_CODE_PROCESS(flow->GetName().c_str(), 0, in)
    TRANSPORT_DELAY_CODE_RESET_TS(in)
    flow->SetOutput(in, 0);

    // Do nothing
    return true;
  }
  friend bool is_src_eos(Flow *f);
};

bool is_src_eos(std::shared_ptr<easymedia::Flow> &f) {
  MockSourceFlow *flow = static_cast<MockSourceFlow *>(f.get());
  if (flow->eos_flag)
    return true;
  return false;
}

MockSourceFlow::MockSourceFlow(const char *param)
    : loop_(false), read_thread_(nullptr), name_("") {
  std::map<std::string, std::string> params;
  if (!parse_media_param_map(param, params)) {
    SetError(-EINVAL);
    return;
  }
  std::string value;
  CHECK_EMPTY_SETERRNO(value, params, KEY_NAME, EINVAL);
  name_ = value;

  value = "";
  CHECK_EMPTY_SETERRNO(value, params, KEY_PATH, EINVAL)
  RKMEDIA_LOGI("INFO: %s open %s...\n", name_.c_str(), value.c_str());
  file = fopen(value.c_str(), "r");
  if (!file) {
    RKMEDIA_LOGE("%s open %s failed!\n", __func__, value.c_str());
    return;
  }

  value = params[KEY_FPS];
  if (value.empty())
    interval_ts = 0;
  else
    interval_ts = 1000 / std::stoi(value); // fps to ms.
  RKMEDIA_LOGI("INFO: %s interval_ts = %d...\n", name_.c_str(), interval_ts);

  // reset end of stream flag.
  eos_flag = 0;

  if (!SetAsSource(std::vector<int>({0}), do_src, name_)) {
    RKMEDIA_LOGE("%s SetAsSource failed!\n", __func__);
    SetError(-EINVAL);
    return;
  }

  loop_ = true;
  read_thread_ = new std::thread(&MockSourceFlow::ReadThreadRun, this);
  if (!read_thread_) {
    loop_ = false;
    SetError(-EINVAL);
    return;
  }
}

MockSourceFlow::~MockSourceFlow() {
  StopAllThread();
  if (read_thread_) {
    source_start_cond_mtx->lock();
    loop_ = false;
    source_start_cond_mtx->notify();
    source_start_cond_mtx->unlock();
    read_thread_->join();
    delete read_thread_;
  }

  if (file) {
    RKMEDIA_LOGI("INFO: %s close file...\n", name_.c_str());
    fclose(file);
  }
}

void MockSourceFlow::ReadThreadRun() {
  source_start_cond_mtx->lock();
  if (down_flow_num == 0)
    source_start_cond_mtx->wait();
  source_start_cond_mtx->unlock();
  while (loop_) {
    std::shared_ptr<MediaBuffer> buffer = MediaBuffer::Alloc(512);
    if (!buffer) {
      RKMEDIA_LOGE("MockSourceFlow: malloc failed! exit read thread.\n");
      break;
    }

#if 0
    //debug io process
    static char id = 0;
    memset(buffer->GetPtr(), id, 256);
    id++;
    memset((char *)buffer->GetPtr() + 256, id, 256);
    id++;
    if (id >= 0x7F) {
      RKMEDIA_LOGI("INFO: %s Test io process get eof!\n", __func__);
      break;
    }
    buffer->SetValidSize(512);
    buffer->SetUSTimeStamp(easymedia::gettimeofday());
    SendInput(buffer, 0);
    easymedia::msleep(1000);
    RKMEDIA_LOGI("");
    continue;
#endif

    int ret = fread(buffer->GetPtr(), 1, buffer->GetSize(), file);
    if (ret > 0) {
      buffer->SetValidSize(ret);
      buffer->SetUSTimeStamp(easymedia::gettimeofday());
      buffer->SetAtomicClock(easymedia::gettimeofday());
      SendInput(buffer, 0);
      // RKMEDIA_LOGI("%s fake sending input %p, %dBytes\n",
      //  GetName().c_str(), buffer.get(), ret);
      // check_specific_buff(buffer, GetName());
    } else {
      buffer->SetValidSize(0);
      buffer->SetEOF(1);
      buffer->SetUSTimeStamp(easymedia::gettimeofday());
      buffer->SetAtomicClock(easymedia::gettimeofday());
      SendInput(buffer, 0);
      RKMEDIA_LOGI("INFO: %s get eos of file! exit read thread.\n",
                   GetName().c_str());
      eos_flag = 1;
      break;
    }
    if (interval_ts > 0)
      easymedia::msleep(interval_ts);
  }
}

DEFINE_FLOW_FACTORY(MockSourceFlow, Flow)
const char *FACTORY(MockSourceFlow)::ExpectedInputDataType() { return nullptr; }
const char *FACTORY(MockSourceFlow)::OutPutDataType() { return ""; }

static bool do_io11(Flow *f, MediaBufferVector &input_vector);
static bool do_io12(Flow *f, MediaBufferVector &input_vector);
static bool do_io21(Flow *f, MediaBufferVector &input_vector);
static bool do_io22(Flow *f, MediaBufferVector &input_vector);

class MockIOFlow : public Flow {
public:
  MockIOFlow(const char *param);
  virtual ~MockIOFlow();
  static const char *GetFlowName() { return "mock_io_flow"; }
  std::string GetName() const { return name_; }
  int GetOutCnt() const { return out_; }

protected:
  // for io21 cached buffer
  std::deque<std::shared_ptr<MediaBuffer>> io21_in0_vector;
  std::deque<std::shared_ptr<MediaBuffer>> io21_in1_vector;

private:
  std::string name_;
  Model thread_model_;
  int in_;
  int out_;

  friend bool do_io11(Flow *f, MediaBufferVector &input_vector);
  friend bool do_io12(Flow *f, MediaBufferVector &input_vector);
  friend bool do_io21(Flow *f, MediaBufferVector &input_vector);
  friend bool do_io22(Flow *f, MediaBufferVector &input_vector);
};

bool do_io11(Flow *f, MediaBufferVector &input_vector) {
  MockIOFlow *flow = static_cast<MockIOFlow *>(f);
  auto in = input_vector[0];
  // return false to tell next flow error occours.
  if (!in)
    return false;
  // check_specific_buff(in, flow->GetName());
  TRANSPORT_DELAY_CODE_PROCESS(flow->GetName().c_str(), 0, in)
  TRANSPORT_DELAY_CODE_RESET_TS(in)
  flow->SetOutput(in, 0);
  // Do nothing
  return true;
}

bool do_io12(Flow *f, MediaBufferVector &input_vector) {
  MockIOFlow *flow = static_cast<MockIOFlow *>(f);
  auto in = input_vector[0];
  // return false to tell next flow error occours.
  if (!in || !in->GetValidSize())
    return false;
  // check_specific_buff(in, flow->GetName());
  TRANSPORT_DELAY_CODE_PROCESS(flow->GetName().c_str(), 0, in)
#if DEBUG_IO_PROCESS
  dump_buffer(in, "do_io12:in");
#endif

  // split one buffer to two buffer.
  size_t buf_size0 = (in->GetValidSize() + 1) / 2;
  size_t buf_size1 = in->GetValidSize() - buf_size0;
  auto sub_buf0 = MediaBuffer::Alloc(buf_size0);
  auto sub_buf1 = MediaBuffer::Alloc(buf_size1);

  if (!sub_buf0 || !sub_buf1) {
    LOG_NO_MEMORY();
    RKMEDIA_LOGE("malloc size0:%zu, size1:%zu\n", buf_size0, buf_size1);
    return false;
  }
  // check_specific_buff(sub_buf0, flow->GetName());
  memcpy(sub_buf0->GetPtr(), in->GetPtr(), buf_size0);
  sub_buf0->SetValidSize(buf_size0);
  sub_buf0->SetUSTimeStamp(easymedia::gettimeofday());

  memcpy(sub_buf1->GetPtr(), (char *)in->GetPtr() + buf_size0, buf_size1);
  sub_buf1->SetValidSize(buf_size1);
  sub_buf1->SetUSTimeStamp(sub_buf0->GetUSTimeStamp());

#if DEBUG_IO_PROCESS
  dump_buffer(sub_buf0, "do_io12:out0");
  dump_buffer(sub_buf1, "do_io12:out1");
#endif

  TRANSPORT_DELAY_CODE_RESET_TS(sub_buf0)
  TRANSPORT_DELAY_CODE_RESET_TS(sub_buf1)
  flow->SetOutput(sub_buf0, 0);
  flow->SetOutput(sub_buf1, 1);

  // Do nothing
  return true;
}

bool do_io21(Flow *f, MediaBufferVector &input_vector) {
  MockIOFlow *flow = static_cast<MockIOFlow *>(f);

  auto in0 = input_vector[0];
  auto in1 = input_vector[1];
  // return false to tell next flow error occours.
  if (!in0 && !in1)
    return false;

    // if (in0)
    // check_specific_buff(in0, flow->GetName());

#if DEBUG_IO_PROCESS
  if (in0)
    dump_buffer(in0, "do_io21:in0");
  if (in1)
    dump_buffer(in1, "do_io21:in1");
#endif

  TRANSPORT_DELAY_CODE_PROCESS(flow->GetName().c_str(), 0, in0)
  TRANSPORT_DELAY_CODE_PROCESS(flow->GetName().c_str(), 1, in1)

  if ((flow->io21_in0_vector.size() > 0) &&
      (flow->io21_in1_vector.size() > 0)) {
    RKMEDIA_LOGE("%s all of 2 inslot have cached buffers.\n", __func__);
    return false;
  }

  if (flow->io21_in0_vector.size() > 0) {
    if (in1) {
      auto tmp = flow->io21_in0_vector.front();
      if (tmp->GetUSTimeStamp() != in1->GetUSTimeStamp()) {
        RKMEDIA_LOGI(
            "ERROR: %s inslot[0] cached buffer's ts not equal in1 buffer's ts,"
            "delta ts=%d\n",
            __func__, (int)(tmp->GetUSTimeStamp() - in1->GetUSTimeStamp()));
        return false;
      }
      size_t tt_size = tmp->GetValidSize() + in1->GetValidSize();
      auto tt_buf = MediaBuffer::Alloc(tt_size);
      memcpy(tt_buf->GetPtr(), tmp->GetPtr(), tmp->GetValidSize());
      memcpy((char *)tt_buf->GetPtr() + tmp->GetValidSize(), in1->GetPtr(),
             in1->GetValidSize());
      flow->io21_in0_vector.pop_front();
      if (in0)
        flow->io21_in0_vector.push_back(in0);
      tt_buf->SetValidSize(tt_size);
      tt_buf->SetUSTimeStamp(gettimeofday());
#if DEBUG_IO_PROCESS
      dump_buffer(tt_buf, "do_io21:out");
#endif
      TRANSPORT_DELAY_CODE_RESET_TS(tt_buf)
      flow->SetOutput(tt_buf, 0);
    } else {
      flow->io21_in0_vector.push_back(in0);
      if (flow->io21_in0_vector.size() > 3)
        RKMEDIA_LOGW("inslot[0] cached too maney buffers!\n");
    }
  } else if (flow->io21_in1_vector.size() > 0) {
    if (in0) {
      auto tmp = flow->io21_in1_vector.front();
      if (tmp->GetUSTimeStamp() != in0->GetUSTimeStamp()) {
        RKMEDIA_LOGI(
            "ERROR: %s inslot[0] cached buffer's ts not equal in1 buffer's ts,"
            "delta ts=%d\n",
            __func__, (int)(tmp->GetUSTimeStamp() - in0->GetUSTimeStamp()));
        return false;
      }
      size_t tt_size = tmp->GetValidSize() + in0->GetValidSize();
      auto tt_buf = MediaBuffer::Alloc(tt_size);
      memcpy(tt_buf->GetPtr(), in0->GetPtr(), in0->GetValidSize());
      memcpy((char *)tt_buf->GetPtr() + in0->GetValidSize(), tmp->GetPtr(),
             tmp->GetValidSize());
      flow->io21_in1_vector.pop_front();
      if (in1)
        flow->io21_in1_vector.push_back(in1);
      tt_buf->SetValidSize(tt_size);
      tt_buf->SetUSTimeStamp(gettimeofday());
#if DEBUG_IO_PROCESS
      dump_buffer(tt_buf, "do_io21:out");
#endif
      TRANSPORT_DELAY_CODE_RESET_TS(tt_buf)
      flow->SetOutput(tt_buf, 0);
    } else {
      flow->io21_in1_vector.push_back(in1);
      if (flow->io21_in1_vector.size() > 3)
        RKMEDIA_LOGW("inslot[1] cached too maney buffers!\n");
    }
  } else {
    // all vector size = 0;
    if (in0 && in1) {
      if (in0->GetUSTimeStamp() == in1->GetUSTimeStamp()) {
        size_t tt_size = in0->GetValidSize() + in1->GetValidSize();
        auto tt_buf = MediaBuffer::Alloc(tt_size);
        memcpy(tt_buf->GetPtr(), in0->GetPtr(), in0->GetValidSize());
        memcpy((char *)tt_buf->GetPtr() + in0->GetValidSize(), in1->GetPtr(),
               in1->GetValidSize());
        tt_buf->SetValidSize(tt_size);
        tt_buf->SetUSTimeStamp(gettimeofday());
#if DEBUG_IO_PROCESS
        dump_buffer(tt_buf, "do_io21:out");
#endif
        TRANSPORT_DELAY_CODE_RESET_TS(tt_buf)
        flow->SetOutput(tt_buf, 0);
      } else {
        RKMEDIA_LOGE("%s inslot[0] and inslot[1] ts should be equal!\n",
                     __func__);
        return false;
      }
    } else {
      if (in0) {
        flow->io21_in0_vector.push_back(in0);
        if (flow->io21_in0_vector.size() > 3)
          RKMEDIA_LOGW("inslot[0] cached too maney buffers!\n");
      } else if (in1) {
        flow->io21_in1_vector.push_back(in1);
        if (flow->io21_in1_vector.size() > 3)
          RKMEDIA_LOGW("inslot[1] cached too maney buffers!\n");
      }
    }
  }

  // Do nothing
  return true;
}

bool do_io22(Flow *f, MediaBufferVector &input_vector) {
  MockIOFlow *flow = static_cast<MockIOFlow *>(f);

  auto in0 = input_vector[0];
  auto in1 = input_vector[1];
  // return false to tell next flow error occours.
  if (!in0 && !in1)
    return false;

  // if (in0)
  // check_specific_buff(in0, flow->GetName());

  TRANSPORT_DELAY_CODE_PROCESS(flow->GetName().c_str(), 0, in0)
  TRANSPORT_DELAY_CODE_PROCESS(flow->GetName().c_str(), 1, in1)
  TRANSPORT_DELAY_CODE_RESET_TS(in0)
  TRANSPORT_DELAY_CODE_RESET_TS(in1)
  if (in0)
    flow->SetOutput(in0, 0);
  if (in1)
    flow->SetOutput(in1, 1);

  // Do nothing
  return true;
}

MockIOFlow::MockIOFlow(const char *param) {
  std::map<std::string, std::string> params;
  if (!parse_media_param_map(param, params)) {
    SetError(-EINVAL);
    return;
  }
  std::string value;
  CHECK_EMPTY_SETERRNO(value, params, KEY_NAME, EINVAL)
  name_ = value;

  value = "";
  CHECK_EMPTY_SETERRNO(value, params, KEY_IN_CNT, EINVAL)
  in_ = atoi(value.c_str());

  value = "";
  CHECK_EMPTY_SETERRNO(value, params, KEY_OUT_CNT, EINVAL)
  out_ = atoi(value.c_str());

  SlotMap sm;
  int input_maxcachenum = in_ * 3;
  ParseParamToSlotMap(params, sm, input_maxcachenum);
  if (sm.thread_model == Model::ASYNCATOMIC)
    RKMEDIA_LOGI("INFO: %s ASYNCATOMIC sleep %fms\n", name_.c_str(),
                 sm.interval);
  if (sm.thread_model == Model::NONE)
    sm.thread_model = Model::ASYNCCOMMON;
  thread_model_ = sm.thread_model;
  if (sm.mode_when_full == InputMode::NONE)
    sm.mode_when_full = InputMode::DROPFRONT;
  for (int i = 0; i < in_; i++) {
    sm.input_slots.push_back(i);
    sm.input_maxcachenum.push_back(input_maxcachenum);
  }

  for (int i = 0; i < out_; i++)
    sm.output_slots.push_back(i);

  if ((in_ == 1) && (out_ == 1)) {
    RKMEDIA_LOGD("IO Flow: 11 use do_io11 process\n");
    sm.process = do_io11;
  } else if ((in_ == 1) && (out_ == 2)) {
    RKMEDIA_LOGD("IO Flow: 12 use do_io12 process\n");
    sm.process = do_io12;
  } else if ((in_ == 2) && (out_ == 1)) {
    RKMEDIA_LOGD("IO Flow: 21 use do_io21 process\n");
    sm.process = do_io21;
  } else if ((in_ == 2) && (out_ == 2)) {
    RKMEDIA_LOGD("IO Flow: 22 use do_io22 process\n");
    sm.process = do_io22;
  } else {
    RKMEDIA_LOGE("%s IOFlow not support io mode(in:%d, out:%d)\n", __func__,
                 in_, out_);
    return;
  }

  if (!InstallSlotMap(sm, name_, -1)) {
    RKMEDIA_LOGI("Fail to InstallSlotMap, %s\n", name_.c_str());
    SetError(-EINVAL);
    return;
  }
}

MockIOFlow::~MockIOFlow() { StopAllThread(); }

DEFINE_FLOW_FACTORY(MockIOFlow, Flow)
const char *FACTORY(MockIOFlow)::ExpectedInputDataType() { return nullptr; }
const char *FACTORY(MockIOFlow)::OutPutDataType() { return ""; }

static bool do_out(Flow *f, MediaBufferVector &input_vector);
class MockSinkFlow : public Flow {
public:
  MockSinkFlow(const char *param);
  ~MockSinkFlow() {
    StopAllThread();
    if (file) {
      RKMEDIA_LOGI("INFO: %s close file...\n", name_.c_str());
      fclose(file);
    }
  }
  static const char *GetFlowName() { return "mock_sink_flow"; }

  std::string GetName() const { return name_; }
  FILE *file;

private:
  std::string name_;
  Model thread_model_;
  int in_;

  friend bool do_out(Flow *f, MediaBufferVector &input_vector) {
    MockSinkFlow *flow = static_cast<MockSinkFlow *>(f);
    auto &in = input_vector[0];
    if (!in) {
      return false;
    }

    // check_specific_buff(in, flow->GetName());

    TRANSPORT_DELAY_CODE_PROCESS(flow->GetName().c_str(), 0, in)
    if (in->GetValidSize()) {
      size_t ret = fwrite(in->GetPtr(), 1, in->GetValidSize(), flow->file);
      if (ret != in->GetValidSize()) {
        RKMEDIA_LOGE("MockSinkFlow: process write file failed!\n");
        return false;
      }
    }

    return true;
  }
};

MockSinkFlow::MockSinkFlow(const char *param) {
  std::map<std::string, std::string> params;
  if (!parse_media_param_map(param, params)) {
    SetError(-EINVAL);
    return;
  }
  std::string value;
  CHECK_EMPTY_SETERRNO(value, params, KEY_NAME, EINVAL)
  name_ = value;

  value = "";
  CHECK_EMPTY_SETERRNO(value, params, KEY_IN_CNT, EINVAL)
  in_ = atoi(value.c_str());

  value = "";
  CHECK_EMPTY_SETERRNO(value, params, KEY_PATH, EINVAL)
  RKMEDIA_LOGI("INFO: %s open %s...\n", name_.c_str(), value.c_str());
  file = fopen(value.c_str(), "wb");
  if (!file) {
    RKMEDIA_LOGE("%s open %s failed!\n", __func__, value.c_str());
    return;
  }

  SlotMap sm;
  int input_maxcachenum = 2;
  ParseParamToSlotMap(params, sm, input_maxcachenum);
  if (sm.thread_model == Model::ASYNCATOMIC)
    RKMEDIA_LOGI("INFO: %s ASYNCATOMIC sleep %fms\n", name_.c_str(),
                 sm.interval);
  if (sm.thread_model == Model::NONE)
    sm.thread_model = Model::ASYNCCOMMON;
  thread_model_ = sm.thread_model;
  if (sm.mode_when_full == InputMode::NONE)
    sm.mode_when_full = InputMode::DROPCURRENT;

  for (int i = 0; i < in_; i++) {
    sm.input_slots.push_back(i);
    sm.input_maxcachenum.push_back(input_maxcachenum);
  }
  sm.process = do_out;
  if (!InstallSlotMap(sm, name_, -1)) {
    RKMEDIA_LOGI("Fail to InstallSlotMap, %s\n", name_.c_str());
    SetError(-EINVAL);
    return;
  }
}

DEFINE_FLOW_FACTORY(MockSinkFlow, Flow)
const char *FACTORY(MockSinkFlow)::ExpectedInputDataType() { return nullptr; }
const char *FACTORY(MockSinkFlow)::OutPutDataType() { return ""; }

} // namespace easymedia

static int file_compare(std::string src, std::string dst) {
  char src_buf[1024] = {0};
  char dst_buf[1024] = {0};
  FILE *srcf, *dstf;

  RKMEDIA_LOGI("## Compare %s with %s ...\n", src.c_str(), dst.c_str());
  srcf = fopen(src.c_str(), "r");
  dstf = fopen(dst.c_str(), "r");
  if (!srcf || !dstf) {
    RKMEDIA_LOGE("file(%s or %s) not exist!\n", src.c_str(), dst.c_str());
    return -1;
  }

  int file_end = 0;
  int ret = 0;
  while (!file_end) {
    int srclen = fread(src_buf, 1, 1024, srcf);
    if (srclen < 1024)
      file_end = 1;
    int dstlen = fread(dst_buf, 1, 1024, dstf);
    if (dstlen < 1024)
      file_end = 1;
    if (srclen != dstlen) {
      RKMEDIA_LOGE("file length not equal, src:%d, dst:%d\n", srclen, dstlen);
      ret = -1;
      break;
    }

    for (int i = 0; i < srclen; i++) {
      if (src_buf[i] != dst_buf[i]) {
        RKMEDIA_LOGE("file context not equal\n");
        ret = -1;
        break;
      }
    }
    if (ret < 0)
      break;
  }

  fclose(srcf);
  fclose(dstf);
  return ret;
}

// ------------------------------------------------------------------
// TEST1: single pipeline test
//    src->io11->sink
//-------------------------------------------------------------------
void single_pipe_run(std::string mode, std::string input, std::string output) {
  std::string flow_name = "mock_src_flow";
  std::string param;

  easymedia::AutoPrintLine atuoprint(__func__);

  // Create source flow
  PARAM_STRING_APPEND(param, KEY_NAME, "src");
  PARAM_STRING_APPEND(param, KEY_PATH, input);
  PARAM_STRING_APPEND(param, KEK_THREAD_SYNC_MODEL, mode);
  if ((mode == KEY_ASYNCCOMMON) || (mode == KEY_ASYNCATOMIC))
    PARAM_STRING_APPEND_TO(param, KEY_FPS, 100);
  RKMEDIA_LOGI("# src params:%s\n", param.c_str());
  auto src = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  assert(src);

  // Create IO flow
  flow_name = "mock_io_flow";
  param = "";
  PARAM_STRING_APPEND(param, KEY_NAME, "io11");
  PARAM_STRING_APPEND(param, KEY_IN_CNT, "1");
  PARAM_STRING_APPEND(param, KEY_OUT_CNT, "1");
  if (mode == KEY_ASYNCATOMIC)
    PARAM_STRING_APPEND_TO(param, KEY_FPS, 100);
  PARAM_STRING_APPEND(param, KEK_THREAD_SYNC_MODEL, mode);
  RKMEDIA_LOGI("# io params:%s\n", param.c_str());
  auto io11 = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  assert(io11);

  // Create sink flow
  flow_name = "mock_sink_flow";
  param = "";
  PARAM_STRING_APPEND(param, KEY_NAME, "sink");
  PARAM_STRING_APPEND(param, KEY_PATH, output);
  PARAM_STRING_APPEND(param, KEY_IN_CNT, "1");
  if (mode == KEY_ASYNCATOMIC)
    PARAM_STRING_APPEND_TO(param, KEY_FPS, 100);
  PARAM_STRING_APPEND(param, KEK_THREAD_SYNC_MODEL, mode);
  RKMEDIA_LOGI("# sink params:%s\n", param.c_str());
  auto sink = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  assert(sink);

  src->AddDownFlow(io11, 0, 0);
  io11->AddDownFlow(sink, 0, 0);
  RKMEDIA_LOGD("#SrcFlow:%p, IO11Flow:%p, SinkFLow:%p\n", src.get(), io11.get(),
               sink.get());

  while (!is_src_eos(src)) {
    easymedia::msleep(100);
  }

  // waite for all flow flush data.
  easymedia::msleep(100);
  src->RemoveDownFlow(io11);
  io11->RemoveDownFlow(sink);
  src.reset();
  io11.reset();
  sink.reset();
}

int single_pipe_test() {
  RKMEDIA_LOGI("========================================\n");
  RKMEDIA_LOGI("========= single_pipe_test =============\n");
  RKMEDIA_LOGI("========================================\n");
  single_pipe_run(KEY_SYNC, "/data/flowInData", "/data/flowOut0");
  single_pipe_run(KEY_ASYNCCOMMON, "/data/flowInData", "/data/flowOut1");
  // single_pipe_run(KEY_ASYNCATOMIC, "/data/flowInData", "/data/flowOut2");

  // compare sink and src data
  int ret = 0;
  RKMEDIA_LOGI("#Result:\n");
  if (!file_compare("/data/flowInData", "/data/flowOut0"))
    RKMEDIA_LOGI("\t Sync mode OK!\n");
  else {
    RKMEDIA_LOGI("\t Async mode Failed!\n");
    ret = -1;
  }

  if (!file_compare("/data/flowInData", "/data/flowOut1"))
    RKMEDIA_LOGI("\t Async mode OK!\n");
  else {
    RKMEDIA_LOGI("\t Sync mode Failed!\n");
    ret = -1;
  }

#if 0
  if (file_compare("/data/flowInData", "/data/flowOut2"))
    RKMEDIA_LOGI("\t Async mode OK!\n");
  else {
    RKMEDIA_LOGI("\t Sync mode Failed!\n");
    ret = -1;
  }
#endif

  return ret;
}

// ------------------------------------------------------------------
// TEST2: multi pipline test
//                      ->IO11 -
//              -> IO12 -      ->IO22 -> IO21 -> sink //ASYNC
//              -       ->IO11 -
//   src-> IO11 -
//              -       ->IO11 -
//              -> IO12 -      ->IO22 -> IO21 -> sink //ASYNC
//                      ->IO11 -
// ------------------------------------------------------------------

int multi_pipe_test() {
  easymedia::AutoPrintLine atuoprint(__func__);

  RKMEDIA_LOGI("========================================\n");
  RKMEDIA_LOGI("========= multi_pipe_test =============\n");
  RKMEDIA_LOGI("========================================\n");

  std::string flow_name = "mock_src_flow";
  std::string param;
  // Create source flow
  PARAM_STRING_APPEND(param, KEY_NAME, "src");
  PARAM_STRING_APPEND(param, KEY_PATH, "/data/flowInData");
  // can't be less then 3ms or will loss packet frequently
  PARAM_STRING_APPEND_TO(param, KEY_FPS, 100);
  RKMEDIA_LOGI("# src params:%s\n", param.c_str());
  auto src = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  assert(src);

  // Create IO11 flow
  flow_name = "mock_io_flow";
  param = "";
  PARAM_STRING_APPEND(param, KEY_NAME, "io11_total");
  PARAM_STRING_APPEND(param, KEY_IN_CNT, "1");
  PARAM_STRING_APPEND(param, KEY_OUT_CNT, "1");
  PARAM_STRING_APPEND(param, KEK_THREAD_SYNC_MODEL, KEY_ASYNCCOMMON);
  RKMEDIA_LOGI("# io params:%s\n", param.c_str());
  auto io11_tt = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  assert(io11_tt);

  // -----------------------
  // ASYNCOMMON pipline 0
  // -----------------------
  flow_name = "mock_io_flow";
  param = "";
  PARAM_STRING_APPEND(param, KEY_NAME, "p0_io11_0");
  PARAM_STRING_APPEND(param, KEY_IN_CNT, "1");
  PARAM_STRING_APPEND(param, KEY_OUT_CNT, "1");
  PARAM_STRING_APPEND(param, KEK_THREAD_SYNC_MODEL, KEY_ASYNCCOMMON);
  RKMEDIA_LOGI("# p0_io11_0 params:%s\n", param.c_str());
  auto p0_io11_0 = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  assert(p0_io11_0);

  flow_name = "mock_io_flow";
  param = "";
  PARAM_STRING_APPEND(param, KEY_NAME, "p0_io11_1");
  PARAM_STRING_APPEND(param, KEY_IN_CNT, "1");
  PARAM_STRING_APPEND(param, KEY_OUT_CNT, "1");
  PARAM_STRING_APPEND(param, KEK_THREAD_SYNC_MODEL, KEY_ASYNCCOMMON);
  RKMEDIA_LOGI("# p0_io11_1 params:%s\n", param.c_str());
  auto p0_io11_1 = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  assert(p0_io11_1);

  // Create IO12 flow
  flow_name = "mock_io_flow";
  param = "";
  PARAM_STRING_APPEND(param, KEY_NAME, "p0_io12");
  PARAM_STRING_APPEND(param, KEY_IN_CNT, "1");
  PARAM_STRING_APPEND(param, KEY_OUT_CNT, "2");
  PARAM_STRING_APPEND(param, KEK_THREAD_SYNC_MODEL, KEY_ASYNCCOMMON);
  RKMEDIA_LOGI("# p0_io12 params:%s\n", param.c_str());
  auto p0_io12 = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  assert(p0_io12);

  // Create IO22 flow
  flow_name = "mock_io_flow";
  param = "";
  PARAM_STRING_APPEND(param, KEY_NAME, "p0_io22");
  PARAM_STRING_APPEND(param, KEY_IN_CNT, "2");
  PARAM_STRING_APPEND(param, KEY_OUT_CNT, "2");
  PARAM_STRING_APPEND(param, KEK_THREAD_SYNC_MODEL, KEY_ASYNCCOMMON);
  RKMEDIA_LOGI("# p0_io22 params:%s\n", param.c_str());
  auto p0_io22 = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  assert(p0_io22);

  // Create IO21 flow
  flow_name = "mock_io_flow";
  param = "";
  PARAM_STRING_APPEND(param, KEY_NAME, "p0_io21");
  PARAM_STRING_APPEND(param, KEY_IN_CNT, "2");
  PARAM_STRING_APPEND(param, KEY_OUT_CNT, "1");
  PARAM_STRING_APPEND(param, KEK_THREAD_SYNC_MODEL, KEY_ASYNCCOMMON);
  RKMEDIA_LOGI("# p0_io21 params:%s\n", param.c_str());
  auto p0_io21 = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  assert(p0_io21);

  // Create sink flow
  flow_name = "mock_sink_flow";
  param = "";
  PARAM_STRING_APPEND(param, KEY_NAME, "p0_sink");
  PARAM_STRING_APPEND(param, KEY_PATH, "/data/flowOut0");
  PARAM_STRING_APPEND(param, KEY_IN_CNT, "1");
  PARAM_STRING_APPEND(param, KEK_THREAD_SYNC_MODEL, KEY_ASYNCCOMMON);
  RKMEDIA_LOGI("# sink params:%s\n", param.c_str());
  auto p0_sink = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  assert(p0_sink);

  // -----------------------
  // ASYNCOMMON pipline 1
  // -----------------------
  flow_name = "mock_io_flow";
  param = "";
  PARAM_STRING_APPEND(param, KEY_NAME, "p1_io11_0");
  PARAM_STRING_APPEND(param, KEY_IN_CNT, "1");
  PARAM_STRING_APPEND(param, KEY_OUT_CNT, "1");
  PARAM_STRING_APPEND(param, KEK_THREAD_SYNC_MODEL, KEY_ASYNCCOMMON);
  RKMEDIA_LOGI("# p1_io11_0 params:%s\n", param.c_str());
  auto p1_io11_0 = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  assert(p1_io11_0);

  flow_name = "mock_io_flow";
  param = "";
  PARAM_STRING_APPEND(param, KEY_NAME, "p1_io11_1");
  PARAM_STRING_APPEND(param, KEY_IN_CNT, "1");
  PARAM_STRING_APPEND(param, KEY_OUT_CNT, "1");
  PARAM_STRING_APPEND(param, KEK_THREAD_SYNC_MODEL, KEY_ASYNCCOMMON);
  RKMEDIA_LOGI("# p1_io11_1 params:%s\n", param.c_str());
  auto p1_io11_1 = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  assert(p1_io11_1);

  // Create IO12 flow
  flow_name = "mock_io_flow";
  param = "";
  PARAM_STRING_APPEND(param, KEY_NAME, "p1_io12");
  PARAM_STRING_APPEND(param, KEY_IN_CNT, "1");
  PARAM_STRING_APPEND(param, KEY_OUT_CNT, "2");
  PARAM_STRING_APPEND(param, KEK_THREAD_SYNC_MODEL, KEY_ASYNCCOMMON);
  RKMEDIA_LOGI("# p1_io12 params:%s\n", param.c_str());
  auto p1_io12 = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  assert(p1_io12);

  // Create IO22 flow
  flow_name = "mock_io_flow";
  param = "";
  PARAM_STRING_APPEND(param, KEY_NAME, "p1_io22");
  PARAM_STRING_APPEND(param, KEY_IN_CNT, "2");
  PARAM_STRING_APPEND(param, KEY_OUT_CNT, "2");
  PARAM_STRING_APPEND(param, KEK_THREAD_SYNC_MODEL, KEY_ASYNCCOMMON);
  RKMEDIA_LOGI("# p1_io22 params:%s\n", param.c_str());
  auto p1_io22 = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  assert(p1_io22);

  // Create IO21 flow
  flow_name = "mock_io_flow";
  param = "";
  PARAM_STRING_APPEND(param, KEY_NAME, "p1_io21");
  PARAM_STRING_APPEND(param, KEY_IN_CNT, "2");
  PARAM_STRING_APPEND(param, KEY_OUT_CNT, "1");
  PARAM_STRING_APPEND(param, KEK_THREAD_SYNC_MODEL, KEY_ASYNCCOMMON);
  RKMEDIA_LOGI("# p1_io21 params:%s\n", param.c_str());
  auto p1_io21 = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  assert(p1_io21);

  // Create sink flow
  flow_name = "mock_sink_flow";
  param = "";
  PARAM_STRING_APPEND(param, KEY_NAME, "p1_sink");
  PARAM_STRING_APPEND(param, KEY_PATH, "/data/flowOut1");
  PARAM_STRING_APPEND(param, KEY_IN_CNT, "1");
  PARAM_STRING_APPEND(param, KEK_THREAD_SYNC_MODEL, KEY_ASYNCCOMMON);
  RKMEDIA_LOGI("# sink params:%s\n", param.c_str());
  auto p1_sink = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  assert(p1_sink);

  RKMEDIA_LOGI("# Flows ptr:\n");
  RKMEDIA_LOGI("\t src:%p\n", src.get());
  RKMEDIA_LOGI("\t io11_tt:%p\n", io11_tt.get());
  RKMEDIA_LOGI("\t p0_io12:%p\n", p0_io12.get());
  RKMEDIA_LOGI("\t p0_io11_0:%p\n", p0_io11_0.get());
  RKMEDIA_LOGI("\t p0_io11_1:%p\n", p0_io11_1.get());
  RKMEDIA_LOGI("\t p0_io22:%p\n", p0_io22.get());
  RKMEDIA_LOGI("\t p0_io21:%p\n", p0_io21.get());
  RKMEDIA_LOGI("\t p0_sink:%p\n", p0_sink.get());
  RKMEDIA_LOGI("\t p1_io12:%p\n", p1_io12.get());
  RKMEDIA_LOGI("\t p1_io11_0:%p\n", p1_io11_0.get());
  RKMEDIA_LOGI("\t p1_io11_1:%p\n", p1_io11_1.get());
  RKMEDIA_LOGI("\t p1_io22:%p\n", p1_io22.get());
  RKMEDIA_LOGI("\t p1_io21:%p\n", p1_io21.get());
  RKMEDIA_LOGI("\t p1_sink:%p\n", p1_sink.get());

  src->AddDownFlow(io11_tt, 0, 0);
  // link pipeline 0
  io11_tt->AddDownFlow(p0_io12, 0, 0);
  p0_io12->AddDownFlow(p0_io11_0, 0, 0);
  p0_io12->AddDownFlow(p0_io11_1, 1, 0);
  p0_io11_0->AddDownFlow(p0_io22, 0, 0);
  p0_io11_1->AddDownFlow(p0_io22, 0, 1);
  p0_io22->AddDownFlow(p0_io21, 0, 0);
  p0_io22->AddDownFlow(p0_io21, 1, 1);
  p0_io21->AddDownFlow(p0_sink, 0, 0);
  // link pipeline 1
  io11_tt->AddDownFlow(p1_io12, 0, 0);
  p1_io12->AddDownFlow(p1_io11_0, 0, 0);
  p1_io12->AddDownFlow(p1_io11_1, 1, 0);
  p1_io11_0->AddDownFlow(p1_io22, 0, 0);
  p1_io11_1->AddDownFlow(p1_io22, 0, 1);
  p1_io22->AddDownFlow(p1_io21, 0, 0);
  p1_io22->AddDownFlow(p1_io21, 1, 1);
  p1_io21->AddDownFlow(p1_sink, 0, 0);

  while (g_need_sleep && !is_src_eos(src)) {
    easymedia::msleep(100);
  }
  // waite for all flow flush data.
  easymedia::msleep(100);
  src->RemoveDownFlow(io11_tt);
  // unlink pipeline0
  io11_tt->RemoveDownFlow(p0_io12);
  p0_io12->RemoveDownFlow(p0_io11_0);
  p0_io12->RemoveDownFlow(p0_io11_1);
  p0_io11_0->RemoveDownFlow(p0_io22);
  p0_io11_1->RemoveDownFlow(p0_io22);
  p0_io22->RemoveDownFlow(p0_io21);
  p0_io22->RemoveDownFlow(p0_io21);
  p0_io21->RemoveDownFlow(p0_sink);
  // unlink pipeline1
  io11_tt->RemoveDownFlow(p1_io12);
  p1_io12->RemoveDownFlow(p1_io11_0);
  p1_io12->RemoveDownFlow(p1_io11_1);
  p1_io11_0->RemoveDownFlow(p1_io22);
  p1_io11_1->RemoveDownFlow(p1_io22);
  p1_io22->RemoveDownFlow(p1_io21);
  p1_io22->RemoveDownFlow(p1_io21);
  p1_io21->RemoveDownFlow(p1_sink);

  src.reset();
  io11_tt.reset();
  p0_io12.reset();
  p0_io11_0.reset();
  p0_io11_1.reset();
  p0_io22.reset();
  p0_io21.reset();
  p0_sink.reset();
  p1_io12.reset();
  p1_io11_0.reset();
  p1_io11_1.reset();
  p1_io22.reset();
  p1_io21.reset();
  p1_sink.reset();

  int ret = 0;

  RKMEDIA_LOGI("## DUMP file md5:\n");
  ret = system("md5sum /data/flowInData");
  ret = system("md5sum /data/flowOut0");
  ret = system("md5sum /data/flowOut1");

  // compare sink and src data
  if (file_compare("/data/flowInData", "/data/flowOut0") ||
      file_compare("/data/flowInData", "/data/flowOut1"))
    ret = -1;

  return ret;
}

// ------------------------------------------------------------------
// TEST3: Dynamic add and remove flow test
// step1:
//                   -> IO11 -> sink
//        src-> IO22 -
//                   -> NULL
//
// step2: [add]
//                           -> sink
//                   -> IO11 -
//        src-> IO22 -       -> sink
//                   -> NULL
//
// setp3: [add]
//                           -> sink
//       src -       -> IO11 -
//           -> IO22 -       -> sink
//       src -       -> sink
//
// setp4: [remove]
//       src -       -> IO11 -> sink
//           -> IO22 -
//       src -       -> sink
// ------------------------------------------------------------------

int dynamic_pipe_test() {
  easymedia::AutoPrintLine atuoprint(__func__);

  RKMEDIA_LOGI("========================================\n");
  RKMEDIA_LOGI("========= dynamic_pipe_test ============\n");
  RKMEDIA_LOGI("========================================\n");

  //-----------------------
  // Step1
  //-----------------------
  std::string flow_name = "mock_src_flow";
  std::string param;
  // Create source flow
  PARAM_STRING_APPEND(param, KEY_NAME, "src0");
  PARAM_STRING_APPEND(param, KEY_PATH, "/data/flowInData");
  // can't be less then 3ms or will loss packet frequently
  PARAM_STRING_APPEND_TO(param, KEY_FPS, 100);
  RKMEDIA_LOGI("# src0 params:%s\n", param.c_str());
  auto src0 = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  assert(src0);

  // Create IO22 flow
  flow_name = "mock_io_flow";
  param = "";
  PARAM_STRING_APPEND(param, KEY_NAME, "io22");
  PARAM_STRING_APPEND(param, KEY_IN_CNT, "2");
  PARAM_STRING_APPEND(param, KEY_OUT_CNT, "2");
  PARAM_STRING_APPEND(param, KEK_THREAD_SYNC_MODEL, KEY_ASYNCCOMMON);
  RKMEDIA_LOGI("# io22 params:%s\n", param.c_str());
  auto io22 = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  assert(io22);

  // Create IO11 flow
  flow_name = "mock_io_flow";
  param = "";
  PARAM_STRING_APPEND(param, KEY_NAME, "io11");
  PARAM_STRING_APPEND(param, KEY_IN_CNT, "1");
  PARAM_STRING_APPEND(param, KEY_OUT_CNT, "1");
  PARAM_STRING_APPEND(param, KEK_THREAD_SYNC_MODEL, KEY_ASYNCCOMMON);
  RKMEDIA_LOGI("# io11 params:%s\n", param.c_str());
  auto io11 = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  assert(io11);

  // Create sink flow
  flow_name = "mock_sink_flow";
  param = "";
  PARAM_STRING_APPEND(param, KEY_NAME, "sink0");
  PARAM_STRING_APPEND(param, KEY_PATH, "/data/flowOut0");
  PARAM_STRING_APPEND(param, KEY_IN_CNT, "1");
  PARAM_STRING_APPEND(param, KEK_THREAD_SYNC_MODEL, KEY_ASYNCCOMMON);
  RKMEDIA_LOGI("# sink0 params:%s\n", param.c_str());
  auto sink0 = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  assert(sink0);

  io22->AddDownFlow(io11, 0, 0);
  io11->AddDownFlow(sink0, 0, 0);
  src0->AddDownFlow(io22, 0, 0);
  RKMEDIA_LOGI("## Step1 keep 2s ...\n");
  easymedia::msleep(2000);

  //-----------------------
  // Step2
  //-----------------------

  // Create sink flow
  flow_name = "mock_sink_flow";
  param = "";
  PARAM_STRING_APPEND(param, KEY_NAME, "sink1");
  PARAM_STRING_APPEND(param, KEY_PATH, "/data/flowOut1");
  PARAM_STRING_APPEND(param, KEY_IN_CNT, "1");
  PARAM_STRING_APPEND(param, KEK_THREAD_SYNC_MODEL, KEY_ASYNCCOMMON);
  RKMEDIA_LOGI("# sink1 params:%s\n", param.c_str());
  auto sink1 = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  assert(sink1);
  io11->AddDownFlow(sink1, 0, 0);
  RKMEDIA_LOGI("## Step2 keep 5s ...\n");
  easymedia::msleep(5000);

  //-----------------------
  // Step3
  //-----------------------

  // Create source flow
  flow_name = "mock_src_flow";
  param = "";
  PARAM_STRING_APPEND(param, KEY_NAME, "src1");
  PARAM_STRING_APPEND(param, KEY_PATH, "/data/flowInData");
  // can't be less then 3ms or will loss packet frequently
  PARAM_STRING_APPEND_TO(param, KEY_FPS, 100);
  RKMEDIA_LOGI("# src1 params:%s\n", param.c_str());
  auto src1 = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  assert(src1);

  // Create sink flow
  flow_name = "mock_sink_flow";
  param = "";
  PARAM_STRING_APPEND(param, KEY_NAME, "sink2");
  PARAM_STRING_APPEND(param, KEY_PATH, "/data/flowOut2");
  PARAM_STRING_APPEND(param, KEY_IN_CNT, "1");
  PARAM_STRING_APPEND(param, KEK_THREAD_SYNC_MODEL, KEY_ASYNCCOMMON);
  RKMEDIA_LOGI("# sink2 params:%s\n", param.c_str());
  auto sink2 = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  assert(sink2);

  io22->AddDownFlow(sink2, 1, 0);
  src1->AddDownFlow(io22, 0, 1);
  RKMEDIA_LOGI("## Step3 keep 5s ...\n");
  easymedia::msleep(5000);

  //-----------------------
  // Step4
  //-----------------------
  io11->RemoveDownFlow(sink1);
  sink1.reset();
  RKMEDIA_LOGI("## Step4 keep to end of stream ...\n");

  while (!is_src_eos(src0) || !is_src_eos(src1)) {
    easymedia::msleep(100);
  }
  // waite for all flow flush data.
  easymedia::msleep(100);

  src0->RemoveDownFlow(io22);
  src1->RemoveDownFlow(io22);
  io22->RemoveDownFlow(io11);
  io22->RemoveDownFlow(sink2);
  io11->RemoveDownFlow(sink0);
  src0.reset();
  src1.reset();
  io22.reset();
  io11.reset();
  src0.reset();
  src1.reset();
  sink0.reset();
  sink1.reset();
  sink2.reset();

  // waite for file sync.
  easymedia::msleep(100);

  int ret = 0;

  RKMEDIA_LOGI("## DUMP file md5:\n");
  ret = system("md5sum /data/flowInData");
  ret = system("md5sum /data/flowOut0");
  ret = system("md5sum /data/flowOut2");

  // flowOut0 should be equal flowInData
  // flowOut1 should be part of flowInData
  // flowOut2 should be equal flowInData
  if (file_compare("/data/flowInData", "/data/flowOut0") ||
      file_compare("/data/flowInData", "/data/flowOut2"))
    ret = -1;

  return ret;
}

// ------------------------------------------------------------------
// TEST4: Repeated destruction test
// ------------------------------------------------------------------

int creat_and_destory_test(int cnt) {
  easymedia::AutoPrintLine atuoprint(__func__);

  RKMEDIA_LOGI("========================================\n");
  RKMEDIA_LOGI("======= creat_and_destory_test =========\n");
  RKMEDIA_LOGI("========================================\n");
  g_need_sleep = 0;

  for (int i = 0; i < cnt; i++)
    multi_pipe_test();

  g_need_sleep = 1;
  return 0;
}

#if 0
// ------------------------------------------------------------------
//TEST5: multi install slotmap test
                             -> IO11 -> sink(V)
  src(a/v async audio video) -
                             -> IO11 -> sink(A)
// ------------------------------------------------------------------

int multi_slotmap_test() {
  // TODO...
}
#endif

int main() {

  if (single_pipe_test()) {
    RKMEDIA_LOGI("==> Single pipe test failed!\n");
    return 0;
  }

  if (multi_pipe_test()) {
    RKMEDIA_LOGI("==> Multi pipe test failed!\n");
    return 0;
  }

  if (dynamic_pipe_test()) {
    RKMEDIA_LOGI("==> Dynamic pipe test failed!\n");
    return 0;
  }

  creat_and_destory_test(100);

  return 0;
}
