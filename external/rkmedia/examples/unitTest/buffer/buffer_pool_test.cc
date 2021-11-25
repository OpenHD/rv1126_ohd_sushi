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
#include "utils.h"

void release_pool_buffer(easymedia::BufferPool *pool) {
  int i = 100;

  while (i-- > 0) {
    auto mb1 = pool->GetBuffer();
    if (mb1) {
      RKMEDIA_LOGI("T1: get msg: ptr:%p, fd:%d, size:%zu\n", mb1->GetPtr(),
                   mb1->GetFD(), mb1->GetSize());
      easymedia::usleep(50000);
    } else {
      RKMEDIA_LOGE("T1: get msb failed!\n");
      pool->DumpInfo();
    }
  }
}

int main() {
  LOG_INIT();

  easymedia::BufferPool pool(10, 1024,
                             easymedia::MediaBuffer::MemType::MEM_COMMON);
  ;

  RKMEDIA_LOGI("#001 Dump Info....\n");
  pool.DumpInfo();

  RKMEDIA_LOGI("--> Alloc 1 buffer from buffer pool\n");
  auto mb0 = pool.GetBuffer();
  RKMEDIA_LOGI("--> mb0: ptr:%p, fd:%d, size:%zu\n", mb0->GetPtr(),
               mb0->GetFD(), mb0->GetSize());

  RKMEDIA_LOGI("#002 Dump Info....\n");
  pool.DumpInfo();

  RKMEDIA_LOGI("--> reset mb0\n");
  mb0.reset();

  RKMEDIA_LOGI("#003 Dump Info....\n");
  pool.DumpInfo();

  std::thread *thread = new std::thread(release_pool_buffer, &pool);

  int i = 100;
  std::list<std::shared_ptr<easymedia::MediaBuffer>> list;
  while (i-- > 0) {
    mb0 = pool.GetBuffer();
    if (mb0) {
      RKMEDIA_LOGI("T0: get msg: ptr:%p, fd:%d, size:%zu\n", mb0->GetPtr(),
                   mb0->GetFD(), mb0->GetSize());
      easymedia::usleep(50000);
    } else {
      RKMEDIA_LOGE("T0: get msb failed!\n");
      pool.DumpInfo();
    }

    list.push_back(mb0);
    if (list.size() >= 10) {
      RKMEDIA_LOGI("--> List size:%zu, sleep 5s....\n", list.size());
      easymedia::usleep(5000000);
      int j = 0;
      while (list.size()) {
        RKMEDIA_LOGI("--> (%d) Free 1 msg from list, sleep 5s...\n", j++);
        list.pop_front();
        easymedia::usleep(5000000);
      }
    }
  }

  thread->join();
  RKMEDIA_LOGI("===== FINISH ====\n");
  return 0;
}
