// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stream.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>

#include "media_type.h"
#include "utils.h"

namespace easymedia {

#define CHECK_FILE(f)                                                          \
  if (!f) {                                                                    \
    errno = EBADF;                                                             \
    return -1;                                                                 \
  }

class FileStream : public Stream {
public:
  FileStream(const char *param);
  virtual ~FileStream() {
    if (file)
      FileStream::Close();
  }
  virtual size_t Read(void *ptr, size_t size, size_t nmemb) final {
    if (!Readable())
      return -1;
    CHECK_FILE(file)
    return fread(ptr, size, nmemb, file);
  }
  virtual int Seek(int64_t offset, int whence) final {
    if (!Seekable())
      return -1;
    CHECK_FILE(file)
    return fseek(file, offset, whence);
  }
  virtual long Tell() final {
    CHECK_FILE(file)
    return ftell(file);
  }
  virtual size_t Write(const void *ptr, size_t size, size_t nmemb) final {
    if (!Writeable())
      return -1;
    CHECK_FILE(file)
    return fwrite(ptr, size, nmemb, file);
  }
  virtual size_t WriteAndClose(const void *ptr, size_t size,
                               size_t nmemb) final {
    if (!Writeable())
      return -1;
    CHECK_FILE(file)
    fwrite(ptr, size, nmemb, file);
    return Close();
  }

  virtual bool Eof() final {
    if (!file) {
      errno = EBADF;
      return true;
    }
    return eof ? eof : !!feof(file);
  }

  virtual int NewStream(std::string new_path) {
    Close();
    path = new_path;
    RKMEDIA_LOGI("NewStream file:%s\n", new_path.c_str());
    return Open();
  }

  virtual int ReName(std::string old_path, std::string new_path) {
    int ret;
    Close();
    ret = rename(old_path.c_str(), new_path.c_str());
    if (ret)
      return ret;
    path = new_path;
    return Open();
  };

protected:
  virtual int Open() {
    if (open_late) {
      open_late = 0;
      return 0;
    }
    if (path.empty() || open_mode.empty())
      return -1;
    file = fopen(path.c_str(), open_mode.c_str());
    if (!file)
      return -1;
    eof = false;
    SetReadable(true);
    SetWriteable(true);
    SetSeekable(true);
    return 0;
  }
  virtual int Close() final {
    if (!file) {
      errno = EBADF;
      return EOF;
    }
    eof = true;
    int ret = fclose(file);
    file = NULL;
    return ret;
  }

private:
  std::string path;
  std::string open_mode;
  std::string save_mode;
  FILE *file;
  bool eof;
  int open_late;
};

FileStream::FileStream(const char *param)
    : file(NULL), eof(true), open_late(0) {
  std::map<std::string, std::string> params;
  std::list<std::pair<const std::string, std::string &>> req_list;
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_PATH, path));
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_OPEN_MODE, open_mode));
  int ret = parse_media_param_match(param, params, req_list);
  save_mode = params[KEY_SAVE_MODE];
  if (save_mode.empty())
    save_mode = KEY_SAVE_MODE_CONTIN;
  if (save_mode == KEY_SAVE_MODE_SINGLE)
    open_late = 1;
  UNUSED(ret);
}

// FileWriteStream
class FileWriteStream : public FileStream {
public:
  FileWriteStream(const char *param) : FileStream(param) {}
  virtual ~FileWriteStream() = default;
  static const char *GetStreamName() { return "file_write_stream"; }
  virtual int Open() final {
    int ret = FileStream::Open();
    if (!ret)
      SetReadable(false);
    return ret;
  }
};

DEFINE_STREAM_FACTORY(FileWriteStream, Stream)

const char *FACTORY(FileWriteStream)::ExpectedInputDataType() {
  return TYPE_ANYTHING;
}

const char *FACTORY(FileWriteStream)::OutPutDataType() { return STREAM_FILE; }

// FileReadStream
class FileReadStream : public FileStream {
public:
  FileReadStream(const char *param) : FileStream(param) {}
  virtual ~FileReadStream() = default;
  static const char *GetStreamName() { return "file_read_stream"; }
  virtual int Open() final {
    int ret = FileStream::Open();
    if (!ret)
      SetWriteable(false);
    return ret;
  }
};

DEFINE_STREAM_FACTORY(FileReadStream, Stream)

const char *FACTORY(FileReadStream)::ExpectedInputDataType() {
  return STREAM_FILE;
}

const char *FACTORY(FileReadStream)::OutPutDataType() { return TYPE_ANYTHING; }

} // namespace easymedia
