#ifndef __FTP_FUNC_H
#define __FTP_FUNC_H

#define CELL_NUM 512

#ifdef __cplusplus
extern "C" {
#endif

int ftp_connect(char *host, int port, char *user, char *pwd);
int ftp_quit(int c_sock);
int ftp_type(int c_sock, char mode);
int ftp_cwd(int c_sock, char *path);
int ftp_cdup(int c_sock);
int ftp_mkdir(int c_sock, char *path);
int ftp_list(int c_sock, char *path, void **data, unsigned long long *data_len);
int ftp_file_download(int c_sock, char *s, char *d,
                      unsigned long long *stor_size, int *stop);
int ftp_file_upload(int c_sock, char *s, char *d, unsigned long long *stor_size,
                    int *stop);
int ftp_file_upload_buf(int c_sock, void *content, int64_t cnt_len,
                        char *destination_path);
int ftp_mvfile(int c_sock, char *source_path, char *destination_path);
int ftp_file_delete(int c_sock, char *filename);
int ftp_folder_delete(int c_sock, char *dir_name);

int ftp_sendcmd(int sock, char *cmd);
int ftp_sendcmd_re(int sock, char *cmd, void *re_buf, ssize_t *len);
int ftp_file_upload_string(int c_sock, char *content, char *destination_path);
int ftp_ls(int c_sock, char *path, void **data, unsigned long long *data_len);

#ifdef __cplusplus
}
#endif

#endif