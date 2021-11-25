// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef __RK_EVENT_
#define __RK_EVENT_
#ifdef __cplusplus
extern "C" {
#endif

#include "rkmedia_common.h"

typedef void (*EventCbFunc)(RK_VOID *pHandle, RK_VOID *event);

#ifdef __cplusplus
}
#endif
#endif // #ifndef __RK_EVENT_
