// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef NDEBUG
#undef NDEBUG
#endif
#ifndef DEBUG
#define DEBUG
#endif

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "buffer.h"
#include "key_string.h"
#include "media_config.h"
#include "utils.h"

#include "decoder.h"

#define MAX_FILE_NUM 9
static char optstr[] = "?i:d:f:b:o:t:w:h:";

static bool eos = true;
static int output_file_fd = -1;

static void dump_output(const std::shared_ptr<easymedia::MediaBuffer> &out) {
  auto out_image = std::static_pointer_cast<easymedia::ImageBuffer>(out);
  // hardware always need aligh width/height, we write the whole buffer with
  // virtual region which may contains invalid data
  if (output_file_fd >= 0 && out_image->GetValidSize() > 0) {
    const ImageInfo &info = out_image->GetImageInfo();
    fprintf(stderr, "got one frame, format: %s <%dx%d>in<%dx%d>\n",
            PixFmtToString(info.pix_fmt), info.width, info.height,
            info.vir_width, info.vir_height);
    ssize_t count = write(output_file_fd, out_image->GetPtr(),
                          CalPixFmtSize(out_image->GetPixelFormat(),
                                        out_image->GetVirWidth(),
                                        out_image->GetVirHeight()));
    if (count < 0) {
      RKMEDIA_LOGI("dump_output: write output_file_fd failed\n");
    }
  }
}

static bool get_output_and_process(easymedia::VideoDecoder *mpp_dec) {
  auto out_buffer = mpp_dec->FetchOutput();
  if (!out_buffer && errno != 0) {
    fprintf(stderr, "fatal error %m\n");
    eos = true; // finish
    return false;
  }
  if (out_buffer && !out_buffer->IsValid() && !out_buffer->IsEOF()) {
    // got a image info buffer
    fprintf(stderr, "got info frame\n");
    // fetch the real frame
    out_buffer = mpp_dec->FetchOutput();
  }
  if (out_buffer) {
    dump_output(std::static_pointer_cast<easymedia::ImageBuffer>(out_buffer));
    if (out_buffer->IsEOF()) {
      eos = true;
      return false; // finish
    }
  }
  return true;
}

static void out_loop(void *argv) {
  easymedia::VideoDecoder *dec = (easymedia::VideoDecoder *)argv;
  while (!eos) {
    if (!get_output_and_process(dec))
      break;
  }
}

static int flen(FILE *f) {
  int len = fseek(f, 0, SEEK_END);
  if (len) {
    fprintf(stderr, "fseek to end failed, %m\n");
    return -1;
  }
  len = ftell(f);
  assert(len > 0);
  rewind(f);
  return len;
}

int main(int argc, char **argv) {
  int c;
  std::string input_dir_path;
  std::string input_file_path;
  std::string output_file_path;
  std::string input_format;
  bool bframe = true;
  bool async = false; // default sync
  int width = 0, height = 0;

  opterr = 1;
  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 'i':
      input_file_path = optarg;
      printf("input file path: %s\n", input_file_path.c_str());
      break;
    case 'd':
      input_dir_path = optarg;
      printf("input directory path: %s\n", input_dir_path.c_str());
      break;
    case 'b':
      bframe = !!atoi(optarg);
      break;
    case 'o':
      output_file_path = optarg;
      printf("output directory path: %s\n", output_file_path.c_str());
      break;
    case 't':
      async = !!atoi(optarg);
      break;
    case 'f':
      input_format = optarg;
      printf("input format: %s\n", input_format.c_str());
      if (input_format == "jpg" || input_format == "jpeg")
        input_format = IMAGE_JPEG;
      else if (input_format == "h264")
        input_format = VIDEO_H264;
      else if (input_format == "h265")
        input_format = VIDEO_H265;
      break;
    case 'w':
      width = atoi(optarg);
      break;
    case 'h':
      height = atoi(optarg);
      break;
    case '?':
    default:
      printf("usage example: \n");
      printf("rkmpp_dec_test -i mpp_dec_test.h264 -f h264 -o output.nv12 -t "
             "0/1\n");
      printf("rkmpp_dec_test -i mpp_dec_test.hevc -f h265 -o output.nv12 -t "
             "0/1\n");
      printf("rkmpp_dec_test -i mpp_dec_test.jpg -f jpeg -w <width> -h "
             "<height> -o output.img\n");
      printf("rkmpp_dec_test -d h264_frames_dir -f h264 -o output.nv12 -t "
             "0/1\n\n");
      printf("On PC:\n\t"
             "ffplay -f rawvideo -video_size <vir_width>x<vir_height> "
             "-pixel_format nv12 -framerate 25 output.nv12\n\t(jpeg output may "
             "not nv12, depend its raw data format in input file)\n");
      exit(0);
    }
  }

  if (input_dir_path.empty() && input_file_path.empty())
    assert(0);
  if (input_format.empty())
    assert(0);
  // the files in input_dir_path be supposed one slice frame,
  // so that split mode is not nessary.
  int split_mode = input_dir_path.empty() ? 1 : 0;
  int timeout = -1; // blocking is not a good solution as input is complex,
                    // you must guaranteed that input is not too bad
  if (async) {
    timeout = 200; // ms
  } else {
    // if block, must have split data before send data to mpp

    // make it no wait when getting decoded frame from mpp
    if (split_mode)
      timeout = 0;
  }
  std::string param;
  PARAM_STRING_APPEND(param, KEY_INPUTDATATYPE, input_format);
  if (!bframe && input_format == VIDEO_H264) {
    // if your h264 no b frame, set 4 will save memory
    PARAM_STRING_APPEND_TO(param, KEY_MPP_GROUP_MAX_FRAMES, 4);
  }
  bool only_sync_mode = false;
  if (input_format == IMAGE_JPEG) {
    // if jpeg, make it be internally managed by mpp
    PARAM_STRING_APPEND_TO(param, KEY_MPP_GROUP_MAX_FRAMES, 0);
    split_mode = 0;
    // NOTE: !!now only support sync call for mpp jpeg dec
    only_sync_mode = true;
    async = false;
    if (input_file_path.empty()) {
      fprintf(stderr, "sorry, pls set one file path for jpeg\n");
      exit(EXIT_FAILURE);
    }
    // need w/h
    assert(width > 0 && height > 0);
  }
  PARAM_STRING_APPEND_TO(param, KEY_MPP_SPLIT_MODE, split_mode);
  PARAM_STRING_APPEND_TO(param, KEY_OUTPUT_TIMEOUT, timeout);

  easymedia::REFLECTOR(Decoder)::DumpFactories();
  std::string mpp_codec = "rkmpp";
  auto mpp_dec = easymedia::REFLECTOR(Decoder)::Create<easymedia::VideoDecoder>(
      mpp_codec.c_str(), param.c_str());
  if (!mpp_dec) {
    fprintf(stderr, "Create decoder %s failed\n", mpp_codec.c_str());
    exit(EXIT_FAILURE);
  }
  eos = false;
  int file_index = 0;
  bool file_eos = false;
  std::string file_suffix =
      (input_format == VIDEO_H264) ? ".h264_frame" : ".jpg";
  FILE *single_file = NULL;
  size_t file_read_size = 16 * 1024; // 16k
  if (!input_file_path.empty()) {
    single_file = fopen(input_file_path.c_str(), "re");
    if (!single_file) {
      fprintf(stderr, "open %s failed, %m\n", input_file_path.c_str());
      exit(EXIT_FAILURE);
    }
  }
  if (!output_file_path.empty()) {
    unlink(output_file_path.c_str());
    output_file_fd =
        open(output_file_path.c_str(), O_WRONLY | O_CREAT | O_CLOEXEC, 0664);
    assert(output_file_fd >= 0);
  }
  if (only_sync_mode) {
    eos = true; // make do not run following code
    assert(single_file);
    int len = flen(single_file);
    if (len <= 0)
      exit(EXIT_FAILURE);
    // auto input = easymedia::MediaBuffer::Alloc(len);
    auto input = easymedia::MediaBuffer::Alloc(
        len, easymedia::MediaBuffer::MemType::MEM_HARD_WARE);
    assert(input);
    size_t out_len = UPALIGNTO16(width) * UPALIGNTO16(height) * 4;
    auto mb = easymedia::MediaBuffer::Alloc2(
        out_len, easymedia::MediaBuffer::MemType::MEM_HARD_WARE);
    assert(mb.GetSize() > 0);
    std::shared_ptr<easymedia::MediaBuffer> output =
        std::make_shared<easymedia::ImageBuffer>(mb);
    assert(output && output->GetSize() == out_len);
    output->SetValidSize(output->GetSize());
    ssize_t rsize = fread(input->GetPtr(), 1, len, single_file);
    fprintf(stderr, "fread size = %d\n", (int)rsize);
    assert(rsize == len);
    input->SetValidSize(rsize);
    int ret = mpp_dec->Process(input, output);
    if (ret)
      fprintf(stderr, "mpp dec process ret = %d\n", ret);
    else
      dump_output(output);
  }

  std::thread *out_thread = nullptr;
  if (async) {
    out_thread = new std::thread(out_loop, mpp_dec.get());
    assert(out_thread);
  }
  std::shared_ptr<easymedia::MediaBuffer> last_buffer;
  bool sent_eos = false;
  while (!eos) {
    if (last_buffer) {
      int ret = mpp_dec->SendInput(last_buffer);
      if (ret != -EAGAIN) {
        if (async && last_buffer->IsEOF()) {
          last_buffer = nullptr;
          break;
        }
        last_buffer = nullptr;
      }
    } else if (!file_eos) {
      std::shared_ptr<easymedia::MediaBuffer> buffer;
      FILE *f = single_file;
      size_t read_size = file_read_size;
      bool read_whole_one = !f || (input_format == IMAGE_JPEG);
      if (!f) {
        input_file_path = input_dir_path;
        input_file_path.append("/")
            .append(std::to_string(file_index))
            .append(file_suffix);
        f = fopen(input_file_path.c_str(), "re");
        if (!f) {
          fprintf(stderr, "open %s failed, %m\n", input_file_path.c_str());
          exit(EXIT_FAILURE);
        }
      }
      if (read_whole_one) {
        read_size = flen(f);
        if (read_size <= 0)
          exit(EXIT_FAILURE);
        file_index++;
        if (file_index > MAX_FILE_NUM ||
            ((input_format == IMAGE_JPEG) && input_dir_path.empty()))
          file_eos = true;
      }

      if (feof(f)) {
        file_eos = true;
        fprintf(stderr, "file end\n");
        goto try_get_frame;
      } else {
        buffer = easymedia::MediaBuffer::Alloc(read_size);
        assert(buffer);
        ssize_t rsize = fread(buffer->GetPtr(), 1, read_size, f);
        fprintf(stderr, "fread size = %d\n", (int)rsize);
        buffer->SetValidSize(rsize);
      }
      if (f != single_file)
        fclose(f);
      buffer->SetUSTimeStamp(easymedia::gettimeofday());
      int ret = mpp_dec->SendInput(buffer);
      if (ret == -EAGAIN) {
        last_buffer = buffer;
        goto try_get_frame;
      }
    } else if (!sent_eos && input_format != IMAGE_JPEG) {
      auto buffer = std::make_shared<easymedia::MediaBuffer>();
      assert(buffer);
      buffer->SetEOF(true);
      buffer->SetUSTimeStamp(easymedia::gettimeofday());
      int ret = mpp_dec->SendInput(buffer);
      sent_eos = true;
      if (ret == -EAGAIN) {
        last_buffer = buffer;
        goto try_get_frame;
      }
      if (async)
        break;
    }
  try_get_frame:
    if (!async) {
      if (!get_output_and_process(mpp_dec.get()))
        break;
    }
  }
  if (single_file)
    fclose(single_file);
  if (async) {
    out_thread->join();
    delete out_thread;
  }
  if (output_file_fd >= 0) {
    close(output_file_fd);
    fsync(output_file_fd);
  }
  return 0;
}
