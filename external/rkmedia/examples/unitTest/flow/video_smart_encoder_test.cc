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
#include "image.h"
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

static char optstr[] = "?:i:w:h:o:f:t:m:";

static void print_usage(char *name) {
  printf("usage example: \n");
  printf("%s -i /dev/video0 -o output.h264 -w 1920 -h 1080 "
         "-f nv12 -t h264 -m /dev/video1\n",
         name);
}

#define ENCODER_ON 1
#define MOVEDETECTION_ON 1

int main(int argc, char **argv) {
#if ENCODER_ON
  int video_fps = 30;
#endif
  int c;
  int video_width = 1920;
  int video_height = 1080;
  int md_video_width = 640;
  int md_video_height = 360;
  std::string pixel_format;
  std::string video_enc_type = VIDEO_H264;

  std::string output_path;
  std::string input_path;
  std::string md_input_path = "/dev/video0";

  std::string flow_name;
  std::string flow_param;
  std::string stream_param;
  std::string enc_param;
  std::string md_param;

  std::shared_ptr<easymedia::Flow> video_read_flow0;
  std::shared_ptr<easymedia::Flow> video_read_flow1;
  std::shared_ptr<easymedia::Flow> video_md_flow;
  std::shared_ptr<easymedia::Flow> video_encoder_flow;
  std::shared_ptr<easymedia::Flow> video_save_flow;

  opterr = 1;
  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 'i':
      input_path = optarg;
      printf("#IN ARGS: input path: %s\n", input_path.c_str());
      break;
    case 'o':
      output_path = optarg;
      printf("#IN ARGS: output file path: %s\n", output_path.c_str());
      break;
    case 'w':
      video_width = atoi(optarg);
      printf("#IN ARGS: video_width: %d\n", video_width);
      break;
    case 'h':
      video_height = atoi(optarg);
      printf("#IN ARGS: video_height: %d\n", video_height);
      break;
    case 'f':
      pixel_format = optarg;
      printf("#IN ARGS: pixel_format: %s\n", pixel_format.c_str());
      break;
    case 't':
      video_enc_type = optarg;
      printf("#IN ARGS: video_enc_type: %s\n", video_enc_type.c_str());
      break;
    case 'm':
      md_input_path = optarg;
      printf("#IN ARGS: md input path: %s\n", md_input_path.c_str());
      break;
    case '?':
    default:
      print_usage(argv[0]);
      exit(0);
    }
  }

  if (input_path.empty() || output_path.empty() || md_input_path.empty()) {
    printf("ERROR: path is not valid!\n");
    exit(EXIT_FAILURE);
  }

  // add prefix for pixformat
  if ((pixel_format == "yuyv422") || (pixel_format == "nv12"))
    pixel_format = "image:" + pixel_format;
  else {
    printf("ERROR: image type:%s not support!\n", pixel_format.c_str());
    print_usage(argv[0]);
    exit(EXIT_FAILURE);
  }

  // add prefix for encoder type
  if ((video_enc_type == "h264") || (video_enc_type == "h265"))
    video_enc_type = "video:" + video_enc_type;
  else {
    printf("ERROR: encoder type:%s not support!\n", video_enc_type.c_str());
    print_usage(argv[0]);
    exit(EXIT_FAILURE);
  }

  signal(SIGINT, sigterm_handler);
  printf("#Dump factroys:");
  easymedia::REFLECTOR(Flow)::DumpFactories();
  printf("#Dump streams:");
  easymedia::REFLECTOR(Stream)::DumpFactories();

  // Reading yuv from camera
  flow_name = "source_stream";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_NAME, "v4l2_capture_stream");
  // PARAM_STRING_APPEND_TO(flow_param, KEY_FPS, video_fps);
  PARAM_STRING_APPEND(flow_param, KEK_THREAD_SYNC_MODEL, KEY_SYNC);
  PARAM_STRING_APPEND(flow_param, KEK_INPUT_MODEL, KEY_DROPFRONT);
  PARAM_STRING_APPEND_TO(flow_param, KEY_INPUT_CACHE_NUM, 5);
  stream_param = "";
  PARAM_STRING_APPEND_TO(stream_param, KEY_USE_LIBV4L2, 1);
  PARAM_STRING_APPEND(stream_param, KEY_DEVICE, input_path);
  // PARAM_STRING_APPEND(param, KEY_SUB_DEVICE, sub_input_path);
  PARAM_STRING_APPEND(stream_param, KEY_V4L2_CAP_TYPE,
                      KEY_V4L2_C_TYPE(VIDEO_CAPTURE));
  PARAM_STRING_APPEND(stream_param, KEY_V4L2_MEM_TYPE,
                      KEY_V4L2_M_TYPE(MEMORY_DMABUF));
  PARAM_STRING_APPEND_TO(stream_param, KEY_FRAMES,
                         4); // if not set, default is 2
  PARAM_STRING_APPEND(stream_param, KEY_OUTPUTDATATYPE, pixel_format);
  PARAM_STRING_APPEND_TO(stream_param, KEY_BUFFER_WIDTH, video_width);
  PARAM_STRING_APPEND_TO(stream_param, KEY_BUFFER_HEIGHT, video_height);

  flow_param = easymedia::JoinFlowParam(flow_param, 1, stream_param);
  printf("\n#VideoCapture %s flow param:\n%s\n", input_path.c_str(),
         flow_param.c_str());
  video_read_flow0 = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!video_read_flow0) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }

  flow_name = "video_enc";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_NAME, "rkmpp");
  PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE, pixel_format);
  PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE, video_enc_type);

  VideoEncoderCfg vcfg;
  memset(&vcfg, 0, sizeof(vcfg));
  vcfg.type = (char *)video_enc_type.c_str();
  vcfg.fps = video_fps;
  vcfg.max_bps = video_width * video_height * video_fps / 14;
  ImageInfo image_info;
  image_info.pix_fmt = StringToPixFmt(pixel_format.c_str());
  image_info.width = video_width;
  image_info.height = video_height;
  image_info.vir_width = video_width;
  image_info.vir_height = video_height;
  enc_param = easymedia::get_video_encoder_config_string(image_info, vcfg);
  flow_param = easymedia::JoinFlowParam(flow_param, 1, enc_param);
  printf("\n#VideoEncoder flow param:\n%s\n", flow_param.c_str());
  video_encoder_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!video_encoder_flow) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }

  flow_name = "file_write_flow";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_PATH, output_path.c_str());
  PARAM_STRING_APPEND(flow_param, KEY_OPEN_MODE,
                      "w+"); // read and close-on-exec
  printf("\n#FileWrite:\n%s\n", flow_param.c_str());
  video_save_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!video_save_flow) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }

  // Reading yuv from camera for md
  flow_name = "source_stream";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_NAME, "v4l2_capture_stream");
  // PARAM_STRING_APPEND_TO(flow_param, KEY_FPS, video_fps);
  PARAM_STRING_APPEND(flow_param, KEK_THREAD_SYNC_MODEL, KEY_SYNC);
  PARAM_STRING_APPEND(flow_param, KEK_INPUT_MODEL, KEY_DROPFRONT);
  PARAM_STRING_APPEND_TO(flow_param, KEY_INPUT_CACHE_NUM, 5);
  stream_param = "";
  PARAM_STRING_APPEND_TO(stream_param, KEY_USE_LIBV4L2, 1);
  PARAM_STRING_APPEND(stream_param, KEY_DEVICE, md_input_path);
  // PARAM_STRING_APPEND(param, KEY_SUB_DEVICE, sub_input_path);
  PARAM_STRING_APPEND(stream_param, KEY_V4L2_CAP_TYPE,
                      KEY_V4L2_C_TYPE(VIDEO_CAPTURE));
  PARAM_STRING_APPEND(stream_param, KEY_V4L2_MEM_TYPE,
                      KEY_V4L2_M_TYPE(MEMORY_DMABUF));
  PARAM_STRING_APPEND_TO(stream_param, KEY_FRAMES,
                         4); // if not set, default is 2
  PARAM_STRING_APPEND(stream_param, KEY_OUTPUTDATATYPE, pixel_format);
  PARAM_STRING_APPEND_TO(stream_param, KEY_BUFFER_WIDTH, md_video_width);
  PARAM_STRING_APPEND_TO(stream_param, KEY_BUFFER_HEIGHT, md_video_height);

  flow_param = easymedia::JoinFlowParam(flow_param, 1, stream_param);
  printf("\n#VideoCapture %s flow param:\n%s\n", md_input_path.c_str(),
         flow_param.c_str());
  video_read_flow1 = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!video_read_flow1) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }

  flow_name = "move_detec";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_NAME, "move_detec");
  PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE, pixel_format);
  PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE, "NULL");
  md_param = "";
  PARAM_STRING_APPEND_TO(md_param, KEY_MD_SINGLE_REF, 1);
  PARAM_STRING_APPEND_TO(md_param, KEY_MD_ORI_WIDTH, video_width);
  PARAM_STRING_APPEND_TO(md_param, KEY_MD_ORI_HEIGHT, video_height);
  PARAM_STRING_APPEND_TO(md_param, KEY_MD_DS_WIDTH, md_video_width);
  PARAM_STRING_APPEND_TO(md_param, KEY_MD_DS_HEIGHT, md_video_height);
  PARAM_STRING_APPEND_TO(md_param, KEY_MD_ROI_CNT, 1);
  ImageRect rect = {0, 0, video_width, video_height};
  PARAM_STRING_APPEND(md_param, KEY_MD_ROI_RECT,
                      easymedia::ImageRectToString(rect));
  flow_param = easymedia::JoinFlowParam(flow_param, 1, md_param);

  printf("\n#MoveDetection flow param:\n%s\n", flow_param.c_str());
  video_md_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!video_md_flow) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }

  printf("\n# Set md flow for encdoer flow:%p!\n", video_md_flow.get());
  easymedia::video_encoder_set_move_detection(video_encoder_flow,
                                              video_md_flow);
  printf("\n# Enable video encoder statistics...\n");
  easymedia::video_encoder_enable_statistics(video_encoder_flow, 1);

  video_encoder_flow->AddDownFlow(video_save_flow, 0, 0);
  video_read_flow0->AddDownFlow(video_encoder_flow, 0, 0);
  video_read_flow1->AddDownFlow(video_md_flow, 0, 0);

  RKMEDIA_LOGI("%s initial finish\n", argv[0]);

  while (!quit) {
    easymedia::msleep(10);
  }
  video_read_flow0->RemoveDownFlow(video_encoder_flow);
  video_read_flow0.reset();
  video_encoder_flow->RemoveDownFlow(video_save_flow);
  video_encoder_flow.reset();
  video_save_flow.reset();
  video_read_flow1->RemoveDownFlow(video_md_flow);
  video_read_flow1.reset();
  video_md_flow.reset();

  return 0;
}
