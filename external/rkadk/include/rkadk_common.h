/*
 * Copyright (c) 2021 Rockchip, Inc. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef __RKADK_COMMON_H__
#define __RKADK_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "rkadk_log.h"
#include <pthread.h>

// dump config info for debug
//#define RKADK_DUMP_CONFIG

// dump isp process result
//#define RKADK_DUMP_ISP_RESULT

// rkispp_m_bypass rkispp_scale0 rkispp_scale1 rkispp_scale2
#define RKADK_ISPP_VI_NODE_CNT 4

// camera sensor count
#define RKADK_MAX_SENSOR_CNT 1

// simultaneous record files num
#define RECORD_FILE_NUM_MAX 2

#define RKADK_MAX_FILE_PATH_LEN 256

typedef unsigned char RKADK_U8;
typedef unsigned short RKADK_U16;
typedef unsigned int RKADK_U32;

typedef signed char RKADK_S8;
typedef short RKADK_S16;
typedef int RKADK_S32;

typedef unsigned long RKADK_UL;
typedef signed long RKADK_SL;

typedef float RKADK_FLOAT;
typedef double RKADK_DOUBLE;

#ifndef _M_IX86
typedef unsigned long long RKADK_U64;
typedef long long RKADK_S64;
#else
typedef unsigned __int64 RKADK_U64;
typedef __int64 RKADK_S64;
#endif

typedef char RKADK_CHAR;
#define RKADK_VOID void

typedef unsigned int RKADK_HANDLE;

typedef RKADK_VOID *RKADK_MW_PTR;

typedef char (*ARRAY_FILE_NAME)[RKADK_MAX_FILE_PATH_LEN];

typedef enum {
  RKADK_FALSE = 0,
  RKADK_TRUE = 1,
} RKADK_BOOL;

#ifndef NULL
#define NULL 0L
#endif

#define RKADK_NULL 0L
#define RKADK_SUCCESS 0
#define RKADK_FAILURE (-1)
#define RKADK_PARAM_NOT_EXIST (-2)

#ifndef UPALIGNTO
#define UPALIGNTO(value, align) ((value + align - 1) & (~(align - 1)))
#endif

/* Pointer Check */
#define RKADK_CHECK_POINTER(p, errcode)                                        \
  do {                                                                         \
    if (!(p)) {                                                                \
      RKADK_LOGE("pointer[%s] is NULL", #p);                                   \
      return errcode;                                                          \
    }                                                                          \
  } while (0)

/* Pointer Check */
#define RKADK_CHECK_POINTER_N(p)                                               \
  do {                                                                         \
    if (!(p)) {                                                                \
      RKADK_LOGE("pointer[%s] is NULL", #p);                                   \
      return;                                                                  \
    }                                                                          \
  } while (0)

/* CameraID Check */
#define RKADK_CHECK_CAMERAID(id, errcode)                                      \
  do {                                                                         \
    if ((id) >= RKADK_MAX_SENSOR_CNT) {                                        \
      RKADK_LOGE("invalid camera id: %d", id);                                 \
      return errcode;                                                          \
    }                                                                          \
  } while (0)

/* Init Check */
#define RKADK_CHECK_INIT(init, errcode)                                        \
  do {                                                                         \
    if (!(init)) {                                                             \
      RKADK_LOGE("[%s] not init", #init);                                      \
      return errcode;                                                          \
    }                                                                          \
  } while (0)

/* Mutex Lock */
#define RKADK_MUTEX_INIT_LOCK(mutex)                                           \
  do {                                                                         \
    (RKADK_VOID) pthread_mutex_init(&mutex, NULL);                             \
  } while (0)
#define RKADK_MUTEX_LOCK(mutex)                                                \
  do {                                                                         \
    (RKADK_VOID) pthread_mutex_lock(&mutex);                                   \
  } while (0)
#define RKADK_MUTEX_UNLOCK(mutex)                                              \
  do {                                                                         \
    (RKADK_VOID) pthread_mutex_unlock(&mutex);                                 \
  } while (0)
#define RKADK_MUTEX_DESTROY(mutex)                                             \
  do {                                                                         \
    (RKADK_VOID) pthread_mutex_destroy(&mutex);                                \
  } while (0)

#define RKADK_CHECK_EQUAL(a, b, mutex, ret)                                    \
  do {                                                                         \
    if (a == b) {                                                              \
      RKADK_MUTEX_UNLOCK(mutex);                                               \
      return ret;                                                              \
    }                                                                          \
  } while (0)

#define RKADK_SWAP16(x)                                                        \
  (((RKADK_U16)(x)&0x00ff) << 8) | (((RKADK_U16)(x)&0xff00) >> 8)

#define RKADK_SWAP32(x)                                                        \
  ((((RKADK_U32)(x) & (0xff000000)) >> 24) |                                   \
   (((RKADK_U32)(x) & (0x00ff0000)) >> 8) |                                    \
   (((RKADK_U32)(x) & (0x0000ff00)) << 8) |                                    \
   (((RKADK_U32)(x) & (0x000000ff)) << 24))

#ifdef __cplusplus
}
#endif
#endif
