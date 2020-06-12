/*
 * coro_async.h
 *
 *  Created on: 2016-12-28
 *      Author: yerungui
 */

#ifndef CORO_ASYNC_H_
#define CORO_ASYNC_H_

#include "coroutine.h"

#include "marsksim/comm/thread/thread.h"

namespace coroutine {

/////////////////running in coroutine utils///////////////////////////////
class WaitThread {
private:
    struct Wrapper_ {
        Mutex mutex;
        mq::MessagePost_t message_timeout;
        mq::MessagePost_t message_running;
        boost_ksim::intrusive_ptr<Wrapper> wrapper;
        int status;
    };
    
public:
    enum {
        kTimeout = 0,
        kFinish,
        kCancel,
    };
    
public:
    WaitThread(const char* _name = NULL)
    : thread_(_name), wrapper_(new Wrapper_) {
        wrapper_->status = kTimeout;
    }
    ~WaitThread() {}
    
    template <typename F>
    typename boost_ksim::disable_if<typename boost_ksim::is_void<typename boost_ksim::result_of<F()>::type>, typename boost_ksim::result_of<F()>::type>::type
    operator ()(const F& _block_func, int64_t _timeout = -1, int* _status = NULL) {
        
        boost_ksim::shared_ptr<Wrapper_> wrapper = wrapper_;
        ScopedLock lock(wrapper_->mutex);
        wrapper->wrapper = RunningCoroutine();
        
        if (0 <= _timeout) { wrapper->message_timeout = Resume(wrapper->wrapper, _timeout); }
        
        typedef typename boost_ksim::result_of<F()>::type R;
        boost_ksim::shared_ptr<R> result(new R);
        
        mq::AsyncResult<R> async_result(_block_func, [wrapper, result](const R& _result, bool _valid) {
            
            ASSERT(_valid);
            
            ScopedLock lock(wrapper->mutex);
            *result = _result;
            if (!wrapper->wrapper) return;
            if (mq::KNullPost != wrapper->message_running)  { return; }
            if (mq::KNullPost == wrapper->message_timeout
                || mq::CancelMessage(wrapper->message_timeout)){
                wrapper->status = kFinish;
                wrapper->message_running = Resume(wrapper->wrapper);
                return;
            }}, NULL);
        
        thread_.start(async_result);
        lock.unlock();
        Yield();
        
        lock.lock();
        if (_status) *_status = wrapper->status;
        return *result;
    }
   
    template <typename F>
    typename boost_ksim::enable_if<typename boost_ksim::is_void<typename boost_ksim::result_of<F()>::type>>::type
    operator ()(const F& _block_func, int64_t _timeout = -1, int* _status = NULL) {
        
        boost_ksim::shared_ptr<Wrapper_> wrapper = wrapper_;
        ScopedLock lock(wrapper_->mutex);
        wrapper->wrapper = RunningCoroutine();
        
        if (0 <= _timeout) { wrapper->message_timeout = Resume(wrapper->wrapper, _timeout); }
        
        mq::AsyncResult<void> async_result(_block_func, [wrapper](bool _valid) {
            
            ASSERT(_valid);
            
            ScopedLock lock(wrapper->mutex);
            if (!wrapper->wrapper) return;
            if (mq::KNullPost != wrapper->message_running)  { return; }
            if (mq::KNullPost == wrapper->message_timeout
                || mq::CancelMessage(wrapper->message_timeout)){
                wrapper->status = kFinish;
                wrapper->message_running = Resume(wrapper->wrapper);
                return;
            }});
        
        thread_.start(async_result);
        lock.unlock();
        Yield();
        
        if (_status) *_status = wrapper->status;
        return;
    }
    
    void Cancel() const {
        ScopedLock lock(wrapper_->mutex);
        if (!wrapper_->wrapper) return;
        if (mq::KNullPost != wrapper_->message_running)  { return; }
        if (mq::KNullPost == wrapper_->message_timeout
            || mq::CancelMessage(wrapper_->message_timeout)){
            wrapper_->status = kCancel;
            wrapper_->message_running = Resume(wrapper_->wrapper);
            return;
        }
    }
    
    const boost_ksim::intrusive_ptr<Wrapper>& wrapper() const { return wrapper_->wrapper;}
    
private:
    WaitThread(const WaitThread&);
    void operator=(const WaitThread&);
    
private:
    Thread  thread_;
    boost_ksim::shared_ptr<Wrapper_> wrapper_;
};

template <typename F>
typename boost_ksim::result_of< F()>::type MessageInvoke(const F& _func) {
    boost_ksim::intrusive_ptr<Wrapper> wrapper = RunningCoroutine();
    
    typedef typename boost_ksim::result_of<F()>::type R;
    mq::AsyncResult<R> result(
                              [_func, wrapper](){
                                Resume(wrapper);
                                return _func();
                              });
    
    mq::AsyncInvoke(result, mq::Post2Handler(mq::RunningMessageID()));
    Yield();
    return result.Result();
}

}

#endif
