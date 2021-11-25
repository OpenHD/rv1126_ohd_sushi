#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "rkmedia_list_method.h"

typedef struct node {
  unsigned char data[MAX_NODE_SIZE];
  struct node *next;
} Node;

int create_rkmedia_link_list(rkmedia_link_list **s, unsigned int node_size) {
  if (*s != NULL) {
    printf("create rkmedia_link_list fail, *s not null\n");
    return -1;
  }
  if (node_size > MAX_NODE_SIZE) {
    printf("create rkmedia_link_list fail, node_size(%u) over max(%u)", node_size, MAX_NODE_SIZE);
    return -1;
  }
  *s = (rkmedia_link_list *)malloc(sizeof(rkmedia_link_list));
  (*s)->top = NULL;
  (*s)->size = 0;
  (*s)->node_size = node_size;
  printf("create rkmedia_link_list success\n");
  return 0;
}

void destory_rkmedia_link_list(rkmedia_link_list **s) {
  Node *t = NULL;
  if (*s == NULL)
    return;
  while ((*s)->top) {
    t = (Node *)((*s)->top);
    (*s)->top = (void *)(t->next);
    free(t);
  }
  free(*s);
  *s = NULL;
}

int rkmedia_link_list_push(rkmedia_link_list *s, void *data) {
  if (!data)
    return -1;
  unsigned char *bit_data = (unsigned char *)data;
  Node *t = NULL;
  t = (Node *)malloc(sizeof(Node));
  memcpy(t->data, bit_data, s->node_size);
  if (s->top == NULL) {
    t->next = NULL;
    s->top = (void *)t;
  } else {
    t->next = (Node *)(s->top);
    s->top = (void *)t;
  }
  s->size++;
  return 0;
}

int rkmedia_link_list_pop(rkmedia_link_list *s, void *data) {
  Node *t = NULL;
  if (s == NULL || s->top == NULL)
    return -1;
  t = (Node *)(s->top);
  unsigned char *bit_data = (unsigned char *)data;
  memcpy(bit_data, t->data, s->node_size);
  s->top = (void *)(t->next);
  free(t);
  s->size--;
  return 0;
}

void rkmedia_link_list_drop(rkmedia_link_list *s) {
  Node *t = NULL;
  if (s == NULL || s->top == NULL)
    return;
  t = (Node *)(s->top);
  s->top = (void *)(t->next);
  free(t);
  s->size--;
}

int rkmedia_link_list_size(rkmedia_link_list *s) {
  if (s == NULL)
    return -1;
  return s->size;
}

int get_head(char_list_t *p_queue) {
  if (p_queue->tail >= p_queue->qty) {
    return p_queue->tail - p_queue->qty;
  }
  return CHAR_QUEUE_SIZE - (p_queue->qty - p_queue->tail);
}

void rk_create_list(char_list_t *p_queue) {
  pthread_mutex_init(&p_queue->list_mutex, NULL);
  pthread_mutex_lock(&p_queue->list_mutex);
  memset(p_queue, 0, sizeof(char_list_t));
  pthread_mutex_unlock(&p_queue->list_mutex);
}

void rk_destroy_list(char_list_t *p_queue) {
  if (!p_queue)
    return;
  pthread_mutex_lock(&p_queue->list_mutex);
  for (int i = 0; i < CHAR_QUEUE_SIZE; i++) {
    if (p_queue->buf[i]) {
      free(p_queue->buf[i]);
      p_queue->buf[i] = NULL;
    }
  }
  memset(p_queue, 0, sizeof(char_list_t));
  pthread_mutex_unlock(&p_queue->list_mutex);
}

int rk_list_push(char_list_t *p_queue, char *p_val) {
  if (p_queue->qty == CHAR_QUEUE_SIZE) {
    usleep(100);
  }
  if (p_queue->tail == CHAR_QUEUE_SIZE) {
    p_queue->tail = 0;
  }
  pthread_mutex_lock(&p_queue->list_mutex);
  p_queue->buf[p_queue->tail] = (char *)malloc(sizeof(char) * SNAP_NAME_LEN);
  memset(p_queue->buf[p_queue->tail], 0, sizeof(char) * SNAP_NAME_LEN);
  strncpy(p_queue->buf[p_queue->tail], p_val, SNAP_NAME_LEN);
  p_queue->tail++;
  p_queue->qty++;
  pthread_mutex_unlock(&p_queue->list_mutex);
  return 0;
}

int rk_list_pop(char_list_t *p_queue, char *p_val) {
  if (!p_queue->qty) {
    return -1;
  }
  pthread_mutex_lock(&p_queue->list_mutex);
  strncpy(p_val, p_queue->buf[get_head(p_queue)], SNAP_NAME_LEN);
  free(p_queue->buf[get_head(p_queue)]);
  p_queue->buf[get_head(p_queue)] = NULL;
  p_queue->qty--;
  pthread_mutex_unlock(&p_queue->list_mutex);
  return 0;
}