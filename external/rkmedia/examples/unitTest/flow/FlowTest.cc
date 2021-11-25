// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <string>
#include <thread>

#include "easymedia/buffer.h"
#include "easymedia/flow.h"
#include "easymedia/media_reflector.h"
#include "easymedia/reflector.h"
#include "easymedia/utils.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::InSequence;
using ::testing::Return;

namespace easymedia {

#define KEY_IN_CNT "in_cnt"
#define KEY_OUT_CNT "out_cnt"

static bool do_src(Flow *f, MediaBufferVector &input_vector);
class MockSourceFlow : public Flow {
public:
  MockSourceFlow(const char *param);
  virtual ~MockSourceFlow();
  static const char *GetFlowName() { return "mock_src_flow"; }
  std::string GetName() const { return name_; }

private:
  void ReadThreadRun();
  bool loop_;
  std::thread *read_thread_;
  std::string name_;
  std::shared_ptr<MediaBuffer> buffer_;
  friend bool do_src(Flow *f, MediaBufferVector &input_vector) {
    MockSourceFlow *flow = static_cast<MockSourceFlow *>(f);
    RKMEDIA_LOGI("Do src %s\n", flow->GetName().c_str());
    int i = 0;
    for (auto in : input_vector) {
      flow->SetOutput(in, i++);
      RKMEDIA_LOGI("Doing in %s %p\n", flow->GetName().c_str(), in.get());
    }
    // Do nothing
    return true;
  }
};

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

  buffer_ = MediaBuffer::Alloc(320 * 240 * 3 / 2);

  if (!SetAsSource(std::vector<int>({0}), do_src, name_)) {
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
}

void MockSourceFlow::ReadThreadRun() {
  source_start_cond_mtx->lock();
  if (down_flow_num == 0)
    source_start_cond_mtx->wait();
  source_start_cond_mtx->unlock();
  while (loop_) {
    if (1) { // if (buffer_.use_count() == 1) {
      SendInput(buffer_, 0);
      RKMEDIA_LOGI("%s fake sending input %p\n", GetName().c_str(),
                   buffer_.get());
    } else {
      RKMEDIA_LOGI("buffer in use %ld\n", buffer_.use_count());
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(33));
  }
}

DEFINE_FLOW_FACTORY(MockSourceFlow, Flow)
const char *FACTORY(MockSourceFlow)::ExpectedInputDataType() { return nullptr; }
const char *FACTORY(MockSourceFlow)::OutPutDataType() { return ""; }

static bool do_io(Flow *f, MediaBufferVector &input_vector);
class MockIOFlow : public Flow {
public:
  MockIOFlow(const char *param);
  virtual ~MockIOFlow();
  static const char *GetFlowName() { return "mock_io_flow"; }
  std::string GetName() const { return name_; }
  int GetOutCnt() const { return out_; }

private:
  std::string name_;
  Model thread_model_;
  int in_;
  int out_;

  friend bool do_io(Flow *f, MediaBufferVector &input_vector) {
    MockIOFlow *flow = static_cast<MockIOFlow *>(f);
    RKMEDIA_LOGI("Do IO %s size input %u\n", flow->GetName().c_str(),
                 input_vector.size());
    for (auto in : input_vector) {
      if (!in) {
        return false;
      }
      for (int i = 0; i < flow->GetOutCnt(); i++) {
        RKMEDIA_LOGI("Doing %p in %s for %d\n", in.get(),
                     flow->GetName().c_str(), i);
        flow->SetOutput(in, i);
      }
    }
    // Do nothing
    return true;
  }
};

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
  if (sm.thread_model == Model::NONE)
    sm.thread_model = Model::ASYNCCOMMON;
  thread_model_ = sm.thread_model;
  if (sm.mode_when_full == InputMode::NONE)
    sm.mode_when_full = InputMode::DROPFRONT;
  for (int i = 0; i < in_; i++) {
    sm.input_slots.push_back(i);
    sm.input_maxcachenum.push_back(input_maxcachenum);
  }
  for (int i = 0; i < in_; i++)
    sm.output_slots.push_back(i);
  sm.process = do_io;
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
  virtual ~MockSinkFlow() {}
  static const char *GetFlowName() { return "mock_sink_flow"; }

  std::string GetName() const { return name_; }

private:
  std::string name_;
  Model thread_model_;
  friend bool do_out(Flow *f, MediaBufferVector &input_vector) {
    RKMEDIA_LOGI("Do OUT\n");
    MockSinkFlow *flow = static_cast<MockSinkFlow *>(f);
    auto &in = input_vector[0];
    if (!in) {
      return false;
    }
    // Do nothing
    RKMEDIA_LOGI("Doing %p in %s\n", in.get(), flow->GetName().c_str());
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

  SlotMap sm;
  int input_maxcachenum = 2;
  ParseParamToSlotMap(params, sm, input_maxcachenum);
  if (sm.thread_model == Model::NONE)
    sm.thread_model = Model::ASYNCCOMMON;
  thread_model_ = sm.thread_model;
  if (sm.mode_when_full == InputMode::NONE)
    sm.mode_when_full = InputMode::DROPCURRENT;
  sm.input_slots.push_back(0);
  sm.input_slots.push_back(1);
  sm.input_maxcachenum.push_back(input_maxcachenum);
  sm.input_maxcachenum.push_back(input_maxcachenum);
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

TEST(FlowTest, AddDownFlow) {
  std::string flow_name = "mock_src_flow";
  std::string param;

  // Create 1st source flow
  PARAM_STRING_APPEND(param, KEY_NAME, "src1");
  auto src1 = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  ASSERT_NE(src1, nullptr);

  // Create 2nd source flow
  param = "";
  PARAM_STRING_APPEND(param, KEY_NAME, "src2");
  auto src2 = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  ASSERT_NE(src2, nullptr);

  // Create 1st IO flow
  flow_name = "mock_io_flow";
  param = "";
  PARAM_STRING_APPEND(param, KEY_NAME, "io1");
  PARAM_STRING_APPEND(param, KEY_IN_CNT, "2");
  PARAM_STRING_APPEND(param, KEY_OUT_CNT, "2");
  auto io1 = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  ASSERT_NE(io1, nullptr);

  // Create 2-1 IO flow
  flow_name = "mock_io_flow";
  param = "";
  PARAM_STRING_APPEND(param, KEY_NAME, "io21");
  PARAM_STRING_APPEND(param, KEY_IN_CNT, "1");
  PARAM_STRING_APPEND(param, KEY_OUT_CNT, "1");
  auto io21 = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  ASSERT_NE(io21, nullptr);

  // Create 2-1 IO flow
  flow_name = "mock_io_flow";
  param = "";
  PARAM_STRING_APPEND(param, KEY_NAME, "io22");
  PARAM_STRING_APPEND(param, KEY_IN_CNT, "1");
  PARAM_STRING_APPEND(param, KEY_OUT_CNT, "1");
  auto io22 = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  ASSERT_NE(io22, nullptr);

  // Create final sink flow
  flow_name = "mock_sink_flow";
  param = "";
  PARAM_STRING_APPEND(param, KEY_NAME, "sink");
  auto sink = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), param.c_str());
  ASSERT_NE(sink, nullptr);

  // --> src1 --> io1 --> io21 --> sink -->
  //              | |                |
  // --> src2 --->| |---> io22 ----->|
  EXPECT_EQ(io21->AddDownFlow(sink, 0, 0), true);
  EXPECT_EQ(io21->GetError(), 0);
  EXPECT_EQ(io22->AddDownFlow(sink, 0, 1), true);
  EXPECT_EQ(io22->GetError(), 0);
  EXPECT_EQ(io1->AddDownFlow(io21, 0, 0), true);
  EXPECT_EQ(io1->GetError(), 0);
  EXPECT_EQ(io1->AddDownFlow(io22, 1, 0), true);
  EXPECT_EQ(io1->GetError(), 0);
  EXPECT_EQ(src1->AddDownFlow(io1, 0, 0), true);
  EXPECT_EQ(src1->GetError(), 0);
  EXPECT_EQ(src2->AddDownFlow(io1, 0, 1), true);
  EXPECT_EQ(src2->GetError(), 0);
  int i = 0;
  while (i++ < 100) {
    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }
  io22->RemoveDownFlow(sink);
  io21->RemoveDownFlow(sink);
  sink.reset();
  io1->RemoveDownFlow(io22);
  io22.reset();
  io1->RemoveDownFlow(io21);
  io21.reset();
  src1->RemoveDownFlow(io1);
  src2->RemoveDownFlow(io1);
  io1.reset();
  src1.reset();
  src2.reset();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
