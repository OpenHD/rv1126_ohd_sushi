#ifndef EVENT_H
#define EVENT_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <fcntl.h>
#include "uvc_connect.pb.h"

void handle_callback(UvcConnectData mEvent);

//事件处理基类
class BaseEvent
{
public:
    // 纯虚函数
    BaseEvent(){};
    virtual ~BaseEvent() = default;
    virtual void receive() = 0;
    //int response_send(int command, void* mCmdData, int size);
};

class Version: public BaseEvent
{
public:
    Version(){};
    virtual ~Version() = default;
    void receive();
};

class Update: public BaseEvent
{
public:
    Update(){};
    virtual ~Update() = default;
    void receive();
};

class HeartBeat: public BaseEvent
{
public:
    HeartBeat(){};
    virtual ~HeartBeat() = default;
    void receive();
};

#ifdef  __cplusplus
}
#endif
#endif
