// Tencent is pleased to support the open source community by making Mars available.
// Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.

// Licensed under the MIT License (the "License"); you may not use this file except in 
// compliance with the License. You may obtain a copy of the License at
// http://opensource.org/licenses/MIT

// Unless required by applicable law or agreed to in writing, software distributed under the License is
// distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
// either express or implied. See the License for the specific language governing permissions and
// limitations under the License.

/*
 * callback.h
 *
 *  Created on: 2018-12-13
 *      Author: zhouzhijie
 */


#ifndef callback_h
#define callback_h
#include "marsksim/comm/thread/thread.h"
#include "message_queue.h"
#include "marsksim/comm/xlogger/xlogger.h"
#include "boost/bind.hpp"

namespace marsksim {

    using namespace MessageQueueksim;

template<class T>
class CallBack {
public:
    typedef boost_ksim::function<void ()> invoke_function;

    //if MessageQueueksim::KNullHandler, will callback in worker thread
    CallBack(const T& _func, MessageHandler_tksim _handler=MessageQueueksim::KNullHandler):cb_handler_(_handler), cb_func_(_func), valid_(true) {}
    
    CallBack():cb_handler_(MessageQueueksim::KNullHandler), valid_(false) {}
    
    void set(const T& _func, MessageHandler_tksim _handler=MessageQueueksim::KNullHandler) {
        ScopedLock lock(mutex_);
        cb_handler_ = _handler;
        cb_func_ = _func;
        valid_ = true;
    }
    
    operator bool() {
        ScopedLock lock(mutex_);
        return valid_;
    }

    void invalidate() {
        //should call in messagequeue of cb_handler_
        xassert2(MessageQueueksim::CurrentThreadMessageQueueksim() == MessageQueueksim::Handler2Queue(cb_handler_));
        ScopedLock lock(mutex_);
        valid_ = false;
    }
    
    template<typename... Args>
    void operator()(const Args&... rest) {
        ScopedLock lock(mutex_);
        if(!valid_) {
            return;
        }
        
        boost_ksim::function<void ()> func = boost_ksim::bind(cb_func_, rest...);
        if(MessageQueueksim::KNullHandler == cb_handler_) {
            func();
            return;
        }
        MessageQueueksim::AsyncInvoke(func, cb_handler_);
    }

private:
    MessageHandler_tksim cb_handler_;
    T cb_func_;
    Mutex mutex_;
    bool valid_;
};

}


#endif /* callback_h */
