#include "ueventthread.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <QDebug>
#include "global_value.h"

#define UEVENT_MSG_LEN 4096
struct luther_gliethttp {
    const char *action;
    const char *path;
    const char *subsystem;
    const char *firmware;
    int major;
    int minor;
};
static int open_luther_gliethttp_socket(void);
static void parse_event(const char *msg, struct luther_gliethttp *luther_gliethttp);

UeventThread::UeventThread(QObject *parent):QThread(parent)
{
}

UeventThread::~UeventThread()
{
    requestInterruption();
    terminate();
    quit();
    wait();
}

void UeventThread::run()
{
    qDebug()<<"ueventthread thread run"<<endl;
    int device_fd = -1;
    char msg[UEVENT_MSG_LEN+2];
    int n;

    device_fd = open_luther_gliethttp_socket();
    do {
        while((n = recv(device_fd, msg, UEVENT_MSG_LEN, 0)) > 0) {
            struct luther_gliethttp luther_gliethttp;

            if(n == UEVENT_MSG_LEN) /* overflow -- discard */
                continue;

            msg[n] = '\0';
            msg[n+1] = '\0';

            parse_event(msg, &luther_gliethttp);
        }
    } while(!isInterruptionRequested());
}

static int open_luther_gliethttp_socket(void)
{
    struct sockaddr_nl addr;
    int sz = 64*1024;
    int s;
    int val = 1;

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = 0;
    addr.nl_groups = NETLINK_KOBJECT_UEVENT;

    s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (s < 0)
        return -1;

    setsockopt(s, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz));
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (void *)&val, sizeof(int));

    if (bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        qDebug("bind error:%s\n", strerror(errno));
        close(s);
        return -1;
    }
    return s;
}

static void parse_event(const char *msg, struct luther_gliethttp *luther_gliethttp)
{
    luther_gliethttp->action = "";
    luther_gliethttp->path = "";
    luther_gliethttp->subsystem = "";
    luther_gliethttp->firmware = "";
    luther_gliethttp->major = -1;
    luther_gliethttp->minor = -1;

    /* Currently ignoring SEQNUM */
    while(*msg){
        if(!strncmp(msg, "ACTION=", 7)){
            msg += 7;
            luther_gliethttp->action = msg;
        }else if(!strncmp(msg, "DEVPATH=", 8)){
            msg += 8;
            luther_gliethttp->path = msg;
        }else if(!strncmp(msg, "SUBSYSTEM=", 10)){
            msg += 10;
            luther_gliethttp->subsystem = msg;
        }else if(!strncmp(msg, "FIRMWARE=", 9)){
            msg += 9;
            luther_gliethttp->firmware = msg;
        }else if(!strncmp(msg, "MAJOR=", 6)){
            msg += 6;
            luther_gliethttp->major = atoi(msg);
        }else if(!strncmp(msg, "MINOR=", 6)){
            msg += 6;
            luther_gliethttp->minor = atoi(msg);
        }
        /* Advance to after the next \0 */
        while(*msg++);
    }

    QRegExp regExp;
    regExp.setPatternSyntax(QRegExp::RegExp);
    regExp.setCaseSensitivity(Qt::CaseSensitive);
    regExp.setPattern("usb...*video[0-9]");

    if(regExp.indexIn(QString(luther_gliethttp->path))<0){
        return;
    }
    if(strcmp(luther_gliethttp->action,"remove")==0 ||
          strcmp(luther_gliethttp->action,"add")==0){
        printf("event{'%s','%s','%s','%s',%d,%d}\n",
               luther_gliethttp->action, luther_gliethttp->path, luther_gliethttp->subsystem,
               luther_gliethttp->firmware, luther_gliethttp->major, luther_gliethttp->minor);
        if(mainWindow!=NULL){
            emit mainWindow->slot_uevent(luther_gliethttp->action, luther_gliethttp->path);
        }
    }
}
