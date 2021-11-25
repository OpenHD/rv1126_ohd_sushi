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

#include <cmath>
#include <string>
#include <unordered_map>

extern "C" {
#define __STDC_CONSTANT_MACROS
#include <libavformat/avformat.h>
}

#include "buffer.h"
#include "encoder.h"
#include "key_string.h"
#include "media_type.h"
#include "muxer.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846 /* pi */
#endif

static const int mp3_sample_rates[] = {96000, 88200, 64000, 48000, 44100,
                                       32000, 24000, 22050, 16000, 12000,
                                       11025, 8000,  0};

static int free_memory(void *buffer) {
  assert(buffer);
  free(buffer);
  return 0;
}

std::shared_ptr<easymedia::MediaBuffer>
mp3_get_extradata(SampleInfo sample_info) {
  size_t dsi_size = 2;
  char *ptr = (char *)malloc(dsi_size);
  assert(ptr);

  uint32_t sample_rate_idx = 0;
  for (; mp3_sample_rates[sample_rate_idx] != 0; sample_rate_idx++)
    if (sample_info.sample_rate == mp3_sample_rates[sample_rate_idx])
      break;

  uint32_t object_type = 2; // MP3 LC by default
  ptr[0] = (object_type << 3) | (sample_rate_idx >> 1);
  ptr[1] = ((sample_rate_idx & 1) << 7) | (sample_info.channels << 3);

  std::shared_ptr<easymedia::MediaBuffer> extra_data =
      std::make_shared<easymedia::MediaBuffer>(ptr, dsi_size, -1, ptr,
                                               free_memory);
  extra_data->SetValidSize(dsi_size);
  return extra_data;
}

template <typename Encoder>
int encode(std::shared_ptr<easymedia::Muxer> mux,
           std::shared_ptr<easymedia::Stream> file_write,
           std::shared_ptr<Encoder> encoder,
           std::shared_ptr<easymedia::MediaBuffer> src, int stream_no) {
  auto enc = encoder;
  int ret = enc->SendInput(src);
  if (ret < 0) {
    fprintf(stderr, "[%d]: frame encode failed, ret=%d\n", stream_no, ret);
    return -1;
  }

  while (ret >= 0) {
    auto out = enc->FetchOutput();
    if (!out) {
      if (errno != EAGAIN) {
        fprintf(stderr, "[%d]: frame fetch failed, ret=%d\n", stream_no, errno);
        ret = errno;
      }
      break;
    }
    size_t out_len = out->GetValidSize();
    if (out_len == 0)
      break;
    fprintf(stderr, "[%d]: frame encoded, out %zu bytes\n\n", stream_no,
            out_len);
    if (mux)
      mux->Write(out, stream_no);

    if (file_write)
      file_write->Write(out->GetPtr(), 1, out_len);
  }

  return ret;
}

std::shared_ptr<easymedia::AudioEncoder>
initAudioEncoder(std::string EncoderName, std::string EncType,
                 SampleInfo &sample) {
  std::string param;
  PARAM_STRING_APPEND(param, KEY_OUTPUTDATATYPE, EncType);
  auto aud_enc = easymedia::REFLECTOR(Encoder)::Create<easymedia::AudioEncoder>(
      EncoderName.c_str(), param.c_str());
  if (!aud_enc) {
    fprintf(stderr, "Create %s encoder failed\n", EncoderName.c_str());
    exit(EXIT_FAILURE);
  }

  MediaConfig aud_enc_config;
  auto &ac = aud_enc_config.aud_cfg;
  ac.sample_info = sample;
  ac.bit_rate = 64000; // 64kbps
  ac.codec_type = StringToCodecType(EncType.c_str());
  aud_enc_config.type = Type::Audio;
  if (!aud_enc->InitConfig(aud_enc_config)) {
    fprintf(stderr, "Init config of rkaudio_aud encoder failed\n");
    exit(EXIT_FAILURE);
  }
  return aud_enc;
}

std::shared_ptr<easymedia::VideoEncoder>
initVideoEncoder(std::string EncoderName, std::string SrcFormat,
                 std::string OutFormat, int w, int h) {
  std::string param;
  PARAM_STRING_APPEND(param, KEY_OUTPUTDATATYPE, OutFormat);
  // If not rkmpp, then it is rkaudio
  if (EncoderName == "rkaudio_vid") {
    if (OutFormat == "video:h264") {
      PARAM_STRING_APPEND(param, KEY_NAME, "libx264");
    } else if (OutFormat == "video:h265") {
      PARAM_STRING_APPEND(param, KEY_NAME, "libx265");
    } else {
      exit(EXIT_FAILURE);
    }
  }
  auto vid_enc = easymedia::REFLECTOR(Encoder)::Create<easymedia::VideoEncoder>(
      EncoderName.c_str(), param.c_str());

  if (!vid_enc) {
    fprintf(stderr, "Create encoder %s failed\n", EncoderName.c_str());
    exit(EXIT_FAILURE);
  }

  PixelFormat fmt = PIX_FMT_NONE;
  if (SrcFormat == "nv12") {
    fmt = PIX_FMT_NV12;
  } else if (SrcFormat == "yuv420p") {
    fmt = PIX_FMT_YUV420P;
  } else {
    fprintf(stderr, "TO BE TESTED <%s:%s,%d>\n", __FILE__, __FUNCTION__,
            __LINE__);
    exit(EXIT_FAILURE);
  }

  // TODO SrcFormat and OutFormat use the same variable
  ImageInfo vid_info = {fmt, w, h, w, h};
  if (EncoderName == "rkmpp") {
    vid_info.vir_width = UPALIGNTO16(w);
    vid_info.vir_height = UPALIGNTO16(h);
  }
  MediaConfig vid_enc_config;
  if (OutFormat == VIDEO_H264 || OutFormat == VIDEO_H265) {
    VideoConfig &vid_cfg = vid_enc_config.vid_cfg;
    ImageConfig &img_cfg = vid_cfg.image_cfg;
    img_cfg.image_info = vid_info;
    vid_cfg.qp_init = 24;
    vid_cfg.qp_step = 4;
    vid_cfg.qp_min = 12;
    vid_cfg.qp_max = 48;
    vid_cfg.bit_rate = w * h * 7;
    if (vid_cfg.bit_rate > 1000000) {
      vid_cfg.bit_rate /= 1000000;
      vid_cfg.bit_rate *= 1000000;
    }
    vid_cfg.frame_rate = 30;
    vid_cfg.level = 52;
    vid_cfg.gop_size = 10; // vid_cfg.frame_rate;
    vid_cfg.profile = 100;
    // vid_cfg.rc_quality = "aq_only"; vid_cfg.rc_mode = "vbr";
    vid_cfg.rc_quality = KEY_HIGHEST;
    vid_cfg.rc_mode = KEY_CBR;
    vid_enc_config.type = Type::Video;
  } else {
    // TODO
    assert(0);
  }

  if (!vid_enc->InitConfig(vid_enc_config)) {
    fprintf(stderr, "Init config of encoder %s failed\n", EncoderName.c_str());
    exit(EXIT_FAILURE);
  }

  return vid_enc;
}

std::shared_ptr<easymedia::SampleBuffer> initAudioBuffer(MediaConfig &cfg) {
  auto &audio_info = cfg.aud_cfg.sample_info;
  fprintf(stderr, "sample number=%d\n", audio_info.nb_samples);
  int aud_size = GetSampleSize(audio_info) * audio_info.nb_samples;
  auto aud_mb = easymedia::MediaBuffer::Alloc2(aud_size);
  auto aud_buffer =
      std::make_shared<easymedia::SampleBuffer>(aud_mb, audio_info);
  aud_buffer->SetValidSize(aud_size);
  assert(aud_buffer && (int)aud_buffer->GetSize() >= aud_size);
  return aud_buffer;
}

std::shared_ptr<easymedia::MediaBuffer>
initVideoBuffer(std::string &EncoderName, ImageInfo &image_info) {
  // The vir_width/vir_height have aligned when init video encoder
  size_t len = CalPixFmtSize(image_info);
  fprintf(stderr, "video buffer len %zu\n", len);
  // Just treat all aligned memory to be hardware memory
  // need to know rkmpp needs DRM managed memory,
  // but rkaudio software encoder doesn't need.
  easymedia::MediaBuffer::MemType MemType =
      EncoderName == "rkmpp" ? easymedia::MediaBuffer::MemType::MEM_HARD_WARE
                             : easymedia::MediaBuffer::MemType::MEM_COMMON;
  auto &&src_mb = easymedia::MediaBuffer::Alloc2(len, MemType);
  assert(src_mb.GetSize() > 0);
  auto src_buffer =
      std::make_shared<easymedia::ImageBuffer>(src_mb, image_info);
  assert(src_buffer && src_buffer->GetSize() >= len);

  return src_buffer;
}

std::shared_ptr<easymedia::Muxer> initMuxer(std::string &output_path) {
  easymedia::REFLECTOR(Muxer)::DumpFactories();
  std::string param;

  char *cut = strrchr((char *)output_path.c_str(), '.');
  if (cut) {
    std::string output_data_type = cut + 1;
    if (output_data_type == "mp4") {
      PARAM_STRING_APPEND(param, KEY_OUTPUTDATATYPE, "mp4");
    } else if (output_data_type == "mp3") {
      PARAM_STRING_APPEND(param, KEY_OUTPUTDATATYPE, "mp3");
    }
  }

  PARAM_STRING_APPEND(param, KEY_PATH, output_path);
  return easymedia::REFLECTOR(Muxer)::Create<easymedia::Muxer>("rkaudio",
                                                               param.c_str());
}

std::shared_ptr<easymedia::Stream> initFileWrite(std::string &output_path) {
  std::string stream_name = "file_write_stream";
  std::string params = "";
  PARAM_STRING_APPEND(params, KEY_PATH, output_path.c_str());
  PARAM_STRING_APPEND(params, KEY_OPEN_MODE, "we"); // write and close-on-exec

  return easymedia::REFLECTOR(Stream)::Create<easymedia::Stream>(
      stream_name.c_str(), params.c_str());
}

std::shared_ptr<easymedia::Stream> initAudioCapture(std::string device,
                                                    SampleInfo &sample_info) {
  std::string stream_name = "alsa_capture_stream";
  std::string params;

  std::string fmt_str = SampleFmtToString(sample_info.fmt);
  std::string rule = "output_data_type=" + fmt_str + "\n";
  if (!easymedia::REFLECTOR(Stream)::IsMatch(stream_name.c_str(),
                                             rule.c_str())) {
    fprintf(stderr, "unsupport data type\n");
    return nullptr;
  }
  PARAM_STRING_APPEND(params, KEY_DEVICE, device.c_str());
  PARAM_STRING_APPEND(params, KEY_SAMPLE_FMT, fmt_str);
  PARAM_STRING_APPEND_TO(params, KEY_CHANNELS, sample_info.channels);
  PARAM_STRING_APPEND_TO(params, KEY_SAMPLE_RATE, sample_info.sample_rate);
  printf("%s params:\n%s\n", stream_name.c_str(), params.c_str());

  return easymedia::REFLECTOR(Stream)::Create<easymedia::Stream>(
      stream_name.c_str(), params.c_str());
}

static char optstr[] = "?t:i:o:w:h:e:c:";

int main(int argc, char **argv) {
  int c;
  std::string input_path;
  std::string output_path;
  int w = 0, h = 0;
  std::string vid_input_format;
  std::string vid_enc_format;
  std::string vid_enc_codec_name = "rkmpp"; // rkmpp, rkaudio_vid
  std::string aud_enc_format = AUDIO_MP3;   // test mp2, mp3
  std::string aud_input_format = AUDIO_PCM_FLTP;
  std::string aud_enc_codec_name = "rkaudio_aud";

  std::string stream_type;
  std::string device_name = "default";

  opterr = 1;
  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 't':
      stream_type = optarg;
      break;
    case 'i': {
      char *cut = strchr(optarg, ':');
      if (cut) {
        cut[0] = 0;
        device_name = optarg;
        if (device_name == "alsa")
          device_name = cut + 1;
      }
      input_path = optarg;
    } break;
    case 'o':
      output_path = optarg;
      break;
    case 'w':
      w = atoi(optarg);
      break;
    case 'h':
      h = atoi(optarg);
      break;
    case 'e': {
      char *cut = strchr(optarg, ':');
      if (cut) {
        cut[0] = 0;
        char *sub = optarg;
        if (!strcmp(sub, "aud")) {
          sub = cut + 1;
          char *subsub = strchr(sub, '_');
          if (subsub) {
            subsub[0] = 0;
            aud_input_format = sub;
            aud_enc_format = subsub + 1;
          }
          break;
        } else if (!strcmp(sub, "vid")) {
          optarg = cut + 1;
        } else {
          exit(EXIT_FAILURE);
        }
      }
      cut = strchr(optarg, '_');
      if (!cut) {
        fprintf(stderr, "input/output format must be cut by \'_\'\n");
        exit(EXIT_FAILURE);
      }
      cut[0] = 0;
      vid_input_format = optarg;
      vid_enc_format = cut + 1;
      if (vid_enc_format == "h264")
        vid_enc_format = VIDEO_H264;
      else if (vid_enc_format == "h265")
        vid_enc_format = VIDEO_H265;
    } break;
    case 'c': {
      char *cut = strchr(optarg, ':');
      if (cut) {
        cut[0] = 0;
        char *sub = optarg;
        if (!strcmp(sub, "aud")) {
          aud_enc_codec_name = cut + 1;
          break;
        } else if (!strcmp(sub, "vid")) {
          optarg = cut + 1;
        } else {
          exit(EXIT_FAILURE);
        }
      }
      cut = strchr(optarg, ':');
      if (cut) {
        cut[0] = 0;
        std::string ff = optarg;
        if (ff == "rkaudio")
          vid_enc_codec_name = cut + 1;
      } else {
        vid_enc_codec_name = optarg;
      }
    } break;
    case '?':
    default:
      printf("usage example: \n");
      printf(
          "rkaudio_enc_mux_test -t video_audio -i input.yuv -o output.mp4 -w "
          "320 -h 240 -e "
          "nv12_h264 -c rkmpp\n");
      printf("rkaudio_enc_mux_test -t audio -i alsa:default -o output.mp3 -e "
             "aud:fltp_mp3 "
             "-c aud:rkaudio_aud\n");
      printf("rkaudio_enc_mux_test -t audio -i alsa:default -o output.mp2 -e "
             "aud:s16le_mp2 "
             "-c aud:rkaudio_aud\n");
      exit(0);
    }
  }
  if (aud_input_format == "u8") {
    aud_input_format = AUDIO_PCM_U8;
  } else if (aud_input_format == "s16le") {
    aud_input_format = AUDIO_PCM_S16;
  } else if (aud_input_format == "s32le") {
    aud_input_format = AUDIO_PCM_S32;
  } else if (aud_input_format == "fltp") {
    aud_input_format = AUDIO_PCM_FLTP;
  }
  if (aud_enc_format == "mp3") {
    aud_enc_format = AUDIO_MP3;
  } else if (aud_enc_format == "mp2") {
    aud_enc_format = AUDIO_MP2;
  }

  printf("stream type: %s\n", stream_type.c_str());
  printf("input file path: %s\n", input_path.c_str());
  printf("output file path: %s\n", output_path.c_str());

  if (!device_name.empty())
    printf("device_name: %s\n", device_name.c_str());

  if (stream_type.find("video") != stream_type.npos) {
    printf("vid_input_format format: %s\n", vid_input_format.c_str());
    printf("vid_enc_format format: %s\n", vid_enc_format.c_str());
  }
  if (stream_type.find("audio") != stream_type.npos) {
    printf("aud_input_format: %s\n", aud_input_format.c_str());
    printf("aud_enc_format: %s\n", aud_enc_format.c_str());
  }

  if (input_path.empty() || output_path.empty())
    exit(EXIT_FAILURE);
  if (stream_type.find("video") != stream_type.npos && (!w || !h))
    exit(EXIT_FAILURE);
  if (stream_type.find("video") != stream_type.npos &&
      (vid_input_format.empty() || vid_enc_format.empty()))
    exit(EXIT_FAILURE);

  int vid_index = 0;
  int aud_index = 0;
  int64_t first_audio_time = 0;
  int64_t first_video_time = 0;
  int64_t vinterval_per_frame = 0;
  int64_t ainterval_per_frame = 0;

  size_t video_frame_len = 0;

  std::shared_ptr<easymedia::VideoEncoder> vid_enc = nullptr;
  std::shared_ptr<easymedia::MediaBuffer> src_buffer = nullptr;
  std::shared_ptr<easymedia::MediaBuffer> dst_buffer = nullptr;

  std::shared_ptr<easymedia::Stream> audio_capture = nullptr;
  std::shared_ptr<easymedia::AudioEncoder> aud_enc = nullptr;
  std::shared_ptr<easymedia::SampleBuffer> aud_buffer = nullptr;

  std::shared_ptr<easymedia::Stream> file_write = nullptr;

  // 0. muxer
  int vid_stream_no = -1;
  int aud_stream_no = -1;
  auto mux = initMuxer(output_path);
  if (!mux) {
    fprintf(stderr,
            "Init Muxer failed and then init file write stream."
            "output_path = %s\n",
            output_path.c_str());

    file_write = initFileWrite(output_path);
    if (!file_write) {
      fprintf(stderr, "Init file write stream failed.\n");
      exit(EXIT_FAILURE);
    }
  }

  easymedia::REFLECTOR(Encoder)::DumpFactories();

  // 1.video stream
  int input_file_fd = -1;
  if (stream_type.find("video") != stream_type.npos) {
    input_file_fd = open(input_path.c_str(), O_RDONLY | O_CLOEXEC);
    assert(input_file_fd >= 0);
    unlink(output_path.c_str());

    vid_enc = initVideoEncoder(vid_enc_codec_name, vid_input_format,
                               vid_enc_format, w, h);
    src_buffer = initVideoBuffer(vid_enc_codec_name,
                                 vid_enc->GetConfig().img_cfg.image_info);
    if (vid_enc_codec_name == "rkmpp") {
      size_t dst_len = CalPixFmtSize(vid_enc->GetConfig().img_cfg.image_info);
      dst_buffer = easymedia::MediaBuffer::Alloc(
          dst_len, easymedia::MediaBuffer::MemType::MEM_HARD_WARE);
      assert(dst_buffer && dst_buffer->GetSize() >= dst_len);
    }

    // TODO SrcFormat and OutFormat use the same variable
    vid_enc->GetConfig().img_cfg.image_info.pix_fmt =
        StringToPixFmt(vid_enc_format.c_str());

    if (mux && !mux->NewMuxerStream(vid_enc->GetConfig(),
                                    vid_enc->GetExtraData(), vid_stream_no)) {
      fprintf(stderr, "NewMuxerStream failed for video\n");
      exit(EXIT_FAILURE);
    }

    vinterval_per_frame =
        1000000LL /* us */ / vid_enc->GetConfig().vid_cfg.frame_rate;

    // TODO SrcFormat and OutFormat use the same variable
    vid_enc->GetConfig().vid_cfg.image_cfg.image_info.pix_fmt =
        StringToPixFmt(std::string("image:").append(vid_input_format).c_str());
    // Since the input is packed yuv images, no padding buffer,
    // we want to read actual pixel size
    video_frame_len = CalPixFmtSize(
        vid_enc->GetConfig().vid_cfg.image_cfg.image_info.pix_fmt, w, h);
  }

  // 2. audio stream.
  SampleInfo sample_info = {SAMPLE_FMT_NONE, 2, 48000, 1024};
  if (stream_type.find("audio") != stream_type.npos) {
    sample_info.fmt = StringToSampleFmt(aud_input_format.c_str());
    audio_capture = initAudioCapture(device_name, sample_info);
    if (!audio_capture) {
      fprintf(stderr, "initAudioCapture failed.\n");
      exit(EXIT_FAILURE);
    }

    aud_enc = initAudioEncoder(aud_enc_codec_name, aud_enc_format, sample_info);
    if (aud_enc_format == AUDIO_MP3) {
      auto extra_data = mp3_get_extradata(sample_info);
      aud_enc->SetExtraData(extra_data);
    }

    aud_buffer = initAudioBuffer(aud_enc->GetConfig());
    assert(aud_buffer && aud_buffer->GetValidSize() > 0);

    auto &audio_info = aud_enc->GetConfig().aud_cfg.sample_info;
    sample_info = audio_info;

    if (mux && !mux->NewMuxerStream(aud_enc->GetConfig(),
                                    aud_enc->GetExtraData(), aud_stream_no)) {
      fprintf(stderr, "NewMuxerStream failed for audio\n");
      exit(EXIT_FAILURE);
    }
    ainterval_per_frame = 1000000LL /* us */ *
                          aud_enc->GetConfig().aud_cfg.sample_info.nb_samples /
                          aud_enc->GetConfig().aud_cfg.sample_info.sample_rate;
  }

  if (mux && !(mux->WriteHeader(aud_stream_no))) {
    fprintf(stderr, "WriteHeader on stream index %d return nullptr\n",
            aud_stream_no);
    exit(EXIT_FAILURE);
  }

  // for rkaudio, WriteHeader once, this call only dump info
  // mux->WriteHeader(aud_stream_no);

  int64_t audio_duration = 10 * 1000 * 1000;

  while (true) {
    if (stream_type.find("video") != stream_type.npos &&
        vid_index * vinterval_per_frame < aud_index * ainterval_per_frame) {
      // video
      ssize_t read_len =
          read(input_file_fd, src_buffer->GetPtr(), video_frame_len);
      if (read_len < 0) {
        // if 0 Continue to drain all encoded buffer
        fprintf(stderr, "%s read len %zu\n", vid_enc_codec_name.c_str(),
                read_len);
        break;
      } else if (read_len == 0 && vid_enc_codec_name == "rkmpp") {
        // rkmpp process does not accept empty buffer
        // it will treat the result of nullptr input as normal
        // though it is ugly, but we cannot change it by now
        fprintf(stderr, "%s read len 0\n", vid_enc_codec_name.c_str());
        break;
      }
      if (first_video_time == 0) {
        first_video_time = easymedia::gettimeofday();
      }

      // feed video buffer
      src_buffer->SetValidSize(read_len); // important
      src_buffer->SetUSTimeStamp(first_video_time +
                                 vid_index * vinterval_per_frame); // important
      vid_index++;
      if (vid_enc_codec_name == "rkmpp") {
        dst_buffer->SetValidSize(dst_buffer->GetSize());
        if (0 != vid_enc->Process(src_buffer, dst_buffer, nullptr)) {
          continue;
        }
        size_t out_len = dst_buffer->GetValidSize();
        fprintf(stderr, "vframe %d encoded, type %s, out %zu bytes\n",
                vid_index,
                dst_buffer->GetUserFlag() & easymedia::MediaBuffer::kIntra
                    ? "I frame"
                    : "P frame",
                out_len);
        if (mux)
          mux->Write(dst_buffer, vid_stream_no);
      } else if (vid_enc_codec_name == "rkaudio_vid") {
        if (0 > encode<easymedia::VideoEncoder>(mux, file_write, vid_enc,
                                                src_buffer, vid_stream_no)) {
          fprintf(stderr, "Encode video frame %d failed\n", vid_index);
          break;
        }
      }
    } else {
      // audio
      if (first_audio_time == 0)
        first_audio_time = easymedia::gettimeofday();

      if (first_audio_time > 0 &&
          easymedia::gettimeofday() - first_audio_time > audio_duration)
        break;

      size_t read_samples =
          audio_capture->Read(aud_buffer->GetPtr(), aud_buffer->GetSampleSize(),
                              sample_info.nb_samples);
      if (!read_samples && errno != EAGAIN) {
        exit(EXIT_FAILURE); // fatal error
      }
      aud_buffer->SetSamples(read_samples);
      aud_buffer->SetUSTimeStamp(first_audio_time +
                                 aud_index * ainterval_per_frame);
      aud_index++;

      if (0 > encode<easymedia::AudioEncoder>(mux, file_write, aud_enc,
                                              aud_buffer, aud_stream_no)) {
        fprintf(stderr, "Encode audio frame %d failed\n", aud_index);
        break;
      }
    }
  }

  if (stream_type.find("video") != stream_type.npos) {
    src_buffer->SetValidSize(0);
    if (0 > encode<easymedia::VideoEncoder>(mux, file_write, vid_enc,
                                            src_buffer, vid_stream_no)) {
      fprintf(stderr, "Drain video frame %d failed\n", vid_index);
    }
    aud_buffer->SetSamples(0);
    if (0 > encode<easymedia::AudioEncoder>(mux, file_write, aud_enc,
                                            aud_buffer, aud_stream_no)) {
      fprintf(stderr, "Drain audio frame %d failed\n", aud_index);
    }
  }

  if (mux) {
    auto buffer = easymedia::MediaBuffer::Alloc(1);
    buffer->SetEOF(true);
    buffer->SetValidSize(0);
    mux->Write(buffer, vid_stream_no);
  }

  close(input_file_fd);
  mux = nullptr;
  vid_enc = nullptr;
  aud_enc = nullptr;

  return 0;
}
