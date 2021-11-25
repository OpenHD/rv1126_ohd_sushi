// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <getopt.h>

#include "common/sample_common.h"
#include "common/sample_fake_isp.h"
#include "rkmedia_api.h"
#include "rkmedia_venc.h"

static int index_raw = 0;
static bool quit = false;
static rk_aiq_sys_ctx_t *g_aiq_ctx = NULL;
static const char *g_rkraw_file = NULL;
static const char *g_video_node = NULL;
static const char *g_subdev_node = NULL;
static bool g_jpg_enable = false;
static bool g_yuv_enable = false;
static bool g_vop_enable = true;
static bool g_free_mem = false;
static uint8_t g_dump_num = 0;
static uint8_t *rkraws[MAX_FAKE_FRAMES_NUM];
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

#define TEST_ARGB32_YELLOW 0xFFFFFF00
#define TEST_ARGB32_RED 0xFFFF0033
#define TEST_ARGB32_BLUE 0xFF003399
#define TEST_ARGB32_TRANS 0x00000000

static void set_argb8888_buffer(RK_U32 *buf, RK_U32 size, RK_U32 color) {
  for (RK_U32 i = 0; buf && (i < size); i++)
    *(buf + i) = color;
}

void video_packet_cb(MEDIA_BUFFER mb) {
  printf("Get JPEG packet[%d]:ptr:%p, fd:%d, size:%zu, mode:%d, channel:%d, "
         "timestamp:%lld\n",
         index_raw, RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetFD(mb),
         RK_MPI_MB_GetSize(mb), RK_MPI_MB_GetModeID(mb),
         RK_MPI_MB_GetChannelID(mb), RK_MPI_MB_GetTimestamp(mb));

  char jpeg_path[64];
  sprintf(jpeg_path, "/tmp/test_jpeg%d.jpeg", index_raw++);
  FILE *file = fopen(jpeg_path, "w");
  if (file) {
    fwrite(RK_MPI_MB_GetPtr(mb), 1, RK_MPI_MB_GetSize(mb), file);
    fclose(file);
  }

  RK_MPI_MB_ReleaseBuffer(mb);
}

void yuv_packet_cb(MEDIA_BUFFER mb) {
  printf("Get NV12 packet[%d]:ptr:%p, fd:%d, size:%zu, mode:%d, channel:%d, "
         "timestamp:%lld\n",
         index_raw, RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetFD(mb),
         RK_MPI_MB_GetSize(mb), RK_MPI_MB_GetModeID(mb),
         RK_MPI_MB_GetChannelID(mb), RK_MPI_MB_GetTimestamp(mb));

  char nv12_path[64];
  sprintf(nv12_path, "/tmp/test_nv12_%d.yuv", index_raw++);
  FILE *file = fopen(nv12_path, "w");
  if (file) {
    fwrite(RK_MPI_MB_GetPtr(mb), 1, RK_MPI_MB_GetSize(mb), file);
    fclose(file);
  }

  RK_MPI_MB_ReleaseBuffer(mb);
}

void release_buffer(void *addr) {
  printf(">>>>release buffer called: addr=%p\n", addr);
}

static RK_CHAR optstr[] = "?::a::f:d:s:w:h:j:y:v:n:p:";
static const struct option long_options[] = {
    {"aiq", optional_argument, NULL, 'a'},
    {"help", optional_argument, NULL, '?'},
    {"device", optional_argument, NULL, 'd'},
    {"subdev", optional_argument, NULL, 's'},
    {"file", optional_argument, NULL, 'f'},
    {"width", optional_argument, NULL, 'w'},
    {"height", optional_argument, NULL, 'h'},
    {"jpg", optional_argument, NULL, 'j'},
    {"yuv", optional_argument, NULL, 'y'},
    {"vop", optional_argument, NULL, 'v'},
    {"num", optional_argument, NULL, 'n'},
    {"free", optional_argument, NULL, 'p'},
    {NULL, 0, NULL, 0},
};

static void print_usage(const RK_CHAR *name) {
  printf("usage example:\n");
#ifdef RKAIQ
  printf("\t%s [-a [iqfiles_dir]]\n", name);
  printf("\t-a | --aiq: enable aiq with dirpath provided, eg:-a "
         "/oem/etc/iqfiles/, "
         "set dirpath empty to using path by default, without this option aiq "
         "should run in other application\n");
  printf("\t-d | --device: fake camera video devices \n");
  printf("\t-s | --subdev: real camera isp sub devices \n");
  printf("\t-f | --file: read rkraw form file \n");
  printf("\t-w | --width: fake camera width \n");
  printf("\t-h | --height: fake camera height \n");
  printf("\t-j | --jepg: enc jpg \n");
  printf("\t-y | --yuv: dump yuv \n");
  printf("\t-v | --vop: vop show \n");
  printf("\t-n | --num: dump num \n");
  printf("\t-p | --put: free mem \n");
#else
  printf("\t%s\n", name);
#endif
}

int main(int argc, char *argv[]) {
  RK_S32 ret;
  RK_U32 u32SrcWidth = 1920;
  RK_U32 u32SrcHeight = 1080;
  IMAGE_TYPE_E enPixFmt = IMAGE_TYPE_NV12;
  const RK_CHAR *pcVideoNode = "rkispp_scale0";
  int c;
  const char *iq_file_dir = NULL;
  while ((c = getopt_long(argc, argv, optstr, long_options, NULL)) != -1) {
    const char *tmp_optarg = optarg;
    switch (c) {
    case 'a':
      if (!optarg && NULL != argv[optind] && '-' != argv[optind][0]) {
        tmp_optarg = argv[optind++];
      }
      if (tmp_optarg) {
        iq_file_dir = (char *)tmp_optarg;
      } else {
        iq_file_dir = "/oem/etc/iqfiles";
      }
      break;
    case 'd':
      g_video_node = optarg;
      pcVideoNode = optarg;
      break;
    case 's':
      g_subdev_node = optarg;
      break;
    case 'f':
      g_rkraw_file = optarg;
      break;
    case 'w':
      u32SrcWidth = atoi(optarg);
      break;
    case 'h':
      u32SrcHeight = atoi(optarg);
      break;
    case 'j':
      g_jpg_enable = atoi(optarg);
      g_yuv_enable = false;
      g_vop_enable = false;
      break;
    case 'y':
      g_yuv_enable = atoi(optarg);
      g_vop_enable = false;
      g_jpg_enable = false;
      break;
    case 'v':
      g_vop_enable = atoi(optarg);
      g_yuv_enable = false;
      g_jpg_enable = false;
      break;
    case 'n':
      g_dump_num = atoi(optarg);
      if (g_dump_num)
        g_dump_num += 2;
      break;
    case 'p':
      g_free_mem = atoi(optarg);
      break;
    case '?':
    default:
      print_usage(argv[0]);
      return 0;
    }
  }

  if (iq_file_dir) {
#ifdef RKAIQ
    printf("#Rkaiq XML DirPath: %s\n", iq_file_dir);
    if (!g_rkraw_file && g_subdev_node && g_video_node) {
      struct mcu_rkaiq_rkraw mcu_rkraws[MAX_FAKE_FRAMES_NUM];
      parse_mcu_rkraws(g_subdev_node, mcu_rkraws);
      alloc_rkraws(rkraws);
      make_rkraws(mcu_rkraws, rkraws);
    }
    rk_aiq_raw_prop_t prop;
    prop.format = RK_PIX_FMT_SBGGR10;
    prop.frame_width = u32SrcWidth;
    prop.frame_height = u32SrcHeight;
    if (g_rkraw_file) {
      prop.rawbuf_type = RK_AIQ_RAW_FILE;
    } else if (g_subdev_node) {
      prop.rawbuf_type = RK_AIQ_RAW_FD;
    }
    g_aiq_ctx = aiq_fake_init(RK_AIQ_WORKING_MODE_NORMAL, iq_file_dir, prop,
                              release_buffer);
#endif
  }

  ret = RK_MPI_SYS_Init();
  if (ret) {
    printf("Sys Init failed! ret=%d\n", ret);
    return -1;
  }

  VI_CHN_ATTR_S vi_chn_attr;
  vi_chn_attr.pcVideoNode = pcVideoNode;
  vi_chn_attr.u32BufCnt = 3;
  vi_chn_attr.u32Width = u32SrcWidth;
  vi_chn_attr.u32Height = u32SrcHeight;
  vi_chn_attr.enPixFmt = enPixFmt;
  vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
  ret = RK_MPI_VI_SetChnAttr(0, 0, &vi_chn_attr);
  ret |= RK_MPI_VI_EnableChn(0, 0);
  if (ret) {
    printf("Create VI[0] failed! ret=%d\n", ret);
    return -1;
  }

  MPP_CHN_S stSrcChn;
  MPP_CHN_S stDestChn;
  VENC_RECV_PIC_PARAM_S stRecvParam;
#ifdef RKAIQ
  if (g_jpg_enable) {
    VENC_CHN_ATTR_S venc_chn_attr;
    memset(&venc_chn_attr, 0, sizeof(venc_chn_attr));
    venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_JPEG;
    venc_chn_attr.stVencAttr.imageType = enPixFmt;
    venc_chn_attr.stVencAttr.u32PicWidth = u32SrcWidth;
    venc_chn_attr.stVencAttr.u32PicHeight = u32SrcHeight;
    venc_chn_attr.stVencAttr.u32VirWidth = u32SrcWidth;
    venc_chn_attr.stVencAttr.u32VirHeight = u32SrcHeight;
    ret = RK_MPI_VENC_CreateJpegLightChn(0, &venc_chn_attr);
    if (ret) {
      printf("Create Venc[0] failed! ret=%d\n", ret);
      return -1;
    }

    MPP_CHN_S stEncChn;
    stEncChn.enModId = RK_ID_VENC;
    stEncChn.s32ChnId = 0;
    ret = RK_MPI_SYS_RegisterOutCb(&stEncChn, video_packet_cb);
    if (ret) {
      printf("Register Output callback failed! ret=%d\n", ret);
      return -1;
    }

    // The encoder defaults to continuously receiving frames from the previous
    // stage. Before performing the bind operation, set s32RecvPicNum to 0 to
    // make the encoding enter the pause state.
    stRecvParam.s32RecvPicNum = 0;
    RK_MPI_VENC_StartRecvFrame(0, &stRecvParam);

    stSrcChn.enModId = RK_ID_VI;
    stSrcChn.s32ChnId = 0;
    stDestChn.enModId = RK_ID_VENC;
    stDestChn.s32ChnId = 0;
    ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (ret) {
      printf("Bind VI[0] to VENC[0]::JPEG failed! ret=%d\n", ret);
      return -1;
    }

    RK_MPI_VENC_RGN_Init(0, NULL);

    BITMAP_S BitMap;
    BitMap.enPixelFormat = PIXEL_FORMAT_ARGB_8888;
    BitMap.u32Width = 64;
    BitMap.u32Height = 256;
    BitMap.pData = malloc(BitMap.u32Width * 4 * BitMap.u32Height);
    RK_U8 *ColorData = (RK_U8 *)BitMap.pData;
    RK_U16 ColorBlockSize = BitMap.u32Height * BitMap.u32Width;
    set_argb8888_buffer((RK_U32 *)ColorData, ColorBlockSize / 4,
                        TEST_ARGB32_YELLOW);
    set_argb8888_buffer((RK_U32 *)(ColorData + ColorBlockSize),
                        ColorBlockSize / 4, TEST_ARGB32_TRANS);
    set_argb8888_buffer((RK_U32 *)(ColorData + 2 * ColorBlockSize),
                        ColorBlockSize / 4, TEST_ARGB32_RED);
    set_argb8888_buffer((RK_U32 *)(ColorData + 3 * ColorBlockSize),
                        ColorBlockSize / 4, TEST_ARGB32_BLUE);

    // Case 1: Canvas and bitmap are equal in size
    OSD_REGION_INFO_S RngInfo;
    RngInfo.enRegionId = REGION_ID_0;
    RngInfo.u32PosX = 0;
    RngInfo.u32PosY = 0;
    RngInfo.u32Width = 64;
    RngInfo.u32Height = 256;
    RngInfo.u8Enable = 1;
    RngInfo.u8Inverse = 0;
    ret = RK_MPI_VENC_RGN_SetBitMap(0, &RngInfo, &BitMap);
    if (ret) {
      printf("ERROR: SetBitMap failed! ret=%d\n", ret);
      return -1;
    }
  } else if (g_vop_enable) {
    int disp_width = 720;
    int disp_height = 1280;
    // rga0 for primary plane
    RGA_ATTR_S stRgaAttr;
    memset(&stRgaAttr, 0, sizeof(stRgaAttr));
    stRgaAttr.bEnBufPool = RK_TRUE;
    stRgaAttr.u16BufPoolCnt = 2;
    stRgaAttr.u16Rotaion = 90;
    stRgaAttr.stImgIn.u32X = 0;
    stRgaAttr.stImgIn.u32Y = 0;
    stRgaAttr.stImgIn.imgType = IMAGE_TYPE_NV12;
    stRgaAttr.stImgIn.u32Width = u32SrcWidth;
    stRgaAttr.stImgIn.u32Height = u32SrcHeight;
    stRgaAttr.stImgIn.u32HorStride = u32SrcWidth;
    stRgaAttr.stImgIn.u32VirStride = u32SrcHeight;
    stRgaAttr.stImgOut.u32X = 0;
    stRgaAttr.stImgOut.u32Y = 0;
    stRgaAttr.stImgOut.imgType = IMAGE_TYPE_RGB888;
    stRgaAttr.stImgOut.u32Width = disp_width;
    stRgaAttr.stImgOut.u32Height = disp_height;
    stRgaAttr.stImgOut.u32HorStride = disp_width;
    stRgaAttr.stImgOut.u32VirStride = disp_height;
    ret = RK_MPI_RGA_CreateChn(0, &stRgaAttr);
    if (ret) {
      printf("Create rga[0] falied! ret=%d\n", ret);
      return -1;
    }

    VO_CHN_ATTR_S stVoAttr = {0};
    // VO[0] for primary plane
    stVoAttr.pcDevNode = "/dev/dri/card0";
    stVoAttr.emPlaneType = VO_PLANE_PRIMARY;
    stVoAttr.enImgType = IMAGE_TYPE_RGB888;
    stVoAttr.u16Zpos = 0;
    stVoAttr.stDispRect.s32X = 0;
    stVoAttr.stDispRect.s32Y = 0;
    stVoAttr.stDispRect.u32Width = disp_width;
    stVoAttr.stDispRect.u32Height = disp_height;
    ret = RK_MPI_VO_CreateChn(0, &stVoAttr);
    if (ret) {
      printf("Create vo[0] failed! ret=%d\n", ret);
      return -1;
    }

    stSrcChn.enModId = RK_ID_VI;
    stSrcChn.s32ChnId = 0;
    stDestChn.enModId = RK_ID_RGA;
    stDestChn.s32ChnId = 0;
    ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (ret) {
      printf("Bind vi[0] to rga[0] failed! ret=%d\n", ret);
      return -1;
    }

    stSrcChn.enModId = RK_ID_RGA;
    stSrcChn.s32ChnId = 0;
    stDestChn.enModId = RK_ID_VO;
    stDestChn.s32ChnId = 0;
    ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (ret) {
      printf("Bind rga[0] to vo[0] failed! ret=%d\n", ret);
      return -1;
    }
  } else {

    MPP_CHN_S stViChn;
    stViChn.enModId = RK_ID_VI;
    stViChn.s32ChnId = 0;
    ret = RK_MPI_SYS_RegisterOutCb(&stViChn, yuv_packet_cb);
    if (ret) {
      printf("Register Output callback failed! ret=%d\n", ret);
      return -1;
    }
    ret = RK_MPI_VI_StartStream(0, 0);
    if (ret) {
      printf("Start VI[0] failed! ret=%d\n", ret);
      return -1;
    }
  }
#endif

  printf("%s initial finish\n", __func__);
  signal(SIGINT, sigterm_handler);

  char cmd[64];
  int qfactor = 0;
  VENC_JPEG_PARAM_S stJpegParam;
  printf("#Usage: input 'quit' to exit programe!\n"
         "peress any other key to capture one picture to file\n");

  while (!quit) {
    if (!g_video_node)
      break;

    if (g_dump_num) {
      do {
        if (g_rkraw_file) {
          rk_aiq_uapi_sysctl_enqueueRkRawFile(g_aiq_ctx, g_rkraw_file);
          printf(">>>>>>>>> enqueue raw file\n");
        } else if (g_subdev_node) {
          rk_aiq_uapi_sysctl_enqueueRkRawBuf(g_aiq_ctx,
                                            (void *)rkraws[(index_raw % MAX_FAKE_FRAMES_NUM)], false);
          printf(">>>>>>>>> enqueue raw fd\n");
        }
        usleep(1000*1000);
      } while(g_dump_num--);
      break;
    }

    fgets(cmd, sizeof(cmd), stdin);
    printf("#Input cmd:%s\n", cmd);
    if (strstr(cmd, "quit")) {
      printf("#Get 'quit' cmd!\n");
      break;
    }

#ifdef RKAIQ
    if (g_rkraw_file) {
      rk_aiq_uapi_sysctl_enqueueRkRawFile(g_aiq_ctx, g_rkraw_file);
      printf(">>>>>>>>> enqueue raw file\n");
    } else if (g_subdev_node) {
      rk_aiq_uapi_sysctl_enqueueRkRawBuf(g_aiq_ctx,
                                         (void *)rkraws[(index_raw % MAX_FAKE_FRAMES_NUM)], false);
      printf(">>>>>>>>> enqueue raw fd\n");
    }
#endif

    if (g_jpg_enable) {
      if (qfactor >= 99)
        qfactor = 1;
      else
        qfactor = ((qfactor + 10) > 99) ? 99 : (qfactor + 10);
      stJpegParam.u32Qfactor = qfactor;
      RK_MPI_VENC_SetJpegParam(0, &stJpegParam);

      stRecvParam.s32RecvPicNum = 1;
      ret = RK_MPI_VENC_StartRecvFrame(0, &stRecvParam);
      if (ret) {
        printf("RK_MPI_VENC_StartRecvFrame failed!\n");
        break;
      }
      usleep(30000); // sleep 30 ms.
    }
  }

#ifdef RKAIQ
  if (g_jpg_enable) {
    RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    ret = RK_MPI_VENC_DestroyChn(0);
    if (ret)
      printf("RK_MPI_VENC_DestroyChn, ret:%d\n", ret);
  } else if (g_vop_enable) {
    stSrcChn.enModId = RK_ID_VI;
    stSrcChn.s32ChnId = 0;
    stDestChn.enModId = RK_ID_RGA;
    stDestChn.s32ChnId = 0;
    ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    if (ret)
      printf("UnBind vi[0] to rga[0] failed! ret=%d\n", ret);

    stSrcChn.enModId = RK_ID_RGA;
    stSrcChn.s32ChnId = 0;
    stDestChn.enModId = RK_ID_VO;
    stDestChn.s32ChnId = 0;
    ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    if (ret)
      printf("UnBind rga[0] to vo[0] failed! ret=%d\n", ret);

    RK_MPI_VO_DestroyChn(0);
    RK_MPI_RGA_DestroyChn(0);
  }
#endif

  ret = RK_MPI_VI_DisableChn(0, 0);
  if (ret)
    printf("RK_MPI_VI_DisableChn, ret:%d\n", ret);

  if (iq_file_dir) {
#ifdef RKAIQ
    if (g_aiq_ctx) {
      free_rkraws(rkraws);
      if (g_free_mem && g_video_node) {
        free_rkisp_reserve_mem(g_subdev_node);
      }
      rk_aiq_uapi_sysctl_stop(g_aiq_ctx, false);
      printf("rk_aiq_uapi_sysctl_deinit enter\n");
      rk_aiq_uapi_sysctl_deinit(g_aiq_ctx);
      printf("rk_aiq_uapi_sysctl_deinit exit\n");
      g_aiq_ctx = NULL;
    }
#endif
  }

  printf("%s exit!\n", __func__);
  return 0;
}
