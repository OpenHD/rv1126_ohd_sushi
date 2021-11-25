#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dbus/dbus.h>
 
/*
 * listen, wait a call or a signal
 */
#define DBUS_SENDER_BUS_NAME        "com.ty3219.sender_app"
 
#define DBUS_RECEIVER_BUS_NAME      "com.ty3219.receiver_app"
#define DBUS_RECEIVER_PATH           "/rockchip/mediaserver/object" //"/com/ty3219/object"
#define DBUS_RECEIVER_INTERFACE     "rockchip.mediaserver.interface"

//#define DBUS_RECEIVER_INTERFACE     "com.ty3219.interface"
#define DBUS_RECEIVER_SIGNAL        "signal"
#define DBUS_RECEIVER_METHOD        "method"
 
#define DBUS_RECEIVER_SIGNAL_RULE   "type='signal',interface='%s'"
#define DBUS_RECEIVER_REPLY_STR     "i am %d, get a message"
 
#define MODE_SIGNAL                 1
#define MODE_METHOD                 2
 
#define DBUS_CLIENT_PID_FILE        "/tmp/dbus-client.pid"
 
/**
 *
 * @param msg
 * @param conn
 */
void reply_method_call(DBusMessage *msg, DBusConnection *conn)
{
    DBusMessage *reply;
    DBusMessageIter reply_arg;
    DBusMessageIter msg_arg;
    dbus_uint32_t serial = 0;
 
    pid_t pid;
    char reply_str[128];
    void *__value;
    char *__value_str;
    int __value_int;
 
    int ret;
 
    pid = getpid();
 
    //创建返回消息reply
    reply = dbus_message_new_method_return(msg);
    if (!reply)
    {
        printf("Out of Memory!\n");
        return;
    }
 
    //在返回消息中填入参数。
    snprintf(reply_str, sizeof(reply_str), DBUS_RECEIVER_REPLY_STR, pid);
    __value_str = reply_str;
    __value = &__value_str;
 
    dbus_message_iter_init_append(reply, &reply_arg);
    if (!dbus_message_iter_append_basic(&reply_arg, DBUS_TYPE_STRING, __value))
    {
        printf("Out of Memory!\n");
        goto out;
    }
 
    //从msg中读取参数，根据传入参数增加返回参数
    if (!dbus_message_iter_init(msg, &msg_arg))
    {
        printf("Message has NO Argument\n");
        goto out;
    }
 
    do
    {
        int ret = dbus_message_iter_get_arg_type(&msg_arg);
        if (DBUS_TYPE_STRING == ret)
        {
            dbus_message_iter_get_basic(&msg_arg, &__value_str);
            printf("I am %d, get Method Argument STRING: %s\n", pid,
                    __value_str);
 
            __value = &__value_str;
            if (!dbus_message_iter_append_basic(&reply_arg,
                    DBUS_TYPE_STRING, __value))
            {
                printf("Out of Memory!\n");
                goto out;
            }
        }
        else if (DBUS_TYPE_INT32 == ret)
        {
            dbus_message_iter_get_basic(&msg_arg, &__value_int);
            printf("I am %d, get Method Argument INT32: %d\n", pid,
                    __value_int);
 
            __value_int++;
            __value = &__value_int;
            if (!dbus_message_iter_append_basic(&reply_arg,
                    DBUS_TYPE_INT32, __value))
            {
                printf("Out of Memory!\n");
                goto out;
            }
        }
        else
        {
            printf("Argument Type ERROR\n");
        }
 
    } while (dbus_message_iter_next(&msg_arg));
 
    //发送返回消息
    if (!dbus_connection_send(conn, reply, &serial))
    {
        printf("Out of Memory\n");
        goto out;
    }
 
    dbus_connection_flush(conn);
out:
    dbus_message_unref(reply);
}
 
/* 监听D-Bus消息，我们在上次的例子中进行修改 */
void dbus_receive(void)
{
    DBusMessage *msg;
    DBusMessageIter arg;
    DBusConnection *connection;
    DBusError err;
 
    pid_t pid;
    char name[64];
    char rule[128];
 
    const char *path;
    void *__value;
    char *__value_str;
    int __value_int;
 
    int ret;
 
    pid = getpid();
 
    dbus_error_init(&err);
    //创建于session D-Bus的连接
    connection = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (!connection)
    {
        if (dbus_error_is_set(&err))
            printf("Connection Error %s\n", err.message);
 
        goto out;
    }
 
    //设置一个BUS name
    if (0 == access(DBUS_CLIENT_PID_FILE, F_OK))
        snprintf(name, sizeof(name), "%s%d", DBUS_RECEIVER_BUS_NAME, pid);
    else
        snprintf(name, sizeof(name), "%s", DBUS_RECEIVER_BUS_NAME);
 
    printf("i am a receiver, PID = %d, name = %s\n", pid, name);
 
    ret = dbus_bus_request_name(connection, name,
                            DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
    if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
    {
        if (dbus_error_is_set(&err))
            printf("Name Error %s\n", err.message);
 
        goto out;
    }
 
    //要求监听某个signal：来自接口test.signal.Type的信号
    snprintf(rule, sizeof(rule), DBUS_RECEIVER_SIGNAL_RULE, DBUS_RECEIVER_INTERFACE);
    dbus_bus_add_match(connection, rule, &err);
    dbus_connection_flush(connection);
    if (dbus_error_is_set(&err))
    {
        printf("Match Error %s\n", err.message);
        goto out;
    }
 
    while (1)
    {
        dbus_connection_read_write(connection, 0);
 
        msg = dbus_connection_pop_message(connection);
        if (msg == NULL)
        {
            sleep(1);
            continue;
        }
 
        path = dbus_message_get_path(msg);
        if (strcmp(path, DBUS_RECEIVER_PATH))
        {
            printf("Wrong PATH: %s\n", path);
            goto next;
        }
 
        printf("Get a Message\n");
        if (dbus_message_is_signal(msg, DBUS_RECEIVER_INTERFACE, DBUS_RECEIVER_SIGNAL))
        {
            printf("Someone Send me a Signal\n");
            if (!dbus_message_iter_init(msg, &arg))
            {
                printf("Message Has no Argument\n");
                goto next;
            }
 
            ret = dbus_message_iter_get_arg_type(&arg);
            if (DBUS_TYPE_STRING == ret)
            {
                dbus_message_iter_get_basic(&arg, &__value_str);
                printf("I am %d, Got Signal with STRING: %s\n",
                        pid, __value_str);
            }
            else if (DBUS_TYPE_INT32 == ret)
            {
                dbus_message_iter_get_basic(&arg, &__value_int);
                printf("I am %d, Got Signal with INT32: %d\n",
                        pid, __value_int);
            }
            else
            {
                printf("Argument Type ERROR\n");
                goto next;
            }
        }
        else if (dbus_message_is_method_call(msg, DBUS_RECEIVER_INTERFACE, DBUS_RECEIVER_METHOD))
        {
            printf("Someone Call My Method\n");
            reply_method_call(msg, connection);
        }
        else
        {
            printf("NOT a Signal OR a Method\n");
        }
next:
        dbus_message_unref(msg);
    }
 
out:
    dbus_error_free(&err);
}
 
/*
 * call a method
 */
static void dbus_send(int mode, char *type, void *value)
{
    DBusConnection *connection;
    DBusError err;
    DBusMessage *msg;
    DBusMessageIter arg;
    DBusPendingCall *pending;
    dbus_uint32_t serial;
 
    int __type;
    void *__value;
    char *__value_str;
    int __value_int;
    pid_t pid;
    int ret;
 
    pid = getpid();
 
    //Step 1: connecting session bus
    /* initialise the erroes */
    dbus_error_init(&err);
 
    /* Connect to Bus*/
    connection = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (!connection)
    {
        if (dbus_error_is_set(&err))
            printf("Connection Err : %s\n", err.message);
 
       goto out1;
    }
 
    //step 2: 设置BUS name，也即连接的名字。
    ret = dbus_bus_request_name(connection, DBUS_SENDER_BUS_NAME,
                            DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
    if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
    {
        if (dbus_error_is_set(&err))
            printf("Name Err : %s\n", err.message);
 
        goto out1;
    }
 
 
    if (!strcasecmp(type, "STRING"))
    {
        __type = DBUS_TYPE_STRING;
        __value_str = value;
        __value = &__value_str;
    }
    else if (!strcasecmp(type, "INT32"))
    {
        __type = DBUS_TYPE_INT32;
        __value_int = atoi(value);
        __value = &__value_int;
    }
    else
    {
        printf("Wrong Argument Type\n");
        goto out1;
    }
 
 
    if (mode == MODE_METHOD)
    {
        printf("Call app[bus_name]=%s, object[path]=%s, interface=%s, method=%s\n",
                DBUS_RECEIVER_BUS_NAME, DBUS_RECEIVER_PATH,
                DBUS_RECEIVER_INTERFACE, DBUS_RECEIVER_METHOD);
 
        //针对目的地地址，创建一个method call消息。
        //Constructs a new message to invoke a method on a remote object.
        msg = dbus_message_new_method_call(
                DBUS_RECEIVER_BUS_NAME, DBUS_RECEIVER_PATH,
                DBUS_RECEIVER_INTERFACE, DBUS_RECEIVER_METHOD);
        if (msg == NULL)
        {
            printf("Message NULL");
            goto out1;
        }
 
        dbus_message_iter_init_append(msg, &arg);
        if (!dbus_message_iter_append_basic(&arg, __type, __value))
        {
            printf("Out of Memory!");
            goto out2;
        }
 
        //发送消息并获得reply的handle 。Queues a message to send, as with dbus_connection_send() , but also returns a DBusPendingCall used to receive a reply to the message.
        if (!dbus_connection_send_with_reply(connection, msg, &pending, -1))
        {
            printf("Out of Memory!");
            goto out2;
        }
 
        if (pending == NULL)
        {
            printf("Pending Call NULL: connection is disconnected ");
            goto out2;
        }
 
        dbus_connection_flush(connection);
        dbus_message_unref(msg);
 
        //waiting a reply，在发送的时候，已经获取了method reply的handle，类型为DBusPendingCall。
        // block until we receive a reply， Block until the pending call is completed.
        dbus_pending_call_block(pending);
        // get the reply message，Gets the reply, or returns NULL if none has been received yet.
        msg = dbus_pending_call_steal_reply(pending);
        if (msg == NULL)
        {
            printf("Reply Null\n");
            goto out1;
        }
 
        // free the pending message handle
        dbus_pending_call_unref(pending);
 
        // read the Arguments
        if (!dbus_message_iter_init(msg, &arg))
        {
            printf("Message has no Argument!\n");
            goto out2;
        }
 
        do
        {
            int ret = dbus_message_iter_get_arg_type(&arg);
            if (DBUS_TYPE_STRING == ret)
            {
                dbus_message_iter_get_basic(&arg, &__value_str);
                printf("I am %d, get Method return STRING: %s\n", pid,
                        __value_str);
            }
            else if (DBUS_TYPE_INT32 == ret)
            {
                dbus_message_iter_get_basic(&arg, &__value_int);
                printf("I am %d, get Method return INT32: %d\n", pid,
                        __value_int);
            }
            else
            {
                printf("Argument Type ERROR\n");
            }
 
        } while (dbus_message_iter_next(&arg));
 
        printf("NO More Argument\n");
    }
    else if (mode == MODE_SIGNAL)
    {
        printf("Signal to object[path]=%s, interface=%s, signal=%s\n",
                DBUS_RECEIVER_PATH, DBUS_RECEIVER_INTERFACE, DBUS_RECEIVER_SIGNAL);
 
        //步骤3:发送一个信号
        //根据图，我们给出这个信号的路径（即可以指向对象），接口，以及信号名，创建一个Message
        msg = dbus_message_new_signal(DBUS_RECEIVER_PATH,
                            DBUS_RECEIVER_INTERFACE, DBUS_RECEIVER_SIGNAL);
        if (!msg)
        {
            printf("Message NULL\n");
            goto out1;
        }
 
        dbus_message_iter_init_append(msg, &arg);
        if (!dbus_message_iter_append_basic(&arg, __type, __value))
        {
            printf("Out of Memory!");
            goto out2;
        }
 
        //将信号从连接中发送
        if (!dbus_connection_send(connection, msg, &serial))
        {
            printf("Out of Memory!\n");
            goto out2;
        }
 
        dbus_connection_flush(connection);
        printf("Signal Send\n");
    }
 
out2:
    dbus_message_unref(msg);
out1:
    dbus_error_free(&err);
}
 
static void usage(void)
{
#define USAGE "usage: ./dbus-client [send | receive] <param>\n" \
    "\treceive -- listen, wait a signal or a method call\n" \
    "\t\tif you want to test signal broadcast, run two receiver like this:\n" \
    "\t\trm -f /tmp/dbus-client.pid\n" \
    "\t\t./dbus-client receive &\n" \
    "\t\techo > /tmp/dbus-client.pid\n" \
    "\t\t./dbus-client receive &\n" \
    "\tsend [mode] [type] [value] -- send a signal or call a method\n" \
    "\t\tmode -- SIGNAL | METHOD\n" \
    "\t\ttype -- STRING | INT32\n" \
    "\t\tvalue -- string or number\n" \
    "\t\texample:\n" \
    "\t\t./dbus-client send SIGNAL STRING hello\n" \
    "\t\t./dbus-client send METHOD INT32 99\n" \
    "\n"
    printf(USAGE);
}
 
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        usage();
        return -1;
    }
 
    if (!strcmp(argv[1], "receive"))
    {
        dbus_receive();
    }
    else if (!strcmp(argv[1], "send"))
    {
        if (argc < 5)
        {
            usage();
        }
        else
        {
            if (!strcasecmp(argv[2], "SIGNAL"))
                dbus_send(MODE_SIGNAL, argv[3], argv[4]);
            else if (!strcasecmp(argv[2], "METHOD"))
                dbus_send(MODE_METHOD, argv[3], argv[4]);
            else
                usage();
        }
    }
    else
    {
        usage();
    }
 
    return 0;
}