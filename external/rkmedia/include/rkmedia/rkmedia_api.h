#ifndef __RKMEDIA_API_
#define __RKMEDIA_API_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "rkmedia_adec.h"
#include "rkmedia_aenc.h"
#include "rkmedia_ai.h"
#include "rkmedia_ao.h"
#include "rkmedia_buffer.h"
#include "rkmedia_common.h"
#include "rkmedia_event.h"
#include "rkmedia_move_detection.h"
#include "rkmedia_muxer.h"
#include "rkmedia_occlusion_detection.h"
#include "rkmedia_rga.h"
#include "rkmedia_vdec.h"
#include "rkmedia_venc.h"
#include "rkmedia_vi.h"
#include "rkmedia_vmix.h"
#include "rkmedia_vo.h"

// Platform resource number configuration
#define VI_MAX_CHN_NUM 8
#define VP_MAX_CHN_NUM 8
#define VENC_MAX_CHN_NUM 16
#define AI_MAX_CHN_NUM 8
#define AO_MAX_CHN_NUM 8
#define AENC_MAX_CHN_NUM 16
#define ADEC_MAX_CHN_NUM 16
#define ALGO_MD_MAX_CHN_NUM VI_MAX_CHN_NUM
#define ALGO_OD_MAX_CHN_NUM VI_MAX_CHN_NUM
#define RGA_MAX_CHN_NUM 16
#define VO_MAX_CHN_NUM 2
#define VDEC_MAX_CHN_NUM 16

typedef RK_S32 VI_PIPE;
typedef RK_S32 VI_CHN;
typedef RK_S32 VP_PIPE;
typedef RK_S32 VP_CHN;
typedef RK_S32 VENC_CHN;
typedef RK_S32 VDEC_CHN;
typedef RK_S32 AI_CHN;
typedef RK_S32 AO_CHN;
typedef RK_S32 AENC_CHN;
typedef RK_S32 ADEC_CHN;
typedef RK_S32 ALGO_MD_CHN;
typedef RK_S32 ALGO_OD_CHN;
typedef RK_S32 RGA_CHN;
typedef RK_S32 VO_CHN;
typedef RK_S32 VMIX_DEV;
typedef RK_S32 VMIX_CHN;
typedef RK_S32 MUXER_CHN;

typedef struct rkMPP_CHN_S {
  MOD_ID_E enModId;
  RK_S32 s32DevId;
  RK_S32 s32ChnId;
} MPP_CHN_S;

typedef struct rkMPP_FPS_ATTR_S {
  RK_S32 s32FpsInNum;
  RK_S32 s32FpsInDen;
  RK_S32 s32FpsOutNum;
  RK_S32 s32FpsOutDen;
} MPP_FPS_ATTR_S;

typedef struct rkMPP_RECV_PIC_PARAM_S {
  RK_S32 s32RecvPicNum;
} MPP_RECV_PIC_PARAM_S;

typedef MPP_RECV_PIC_PARAM_S VENC_RECV_PIC_PARAM_S;

/********************************************************************
 * SYS Ctrl api
 ********************************************************************/
_CAPI RK_S32 RK_MPI_SYS_Init();
_CAPI RK_VOID RK_MPI_SYS_DumpChn(MOD_ID_E enModId);
_CAPI RK_S32 RK_MPI_SYS_Bind(const MPP_CHN_S *pstSrcChn,
                             const MPP_CHN_S *pstDestChn);
_CAPI RK_S32 RK_MPI_SYS_UnBind(const MPP_CHN_S *pstSrcChn,
                               const MPP_CHN_S *pstDestChn);
_CAPI RK_S32 RK_MPI_SYS_RegisterOutCb(const MPP_CHN_S *pstChn, OutCbFunc cb);
_CAPI RK_S32 RK_MPI_SYS_RegisterOutCbEx(const MPP_CHN_S *pstChn, OutCbFuncEx cb,
                                        RK_VOID *pHandle);
_CAPI RK_S32 RK_MPI_SYS_RegisterEventCb(const MPP_CHN_S *pstChn,
                                        RK_VOID *pHandle, EventCbFunc cb);
_CAPI RK_S32 RK_MPI_SYS_SendMediaBuffer(MOD_ID_E enModID, RK_S32 s32ChnID,
                                        MEDIA_BUFFER buffer);
_CAPI RK_S32 RK_MPI_SYS_DevSendMediaBuffer(MOD_ID_E enModID, RK_S32 s32DevId,
                                           RK_S32 s32ChnID,
                                           MEDIA_BUFFER buffer);
_CAPI RK_S32 RK_MPI_SYS_SetMediaBufferDepth(MOD_ID_E enModID, RK_S32 s32ChnID,
                                            RK_S32 u32Depth);
_CAPI RK_S32 RK_MPI_SYS_StartGetMediaBuffer(MOD_ID_E enModID, RK_S32 s32ChnID);
_CAPI RK_S32 RK_MPI_SYS_StopGetMediaBuffer(MOD_ID_E enModID, RK_S32 s32ChnID);
_CAPI MEDIA_BUFFER RK_MPI_SYS_GetMediaBuffer(MOD_ID_E enModID, RK_S32 s32ChnID,
                                             RK_S32 s32MilliSec);
_CAPI RK_S32 RK_MPI_SYS_SetFrameRate(MOD_ID_E enModID, RK_S32 s32ChnID,
                                     MPP_FPS_ATTR_S *pstFpsAttr);
_CAPI RK_S32
RK_MPI_SYS_StartRecvFrame(MOD_ID_E enModID, RK_S32 s32ChnID,
                          const MPP_RECV_PIC_PARAM_S *pstRecvParam);
_CAPI RK_S32 RK_MPI_LOG_SetLevelConf(LOG_LEVEL_CONF_S *pstConf);
_CAPI RK_S32 RK_MPI_LOG_GetLevelConf(LOG_LEVEL_CONF_S *pstConf);

/********************************************************************
 * Vi api
 ********************************************************************/
_CAPI RK_S32 RK_MPI_VI_SetChnAttr(VI_PIPE ViPipe, VI_CHN ViChn,
                                  const VI_CHN_ATTR_S *pstChnAttr);
_CAPI RK_S32 RK_MPI_VI_GetChnAttr(VI_PIPE ViPipe, VI_CHN ViChn,
                                  VI_CHN_ATTR_S *pstChnAttr);
_CAPI RK_S32 RK_MPI_VI_EnableChn(VI_PIPE ViPipe, VI_CHN ViChn);
_CAPI RK_S32 RK_MPI_VI_DisableChn(VI_PIPE ViPipe, VI_CHN ViChn);
_CAPI RK_S32 RK_MPI_VI_StartRegionLuma(VI_CHN ViChn);
_CAPI RK_S32 RK_MPI_VI_StopRegionLuma(VI_CHN ViChn);
_CAPI RK_S32 RK_MPI_VI_GetChnRegionLuma(
    VI_PIPE ViPipe, VI_CHN ViChn, const VIDEO_REGION_INFO_S *pstRegionInfo,
    RK_U64 *pu64LumaData, RK_S32 s32MilliSec);
_CAPI RK_S32 RK_MPI_VI_StartStream(VI_PIPE ViPipe, VI_CHN ViChn);
_CAPI RK_S32 RK_MPI_VI_SetUserPic(VI_CHN ViChn,
                                  VI_USERPIC_ATTR_S *pstUsrPicAttr);
_CAPI RK_S32 RK_MPI_VI_EnableUserPic(VI_CHN ViChn);
_CAPI RK_S32 RK_MPI_VI_DisableUserPic(VI_CHN ViChn);
_CAPI RK_S32 RK_MPI_VI_RGN_SetCover(VI_PIPE ViPipe, VI_CHN ViChn,
                                    const OSD_REGION_INFO_S *pstRgnInfo,
                                    const COVER_INFO_S *pstCoverInfo);
_CAPI RK_S32 RK_MPI_VI_RGN_SetBitMap(VI_PIPE ViPipe, VI_CHN ViChn,
                                     const OSD_REGION_INFO_S *pstRgnInfo,
                                     const BITMAP_S *pstBitmap);
_CAPI RK_S32 RK_MPI_VI_GetStatus(VI_CHN ViChn);

/********************************************************************
 * Venc api
 ********************************************************************/
_CAPI RK_S32 RK_MPI_VENC_CreateChn(VENC_CHN VencChn,
                                   VENC_CHN_ATTR_S *stVencChnAttr);
// Deprecated application interfaces will be abandoned
_CAPI RK_S32 RK_MPI_VENC_CreateJpegLightChn(VENC_CHN VencChn,
                                            VENC_CHN_ATTR_S *stVencChnAttr);
_CAPI RK_S32 RK_MPI_VENC_GetVencChnAttr(VENC_CHN VencChn,
                                        VENC_CHN_ATTR_S *stVencChnAttr);
_CAPI RK_S32 RK_MPI_VENC_SetVencChnAttr(VENC_CHN VencChn,
                                        VENC_CHN_ATTR_S *stVencChnAttr);
_CAPI RK_S32 RK_MPI_VENC_GetChnParam(VENC_CHN VencChn,
                                     VENC_CHN_PARAM_S *stVencChnParam);
_CAPI RK_S32 RK_MPI_VENC_SetChnParam(VENC_CHN VencChn,
                                     VENC_CHN_PARAM_S *stVencChnParam);
_CAPI RK_S32 RK_MPI_VENC_GetRcParam(VENC_CHN VencChn,
                                    VENC_RC_PARAM_S *pstRcParam);
_CAPI RK_S32 RK_MPI_VENC_SetRcParam(VENC_CHN VencChn,
                                    const VENC_RC_PARAM_S *pstRcParam);
_CAPI RK_S32 RK_MPI_VENC_SetJpegParam(VENC_CHN VencChn,
                                      const VENC_JPEG_PARAM_S *pstJpegParam);

_CAPI RK_S32 RK_MPI_VENC_SetRcMode(VENC_CHN VencChn, VENC_RC_MODE_E RcMode);
_CAPI RK_S32 RK_MPI_VENC_SetRcQuality(VENC_CHN VencChn,
                                      VENC_RC_QUALITY_E RcQuality);
_CAPI RK_S32 RK_MPI_VENC_SetBitrate(VENC_CHN VencChn, RK_U32 u32BitRate,
                                    RK_U32 u32MinBitRate, RK_U32 u32MaxBitRate);
_CAPI RK_S32 RK_MPI_VENC_RequestIDR(VENC_CHN VencChn, RK_BOOL bInstant);
_CAPI RK_S32 RK_MPI_VENC_SetFps(VENC_CHN VencChn, RK_U8 u8OutNum,
                                RK_U8 u8OutDen, RK_U8 u8InNum, RK_U8 u8InDen);
_CAPI RK_S32 RK_MPI_VENC_SetGop(VENC_CHN VencChn, RK_U32 u32Gop);
_CAPI RK_S32 RK_MPI_VENC_SetAvcProfile(VENC_CHN VencChn, RK_U32 u32Profile,
                                       RK_U32 u32Level);
_CAPI RK_S32 RK_MPI_VENC_InsertUserData(VENC_CHN VencChn, RK_U8 *pu8Data,
                                        RK_U32 u32Len);
_CAPI RK_S32 RK_MPI_VENC_SetResolution(
    VENC_CHN VencChn, VENC_RESOLUTION_PARAM_S stResolutionParam);

_CAPI RK_S32 RK_MPI_VENC_SetRotation(VENC_CHN VeChn,
                                     VENC_ROTATION_E rotation_rate);

_CAPI RK_S32 RK_MPI_VENC_GetRoiAttr(VENC_CHN VencChn,
                                    VENC_ROI_ATTR_S *pstRoiAttr,
                                    RK_S32 roi_index);
_CAPI RK_S32 RK_MPI_VENC_SetRoiAttr(VENC_CHN VencChn,
                                    const VENC_ROI_ATTR_S *pstRoiAttr,
                                    RK_S32 region_cnt);

_CAPI RK_S32 RK_MPI_VENC_SetGopMode(VENC_CHN VencChn,
                                    VENC_GOP_ATTR_S *pstGopModeAttr);

_CAPI RK_S32 RK_MPI_VENC_RGN_Init(VENC_CHN VencChn,
                                  VENC_COLOR_TBL_S *stColorTbl);

_CAPI RK_S32 RK_MPI_VENC_RGN_SetBitMap(VENC_CHN VencChn,
                                       const OSD_REGION_INFO_S *pstRgnInfo,
                                       const BITMAP_S *pstBitmap);
_CAPI RK_S32 RK_MPI_VENC_RGN_SetCover(VENC_CHN VencChn,
                                      const OSD_REGION_INFO_S *pstRgnInfo,
                                      const COVER_INFO_S *pstCoverInfo);
_CAPI RK_S32 RK_MPI_VENC_RGN_SetCoverEx(VENC_CHN VencChn,
                                        const OSD_REGION_INFO_S *pstRgnInfo,
                                        const COVER_INFO_S *pstCoverInfo);
_CAPI RK_S32 RK_MPI_VENC_RGN_SetPaletteId(
    VENC_CHN VencChn, const OSD_REGION_INFO_S *pstRgnInfo,
    const OSD_COLOR_PALETTE_BUF_S *pstColPalBuf);
_CAPI RK_S32 RK_MPI_VENC_StartRecvFrame(
    VENC_CHN VencChn, const VENC_RECV_PIC_PARAM_S *pstRecvParam);
_CAPI RK_S32 RK_MPI_VENC_DestroyChn(VENC_CHN VencChn);
_CAPI RK_S32 RK_MPI_VENC_GetFd(VENC_CHN VencChn);
_CAPI RK_S32 RK_MPI_VENC_QueryStatus(VENC_CHN VencChn,
                                     VENC_CHN_STATUS_S *pstStatus);

_CAPI RK_S32 RK_MPI_VENC_SetSuperFrameStrategy(
    VENC_CHN VeChn, const VENC_SUPERFRAME_CFG_S *pstSuperFrmParam);
_CAPI RK_S32 RK_MPI_VENC_GetSuperFrameStrategy(
    VENC_CHN VeChn, VENC_SUPERFRAME_CFG_S *pstSuperFrmParam);

/********************************************************************
 * Ai api
 ********************************************************************/
_CAPI RK_S32 RK_MPI_AI_SetChnAttr(VENC_CHN AiChn, const AI_CHN_ATTR_S *pstAttr);
_CAPI RK_S32 RK_MPI_AI_EnableChn(AI_CHN AiChn);
_CAPI RK_S32 RK_MPI_AI_DisableChn(AI_CHN AiChn);
_CAPI RK_S32 RK_MPI_AI_SetVolume(AI_CHN AiChn, RK_S32 s32Volume);
_CAPI RK_S32 RK_MPI_AI_GetVolume(AI_CHN AiChn, RK_S32 *ps32Volume);
_CAPI RK_S32 RK_MPI_AI_SetTalkVqeAttr(AI_CHN AiChn,
                                      AI_TALKVQE_CONFIG_S *pstVqeConfig);
_CAPI RK_S32 RK_MPI_AI_GetTalkVqeAttr(AI_CHN AiChn,
                                      AI_TALKVQE_CONFIG_S *pstVqeConfig);
_CAPI RK_S32 RK_MPI_AI_SetRecordVqeAttr(AI_CHN AiChn,
                                        AI_RECORDVQE_CONFIG_S *pstVqeConfig);
_CAPI RK_S32 RK_MPI_AI_GetRecordVqeAttr(AI_CHN AiChn,
                                        AI_RECORDVQE_CONFIG_S *pstVqeConfig);
_CAPI RK_S32 RK_MPI_AI_EnableVqe(AI_CHN AiChn);
_CAPI RK_S32 RK_MPI_AI_StartStream(AI_CHN AiChn);
_CAPI RK_S32 RK_MPI_AI_DisableVqe(AI_CHN AiChn);

/********************************************************************
 * Ao api
 ********************************************************************/
_CAPI RK_S32 RK_MPI_AO_SetChnAttr(AO_CHN AoChn, const AO_CHN_ATTR_S *pstAttr);
_CAPI RK_S32 RK_MPI_AO_EnableChn(AO_CHN AoChn);
_CAPI RK_S32 RK_MPI_AO_DisableChn(AO_CHN AoChn);
_CAPI RK_S32 RK_MPI_AO_SetVolume(AO_CHN AoChn, RK_S32 s32Volume);
_CAPI RK_S32 RK_MPI_AO_GetVolume(AO_CHN AoChn, RK_S32 *ps32Volume);
_CAPI RK_S32 RK_MPI_AO_SetVqeAttr(AO_CHN AoChn, AO_VQE_CONFIG_S *pstVqeConfig);
_CAPI RK_S32 RK_MPI_AO_GetVqeAttr(AO_CHN AoChn, AO_VQE_CONFIG_S *pstVqeConfig);
_CAPI RK_S32 RK_MPI_AO_EnableVqe(AO_CHN AoChn);
_CAPI RK_S32 RK_MPI_AO_DisableVqe(AO_CHN AoChn);
_CAPI RK_S32 RK_MPI_AO_QueryChnStat(AO_CHN AoChn, AO_CHN_STATE_S *pstStatus);
_CAPI RK_S32 RK_MPI_AO_ClearChnBuf(AO_CHN AoChn);

/********************************************************************
 * Aenc api
 ********************************************************************/
_CAPI RK_S32 RK_MPI_AENC_CreateChn(AENC_CHN AencChn,
                                   const AENC_CHN_ATTR_S *pstAttr);
_CAPI RK_S32 RK_MPI_AENC_DestroyChn(AENC_CHN AencChn);
_CAPI RK_S32 RK_MPI_AENC_GetFd(AENC_CHN AencChn);
_CAPI RK_S32 RK_MPI_AENC_SetMute(AENC_CHN AencChn, RK_BOOL bEnable);

/********************************************************************
 * Algorithm::Move Detection api
 ********************************************************************/
_CAPI RK_S32 RK_MPI_ALGO_MD_CreateChn(ALGO_MD_CHN MdChn,
                                      const ALGO_MD_ATTR_S *pstChnAttr);
_CAPI RK_S32 RK_MPI_ALGO_MD_DestroyChn(ALGO_MD_CHN MdChn);
_CAPI RK_S32 RK_MPI_ALGO_MD_SetRegions(ALGO_MD_CHN MdChn, RECT_S *stRoiRects,
                                       RK_S32 regionNum);
_CAPI RK_S32 RK_MPI_ALGO_MD_EnableSwitch(ALGO_MD_CHN MdChn, RK_BOOL bEnable);

/********************************************************************
 * Algorithm::Occlusion Detection api
 ********************************************************************/
_CAPI RK_S32 RK_MPI_ALGO_OD_CreateChn(ALGO_OD_CHN OdChn,
                                      const ALGO_OD_ATTR_S *pstChnAttr);
_CAPI RK_S32 RK_MPI_ALGO_OD_DestroyChn(ALGO_OD_CHN OdChn);
_CAPI RK_S32 RK_MPI_ALGO_OD_EnableSwitch(ALGO_OD_CHN OdChn, RK_BOOL bEnable);

/********************************************************************
 * Rga api
 ********************************************************************/
_CAPI RK_S32 RK_MPI_RGA_CreateChn(RGA_CHN RgaChn, RGA_ATTR_S *pstAttr);
_CAPI RK_S32 RK_MPI_RGA_DestroyChn(RGA_CHN RgaChn);
_CAPI RK_S32 RK_MPI_RGA_SetChnAttr(RGA_CHN RgaChn, const RGA_ATTR_S *pstAttr);
_CAPI RK_S32 RK_MPI_RGA_GetChnAttr(RGA_CHN RgaChn, RGA_ATTR_S *pstAttr);
_CAPI RK_S32 RK_MPI_RGA_RGN_SetBitMap(RGA_CHN RgaChn,
                                      const OSD_REGION_INFO_S *pstRgnInfo,
                                      const BITMAP_S *pstBitmap);
_CAPI RK_S32 RK_MPI_RGA_GetChnRegionLuma(
    RGA_CHN RgaChn, const VIDEO_REGION_INFO_S *pstRegionInfo,
    RK_U64 *pu64LumaData, RK_S32 s32MilliSec);
_CAPI RK_S32 RK_MPI_RGA_RGN_SetCover(RGA_CHN RgaChn,
                                     const OSD_REGION_INFO_S *pstRgnInfo,
                                     const COVER_INFO_S *pstCoverInfo);

/********************************************************************
 * Adec api
 ********************************************************************/
_CAPI RK_S32 RK_MPI_ADEC_CreateChn(ADEC_CHN AdecChn,
                                   const ADEC_CHN_ATTR_S *pstAttr);
_CAPI RK_S32 RK_MPI_ADEC_DestroyChn(ADEC_CHN AdecChn);

/********************************************************************
 * VO api
 ********************************************************************/
_CAPI RK_S32 RK_MPI_VO_CreateChn(VO_CHN VoChn, const VO_CHN_ATTR_S *pstAttr);
_CAPI RK_S32 RK_MPI_VO_GetChnAttr(VO_CHN VoChn, VO_CHN_ATTR_S *pstAttr);
_CAPI RK_S32 RK_MPI_VO_SetChnAttr(VO_CHN VoChn, const VO_CHN_ATTR_S *pstAttr);
_CAPI RK_S32 RK_MPI_VO_DestroyChn(VO_CHN VoChn);

/********************************************************************
 * VDEC api
 ********************************************************************/
_CAPI RK_S32 RK_MPI_VDEC_CreateChn(VDEC_CHN VdChn,
                                   const VDEC_CHN_ATTR_S *pstAttr);
_CAPI RK_S32 RK_MPI_VDEC_DestroyChn(VDEC_CHN VdChn);

/********************************************************************
 * VMIX api
 ********************************************************************/
_CAPI RK_S32 RK_MPI_VMIX_CreateDev(VMIX_DEV VmDev, VMIX_DEV_INFO_S *pstDevInfo);
_CAPI RK_S32 RK_MPI_VMIX_DestroyDev(VMIX_DEV VmDev);
_CAPI RK_S32 RK_MPI_VMIX_EnableChn(VMIX_DEV VmDev, VMIX_CHN VmChn);
_CAPI RK_S32 RK_MPI_VMIX_DisableChn(VMIX_DEV VmDev, VMIX_CHN VmChn);
_CAPI RK_S32 RK_MPI_VMIX_SetLineInfo(VMIX_DEV VmDev, VMIX_CHN VmChn,
                                     VMIX_LINE_INFO_S VmLine);
_CAPI RK_S32 RK_MPI_VMIX_ShowChn(VMIX_DEV VmDev, VMIX_CHN VmChn);
_CAPI RK_S32 RK_MPI_VMIX_HideChn(VMIX_DEV VmDev, VMIX_CHN VmChn);
_CAPI RK_S32 RK_MPI_VMIX_RGN_SetBitMap(VMIX_DEV VmDev, VMIX_CHN VmChn,
                                       const OSD_REGION_INFO_S *pstRgnInfo,
                                       const BITMAP_S *pstBitmap);
_CAPI RK_S32 RK_MPI_VMIX_GetRegionLuma(VMIX_DEV VmDev,
                                       const VIDEO_REGION_INFO_S *pstRegionInfo,
                                       RK_U64 *pu64LumaData,
                                       RK_S32 s32MilliSec);
_CAPI RK_S32 RK_MPI_VMIX_GetChnRegionLuma(
    VMIX_DEV VmDev, VMIX_CHN VmChn, const VIDEO_REGION_INFO_S *pstRegionInfo,
    RK_U64 *pu64LumaData, RK_S32 s32MilliSec);
_CAPI RK_S32 RK_MPI_VMIX_RGN_SetCover(VMIX_DEV VmDev, VMIX_CHN VmChn,
                                      const OSD_REGION_INFO_S *pstRgnInfo,
                                      const COVER_INFO_S *pstCoverInfo);

/********************************************************************
 * MUXER api
 ********************************************************************/
_CAPI RK_S32 RK_MPI_MUXER_EnableChn(MUXER_CHN VmChn, MUXER_CHN_ATTR_S *pstAttr);
_CAPI RK_S32 RK_MPI_MUXER_DisableChn(MUXER_CHN VmChn);
_CAPI RK_S32 RK_MPI_MUXER_Bind(const MPP_CHN_S *pstSrcChn,
                               const MUXER_CHN_S *pstDestChn);
_CAPI RK_S32 RK_MPI_MUXER_UnBind(const MPP_CHN_S *pstSrcChn,
                                 const MUXER_CHN_S *pstDestChn);
_CAPI RK_S32 RK_MPI_MUXER_StreamStart(MUXER_CHN VmChn);
_CAPI RK_S32 RK_MPI_MUXER_StreamStop(MUXER_CHN VmChn);
_CAPI RK_S32 RK_MPI_MUXER_ManualSplit(MUXER_CHN VmChn,
                                      MUXER_MANUAL_SPLIT_ATTR_S *pstSplitAttr);

/********************************************************************
 * Vp api
 ********************************************************************/
_CAPI RK_S32 RK_MPI_VP_SetChnAttr(VP_PIPE VpPipe, VP_CHN VpChn,
                                  const VP_CHN_ATTR_S *pstChnAttr);
_CAPI RK_S32 RK_MPI_VP_EnableChn(VP_PIPE VpPipe, VP_CHN VpChn);
_CAPI RK_S32 RK_MPI_VP_DisableChn(VP_PIPE VpPipe, VP_CHN VpChn);

#ifdef __cplusplus
}
#endif
#endif //__RKMEDIA_API_
