#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "socket.h"

union {
  struct cmsghdr cm;
  char control[CMSG_SPACE(sizeof(int))];
} control_un;

int cli_begin(char *name) {
  int fd;
  int len = 0;

  fd = cli_connect(CS_PATH);
  if (fd < 0)
    return fd;
  len = strlen(name) + 1;
  sock_write(fd, (char *)&len, sizeof(int));
  sock_write(fd, name, len);

  return fd;
}

int cli_end(int fd) {
  int ret;

  sock_read(fd, &ret, sizeof(int));
  close(fd);

  return 0;
}

int cli_connect(const char *name) {
  int ret;
  int fd, len;
  struct sockaddr_un unix_addr;

  if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    return (-1);

  memset(&unix_addr, 0, sizeof(unix_addr));
  unix_addr.sun_family = AF_UNIX;
  strcpy(unix_addr.sun_path, name);
  len = sizeof(unix_addr.sun_family) + strlen(unix_addr.sun_path);

  if (connect(fd, (struct sockaddr *)&unix_addr, len) < 0)
    goto error;

  if (sock_read(fd, &ret, sizeof(int)) == SOCKERR_CLOSED)
    return -1;

  return (fd);

error:
  printf("%s err\n", __func__);
  close(fd);
  return (-1);
}

int serv_listen(const char *name) {
  int fd, len;
  struct sockaddr_un unix_addr;

  if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    return (-1);

  fcntl(fd, F_SETFD, FD_CLOEXEC);

  unlink(name);
  memset(&unix_addr, 0, sizeof(unix_addr));
  unix_addr.sun_family = AF_UNIX;
  strcpy(unix_addr.sun_path, name);
  len = sizeof(unix_addr.sun_family) + strlen(unix_addr.sun_path);

  if (bind(fd, (struct sockaddr *)&unix_addr, len) < 0)
    goto error;
  if (chmod(name, 0666) < 0)
    goto error;

  if (listen(fd, 5) < 0)
    goto error;

  return (fd);

error:
  close(fd);
  return (-1);
}

int serv_accept(int fd) {
  int ret;
  socklen_t len;
  struct sockaddr_un unix_addr;

  len = sizeof(unix_addr);

  if ((ret = accept(fd, (struct sockaddr *)&unix_addr, &len)) < 0)
    return (-1);

  return ret;
}

int sock_write(int fd, const void *buff, int count) {
  char *pts = (char *)buff;
  int status = 0, n;

  if (count < 0)
    return SOCKERR_OK;

  while (status != count) {
    n = write(fd, (void *)&pts[status], count - status);

    if (n < 0) {
      if (errno == EPIPE)
        return SOCKERR_CLOSED;
      else if (errno == EINTR)
        continue;
      else
        return SOCKERR_IO;
    }
    status += n;
  }

  return status;
}

int sock_read(int fd, void *buff, int count) {
  char *pts = (char *)buff;
  int status = 0, n;

  if (count <= 0)
    return SOCKERR_OK;
  while (status != count) {
    n = read(fd, (void *)&pts[status], count - status);

    if (n < 0) {
      if (errno == EINTR)
        continue;
      else
        return SOCKERR_IO;
    }

    if (n == 0)
      return SOCKERR_CLOSED;

    status += n;
  }

  return status;
}

int sock_send_devfd(int fd, int devfd) {
  struct msghdr msg;
  struct iovec iov[1];
  char buf[100];
  struct cmsghdr *pcmsg;

  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  iov[0].iov_base = buf;
  iov[0].iov_len = 100;
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;

  msg.msg_control = control_un.control;
  msg.msg_controllen = sizeof(control_un.control);

  pcmsg = CMSG_FIRSTHDR(&msg);
  pcmsg->cmsg_len = CMSG_LEN(sizeof(int));
  pcmsg->cmsg_level = SOL_SOCKET;
  pcmsg->cmsg_type = SCM_RIGHTS;
  *((int *)CMSG_DATA(pcmsg)) = devfd;

  return sendmsg(fd, &msg, 0);
}

int sock_recv_devfd(int fd, int *devfd) {
  char buf[100];
  struct msghdr msg;
  struct iovec iov[1];
  struct cmsghdr *pcmsg;

  msg.msg_name = NULL;
  msg.msg_namelen = 0;

  iov[0].iov_base = buf;
  iov[0].iov_len = 100;
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;

  msg.msg_control = control_un.control;
  msg.msg_controllen = sizeof(control_un.control);

  recvmsg(fd, &msg, 0);

  if ((pcmsg = CMSG_FIRSTHDR(&msg)) != NULL &&
      (pcmsg->cmsg_len == CMSG_LEN(sizeof(int)))) {
    if (pcmsg->cmsg_level != SOL_SOCKET)
      return -1;

    if (pcmsg->cmsg_type != SCM_RIGHTS)
      return -1;

    *devfd = *((int *)CMSG_DATA(pcmsg));
    return 0;
  }

  return -1;
}