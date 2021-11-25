// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __RKMEDIA_COMMON_
#define __RKMEDIA_COMMON_

#ifdef __cplusplus
extern "C" {
#endif

#define _CAPI __attribute__((visibility("default")))

typedef unsigned char RK_U8;
typedef unsigned short RK_U16;
typedef unsigned int RK_U32;

typedef signed char RK_S8;
typedef short RK_S16;
typedef int RK_S32;

typedef unsigned long RK_UL;
typedef signed long RK_SL;

typedef float RK_FLOAT;
typedef double RK_DOUBLE;

#ifndef _M_IX86
typedef unsigned long long RK_U64;
typedef long long RK_S64;
#else
typedef unsigned __int64 RK_U64;
typedef __int64 RK_S64;
#endif

typedef char RK_CHAR;
#define RK_VOID void

typedef unsigned int RK_HANDLE;

/*----------------------------------------------*
 * const defination                             *
 *----------------------------------------------*/
typedef enum {
  RK_FALSE = 0,
  RK_TRUE = 1,
} RK_BOOL;

#ifndef NULL
#define NULL 0L
#endif

#define RK_NULL 0L
#define RK_SUCCESS 0
#define RK_FAILURE (-1)

#define MAX_FILE_PATH_LEN 256

typedef enum rk_IMAGE_TYPE_E {
  IMAGE_TYPE_UNKNOW = 0,
  IMAGE_TYPE_GRAY8,
  IMAGE_TYPE_GRAY16,
  IMAGE_TYPE_YUV420P,
  IMAGE_TYPE_NV12,
  IMAGE_TYPE_NV21,
  IMAGE_TYPE_YV12,
  IMAGE_TYPE_FBC2,
  IMAGE_TYPE_FBC0,
  IMAGE_TYPE_YUV422P,
  IMAGE_TYPE_NV16,
  IMAGE_TYPE_NV61,
  IMAGE_TYPE_YV16,
  IMAGE_TYPE_YUYV422,
  IMAGE_TYPE_UYVY422,
  IMAGE_TYPE_YUV444SP,
  IMAGE_TYPE_RGB332,
  IMAGE_TYPE_RGB565,
  IMAGE_TYPE_BGR565,
  IMAGE_TYPE_RGB888,
  IMAGE_TYPE_BGR888,
  IMAGE_TYPE_ARGB8888,
  IMAGE_TYPE_ABGR8888,
  IMAGE_TYPE_RGBA8888,
  IMAGE_TYPE_BGRA8888,
  IMAGE_TYPE_JPEG,

  IMAGE_TYPE_BUTT
} IMAGE_TYPE_E;
typedef enum rk_CODEC_TYPE_E {
  RK_CODEC_TYPE_NONE = -1,
  // Audio
  RK_CODEC_TYPE_MP3,
  RK_CODEC_TYPE_MP2,
  RK_CODEC_TYPE_VORBIS,
  RK_CODEC_TYPE_G711A,
  RK_CODEC_TYPE_G711U,
  RK_CODEC_TYPE_G726,
  // Video
  RK_CODEC_TYPE_H264,
  RK_CODEC_TYPE_H265,
  RK_CODEC_TYPE_JPEG,
  RK_CODEC_TYPE_MJPEG,
  RK_CODEC_TYPE_NB
} CODEC_TYPE_E;
typedef enum rkMOD_ID_E {
  RK_ID_UNKNOW = 0,
  RK_ID_VB,
  RK_ID_SYS,
  RK_ID_VDEC,
  RK_ID_VENC,
  RK_ID_H264E,
  RK_ID_JPEGE,
  RK_ID_H265E,
  RK_ID_VO,
  RK_ID_VI,
  RK_ID_VP,
  RK_ID_AIO,
  RK_ID_AI,
  RK_ID_AO,
  RK_ID_AENC,
  RK_ID_ADEC,
  RK_ID_ALGO_MD,
  RK_ID_ALGO_OD,
  RK_ID_RGA,
  RK_ID_VMIX,
  RK_ID_MUXER,
  RK_ID_BUTT,
} MOD_ID_E;

enum {
  /***********************************
   * Common error types
   **********************************/
  RK_ERR_SYS_OK = 0,
  RK_ERR_SYS_NULL_PTR,
  RK_ERR_SYS_NOTREADY,
  RK_ERR_SYS_NOT_PERM,
  RK_ERR_SYS_NOMEM,
  RK_ERR_SYS_ILLEGAL_PARAM,
  RK_ERR_SYS_BUSY,
  RK_ERR_SYS_NOT_SUPPORT,
  RK_ERR_SYS_OP_REPEAT,
  RK_ERR_SYS_ERR_STATUS,

  /***********************************
   * VideoInput error types
   **********************************/
  /* invlalid channel ID */
  RK_ERR_VI_INVALID_CHNID = 10,
  /* system is busy*/
  RK_ERR_VI_BUSY,
  /* channel exists */
  RK_ERR_VI_EXIST,
  RK_ERR_VI_NOT_CONFIG,
  RK_ERR_VI_TIMEOUT,
  RK_ERR_VI_BUF_EMPTY,
  RK_ERR_VI_ILLEGAL_PARAM,
  RK_ERR_VI_NOTREADY,

  /***********************************
   * VideoEncoder error types
   **********************************/
  /* invlalid channel ID */
  RK_ERR_VENC_INVALID_CHNID = 20,
  /* at lease one parameter is illagal ,eg, an illegal enumeration value  */
  RK_ERR_VENC_ILLEGAL_PARAM,
  /* channel exists */
  RK_ERR_VENC_EXIST,
  /* channel exists */
  RK_ERR_VENC_UNEXIST,
  /* using a NULL point */
  RK_ERR_VENC_NULL_PTR,
  /* try to enable or initialize system,device or channel,
     before configing attrib */
  RK_ERR_VENC_NOT_CONFIG,
  /* operation is not supported by NOW */
  RK_ERR_VENC_NOT_SUPPORT,
  /* operation is not permitted ,eg, try to change stati attribute */
  RK_ERR_VENC_NOT_PERM,
  /* failure caused by malloc memory */
  RK_ERR_VENC_NOMEM,
  /* failure caused by malloc buffer */
  RK_ERR_VENC_NOBUF,
  /* no data in buffer */
  RK_ERR_VENC_BUF_EMPTY,
  /* no buffer for new data */
  RK_ERR_VENC_BUF_FULL,
  /* system is not ready,had not initialed or loaded*/
  RK_ERR_VENC_NOTREADY,
  /* system is busy*/
  RK_ERR_VENC_BUSY,

  /***********************************
   * AI::Audio input error types
   **********************************/
  /* invlalid device ID */
  RK_ERR_AI_INVALID_CHNID = 40,
  /* system is busy*/
  RK_ERR_AI_BUSY,
  RK_ERR_AI_EXIST,
  /* system is not open, had not initaled or enabled*/
  RK_ERR_AI_NOTOPEN,
  RK_ERR_AI_NOT_CONFIG,

  /***********************************
   * AO::Audio output error types
   **********************************/
  /* invlalid device ID */
  RK_ERR_AO_INVALID_CHNID = 50,
  /* system is busy*/
  RK_ERR_AO_BUSY,
  /* system is not ready,had not initialed or loaded*/
  RK_ERR_AO_NOTREADY,
  /* system is not open, had not initaled or enabled*/
  RK_ERR_AO_NOTOPEN,
  RK_ERR_AO_ILLEGAL_PARAM,

  /***********************************
   * AENC::Audio encoder error types
   **********************************/
  /* invlalid device ID */
  RK_ERR_AENC_INVALID_CHNID = 60,
  /* system is busy*/
  RK_ERR_AENC_BUSY,
  /* codec not support*/
  RK_ERR_AENC_CODEC_NOT_SUPPORT,
  RK_ERR_AENC_NOTREADY,

  /***********************************
   * Algorithm::MoveDetection error types
   **********************************/
  /* invlalid channel ID */
  RK_ERR_ALGO_MD_INVALID_CHNID = 70,
  /* system is busy*/
  RK_ERR_ALGO_MD_BUSY,
  /* channel exists */
  RK_ERR_ALGO_MD_EXIST,
  RK_ERR_ALGO_MD_NOT_CONFIG,
  /* at lease one parameter is illagal ,eg, an illegal enumeration value  */
  RK_ERR_ALGO_MD_ILLEGAL_PARAM,

  /***********************************
   * Algorithm::OcclusionDetection error types
   **********************************/
  /* invlalid channel ID */
  RK_ERR_ALGO_OD_INVALID_CHNID = 80,
  /* system is busy*/
  RK_ERR_ALGO_OD_BUSY,
  /* channel exists */
  RK_ERR_ALGO_OD_EXIST,
  RK_ERR_ALGO_OD_NOT_CONFIG,
  /* at lease one parameter is illagal ,eg, an illegal enumeration value  */
  RK_ERR_ALGO_OD_ILLEGAL_PARAM,

  /***********************************
   * RGA::rga error types
   **********************************/
  /* invlalid channel ID */
  RK_ERR_RGA_INVALID_CHNID = 90,
  /* system is busy*/
  RK_ERR_RGA_BUSY,
  /* channel exists */
  RK_ERR_RGA_EXIST,
  RK_ERR_RGA_NOT_CONFIG,
  /* at lease one parameter is illagal ,eg, an illegal enumeration value  */
  RK_ERR_RGA_ILLEGAL_PARAM,
  RK_ERR_RGA_NOTREADY,

  /***********************************
   * ADEC::Audio decoder error types
   **********************************/
  /* invlalid device ID */
  RK_ERR_ADEC_INVALID_CHNID = 100,
  /* system is busy*/
  RK_ERR_ADEC_BUSY,
  /* codec not support*/
  RK_ERR_ADEC_CODEC_NOT_SUPPORT,

  /***********************************
   * VO::Video Output error types
   **********************************/
  RK_ERR_VO_INVALID_CHNID = 110,
  RK_ERR_VO_BUSY,
  RK_ERR_VO_EXIST,
  RK_ERR_VO_NOT_CONFIG,
  RK_ERR_VO_TIMEOUT,
  RK_ERR_VO_BUF_EMPTY,
  RK_ERR_VO_ILLEGAL_PARAM,
  RK_ERR_VO_NOTREADY,

  /***********************************
   * VDEC::Video Output error types
   **********************************/
  RK_ERR_VDEC_INVALID_CHNID = 120,
  RK_ERR_VDEC_BUSY,
  RK_ERR_VDEC_EXIST,
  RK_ERR_VDEC_NOT_CONFIG,
  RK_ERR_VDEC_TIMEOUT,
  RK_ERR_VDEC_BUF_EMPTY,
  RK_ERR_VDEC_ILLEGAL_PARAM,
  RK_ERR_VDEC_NOTREADY,

  /***********************************
   * VMIX::Video Mixer error types
   **********************************/
  RK_ERR_VMIX_INVALID_DEVID = 130,
  RK_ERR_VMIX_INVALID_CHNID,
  RK_ERR_VMIX_BUSY,
  RK_ERR_VMIX_EXIST,
  RK_ERR_VMIX_ILLEGAL_PARAM,
  RK_ERR_VMIX_NOTREADY,
  RK_ERR_VMIX_NOTOPEN,

  /***********************************
   * MUXER::Muxer error types
   **********************************/
  RK_ERR_MUXER_INVALID_CHNID = 140,
  RK_ERR_MUXER_BUSY,
  RK_ERR_MUXER_EXIST,
  RK_ERR_MUXER_ILLEGAL_PARAM,
  RK_ERR_MUXER_NOTREADY,
  RK_ERR_MUXER_NOTSUPPORT,

  /***********************************
   * VideoProcess error types
   **********************************/
  /* invlalid channel ID */
  RK_ERR_VP_INVALID_CHNID = 150,
  /* system is busy*/
  RK_ERR_VP_BUSY,
  /* channel exists */
  RK_ERR_VP_EXIST,
  RK_ERR_VP_NOT_CONFIG,
  RK_ERR_VP_TIMEOUT,
  RK_ERR_VP_BUF_EMPTY,
  RK_ERR_VP_ILLEGAL_PARAM,
  RK_ERR_VP_NOTREADY,

  RK_ERR_BUIT,
};

typedef enum rkSAMPLE_FORMAT_E {
  RK_SAMPLE_FMT_NONE = -1,
  RK_SAMPLE_FMT_U8,
  RK_SAMPLE_FMT_S16,
  RK_SAMPLE_FMT_S32,
  RK_SAMPLE_FMT_FLT,
  RK_SAMPLE_FMT_U8P,
  RK_SAMPLE_FMT_S16P,
  RK_SAMPLE_FMT_S32P,
  RK_SAMPLE_FMT_FLTP,
  RK_SAMPLE_FMT_G711A,
  RK_SAMPLE_FMT_G711U,
  RK_SAMPLE_FMT_NB
} SAMPLE_FORMAT_E;

typedef struct rkSIZE_S {
  RK_U32 u32Width;
  RK_U32 u32Height;
} SIZE_S;

typedef struct rkRECT_S {
  RK_S32 s32X;
  RK_S32 s32Y;
  RK_U32 u32Width;
  RK_U32 u32Height;
} RECT_S;

typedef struct rkLOG_LEVEL_CONF_S {
  MOD_ID_E enModId;
  RK_S32 s32Level;
  RK_CHAR cModName[16];
} LOG_LEVEL_CONF_S;

#ifdef __cplusplus
}
#endif
#endif // #ifndef __RKMEDIA_COMMON_
