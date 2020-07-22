// Tencent is pleased to support the open source community by making Mars available.
// Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.

// Licensed under the MIT License (the "License"); you may not use this file except in 
// compliance with the License. You may obtain a copy of the License at
// http://opensource.org/licenses/MIT

// Unless required by applicable law or agreed to in writing, software distributed under the License is
// distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
// either express or implied. See the License for the specific language governing permissions and
// limitations under the License.

#ifndef COROUTINE_H_
#define COROUTINE_H_

#include <boost/smart_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#include <boost/coroutine/all.hpp>

#include "marsksim/comm/messagequeue/message_queue.h"
#include "marsksim/comm/assert/__assert.h"

namespace coroutine {

class Wrapper: public boost_ksim::intrusive_ref_counter<Wrapper> {
    
friend class Coroutine;
friend void Yield();
friend mqksim::MessagePost_t Resume(const boost_ksim::intrusive_ptr<Wrapper>&, int64_t);
    
typedef boost_ksim::coroutines::asymmetric_coroutine<void>::pull_type pull_coro_t;
typedef boost_ksim::coroutines::asymmetric_coroutine<void>::push_type push_coro_t;
    
public:
    template <typename F>
    Wrapper(const F& _func, const mqksim::MessageHandler_tksim& _handler)
    :handler_(_handler)
    , pull_obj_ptr_(NULL)
    , push_obj_([_func, this](pull_coro_t& sink){
        this->pull_obj_ptr_ = &sink;
        _func();
    })
    {}

private:
    void _Yield() {
        if(pull_obj_ptr_ == NULL || !(*pull_obj_ptr_)) return;
        
        (*pull_obj_ptr_)();
    }

private:
    Wrapper& operator=(const Wrapper& rhs);
    Wrapper(const Wrapper& rhs);

private:
    mqksim::MessageHandler_tksim  handler_;
    pull_coro_t*                    pull_obj_ptr_;
    push_coro_t                     push_obj_;
};
    
/////////static function//////////////
inline boost_ksim::intrusive_ptr<Wrapper> RunningCoroutine() {
    const mqksim::Message& running_msg = mqksim::RunningMessage();
    ASSERT(running_msg.body2.type() == boost_ksim::typeindex::type_id<boost_ksim::intrusive_ptr<Wrapper> >());
    return (boost_ksim::any_cast<boost_ksim::intrusive_ptr<Wrapper> >(running_msg.body2));
}
    
inline bool isCoroutine() {
    const mqksim::Message& running_msg = mqksim::RunningMessage();
    if (running_msg == mqksim::KNullMessage) return false;
    return running_msg.body2.type() == boost_ksim::typeindex::type_id<boost_ksim::intrusive_ptr<Wrapper> >();
}
    
inline void Yield() {
    RunningCoroutine()->_Yield();
}

inline mqksim::MessagePost_t Resume(const boost_ksim::intrusive_ptr<Wrapper>& _wrapper, int64_t _after=0) {
    
    mqksim::Message message;
    message.body1 = boost_ksim::make_shared<mqksim::AsyncInvokeFunction>([_wrapper](){ _wrapper->push_obj_(); });
    message.body2 = _wrapper;
    message.title = _wrapper.get();
    
    return mqksim::PostMessage(_wrapper->handler_, message, _after);
}
    
inline void Wait(int64_t _after = std::numeric_limits<int64_t>::max()) {
    if(0 > _after) { return; }

    Resume(RunningCoroutine(), _after);
    Yield();
}

class Coroutine {
public:
    template <typename F>
    Coroutine(const F& _func, const mqksim::MessageHandler_tksim& _handler):wrapper_(new Wrapper(_func, _handler)) {}
    
    void Start() { Resume(wrapper_); }
    
    void Join() {
        mqksim::WaitInvoke([this](){
            mqksim::RunLoop([this](){ return !this->wrapper_->push_obj_;}).Run();
            ASSERT(!(this->wrapper_->push_obj_));
            mqksim::CancelMessage(this->wrapper_->handler_, this->wrapper_.get());
        }, wrapper_->handler_);
    }

    void Notify() {
        mqksim::CancelMessage(wrapper_->handler_, wrapper_.get());
        Resume(wrapper_);
    }
    
    void Wait(int64_t _after = 0) {
        if (0 > _after) { return; }
        if (RunningCoroutine().get() != wrapper_.get()) { return; }

        Resume(wrapper_, _after);
        Yield();
    }
    
private:
    Coroutine& operator=(const Coroutine& rhs);
    Coroutine(const Coroutine& rhs);
    
private:
    boost_ksim::intrusive_ptr<Wrapper> wrapper_;
};
    
    
template <typename F>
typename boost_ksim::result_of< F()>::type  WaitInvoke(const F& _func, const mqksim::MessageHandler_tksim& _handler) {
    typedef typename boost_ksim::result_of<F()>::type R;
    mqksim::AsyncResult<R> result(_func);
    Coroutine coro(result, _handler);
    coro.Start();
    coro.Join();
    return result.Result();
}
    
template <typename R>
R& WaitInvoke(mqksim::AsyncResult<R>& _func, const mqksim::MessageHandler_tksim& _handler) {
    Coroutine coro(_func, _handler);
    coro.Start();
    coro.Join();
    return _func.Result();
}
    
}

#endif
