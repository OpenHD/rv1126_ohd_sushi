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

int MoveDetectionEventProc(std::shared_ptr<easymedia::Flow> flow, bool &loop) {
  while (loop) {
    flow->EventHookWait();
    auto msg = flow->GetEventMessage();
    auto param = flow->GetEventParam(msg);
    if (param == nullptr)
      continue;

    if (param->GetId() == MSG_FLOW_EVENT_INFO_MOVEDETECTION) {
      MoveDetectEvent *mdevent = (MoveDetectEvent *)param->GetParams();
      if (mdevent) {
        printf("@@@ MD: Get movement info[%d]: ORI:%dx%d, DS:%dx%d\n",
               mdevent->info_cnt, mdevent->ori_width, mdevent->ori_height,
               mdevent->ds_width, mdevent->ds_height);
        MoveDetecInfo *mdinfo = mdevent->data;
        for (int i = 0; i < mdevent->info_cnt; i++) {
          printf("--> %d rect:(%d, %d, %d, %d)\n", i, mdinfo->x, mdinfo->y,
                 mdinfo->w, mdinfo->h);
          mdinfo++;
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
  std::string md_param;

  std::shared_ptr<easymedia::Flow> video_read_flow;
  std::shared_ptr<easymedia::Flow> video_md_flow;

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

  flow_name = "move_detec";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_NAME, "move_detec");
  PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE, pixel_format);
  PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE, "NULL");
  md_param = "";
  PARAM_STRING_APPEND_TO(md_param, KEY_MD_SINGLE_REF, 1);
  PARAM_STRING_APPEND_TO(md_param, KEY_MD_ORI_WIDTH, 1920);
  PARAM_STRING_APPEND_TO(md_param, KEY_MD_ORI_HEIGHT, 1080);
  PARAM_STRING_APPEND_TO(md_param, KEY_MD_DS_WIDTH, video_width);
  PARAM_STRING_APPEND_TO(md_param, KEY_MD_DS_HEIGHT, video_height);
  PARAM_STRING_APPEND_TO(md_param, KEY_MD_ROI_CNT, 1);
  ImageRect rect = {0, 0, video_width / 2, video_height / 2};
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

  video_md_flow->RegisterEventHandler(video_md_flow, MoveDetectionEventProc);
  video_read_flow->AddDownFlow(video_md_flow, 0, 0);
  RKMEDIA_LOGI("%s initial finish\n", argv[0]);
  easymedia::msleep(30000);

  RKMEDIA_LOGI("#Disable roi function for 10s....\n");
  video_md_flow->Control(easymedia::S_MD_ROI_ENABLE, 0);
  easymedia::msleep(10000);

  RKMEDIA_LOGI("#Enable roi function....\n");
  video_md_flow->Control(easymedia::S_MD_ROI_ENABLE, 1);
  RKMEDIA_LOGI("#Enable sensitivity....\n");
  video_md_flow->Control(easymedia::S_MD_SENSITIVITY, 3);
  easymedia::msleep(3000);

  RKMEDIA_LOGI("#Set new roi rects with ImageRect....\n");
  ImageRect retcs[2];
  retcs[0].x = 0;
  retcs[0].y = 0;
  retcs[0].w = 32;
  retcs[0].h = 32;
  retcs[1].x = 32;
  retcs[1].y = 32;
  retcs[1].w = 32;
  retcs[1].h = 32;
  // video_md_flow->Control(easymedia::S_MD_ROI_RECTS, retcs, 2);
  video_move_detect_set_rects(video_md_flow, retcs, 2);
  easymedia::msleep(3000);

  RKMEDIA_LOGI("#Set new roi rects with String....\n");
  std::string retcs_str = "(0,0,64,64)(64,64,64,64)(128,128,64,64)";
  video_move_detect_set_rects(video_md_flow, retcs_str);

  while (!quit) {
    easymedia::msleep(10);
  }

  video_read_flow->RemoveDownFlow(video_md_flow);
  video_md_flow.reset();
  video_md_flow.reset();

  return 0;
}
