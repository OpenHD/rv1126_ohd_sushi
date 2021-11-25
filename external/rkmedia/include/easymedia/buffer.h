// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EASYMEDIA_BUFFER_H_
#define EASYMEDIA_BUFFER_H_

#include <stdint.h>
#include <string.h>
#include <sys/time.h>

#include <memory>
#include <string>
#include <vector>

#include "image.h"
#include "lock.h"
#include "media_type.h"
#include "rknn_user.h"
#include "sound.h"

#define NUM_DATA_POINTERS 8

typedef int (*DeleteFun)(void *arg);

namespace easymedia {

/* memory type definitions. */
enum drm_rockchip_gem_mem_type {
  /* Physically Continuous memory. */
  ROCKCHIP_BO_CONTIG = 1 << 0,
  /* cachable mapping. */
  ROCKCHIP_BO_CACHABLE = 1 << 1,
  /* write-combine mapping. */
  ROCKCHIP_BO_WC = 1 << 2,
  ROCKCHIP_BO_SECURE = 1 << 3,
  ROCKCHIP_BO_MASK = ROCKCHIP_BO_CONTIG | ROCKCHIP_BO_CACHABLE | ROCKCHIP_BO_WC
};

#ifdef RKMEDIA_TIMESTAMP_DEBUG
class TimeStampNode {
public:
  TimeStampNode(std::string name, int64_t ts) : NodeName(name), NodeTs(ts) {}
  TimeStampNode(const TimeStampNode &TsNode) {
    NodeName = TsNode.NodeName;
    NodeTs = TsNode.NodeTs;
  }

  ~TimeStampNode() = default;

  TimeStampNode operator=(const TimeStampNode &TsNode) {
    if (this != &TsNode) {
      NodeName = TsNode.NodeName;
      NodeTs = TsNode.NodeTs;
    }
    return *this;
  }

  std::string NodeName;
  int64_t NodeTs;
};

class TimeStampRecorder {
public:
  TimeStampRecorder() { TsNodeList.clear(); }

  ~TimeStampRecorder() { TsNodeList.clear(); }

  void Record(std::string name, int64_t ts) {
    TsNodeListMtx.lock();
    TsNodeList.push_back(TimeStampNode(name, ts));
    TsNodeListMtx.unlock();
  }

  void Reset() {
    TsNodeListMtx.lock();
    TsNodeList.clear();
    TsNodeListMtx.unlock();
  }

  void Dump() {
    int64_t ts_pre = 0;
    int64_t ts_cur = 0;
    TsNodeListMtx.lock();
    for (size_t i = 0; i < TsNodeList.size(); i++) {
      ts_cur = TsNodeList[i].NodeTs;
      printf("#[%s]:[%lld]: %0.3fms\n", TsNodeList[i].NodeName.c_str(),
             TsNodeList[i].NodeTs, (ts_cur - ts_pre) / 1000.0);
      ts_pre = ts_cur;
    }
    TsNodeListMtx.unlock();
  }

  std::vector<TimeStampNode> TsNodeList;
  std::mutex TsNodeListMtx;
};
#endif // RKMEDIA_TIMESTAMP_DEBUG

// wrapping existing buffer
class _API MediaBuffer {
public:
  // video flags
  static const uint32_t kExtraIntra = (1 << 0); // special, such as sps pps
  static const uint32_t kIntra = (1 << 1);
  static const uint32_t kPredicted = (1 << 2);
  static const uint32_t kBiPredictive = (1 << 3);
  static const uint32_t kBiDirectional = (1 << 4);
  static const uint32_t kSingleNalUnit = (1 << 5);

  // special flags
  static const uint32_t kBuildinLibvorbisenc = (1 << 16);

  MediaBuffer()
      : ptr(nullptr), size(0), fd(-1), handle(0), dev_fd(-1), valid_size(0),
        type(Type::None), user_flag(0), ustimestamp(0), eof(false),
        tsvc_level(-1), dbg_info(nullptr) {
#ifdef RKMEDIA_TIMESTAMP_DEBUG
    TsRecorder = std::make_shared<TimeStampRecorder>();
#endif
  }
  // Set userdata and delete function if you want free resource when destrut.
  MediaBuffer(void *buffer_ptr, size_t buffer_size, int buffer_fd = -1,
              int buffer_handle = 0, int device_fd = -1,
              void *user_data = nullptr, DeleteFun df = nullptr)
      : ptr(buffer_ptr), size(buffer_size), fd(buffer_fd),
        handle(buffer_handle), dev_fd(device_fd), valid_size(0),
        type(Type::None), user_flag(0), ustimestamp(0), eof(false),
        tsvc_level(-1), dbg_info(nullptr) {
    SetUserData(user_data, df);
#ifdef RKMEDIA_TIMESTAMP_DEBUG
    TsRecorder = std::make_shared<TimeStampRecorder>();
#endif
  }
  virtual ~MediaBuffer() = default;
  virtual PixelFormat GetPixelFormat() const { return PIX_FMT_NONE; }
  virtual SampleFormat GetSampleFormat() const { return SAMPLE_FMT_NONE; }
  void BeginCPUAccess(bool readonly);
  void EndCPUAccess(bool readonly);
  int GetDevFD() const { return dev_fd; }
  void SetDevFD(int new_dev_fd) { dev_fd = new_dev_fd; };
  int GetHandle() const { return handle; }
  void SetHandle(int new_handle) { handle = new_handle; }
  int GetFD() const { return fd; }
  void SetFD(int new_fd) { fd = new_fd; }
  void *GetPtr() const { return ptr; }
  void SetPtr(void *addr) { ptr = addr; }
  void **GetPtrArrayBase() { return ptrArray; }
  void *GetPtrArray(size_t index) const { return ptrArray[index]; }
  void SetPtrArray(void *addr, size_t index) { ptrArray[index] = addr; }
  size_t GetSize() const { return size; }
  void SetSize(size_t s) { size = s; }
  size_t GetValidSize() const { return valid_size; }
  void SetValidSize(size_t s) { valid_size = s; }
  Type GetType() const { return type; }
  // be careful to set type, depends on final buffer class.
  // Maybe it should be set Protected.
  void SetType(Type t) { type = t; }
  uint32_t GetUserFlag() const { return user_flag; }
  void SetUserFlag(uint32_t flag) { user_flag = flag; }
  // microsecond
  int64_t GetUSTimeStamp() const { return ustimestamp; }
  struct timeval GetTimeVal() const {
    struct timeval ret;
    ret.tv_sec = ustimestamp / 1000000LL;
    ret.tv_usec = ustimestamp % 1000000LL;
    return ret;
  }
  void SetUSTimeStamp(int64_t us) { ustimestamp = us; }
  void SetTimeVal(const struct timeval &val) {
    ustimestamp = val.tv_sec * 1000000LL + val.tv_usec;
  }
  bool IsEOF() const { return eof; }
  void SetEOF(bool val) { eof = val; }
  int GetTsvcLevel() { return tsvc_level; }
  void SetTsvcLevel(int _level) { tsvc_level = _level; }

  void SetUserData(void *user_data, DeleteFun df) {
    if (user_data) {
      if (df)
        userdata.reset(user_data, df);
      else
        userdata.reset(user_data, [](void *) {}); // do nothing when delete
    } else {
      userdata.reset();
    }
  }
  int64_t GetAtomicClock() const { return atomic_clock; }
  struct timeval GetAtomicTimeVal() const {
    struct timeval ret;
    ret.tv_sec = atomic_clock / 1000000LL;
    ret.tv_usec = atomic_clock % 1000000LL;
    return ret;
  }
  void SetAtomicClock(int64_t us) { atomic_clock = us; }
  void SetAtomicTimeVal(const struct timeval &val) {
    atomic_clock = val.tv_sec * 1000000LL + val.tv_usec;
  }

  void SetUserData(std::shared_ptr<void> user_data) { userdata = user_data; }
  std::shared_ptr<void> GetUserData() { return userdata; }

  void SetRelatedSPtr(const std::shared_ptr<void> &rdata, int index = -1) {
    if (index < 0) {
      related_sptrs.push_back(rdata);
      return;
    } else if (index >= (int)related_sptrs.size()) {
      related_sptrs.resize(index + 1);
    }
    related_sptrs[index] = rdata;
  }
  std::vector<std::shared_ptr<void>> &GetRelatedSPtrs() {
    return related_sptrs;
  }

  bool IsValid() { return valid_size > 0; }
  bool IsHwBuffer() { return fd >= 0; }

  enum class MemType {
    MEM_COMMON,
    MEM_HARD_WARE,
  };
  static std::shared_ptr<MediaBuffer>
  Alloc(size_t size, MemType type = MemType::MEM_COMMON,
        unsigned int flag = ROCKCHIP_BO_CACHABLE);
  static MediaBuffer Alloc2(size_t size, MemType type = MemType::MEM_COMMON,
                            unsigned int flag = ROCKCHIP_BO_CACHABLE);
  static std::shared_ptr<MediaBuffer>
  Clone(MediaBuffer &src, MemType dst_type = MemType::MEM_COMMON);
  static std::shared_ptr<MediaBuffer>
  Clone2(MediaBuffer &src, MemType dst_type = MemType::MEM_COMMON);

  void *GetDbgInfo() const { return dbg_info; }
  void SetDbgInfo(void *addr) { dbg_info = addr; }
  size_t GetDbgInfoSize() const { return dbg_info_size; }
  void SetDbgInfoSize(size_t s) { dbg_info_size = s; }

#ifdef RKMEDIA_TIMESTAMP_DEBUG
  void TimeStampRecord(std::string name, int64_t ts) {
    TsRecorder->Record(name, ts);
  }
  void TimeStampCopy(std::shared_ptr<MediaBuffer> mb) {
    TsRecorder = mb->TsRecorder;
  }
  void TimeStampReset() { TsRecorder->Reset(); }
  void TimeStampDump() { TsRecorder->Dump(); }

  std::shared_ptr<TimeStampRecorder> TsRecorder;
#endif // RKMEDIA_TIMESTAMP_DEBUG

private:
  // copy attributs except buffer
  void CopyAttribute(MediaBuffer &src_attr);

  void *ptr; // buffer virtual address
  void
      *ptrArray[NUM_DATA_POINTERS]; // pointer to the channel planes, like
                                    // AV_NUM_DATA_POINTERS in libavutil/frame.h
  size_t size;
  int fd;            // buffer fd
  int handle;        // buffer handle
  int dev_fd;        // device fd
  size_t valid_size; // valid data size, less than above size
  Type type;
  uint32_t user_flag;
  int64_t ustimestamp;
  int64_t atomic_clock;
  bool eof;
  int tsvc_level; // for avc/hevc encoder
  void *dbg_info; // Point to extra info to debug buffer. Note: user release.
  size_t dbg_info_size; // Debug info size.
  std::shared_ptr<void> userdata;
  std::vector<std::shared_ptr<void>> related_sptrs;
};

MediaBuffer::MemType StringToMemType(const char *s);

// Audio sample buffer
class _API SampleBuffer : public MediaBuffer {
public:
  SampleBuffer() { ResetValues(); }
  SampleBuffer(const MediaBuffer &buffer, SampleFormat fmt = SAMPLE_FMT_NONE)
      : MediaBuffer(buffer) {
    ResetValues();
    sample_info.fmt = fmt;
  }
  SampleBuffer(const MediaBuffer &buffer, const SampleInfo &info)
      : MediaBuffer(buffer), sample_info(info) {
    SetType(Type::Audio);
  }
  virtual ~SampleBuffer() = default;
  virtual SampleFormat GetSampleFormat() const override {
    return sample_info.fmt;
  }

  SampleInfo &GetSampleInfo() { return sample_info; }
  size_t GetSampleSize() const { return ::GetSampleSize(sample_info); }
  void SetSamples(int num) {
    sample_info.nb_samples = num;
    SetValidSize(num * GetSampleSize());
  }
  int GetSamples() const { return sample_info.nb_samples; }
  void SetChannels(int num) { sample_info.channels = num; }
  int GetChannels() { return sample_info.channels; }

private:
  void ResetValues() {
    SetType(Type::Audio);
    memset(&sample_info, 0, sizeof(sample_info));
    sample_info.fmt = SAMPLE_FMT_NONE;
  }
  SampleInfo sample_info;
};

// Image buffer
class _API ImageBuffer : public MediaBuffer {
public:
  ImageBuffer() { ResetValues(); }
  ImageBuffer(const MediaBuffer &buffer) : MediaBuffer(buffer) {
    ResetValues();
  }
  ImageBuffer(const MediaBuffer &buffer, const ImageInfo &info)
      : MediaBuffer(buffer), image_info(info) {
    SetType(Type::Image);
    // if set a valid info, set valid size
    size_t s = CalPixFmtSize(info);
    if (s > 0)
      SetValidSize(s);
  }
  virtual ~ImageBuffer() = default;
  virtual PixelFormat GetPixelFormat() const override {
    return image_info.pix_fmt;
  }
  int GetWidth() const { return image_info.width; }
  int GetHeight() const { return image_info.height; }
  int GetVirWidth() const { return image_info.vir_width; }
  int GetVirHeight() const { return image_info.vir_height; }
  ImageInfo &GetImageInfo() { return image_info; }
  std::list<RknnResult> &GetRknnResult() { return nn_result; };

private:
  void ResetValues() {
    SetType(Type::Image);
    memset(&image_info, 0, sizeof(image_info));
    image_info.pix_fmt = PIX_FMT_NONE;
  }
  ImageInfo image_info;
  std::list<RknnResult> nn_result;
};

class MediaGroupBuffer {
public:
  MediaGroupBuffer()
      : pool(nullptr), ptr(nullptr), size(0), fd(-1), handle(0), dev_fd(-1) {}
  // Set userdata and delete function if you want free resource when destrut.
  MediaGroupBuffer(void *buffer_ptr, size_t buffer_size, int buffer_fd = -1,
                   int buffer_handle = 0, int device_fd = -1,
                   void *user_data = nullptr, DeleteFun df = nullptr)
      : pool(nullptr), ptr(buffer_ptr), size(buffer_size), fd(buffer_fd),
        handle(buffer_handle), dev_fd(device_fd) {
    SetUserData(user_data, df);
  }
  virtual ~MediaGroupBuffer() = default;

  void SetUserData(void *user_data, DeleteFun df) {
    if (user_data) {
      if (df)
        userdata.reset(user_data, df);
      else
        userdata.reset(user_data, [](void *) {}); // do nothing when delete
    } else {
      userdata.reset();
    }
  }

  void SetBufferPool(void *bp) { pool = bp; }

  int GetFD() const { return fd; }
  void *GetPtr() const { return ptr; }
  size_t GetSize() const { return size; }
  int GetHandle() const { return handle; }
  int GetDevFD() const { return dev_fd; }

  static MediaGroupBuffer *
  Alloc(size_t size,
        MediaBuffer::MemType type = MediaBuffer::MemType::MEM_COMMON,
        unsigned int flag = ROCKCHIP_BO_CACHABLE);

public:
  void *pool;

private:
  void *ptr; // buffer virtual address
  size_t size;
  int fd;     // buffer fd
  int handle; // buffer handle
  int dev_fd; // device fd

  std::shared_ptr<void> userdata;
};

class _API BufferPool {
public:
  BufferPool(int cnt, int size, MediaBuffer::MemType type);
  BufferPool(int cnt, int size, MediaBuffer::MemType type, unsigned int flag);
  ~BufferPool();

  std::shared_ptr<MediaBuffer> GetBuffer(bool block = true);
  int PutBuffer(MediaGroupBuffer *mgb);

  void DumpInfo();

private:
  std::list<MediaGroupBuffer *> ready_buffers;
  std::list<MediaGroupBuffer *> busy_buffers;
  ConditionLockMutex mtx;
  int buf_cnt;
  int buf_size;
};

} // namespace easymedia

#endif // EASYMEDIA_BUFFER_H_
