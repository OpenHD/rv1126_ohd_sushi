#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <string>

#include "buffer.h"
#include "control.h"
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

static char optstr[] = "?:i:w:h:f:";

static void print_usage(char *name) {
  printf("usage example: \n");
  printf("%s -i /dev/video0 -w 640 -h 480 -f nv12\n", name);
}

int OcclusionDetectionEventProc(std::shared_ptr<easymedia::Flow> flow,
                                bool &loop) {
  while (loop) {
    flow->EventHookWait();
    auto msg = flow->GetEventMessage();
    auto param = flow->GetEventParam(msg);
    if (param == nullptr)
      continue;

    if (param->GetId() == MSG_FLOW_EVENT_INFO_OCCLUSIONDETECTION) {
      OcclusionDetectEvent *odevent =
          (OcclusionDetectEvent *)param->GetParams();
      if (odevent) {
        printf("@@@ MD: Get occlusion info[%d]: IMG:%dx%d\n", odevent->info_cnt,
               odevent->img_width, odevent->img_height);
        OcclusionDetecInfo *odinfo = odevent->data;
        for (int i = 0; i < odevent->info_cnt; i++) {
          printf("--> %d rect:(%d, %d, %d, %d), occlusion:%d\n", i, odinfo->x,
                 odinfo->y, odinfo->w, odinfo->h, odinfo->occlusion);
          odinfo++;
        }
      }
    } else
      printf("Get error event type!\n");
  }

  return 0;
}

int main(int argc, char **argv) {
  int video_fps = 23;
  int c;
  int video_width = 640;
  int video_height = 480;
  std::string pixel_format;
  std::string input_path;

  std::string flow_name;
  std::string flow_param;
  std::string stream_param;
  std::string od_param;

  std::shared_ptr<easymedia::Flow> video_read_flow;
  std::shared_ptr<easymedia::Flow> video_od_flow;
  std::shared_ptr<easymedia::Flow> video_process_flow;
  std::shared_ptr<easymedia::Flow> video_display_flow;

  opterr = 1;
  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 'i':
      input_path = optarg;
      printf("#IN ARGS: input path: %s\n", input_path.c_str());
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
    case '?':
    default:
      print_usage(argv[0]);
      exit(0);
    }
  }

  if (input_path.empty()) {
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

  signal(SIGINT, sigterm_handler);
  printf("#Dump factroys:");
  easymedia::REFLECTOR(Flow)::DumpFactories();
  printf("#Dump streams:");
  easymedia::REFLECTOR(Stream)::DumpFactories();

  LOG_INIT();

  // Reading yuv from camera
  flow_name = "source_stream";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_NAME, "v4l2_capture_stream");
  PARAM_STRING_APPEND_TO(flow_param, KEY_FPS, video_fps);
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
  video_read_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!video_read_flow) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }

  flow_name = "occlusion_detec";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_NAME, "occlusion_detec");
  PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE, pixel_format);
  PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE, "NULL");
  od_param = "";
  PARAM_STRING_APPEND_TO(od_param, KEY_OD_WIDTH, video_width);
  PARAM_STRING_APPEND_TO(od_param, KEY_OD_HEIGHT, video_height);
  PARAM_STRING_APPEND_TO(od_param, KEY_OD_ROI_CNT, 1);
  ImageRect rect = {0, 0, video_width, video_height};
  PARAM_STRING_APPEND(od_param, KEY_OD_ROI_RECT,
                      easymedia::ImageRectToString(rect));
  flow_param = easymedia::JoinFlowParam(flow_param, 1, od_param);

  printf("\n#OcclusionDetection flow param:\n%s\n", flow_param.c_str());
  video_od_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!video_od_flow) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }

  flow_name = "filter";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_NAME, "rkrga");
  PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE, IMAGE_NV12);
  // Set output buffer type.
  PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE, IMAGE_RGB888);
  // Set output buffer size.
  PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_WIDTH, 720);
  PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_HEIGHT, 1280);
  PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_VIR_WIDTH, 720);
  PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_VIR_HEIGHT, 1280);
  std::string filter_param = "";
  ImageRect src_rect = {0, 0, 720, 1280};
  src_rect.w = video_width;
  src_rect.h = video_height;
  ImageRect dst_rect = {0, 0, 720, 1280};
  std::vector<ImageRect> rect_vect;
  rect_vect.push_back(src_rect);
  rect_vect.push_back(dst_rect);
  PARAM_STRING_APPEND(filter_param, KEY_BUFFER_RECT,
                      easymedia::TwoImageRectToString(rect_vect).c_str());
  PARAM_STRING_APPEND_TO(filter_param, KEY_BUFFER_ROTATE, 90);
  flow_param = easymedia::JoinFlowParam(flow_param, 1, filter_param);
  printf("\n#Rkrga Filter flow param:\n%s\n", flow_param.c_str());
  video_process_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!video_process_flow) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }

  flow_name = "output_stream";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_NAME, "drm_output_stream");
  // PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE, IMAGE_NV12);

  stream_param = "";
  PARAM_STRING_APPEND(stream_param, KEY_DEVICE, "/dev/dri/card0");
  PARAM_STRING_APPEND_TO(stream_param, KEY_BUFFER_WIDTH, 720);
  PARAM_STRING_APPEND_TO(stream_param, KEY_BUFFER_HEIGHT, 1280);
  PARAM_STRING_APPEND_TO(stream_param, KEY_BUFFER_VIR_WIDTH, 720);
  PARAM_STRING_APPEND_TO(stream_param, KEY_BUFFER_VIR_HEIGHT, 1280);
  // PARAM_STRING_APPEND(stream_param, KEY_OUTPUTDATATYPE, IMAGE_NV12);
  PARAM_STRING_APPEND(stream_param, "framerate", "60");
  PARAM_STRING_APPEND(stream_param, "plane_type", "Primary");
  PARAM_STRING_APPEND(stream_param, "ZPOS", "0");
  PARAM_STRING_APPEND(stream_param, KEY_OUTPUTDATATYPE, IMAGE_RGB888);
  flow_param = easymedia::JoinFlowParam(flow_param, 1, stream_param);
  printf("\n#DrmDisplay:\n%s\n", flow_param.c_str());
  video_display_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!video_display_flow) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }

  video_od_flow->RegisterEventHandler(video_od_flow,
                                      OcclusionDetectionEventProc);
  video_read_flow->AddDownFlow(video_od_flow, 0, 0);
  video_read_flow->AddDownFlow(video_process_flow, 0, 0);
  video_process_flow->AddDownFlow(video_display_flow, 0, 0);
  RKMEDIA_LOGI("%s initial finish\n", argv[0]);

#if 0
  easymedia::msleep(30000);

  RKMEDIA_LOGI("#Disable roi function for 10s....\n");
  video_od_flow->Control(easymedia::S_OD_ROI_ENABLE, 0);
  easymedia::msleep(10000);

  RKMEDIA_LOGI("#Enable roi function....\n");
  video_od_flow->Control(easymedia::S_OD_ROI_ENABLE, 1);
  easymedia::msleep(3000);

  RKMEDIA_LOGI("#Set new roi rects with ImageRect....\n");
  ImageRect retcs[2];
  retcs[0].x = 0;
  retcs[0].y = 0;
  retcs[0].w = video_width;
  retcs[0].h = video_height / 2;
  retcs[1].x = 0;
  retcs[1].y = video_height / 2;
  retcs[1].w = video_width;
  retcs[1].h = video_height / 2;
  video_od_flow->Control(easymedia::S_OD_ROI_RECTS, retcs, 2);
  easymedia::msleep(3000);
#endif

  while (!quit) {
    easymedia::msleep(10);
  }

  video_read_flow->RemoveDownFlow(video_od_flow);
  video_read_flow.reset();
  video_od_flow.reset();

  return 0;
}
