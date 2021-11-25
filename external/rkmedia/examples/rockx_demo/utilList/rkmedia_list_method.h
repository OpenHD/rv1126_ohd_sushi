// Copyright 2021 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __RKMEDIA_LIST_METHOD_H__
#define __RKMEDIA_LIST_METHOD_H__

#include <pthread.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX_NODE_SIZE
#define MAX_NODE_SIZE 4096
#endif

#ifndef CHAR_QUEUE_SIZE
#define CHAR_QUEUE_SIZE 30
#endif

#ifndef SNAP_NAME_LEN
#define SNAP_NAME_LEN 64
#endif

typedef struct my_stack {
  int size;
  unsigned int node_size;
  void *top;
} rkmedia_link_list;

typedef struct char_list_s {
  int qty;
  int tail;
  pthread_mutex_t list_mutex;
  char *buf[CHAR_QUEUE_SIZE];
} char_list_t;

int create_rkmedia_link_list(rkmedia_link_list **s, unsigned int node_size);
void destory_rkmedia_link_list(rkmedia_link_list **s);
int rkmedia_link_list_push(rkmedia_link_list *s, void *data);
int rkmedia_link_list_pop(rkmedia_link_list *s, void *data);
void rkmedia_link_list_drop(rkmedia_link_list *s);
int rkmedia_link_list_size(rkmedia_link_list *s);

int get_head(char_list_t *p_queue);
void rk_create_list(char_list_t *p_queue);
void rk_destroy_list(char_list_t *p_queue);
int rk_list_push(char_list_t *p_queue, char *p_val);
int rk_list_pop(char_list_t *p_queue, char *p_val);

#ifdef __cplusplus
}
#endif

#endif // __RKMEDIA_LIST_METHOD_H__