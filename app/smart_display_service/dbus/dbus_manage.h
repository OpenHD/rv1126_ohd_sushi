#ifndef __DEBUS_MANAGE_H__
#define __DEBUS_MANAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <dbus/dbus.h>

#define UVC_SERVER   "rockchip.uvcserver"
#define UVC_PATH      "/"



/*
 * listen, wait a call or a signal
 */
#define DBUS_SENDER_BUS_NAME        "rockchip.mediaserver.control"
#define DBUS_RECEIVER_BUS_NAME      "rockchip.uvcserver"
#define DBUS_RECEIVER_PATH          "/rockchip/mediaserver/control/uvc"
#define DBUS_RECEIVER_INTERFACE     "rockchip.mediaserver.control.uvc"

#define DBUS_RECEIVER_SIGNAL        "signal"
#define DBUS_RECEIVER_METHOD        "method"
 
#define DBUS_RECEIVER_SIGNAL_RULE   "type='signal',interface='%s'"
#define DBUS_RECEIVER_REPLY_STR     "i am %d, get a message"
 
#define MODE_SIGNAL                 1
#define MODE_METHOD                 2
 
#define DBUS_CLIENT_PID_FILE        "/tmp/dbus-client.pid"

/////////////////////////////
#ifdef USE_MEDIASERVER
#define MEDIASERVER            "rockchip.mediaserver.control"
#define MEDIASERVER_PATH       "/rockchip/mediaserver/control/feature"
#define MEDIASERVER_INTERFACE  "rockchip.mediaserver.control.feature"
#endif

#define USE_AISERVER "ON"
#ifdef USE_AISERVER
#define MEDIASERVER            "rockchip.aiserver.control"
#define MEDIASERVER_PATH       "/rockchip/aiserver/control/graph"
#define MEDIASERVER_INTERFACE  "rockchip.aiserver.control.graph"
#endif



DBusConnection* dbus_init(void);
void dbus_send(DBusConnection* connect, int mode, char *type, void *value);
void mediaserver_set_nnstatus(char *config);
void aiserver_start_matting(char *mode, int value);
void aiserver_start_nn(char *mode, int value);
void aiserver_start_eptz(int value);
void aiserver_set_nnstatus(char* mode, int status);
void aiserver_set_npuctl_analyse(int modelType, const char* uuid);
void aiserver_set_nn_parameter(char* mode, float value);
extern DBusConnection* g_connection;

#ifdef __cplusplus
}
#endif

#endif

