#ifndef __DBUS_HELPS
#define __DBUS_HELPS

#ifdef __cplusplus
extern "C" {
#endif

struct UserData {
    pthread_mutex_t mutex;
    DBusConnection *connection;
    char *json_str;
};

typedef int (*dbus_method_return_func_t)(DBusMessageIter *iter,
                                         const char *error, void *user_data);

typedef void (*dbus_append_func_t)(DBusMessageIter *iter,
                                   void *user_data);

int dbus_method_call(DBusConnection *connection,
                     const char *service, const char *path, const char *interface,
                     const char *method, dbus_method_return_func_t cb,
                     void * user_data, dbus_append_func_t append_fn,
                     void *append_data);

void dbus_wait(struct UserData* userdata);
void dbus_deconnection(struct UserData* userdata);
int dbus_async(struct UserData* userdata);
struct UserData* dbus_connection(void);
int populate_get(DBusMessageIter *iter, const char *error, void *user_data);
int populate_set(DBusMessageIter *iter, const char *error, void *user_data);
void append_path(DBusMessageIter *iter, void *user_data);
void append_int(DBusMessageIter *iter, void *user_data);

typedef void (*dbus_signal_func_t)(void *user_data);

void dbus_monitor_signal_registered(char *interface, char *signal, dbus_signal_func_t cb);
void dbus_monitor_signal_unregistered(char *interface, char *signal, dbus_signal_func_t cb);
void disable_loop(void);

#ifdef __cplusplus
}
#endif

#endif