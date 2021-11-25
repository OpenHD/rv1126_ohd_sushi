#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <inttypes.h>

#include <glib.h>

#include <pthread.h>
#include <gdbus.h>
#include "utils/log.h"
#include "dbus_helps.h"

#define TIMEOUT         120000
static DBusConnection *dbusconn = 0;
int need_loop = 1;
GMainLoop *main_loop = NULL;

struct dbus_callback {
    dbus_method_return_func_t cb;
    void *user_data;
};

struct DbusSignal {
    char *interface;
    char *signal;
    dbus_signal_func_t cb;
    struct DbusSignal *next;
};

struct DbusUserdata {
    struct UserData* userdata;
    struct DbusUserdata* next;
};

static struct DbusSignal *dbus_signal_list = NULL;
static struct DbusUserdata *dbus_userdata_list = NULL;
static pthread_mutex_t mutex;
static pthread_mutex_t dbus_userdata_mutex = PTHREAD_MUTEX_INITIALIZER;

static void add_dbus_userdata(struct UserData *userdata)
{
    struct DbusUserdata *tmp;

    pthread_mutex_lock(&dbus_userdata_mutex);
    tmp = dbus_userdata_list;
    if (tmp == NULL) {
        dbus_userdata_list = (struct DbusUserdata*)malloc(sizeof(struct DbusUserdata));
        dbus_userdata_list->userdata = userdata;
        dbus_userdata_list->next = NULL;
    } else {
        while (tmp->next) {
            if (tmp->next->userdata == userdata) {
                pthread_mutex_unlock(&dbus_userdata_mutex);
                return;
            }
            tmp = tmp->next;
        }
        tmp->next = (struct DbusUserdata*)malloc(sizeof(struct DbusUserdata));
        tmp->next->userdata = userdata;
        tmp->next->next = NULL;
    }
    pthread_mutex_unlock(&dbus_userdata_mutex);
}

static int check_dbus_userdata(struct UserData *userdata)
{
    struct DbusUserdata *tmp;

    pthread_mutex_lock(&dbus_userdata_mutex);
    tmp = dbus_userdata_list;
    while (tmp) {
        if (tmp->userdata == userdata) {
            pthread_mutex_unlock(&dbus_userdata_mutex);
            return 1;
        }
        tmp = tmp->next;
    }
    pthread_mutex_unlock(&dbus_userdata_mutex);

    return 0;
}

static void del_dbus_userdata(struct UserData *userdata)
{
    struct DbusUserdata *tmp;

    pthread_mutex_lock(&dbus_userdata_mutex);
    tmp = dbus_userdata_list;
    if (tmp) {
        if (tmp->userdata == userdata) {
            dbus_userdata_list = dbus_userdata_list->next;
            free(tmp);
        } else {
            while (tmp->next) {
                if (tmp->next->userdata == userdata) {
                    struct DbusUserdata *next = tmp->next;
                    tmp->next = next->next;
                    free(next);
                    break;
                }
                tmp = tmp->next;
            }
        }
    }
    pthread_mutex_unlock(&dbus_userdata_mutex);
}

static void dbus_method_reply(DBusPendingCall *call, void *user_data)
{
    struct dbus_callback *callback = user_data;
    int res = 0;
    DBusMessage *reply;
    DBusMessageIter iter;

    reply = dbus_pending_call_steal_reply(call);
    dbus_pending_call_unref(call);
    if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
        DBusError err;

        dbus_error_init(&err);
        dbus_set_error_from_message(&err, reply);

        callback->cb(NULL, err.message, callback->user_data);

        dbus_error_free(&err);
        goto end;
    }

    dbus_message_iter_init(reply, &iter);
    res = callback->cb(&iter, NULL, callback->user_data);

end:

    g_free(callback);
    dbus_message_unref(reply);
}

static int send_method_call(DBusConnection *connection,
                            DBusMessage *message, dbus_method_return_func_t cb,
                            void *user_data)
{
    int res = -ENXIO;
    DBusPendingCall *call;
    struct dbus_callback *callback;

    if (!dbus_connection_send_with_reply(connection, message, &call, TIMEOUT))
        goto end;

    if (!call)
        goto end;

    if (cb) {
        callback = g_new0(struct dbus_callback, 1);
        callback->cb = cb;
        callback->user_data = user_data;
        dbus_pending_call_set_notify(call, dbus_method_reply,
                                     callback, NULL);
        res = -EINPROGRESS;
    }

end:
    dbus_message_unref(message);
    return res;
}

int dbus_method_call(DBusConnection *connection,
                     const char *service, const char *path, const char *interface,
                     const char *method, dbus_method_return_func_t cb,
                     void *user_data, dbus_append_func_t append_func,
                     void *append_data)
{
    DBusMessage *message;
    DBusMessageIter iter;

    message = dbus_message_new_method_call(service, path, interface,
                                           method);

    if (!message)
        return -ENOMEM;

    if (append_func) {
        dbus_message_iter_init_append(message, &iter);
        append_func(&iter, append_data);
    }

    return send_method_call(connection, message, cb, user_data);
}

void append_path(DBusMessageIter *iter, void *user_data)
{
    const char *path = user_data;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &path);
}

void append_int(DBusMessageIter *iter, void *user_data)
{
    int *value = user_data;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, value);
}

int populate_set(DBusMessageIter *iter, const char *error,
                 void *user_data)
{
    char *json;
    struct UserData* userdata = user_data;

    if (userdata)
        pthread_mutex_unlock(&userdata->mutex);

    return 0;
}

int populate_get(DBusMessageIter *iter, const char *error,
                 void *user_data)
{
    char *json;
    struct UserData* userdata = user_data;

    if (error) {
        LOG_ERROR("%s err\n", __func__);
        if (userdata && check_dbus_userdata(userdata))
            pthread_mutex_unlock(&userdata->mutex);

        return 0;
    }

    dbus_message_iter_get_basic(iter, &json);

    if (userdata && check_dbus_userdata(userdata)) {
        userdata->json_str = g_strdup(json);
        pthread_mutex_unlock(&userdata->mutex);
    }

    return 0;
}

static void *loop_thread(void *arg)
{
    main_loop = g_main_loop_new(NULL, FALSE);

    //g_timeout_add(100, time_cb, NULL);
    g_main_loop_run(main_loop);

    if (main_loop)
        g_main_loop_unref(main_loop);

    main_loop = NULL;

    return 0;
}

int dbus_async(struct UserData* userdata)
{
    struct timespec tout;

    if (need_loop && main_loop == NULL) {
        pthread_t tid;
        pthread_create(&tid, NULL, (void*)loop_thread, NULL);
    }

    clock_gettime(CLOCK_REALTIME, &tout);
    tout.tv_sec += 1;

    if (pthread_mutex_timedlock(&userdata->mutex, &tout) != 0) {
        LOG_ERROR("%s again get\n", __func__);
        return -1;
    }

    pthread_mutex_unlock(&userdata->mutex);

    return 0;
}

void dbus_deconnection(struct UserData* userdata)
{
    del_dbus_userdata(userdata);
    dbus_connection_unref(userdata->connection);
    if (userdata)
        g_free(userdata);
}

struct UserData* dbus_connection(void)
{
    DBusError dbus_err;
    struct UserData* userdata;

    userdata = malloc(sizeof(struct UserData));
    memset(userdata, 0, sizeof(struct UserData));
    pthread_mutex_init(&userdata->mutex, NULL);

    dbus_error_init(&dbus_err);
    userdata->connection = g_dbus_setup_bus(DBUS_BUS_SYSTEM, NULL, &dbus_err);
    pthread_mutex_lock(&userdata->mutex);
    add_dbus_userdata(userdata);

    return userdata;
}

static DBusHandlerResult dbus_monitor_changed(
    DBusConnection *connection,
    DBusMessage *message, void *user_data)
{
    bool *enabled = user_data;
    DBusMessageIter iter;
    DBusHandlerResult handled;

    handled = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    pthread_mutex_lock(&mutex);
    struct DbusSignal *tmp = dbus_signal_list;

    while(tmp) {
       if (dbus_message_is_signal(message, tmp->interface,
                               tmp->signal)) {
            char *json_str;
            handled = DBUS_HANDLER_RESULT_HANDLED;

            dbus_message_iter_init(message, &iter);
            dbus_message_iter_get_basic(&iter, &json_str);
            if (tmp->cb)
                tmp->cb(json_str);
        }

        tmp = tmp->next;
    }
    pthread_mutex_unlock(&mutex);

    return handled;
}

static struct DbusSignal *check_dbus_signel(char *interface, char *signal, dbus_signal_func_t cb)
{
    struct DbusSignal *tmp = dbus_signal_list;

    while(tmp) {
        if ((cb == tmp->cb) && g_str_equal(interface, tmp->interface) && (g_str_equal(signal, tmp->signal)))
            break;
        tmp = tmp->next;
    }

    return tmp;
}

static void add_dbus_sigal(struct DbusSignal *dbus_signal)
{
    struct DbusSignal *tmp = dbus_signal_list;
    if (tmp == NULL) {
        dbus_signal_list = dbus_signal;
    } else {
        while(tmp->next) tmp = tmp->next;
        tmp->next = dbus_signal;
    }
}

void dbus_monitor_signal_registered(char *interface, char *signal, dbus_signal_func_t cb)
{
    DBusError err;
    char *tmp;

    if (need_loop && main_loop == NULL) {
        pthread_t tid;
        pthread_create(&tid, NULL, (void*)loop_thread, NULL);
    }

    dbus_error_init(&err);
    if (dbusconn == NULL) {
        pthread_mutex_init(&mutex, NULL);
        dbusconn = g_dbus_setup_bus(DBUS_BUS_SYSTEM, NULL, &err);
        dbus_connection_add_filter(dbusconn, dbus_monitor_changed, NULL, NULL);
    }
    if (check_dbus_signel(interface, signal, cb) == NULL) {
        struct DbusSignal *dbus_signal = (struct DbusSignal *)malloc(sizeof(struct DbusSignal));
        dbus_signal->interface = g_strdup(interface);
        dbus_signal->signal = g_strdup(signal);
        dbus_signal->cb = cb;
        dbus_signal->next = NULL;
        add_dbus_sigal(dbus_signal);
        char *tmp = g_strdup_printf("type='signal',interface='%s'", interface);
        dbus_bus_add_match(dbusconn, tmp, &err);
        g_free(tmp);
    }
}

void dbus_monitor_signal_unregistered(char *interface, char *signal, dbus_signal_func_t cb)
{
    struct DbusSignal *tmp = dbus_signal_list;

    pthread_mutex_lock(&mutex);
    if ((cb == tmp->cb) && g_str_equal(interface, tmp->interface) && (g_str_equal(signal, tmp->signal))) {
        g_free(tmp->interface);
        g_free(tmp->signal);
        dbus_signal_list = tmp->next;
        free(tmp);
    } else {
        struct DbusSignal *tmp_pre = tmp;
        tmp = tmp->next;
        while(tmp) {
            if ((cb == tmp->cb) && g_str_equal(interface, tmp->interface) && (g_str_equal(signal, tmp->signal))) {
                g_free(tmp->interface);
                g_free(tmp->signal);
                tmp_pre = tmp->next;
                free(tmp);
                break;
            }
            tmp = tmp->next;
        }
    }
    pthread_mutex_unlock(&mutex);
}

void disable_loop(void)
{
    need_loop = 0;
}
