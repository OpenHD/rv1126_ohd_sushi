// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef SHM_CONTROL_H_
#define SHM_CONTROL_H_

#include <stdint.h>
#include <vector>
#include "shmc/shm_queue.h"
#include <thread>
#include <mutex>


namespace {
constexpr const char* kShmKey = "0x10007";
}  // namespace
using namespace shmc;

namespace rndis_server {

class ShmControl {
 public:
  ShmControl();
  ~ShmControl() {}
 void StartSendThread();
  bool Stop();
  void StopUvcEngine(bool stop);
  bool isStopUvcEngine(){return stop_uvc_engine;}


    shmc::ShmQueue<SVIPC> queue_r_;
    std::mutex queue_mtx;
    shmc::SVIPC alloc_;
    std::thread *send_process;
private:
    bool stop_uvc_engine;
};

}  // namespace rndis_server

#endif  // SHM_CONTROL_H_

