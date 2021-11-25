#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <fcntl.h>
#include <netdb.h>
#include <sys/stat.h>

#include "ftp_func.h"

int socket_connect(char *host, int port) {
  struct sockaddr_in address;
  int sk;
  int opvalue;
  socklen_t slen;

  opvalue = 8;
  slen = sizeof(opvalue);
  memset(&address, 0, sizeof(address));

  if ((sk = socket(AF_INET, SOCK_STREAM, 0)) < 0 ||
      setsockopt(sk, IPPROTO_IP, IP_TOS, &opvalue, slen) < 0)
    return -1;

  //set timeout
  struct timeval timeo = {15, 0};
  setsockopt(sk, SOL_SOCKET, SO_SNDTIMEO, &timeo, sizeof(timeo));
  setsockopt(sk, SOL_SOCKET, SO_RCVTIMEO, &timeo, sizeof(timeo));

  address.sin_family = AF_INET;
  address.sin_port = htons((unsigned short)port);

  struct hostent *server = gethostbyname(host);
  if (!server)
    return -1;

  memcpy(&address.sin_addr.s_addr, server->h_addr, server->h_length);

  if (connect(sk, (struct sockaddr *)&address, sizeof(address)) == -1)
    return -1;

  return sk;
}

int connect_server(char *host, int port) {
  int ctrl_sock;
  char buf[CELL_NUM];
  int result;
  ssize_t len;

  ctrl_sock = socket_connect(host, port);
  if (ctrl_sock == -1) {
    return -1;
  }

  len = recv(ctrl_sock, buf, CELL_NUM, 0);
  buf[len] = 0;
  sscanf(buf, "%d", &result);
  if (result != 220) {
    close(ctrl_sock);
    return -1;
  }

  return ctrl_sock;
}

int ftp_sendcmd_re(int sock, char *cmd, void *re_buf, ssize_t *len) {
  char buf[CELL_NUM];
  ssize_t r_len;

  if (send(sock, cmd, strlen(cmd), 0) == -1)
    return -1;

  r_len = recv(sock, buf, CELL_NUM, 0);
  if (r_len < 1)
    return -1;

  buf[r_len] = 0;

  if (len != NULL)
    *len = r_len;

  if (re_buf != NULL)
    sprintf(re_buf, "%s", buf);

  return 0;
}

int ftp_sendcmd(int sock, char *cmd) {
  char buf[CELL_NUM];
  int result;
  ssize_t len;

  result = ftp_sendcmd_re(sock, cmd, buf, &len);
  if (result == 0)
    sscanf(buf, "%d", &result);

  return result;
}

int login_server(int sock, char *user, char *pwd) {
  char buf[128];
  int result;

  sprintf(buf, "USER %s\r\n", user);
  result = ftp_sendcmd(sock, buf);
  if (result == 230)
    return 0;
  else if (result == 331) {
    sprintf(buf, "PASS %s\r\n", pwd);
    if (ftp_sendcmd(sock, buf) != 230)
      return -1;

    return 0;
  } else
    return -1;
}
int create_datasock(int ctrl_sock) {
  int lsn_sock;
  int port;
  int len;
  struct sockaddr_in sin;
  char cmd[128];

  lsn_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (lsn_sock == -1)
    return -1;
  memset((char *)&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  if (bind(lsn_sock, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
    close(lsn_sock);
    return -1;
  }

  if (listen(lsn_sock, 2) == -1) {
    close(lsn_sock);
    return -1;
  }

  len = sizeof(struct sockaddr);
  if (getsockname(lsn_sock, (struct sockaddr *)&sin, (socklen_t *)&len) == -1) {
    close(lsn_sock);
    return -1;
  }
  port = sin.sin_port;

  if (getsockname(ctrl_sock, (struct sockaddr *)&sin, (socklen_t *)&len) ==
      -1) {
    close(lsn_sock);
    return -1;
  }

  sprintf(cmd, "PORT %d,%d,%d,%d,%d,%d\r\n", sin.sin_addr.s_addr & 0x000000FF,
          (sin.sin_addr.s_addr & 0x0000FF00) >> 8,
          (sin.sin_addr.s_addr & 0x00FF0000) >> 16,
          (sin.sin_addr.s_addr & 0xFF000000) >> 24, port >> 8, port & 0xff);

  if (ftp_sendcmd(ctrl_sock, cmd) != 200) {
    close(lsn_sock);
    return -1;
  }
  return lsn_sock;
}

int ftp_pasv_connect(int c_sock) {
  int r_sock;
  int send_re;
  ssize_t len;
  int addr[6];
  char buf[CELL_NUM];
  char re_buf[CELL_NUM];

  bzero(buf, sizeof(buf));
  sprintf(buf, "PASV\r\n");
  send_re = ftp_sendcmd_re(c_sock, buf, re_buf, &len);
  if (send_re == 0)
    sscanf(re_buf, "%*[^(](%d,%d,%d,%d,%d,%d)", &addr[0], &addr[1], &addr[2],
           &addr[3], &addr[4], &addr[5]);

  bzero(buf, sizeof(buf));
  sprintf(buf, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
  r_sock = socket_connect(buf, addr[4] * 256 + addr[5]);

  return r_sock;
}

int ftp_type(int c_sock, char mode) {
  char buf[128];
  sprintf(buf, "TYPE %c\r\n", mode);
  if (ftp_sendcmd(c_sock, buf) != 200)
    return -1;
  else
    return 0;
}

int ftp_cwd(int c_sock, char *path) {
  char buf[128];
  int re;
  sprintf(buf, "CWD %s\r\n", path);
  re = ftp_sendcmd(c_sock, buf);
  // LOG_DEBUG("ftp_cwd signal is %d\n", re);
  if (re != 250)
    return -1;
  else
    return 0;
}

int ftp_cdup(int c_sock) {
  int re;
  re = ftp_sendcmd(c_sock, "CDUP\r\n");
  if (re != 250)
    return re;
  else
    return 0;
}

int ftp_mkdir(int c_sock, char *path) {
  char buf[CELL_NUM];
  int re;
  sprintf(buf, "MKD %s\r\n", path);
  re = ftp_sendcmd(c_sock, buf);
  if (re != 257)
    return re;
  else
    return 0;
}

int ftp_list(int c_sock, char *path, void **data,
             unsigned long long *data_len) {
  int d_sock;
  char buf[CELL_NUM];
  int send_re;
  int result;
  ssize_t len, buf_len, total_len;

  d_sock = ftp_pasv_connect(c_sock);
  if (d_sock == -1)
    return -1;

  bzero(buf, sizeof(buf));
  sprintf(buf, "LIST %s\r\n", path);
  send_re = ftp_sendcmd(c_sock, buf);
  if (send_re >= 300 || send_re == 0)
    return send_re;

  len = total_len = 0;
  buf_len = CELL_NUM;
  void *re_buf = malloc(buf_len);
  while ((len = recv(d_sock, buf, CELL_NUM, 0)) > 0) {
    if (total_len + len > buf_len) {
      buf_len *= 2;
      void *re_buf_n = malloc(buf_len);
      memcpy(re_buf_n, re_buf, total_len);
      free(re_buf);
      re_buf = re_buf_n;
    }
    memcpy(re_buf + total_len, buf, len);
    total_len += len;
  }
  if (total_len + 1 > buf_len) {
    buf_len += 1;
    void *re_buf_n = malloc(buf_len);
    memcpy(re_buf_n, re_buf, total_len);
    free(re_buf);
    re_buf = re_buf_n;
  }
  bzero(buf, sizeof(buf));
  // sprintf(buf, "\0");
  memcpy(re_buf + total_len, buf, 1);
  total_len += 1;
  close(d_sock);

  bzero(buf, sizeof(buf));
  len = recv(c_sock, buf, CELL_NUM, 0);
  buf[len] = 0;
  sscanf(buf, "%d", &result);
  if (result != 226) {
    free(re_buf);
    return result;
  }

  *data = re_buf;
  *data_len = total_len;

  return 0;
}

int ftp_ls(int c_sock, char *path, void **data, unsigned long long *data_len) {
  int d_sock;
  char buf[CELL_NUM];
  int send_re;
  int result;
  ssize_t len, buf_len, total_len;

  d_sock = ftp_pasv_connect(c_sock);
  if (d_sock == -1)
    return -1;

  bzero(buf, sizeof(buf));
  sprintf(buf, "ls %s\r\n", path);
  send_re = ftp_sendcmd(c_sock, buf);
  if (send_re >= 300 || send_re == 0)
    return send_re;

  len = total_len = 0;
  buf_len = CELL_NUM;
  void *re_buf = malloc(buf_len);
  while ((len = recv(d_sock, buf, CELL_NUM, 0)) > 0) {
    if (total_len + len > buf_len) {
      buf_len *= 2;
      void *re_buf_n = malloc(buf_len);
      memcpy(re_buf_n, re_buf, total_len);
      free(re_buf);
      re_buf = re_buf_n;
    }
    memcpy(re_buf + total_len, buf, len);
    total_len += len;
  }
  if (total_len + 1 > buf_len) {
    buf_len += 1;
    void *re_buf_n = malloc(buf_len);
    memcpy(re_buf_n, re_buf, total_len);
    free(re_buf);
    re_buf = re_buf_n;
  }
  bzero(buf, sizeof(buf));
  // sprintf(buf, "\0");
  memcpy(re_buf + total_len, buf, 1);
  total_len += 1;
  close(d_sock);

  bzero(buf, sizeof(buf));
  len = recv(c_sock, buf, CELL_NUM, 0);
  buf[len] = 0;
  sscanf(buf, "%d", &result);
  if (result != 226) {
    free(re_buf);
    return result;
  }

  *data = re_buf;
  *data_len = total_len;

  return 0;
}

int ftp_file_download(int c_sock, char *s, char *d,
                      unsigned long long *stor_size, int *stop) {
  int d_sock;
  ssize_t len;
  ssize_t write_len;
  char buf[CELL_NUM];
  int handle;
  int result;

  handle = open(d, O_WRONLY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
  if (handle == -1)
    return -1;

  ftp_type(c_sock, 'I');

  d_sock = ftp_pasv_connect(c_sock);
  if (d_sock == -1) {
    close(handle);
    return -1;
  }

  bzero(buf, sizeof(buf));
  sprintf(buf, "RETR %s\r\n", s);
  result = ftp_sendcmd(c_sock, buf);
  if (result >= 300 || result == 0) {
    close(handle);
    return result;
  }

  bzero(buf, sizeof(buf));
  while ((len = recv(d_sock, buf, CELL_NUM, 0)) > 0) {
    write_len = write(handle, buf, len);
    if (write_len != len || (stop != NULL && *stop)) {
      close(d_sock);
      close(handle);
      return -1;
    }

    if (stor_size != NULL)
      *stor_size += write_len;
  }
  close(d_sock);
  close(handle);

  bzero(buf, sizeof(buf));
  len = recv(c_sock, buf, CELL_NUM, 0);
  buf[len] = 0;
  sscanf(buf, "%d", &result);
  if (result >= 300)
    return result;
  return 0;
}
int ftp_file_upload(int c_sock, char *source_path, char *destination_path,
                    unsigned long long *stor_size, int *stop) {
  int d_sock;
  ssize_t len;
  ssize_t send_len;
  char buf[CELL_NUM];
  int handle;
  int send_re;
  int result;

  handle = open(source_path, O_RDONLY);
  if (handle == -1)
    return -1;

  ftp_type(c_sock, 'I');

  d_sock = ftp_pasv_connect(c_sock);
  if (d_sock == -1) {
    close(handle);
    return -1;
  }

  bzero(buf, sizeof(buf));
  sprintf(buf, "STOR %s\r\n", destination_path);
  send_re = ftp_sendcmd(c_sock, buf);
  if (send_re >= 300 || send_re == 0) {
    close(handle);
    return send_re;
  }

  bzero(buf, sizeof(buf));
  while ((len = read(handle, buf, CELL_NUM)) > 0) {
    send_len = send(d_sock, buf, len, 0);
    if (send_len != len || (stop != NULL && *stop)) {
      close(d_sock);
      close(handle);
      return -1;
    }

    if (stor_size != NULL)
      *stor_size += send_len;
  }
  close(d_sock);
  close(handle);

  bzero(buf, sizeof(buf));
  len = recv(c_sock, buf, CELL_NUM, 0);
  buf[len] = 0;
  sscanf(buf, "%d", &result);
  if (result >= 300)
    return result;
  return 0;
}

int ftp_file_upload_buf(int c_sock, void *content, int64_t cnt_len,
                        char *destination_path) {
  int d_sock;
  ssize_t len;
  ssize_t send_len;
  char buf[CELL_NUM];
  int send_re;
  int result;

  ftp_type(c_sock, 'I');

  d_sock = ftp_pasv_connect(c_sock);
  if (d_sock == -1) {
    return -1;
  }

  bzero(buf, sizeof(buf));
  sprintf(buf, "STOR %s\r\n", destination_path);
  send_re = ftp_sendcmd(c_sock, buf);
  if (send_re >= 300 || send_re == 0) {
    printf("%s: send cmd fail, code is %d", __FUNCTION__, send_re);
    return send_re;
  }

  int64_t start_mark = 0;
  bzero(buf, sizeof(buf));
  if (NULL != content) {
    while (start_mark < cnt_len) {
      len = (cnt_len - start_mark) > CELL_NUM - 1 ? CELL_NUM - 1
                                                  : (cnt_len - start_mark);
      memcpy(buf, content + start_mark, len);
      send_len = send(d_sock, buf, len, 0);
      if (send_len != len) {
        close(d_sock);
        return -1;
      }
      start_mark += len;
      bzero(buf, sizeof(buf));
    }
  }
  close(d_sock);

  bzero(buf, sizeof(buf));
  len = recv(c_sock, buf, CELL_NUM, 0);
  buf[len] = 0;
  sscanf(buf, "%d", &result);
  if (result >= 300) {
    printf("%s: result fail is %d", __FUNCTION__, result);
    return result;
  }
  return 0;
}

int ftp_file_upload_string(int c_sock, char *content, char *destination_path) {
  int d_sock;
  ssize_t len;
  ssize_t send_len;
  char buf[CELL_NUM];
  int send_re;
  int result;

  ftp_type(c_sock, 'I');

  d_sock = ftp_pasv_connect(c_sock);
  if (d_sock == -1) {
    return -1;
  }

  bzero(buf, sizeof(buf));
  sprintf(buf, "STOR %s\r\n", destination_path);
  send_re = ftp_sendcmd(c_sock, buf);
  if (send_re >= 300 || send_re == 0) {
    printf("%s: send cmd fail, code is %d", __FUNCTION__, send_re);
    return send_re;
  }

  bzero(buf, sizeof(buf));
  if (NULL != content) {
    len = strlen(content) > CELL_NUM - 1 ? CELL_NUM - 1 : strlen(content);
    strncpy(buf, content, len);
    send_len = send(d_sock, buf, len, 0);
    if (send_len != len) {
      close(d_sock);
      return -1;
    }
  }
  close(d_sock);

  bzero(buf, sizeof(buf));
  len = recv(c_sock, buf, CELL_NUM, 0);
  buf[len] = 0;
  sscanf(buf, "%d", &result);
  if (result >= 300) {
    printf("%s: result fail is %d", __FUNCTION__, result);
    return result;
  }
  return 0;
}
int ftp_mvfile(int c_sock, char *source_path, char *destination_path) {
  char buf[CELL_NUM];
  int re;

  sprintf(buf, "RNFR %s\r\n", source_path);
  re = ftp_sendcmd(c_sock, buf);
  if (re != 350)
    return re;
  sprintf(buf, "RNTO %s\r\n", destination_path);
  re = ftp_sendcmd(c_sock, buf);
  if (re != 250)
    return re;
  return 0;
}

int ftp_file_delete(int c_sock, char *filename) {
  char buf[CELL_NUM];
  int re;

  sprintf(buf, "DELE %s\r\n", filename);
  re = ftp_sendcmd(c_sock, buf);
  if (re != 250)
    return re;
  return 0;
}

int ftp_deletefolder(int c_sock, char *dir_name) {
  char buf[CELL_NUM];
  int re;

  sprintf(buf, "RMD %s\r\n", dir_name);
  re = ftp_sendcmd(c_sock, buf);
  if (re != 250)
    return re;
  return 0;
}

// return -1 when fail
int ftp_connect(char *host, int port, char *user, char *pwd) {
  int c_sock;
  c_sock = connect_server(host, port);
  if (c_sock == -1)
    return -1;
  if (login_server(c_sock, user, pwd) == -1) {
    close(c_sock);
    return -1;
  }
  return c_sock;
}

int ftp_quit(int c_sock) {
  int res = 0;
  res = ftp_sendcmd(c_sock, "QUIT\r\n");
  close(c_sock);
  return res;
}