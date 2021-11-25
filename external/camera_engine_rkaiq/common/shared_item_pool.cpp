/*
 *  Copyright (c) 2019 Rockchip Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "shared_item_pool.h"

namespace RkCam {

template<typename T>
SharedItemPool<T>::SharedItemPool(const char* name, uint32_t max_count)
    : _name(name ? name : "default")
    , _allocated_num(0)
    , _max_count(max_count)
{
    LOG1("ENTER SharedItemPool<%s>:%s", _name, __FUNCTION__);
    uint32_t i = 0;

    XCAM_ASSERT (_max_count);

    for (i = _allocated_num; i < max_count; ++i) {
        SmartPtr<T> new_data = allocate_data ();
        if (!new_data.ptr ())
            break;
        _item_list.push (new_data);
    }

    if (i != _max_count) {
        LOGW("SharedItemPool<%s> expect to reserve %d data but only reserved %d",
             _name, max_count, i);
    }
    _max_count = i;
    _allocated_num = _max_count;

    LOG1("EXIT SharedItemPool<%s>:%s", _name, __FUNCTION__);
}

template<typename T>
SharedItemPool<T>::~SharedItemPool()
{
    LOG1("ENTER SharedItemPool<%s>:%s", _name, __FUNCTION__);
    LOG1("EXIT SharedItemPool<%s>:%s", _name, __FUNCTION__);
}

template<typename T>
SmartPtr<T>
SharedItemPool<T>::allocate_data ()
{
    return new T();
}

template<typename T>
void
SharedItemPool<T>::release (SmartPtr<T> &data)
{
    LOG1("ENTER SharedItemPool<%s>:%s", _name, __FUNCTION__);
    _item_list.push (data);
    LOG1("EXIT SharedItemPool<%s>:%s", _name, __FUNCTION__);
}

template<typename T>
SmartPtr<SharedItemProxy<T>>
                          SharedItemPool<T>::get_item()
{
    LOG1("ENTER SharedItemPool<%s>:%s", _name, __FUNCTION__);
    SmartPtr<SharedItemProxy<T>> ret_buf;
    SmartPtr<T> data;

    data = _item_list.pop ();
    if (!data.ptr ()) {
        LOGD("SharedItemPool<%s> failed to get buffer", _name);
        return NULL;
    }
    ret_buf = new SharedItemProxy<T>(data);
    ret_buf->set_buf_pool (SmartPtr<SharedItemPool<T>>(this));
    LOG1("EXIT SharedItemPool<%s>:%s", _name, __FUNCTION__);

    return ret_buf;
}

};
