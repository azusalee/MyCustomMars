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
 * MessageQueueksim.cpp
 *
 *  Created on: 2013-4-3
 *      Author: yerungui
 */

#include <map>
#include <list>
#include <string>
#include <algorithm>
#ifndef _WIN32
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#endif

#include "boost/bind.hpp"

#include "comm/thread/lock.h"
#include "comm/anr.h"
#include "comm/messagequeue/message_queue.h"
#include "comm/time_utils.h"
#include "comm/bootrun.h"
#ifdef __APPLE__
#include "comm/debugger/debugger_utils.h"
#endif

#ifdef ANDROID
#include "android/fatal_assert.h"
#endif

#undef min

namespace MessageQueueksim {

#define MAX_MQ_SIZE 5000

static unsigned int __MakeSeq() {
    static unsigned int s_seq = 0;

    return ++s_seq;
}

struct MessageWrapper {
    MessageWrapper(const MessageHandler_tksim& _handlerid, const Message& _message, const MessageTiming& _timing, unsigned int _seq)
        : message(_message), timing(_timing) {
        postid.reg = _handlerid;
        postid.seq = _seq;
        periodstatus = kImmediately;
        record_time = 0;

        if (kImmediately != _timing.type) {
            periodstatus = kAfter;
            record_time = ::gettickcount();
        }
    }

    ~MessageWrapper() {
        if (wait_end_cond)
            wait_end_cond->notifyAll();
    }

    MessagePost_t postid;
    Message message;

    MessageTiming timing;
    TMessageTiming periodstatus;
    uint64_t record_time;
    boost_ksim::shared_ptr<Condition> wait_end_cond;
};

struct HandlerWrapper {
    HandlerWrapper(const MessageHandler& _handler, bool _recvbroadcast, const MessageQueueksim_t& _messagequeueid, unsigned int _seq)
        : handler(_handler), recvbroadcast(_recvbroadcast) {
        reg.seq = _seq;
        reg.queue = _messagequeueid;
    }

    MessageHandler_tksim reg;
    MessageHandler handler;
    bool recvbroadcast;
};

struct RunLoopInfo {
    RunLoopInfo():runing_message(NULL) { runing_cond = boost_ksim::make_shared<Condition>();}
    
    boost_ksim::shared_ptr<Condition> runing_cond;
    MessagePost_t runing_message_id;
    Message* runing_message;
    std::list <MessageHandler_tksim> runing_handler;
};
    
class Cond : public RunloopCond {
public:
    Cond(){}
    
public:
    const boost_ksim::typeindex::type_info& type() const {
        return boost_ksim::typeindex::type_id<Cond>().type_info();
    }
    
    virtual void Wait(ScopedLock& _lock, long _millisecond) {
        cond_.wait(_lock, _millisecond);
    }
    virtual void Notify(ScopedLock& _lock) {
        cond_.notifyAll(_lock);
    }
    
private:
    Cond(const Cond&);
    void operator=(const Cond&);
    
private:
    Condition cond_;
};
    
struct MessageQueueksimContent {
    MessageQueueksimContent(): breakflag(false) {}

#if defined(ANDROID)
    MessageQueueksimContent(const MessageQueueksimContent&): breakflag(false) { /*ASSERT(false);*/ }
#endif

    MessageHandler_tksim invoke_reg;
    bool breakflag;
    boost_ksim::shared_ptr<RunloopCond> breaker;
    std::list<MessageWrapper*> lst_message;
    std::list<HandlerWrapper*> lst_handler;
    
    std::list<RunLoopInfo> lst_runloop_info;
    
private:
    void operator=(const MessageQueueksimContent&);
    
#if !defined(ANDROID)
    MessageQueueksimContent(const MessageQueueksimContent&);
#endif
};

#define sg_messagequeue_mapksim_mutex messagequeue_mapksim_mutex()
static Mutex& messagequeue_mapksim_mutex() {
    static Mutex* mutex = new Mutex;
    return *mutex;
}
#define sg_messagequeue_mapksim messagequeue_mapksim()
static std::map<MessageQueueksim_t, MessageQueueksimContent>& messagequeue_mapksim() {
    static std::map<MessageQueueksim_t, MessageQueueksimContent>* mq_map = new std::map<MessageQueueksim_t, MessageQueueksimContent>;
    return *mq_map;
}


static std::string DumpMessage(const std::list<MessageWrapper*>& _message_lst) {
    XMessage xmsg;
    xmsg(TSF"**************Dump MQ Message**************size:%_\n", _message_lst.size());
    int index = 0;
    for (auto msg : _message_lst) {
        xmsg(TSF"postid:%_, timing:%_, record_time:%_, message:%_\n", msg->postid.ToString(), msg->timing.ToString(), msg->record_time, msg->message.ToString());
        if (++index>50)
            break;
    }
    return xmsg.String();
}
std::string DumpMQ(const MessageQueueksim_t& _msq_queue_id) {
    ScopedLock lock(sg_messagequeue_mapksim_mutex);
    const MessageQueueksim_t& id = _msq_queue_id;
    
    std::map<MessageQueueksim_t, MessageQueueksimContent>::iterator pos = sg_messagequeue_mapksim.find(id);
    if (sg_messagequeue_mapksim.end() == pos) {
        //ASSERT2(false, "%" PRIu64, id);
        xinfo2(TSF"message queue not found.");
        return "";
    }
    
    MessageQueueksimContent& content = pos->second;
    return DumpMessage(content.lst_message);
}

MessageQueueksim_t CurrentThreadMessageQueueksim() {
    ScopedLock lock(sg_messagequeue_mapksim_mutex);
    MessageQueueksim_t id = (MessageQueueksim_t)ThreadUtilksim::currentthreadid();

    if (sg_messagequeue_mapksim.end() == sg_messagequeue_mapksim.find(id)) id = KInvalidQueueID;

    return id;
}

MessageQueueksim_t TID2MessageQueueksim(thread_tid _tid) {
    ScopedLock lock(sg_messagequeue_mapksim_mutex);
    MessageQueueksim_t id = (MessageQueueksim_t)_tid;

    if (sg_messagequeue_mapksim.end() == sg_messagequeue_mapksim.find(id))id = KInvalidQueueID;

    return id;
}
    
thread_tid  MessageQueueksim2TID(MessageQueueksim_t _id) {
    ScopedLock lock(sg_messagequeue_mapksim_mutex);
    MessageQueueksim_t& id = _id;
    
    if (sg_messagequeue_mapksim.end() == sg_messagequeue_mapksim.find(id)) return 0;
    
    return (thread_tid)id;
}

void WaitForRunningLockEnd(const MessagePost_t&  _message) {
    if (Handler2Queue(Post2Handler(_message)) == CurrentThreadMessageQueueksim()) return;

    ScopedLock lock(sg_messagequeue_mapksim_mutex);
    const MessageQueueksim_t& id = Handler2Queue(Post2Handler(_message));

    std::map<MessageQueueksim_t, MessageQueueksimContent>::iterator pos = sg_messagequeue_mapksim.find(id);
    if (sg_messagequeue_mapksim.end() == pos) return;
    MessageQueueksimContent& content = pos->second;
    
    if (content.lst_runloop_info.empty()) return;
    
    auto find_it = std::find_if(content.lst_runloop_info.begin(), content.lst_runloop_info.end(),
                                [&_message](const RunLoopInfo& _v){ return _message == _v.runing_message_id; });
    
    if (find_it == content.lst_runloop_info.end()) return;

    boost_ksim::shared_ptr<Condition> runing_cond = find_it->runing_cond;
    runing_cond->wait(lock);
}

void WaitForRunningLockEnd(const MessageQueueksim_t&  _messagequeueid) {
    if (_messagequeueid == CurrentThreadMessageQueueksim()) return;

    ScopedLock lock(sg_messagequeue_mapksim_mutex);
    const MessageQueueksim_t& id = _messagequeueid;

    std::map<MessageQueueksim_t, MessageQueueksimContent>::iterator pos = sg_messagequeue_mapksim.find(id);
    if (sg_messagequeue_mapksim.end() == pos) return;
    MessageQueueksimContent& content = pos->second;

    if (content.lst_runloop_info.empty()) return;
    if (KNullPost == content.lst_runloop_info.front().runing_message_id) return;

    boost_ksim::shared_ptr<Condition> runing_cond = content.lst_runloop_info.front().runing_cond;
    runing_cond->wait(lock);
}

void WaitForRunningLockEnd(const MessageHandler_tksim&  _handler) {
    if (Handler2Queue(_handler) == CurrentThreadMessageQueueksim()) return;

    ScopedLock lock(sg_messagequeue_mapksim_mutex);
    const MessageQueueksim_t& id = Handler2Queue(_handler);

    std::map<MessageQueueksim_t, MessageQueueksimContent>::iterator pos = sg_messagequeue_mapksim.find(id);
    if (sg_messagequeue_mapksim.end() == pos) { return; }
    MessageQueueksimContent& content = pos->second;
    if (content.lst_runloop_info.empty()) return;

    for(auto& i : content.lst_runloop_info) {
        for (auto& x : i.runing_handler) {
            if (_handler==x) {
                boost_ksim::shared_ptr<Condition> runing_cond = i.runing_cond;
                runing_cond->wait(lock);
                return;
            }
        }
    }
}

void BreakMessageQueueksimRunloop(const MessageQueueksim_t&  _messagequeueid) {
    ASSERT(0 != _messagequeueid);

    ScopedLock lock(sg_messagequeue_mapksim_mutex);
    const MessageQueueksim_t& id = _messagequeueid;

    std::map<MessageQueueksim_t, MessageQueueksimContent>::iterator pos = sg_messagequeue_mapksim.find(id);
    if (sg_messagequeue_mapksim.end() == pos) {
        //ASSERT2(false, "%llu", (unsigned long long)id);
        return;
    }

    pos->second.breakflag = true;
    pos->second.breaker->Notify(lock);
}

MessageHandler_tksim InstallMessageHandler(const MessageHandler& _handler, bool _recvbroadcast, const MessageQueueksim_t& _messagequeueid) {
    ASSERT(bool(_handler));

    ScopedLock lock(sg_messagequeue_mapksim_mutex);
    const MessageQueueksim_t& id = _messagequeueid;

    std::map<MessageQueueksim_t, MessageQueueksimContent>::iterator pos = sg_messagequeue_mapksim.find(id);
    if (sg_messagequeue_mapksim.end() == pos) {
        ASSERT2(false, "%llu", (unsigned long long)id);
        return KNullHandler;
    }

    HandlerWrapper* handler = new HandlerWrapper(_handler, _recvbroadcast, _messagequeueid, __MakeSeq());
    pos->second.lst_handler.push_back(handler);
    return handler->reg;
}

void UnInstallMessageHandler(const MessageHandler_tksim& _handlerid) {
    ASSERT(0 != _handlerid.queue);
    ASSERT(0 != _handlerid.seq);

    if (0 == _handlerid.queue || 0 == _handlerid.seq) return;

    ScopedLock lock(sg_messagequeue_mapksim_mutex);
    const MessageQueueksim_t& id = _handlerid.queue;

    std::map<MessageQueueksim_t, MessageQueueksimContent>::iterator pos = sg_messagequeue_mapksim.find(id);
    if (sg_messagequeue_mapksim.end() == pos) return;

    MessageQueueksimContent& content = pos->second;

    for (std::list<HandlerWrapper*>::iterator it = content.lst_handler.begin(); it != content.lst_handler.end(); ++it) {
        if (_handlerid == (*it)->reg) {
            delete(*it);
            content.lst_handler.erase(it);
            break;
        }
    }
}

MessagePost_t PostMessage(const MessageHandler_tksim& _handlerid, const Message& _message, const MessageTiming& _timing) {
    ScopedLock lock(sg_messagequeue_mapksim_mutex);
    const MessageQueueksim_t& id = _handlerid.queue;

    std::map<MessageQueueksim_t, MessageQueueksimContent>::iterator pos = sg_messagequeue_mapksim.find(id);
    if (sg_messagequeue_mapksim.end() == pos) {
        //ASSERT2(false, "%" PRIu64, id);
        return KNullPost;
    }

    MessageQueueksimContent& content = pos->second;
    if(content.lst_message.size() >= MAX_MQ_SIZE) {
        xwarn2(TSF"%_", DumpMessage(content.lst_message));
        ASSERT2(false, "Over MAX_MQ_SIZE");
        return KNullPost;
    }

    MessageWrapper* messagewrapper = new MessageWrapper(_handlerid, _message, _timing, __MakeSeq());

    content.lst_message.push_back(messagewrapper);
    content.breaker->Notify(lock);
    return messagewrapper->postid;
}

MessagePost_t SingletonksimMessage(bool _replace, const MessageHandler_tksim& _handlerid, const Message& _message, const MessageTiming& _timing) {
    ScopedLock lock(sg_messagequeue_mapksim_mutex);
    const MessageQueueksim_t& id = _handlerid.queue;

    std::map<MessageQueueksim_t, MessageQueueksimContent>::iterator pos = sg_messagequeue_mapksim.find(id);
    if (sg_messagequeue_mapksim.end() == pos) return KNullPost;

    MessageQueueksimContent& content = pos->second;

    MessagePost_t post_id;

    for (std::list<MessageWrapper*>::iterator it = content.lst_message.begin(); it != content.lst_message.end(); ++it) {
        if ((*it)->postid.reg == _handlerid && (*it)->message == _message) {
            if (_replace) {
                post_id = (*it)->postid;
                delete(*it);
                content.lst_message.erase(it);
                break;
            } else {
                return (*it)->postid;
            }
        }
    }
    
    if(content.lst_message.size() >= MAX_MQ_SIZE) {
        xwarn2(TSF"%_", DumpMessage(content.lst_message));
        ASSERT2(false, "Over MAX_MQ_SIZE");
        return KNullPost;
    }

    MessageWrapper* messagewrapper = new MessageWrapper(_handlerid, _message, _timing, 0 != post_id.seq ? post_id.seq : __MakeSeq());
    content.lst_message.push_back(messagewrapper);
    content.breaker->Notify(lock);
    return messagewrapper->postid;
}

MessagePost_t BroadcastMessage(const MessageQueueksim_t& _messagequeueid,  const Message& _message, const MessageTiming& _timing) {
    ScopedLock lock(sg_messagequeue_mapksim_mutex);
    const MessageQueueksim_t& id = _messagequeueid;

    std::map<MessageQueueksim_t, MessageQueueksimContent>::iterator pos = sg_messagequeue_mapksim.find(id);
    if (sg_messagequeue_mapksim.end() == pos) {
        ASSERT2(false, "%" PRIu64, id);
        return KNullPost;
    }

    MessageQueueksimContent& content = pos->second;
    if(content.lst_message.size() >= MAX_MQ_SIZE) {
        xwarn2(TSF"%_", DumpMessage(content.lst_message));
        ASSERT2(false, "Over MAX_MQ_SIZE");
        return KNullPost;
    }

    MessageHandler_tksim reg;
    reg.queue = _messagequeueid;
    reg.seq = 0;
    MessageWrapper* messagewrapper = new MessageWrapper(reg, _message, _timing, __MakeSeq());

    content.lst_message.push_back(messagewrapper);
    content.breaker->Notify(lock);
    return messagewrapper->postid;
}

static int64_t __ComputerWaitTime(const MessageWrapper& _wrap) {
    int64_t wait_time = 0;

    if (kImmediately == _wrap.timing.type) {
        wait_time = 0;
    } else if (kAfter == _wrap.timing.type) {
        int64_t time_cost = ::gettickspan(_wrap.record_time);
        wait_time =  _wrap.timing.after - time_cost;
    } else if (kPeriod == _wrap.timing.type) {
        int64_t time_cost = ::gettickspan(_wrap.record_time);

        if (kAfter == _wrap.periodstatus) {
            wait_time =  _wrap.timing.after - time_cost;
        } else if (kPeriod == _wrap.periodstatus) {
            wait_time =  _wrap.timing.period - time_cost;
        }
    }

    return 0 < wait_time ? wait_time : 0;
}

MessagePost_t FasterMessage(const MessageHandler_tksim& _handlerid, const Message& _message, const MessageTiming& _timing) {
    ScopedLock lock(sg_messagequeue_mapksim_mutex);
    const MessageQueueksim_t& id = _handlerid.queue;

    std::map<MessageQueueksim_t, MessageQueueksimContent>::iterator pos = sg_messagequeue_mapksim.find(id);
    if (sg_messagequeue_mapksim.end() == pos) return KNullPost;

    MessageQueueksimContent& content = pos->second;

    MessageWrapper* messagewrapper = new MessageWrapper(_handlerid, _message, _timing, __MakeSeq());

    for (std::list<MessageWrapper*>::iterator it = content.lst_message.begin(); it != content.lst_message.end(); ++it) {
        if ((*it)->postid.reg == _handlerid && (*it)->message == _message) {
            if (__ComputerWaitTime(**it) < __ComputerWaitTime(*messagewrapper)) {
                delete messagewrapper;
                return (*it)->postid;
            }

            messagewrapper->postid = (*it)->postid;
            delete(*it);
            content.lst_message.erase(it);
            break;
        }
    }

    if(content.lst_message.size() >= MAX_MQ_SIZE) {
        xwarn2(TSF"%_", DumpMessage(content.lst_message));
        ASSERT2(false, "Over MAX_MQ_SIZE");
        delete messagewrapper;
        return KNullPost;
    }
    content.lst_message.push_back(messagewrapper);
    content.breaker->Notify(lock);
    return messagewrapper->postid;
}

bool WaitMessage(const MessagePost_t& _message, long _timeoutInMs) {
    bool is_in_mq = Handler2Queue(Post2Handler(_message)) == CurrentThreadMessageQueueksim();

    ScopedLock lock(sg_messagequeue_mapksim_mutex);
    const MessageQueueksim_t& id = Handler2Queue(Post2Handler(_message));
    std::map<MessageQueueksim_t, MessageQueueksimContent>::iterator pos = sg_messagequeue_mapksim.find(id);
    if (sg_messagequeue_mapksim.end() == pos) return false;
    MessageQueueksimContent& content = pos->second;

    auto find_it = std::find_if(content.lst_message.begin(), content.lst_message.end(),
                                [&_message](const MessageWrapper * const &_v) {
                                    return _message == _v->postid;
                                });
    
    if (find_it == content.lst_message.end()) {
        auto find_it = std::find_if(content.lst_runloop_info.begin(), content.lst_runloop_info.end(),
                     [&_message](const RunLoopInfo& _v){ return _message == _v.runing_message_id; });
        
        if (find_it != content.lst_runloop_info.end()) {
            if (is_in_mq) return false;
            
            boost_ksim::shared_ptr<Condition> runing_cond = find_it->runing_cond;
            if(_timeoutInMs < 0) {
                runing_cond->wait(lock);
            } else {
                int ret = runing_cond->wait(lock, _timeoutInMs);
                return ret==0;
            }
        }
    } else {
        
        if (is_in_mq) {
            lock.unlock();
            RunLoop( [&_message](){
                        MessageQueueksimContent& content = sg_messagequeue_mapksim[Handler2Queue(Post2Handler(_message))];
                        return content.lst_message.end() == std::find_if(content.lst_message.begin(), content.lst_message.end(),
                                                                [&_message](const MessageWrapper *  const &_v) {
                                                                    return _message == _v->postid;
                                                                });
            }).Run();
            
        } else {
            if (!((*find_it)->wait_end_cond))(*find_it)->wait_end_cond = boost_ksim::make_shared<Condition>();

            boost_ksim::shared_ptr<Condition> wait_end_cond = (*find_it)->wait_end_cond;
            if(_timeoutInMs < 0) {
                wait_end_cond->wait(lock);
            } else {
                int ret = wait_end_cond->wait(lock, _timeoutInMs);
                return ret==0;
            }
        }
    }

    return true;
}

bool FoundMessage(const MessagePost_t& _message) {
    ScopedLock lock(sg_messagequeue_mapksim_mutex);
    const MessageQueueksim_t& id = Handler2Queue(Post2Handler(_message));

    std::map<MessageQueueksim_t, MessageQueueksimContent>::iterator pos = sg_messagequeue_mapksim.find(id);
    if (sg_messagequeue_mapksim.end() == pos) return false;
    MessageQueueksimContent& content = pos->second;
    if (content.lst_runloop_info.empty()) return false;

    auto find_it = std::find_if(content.lst_runloop_info.begin(), content.lst_runloop_info.end(),
                                [&_message](const RunLoopInfo& _v){ return _message == _v.runing_message_id; });
    
    if (find_it != content.lst_runloop_info.end())  { return true; }

    for (std::list<MessageWrapper*>::iterator it = content.lst_message.begin(); it != content.lst_message.end(); ++it) {
        if (_message == (*it)->postid) { return true;}
    }

    return false;
}

bool CancelMessage(const MessagePost_t& _postid) {
    ASSERT(0 != _postid.reg.queue);
    ASSERT(0 != _postid.seq);

    // 0==_postid.reg.seq for BroadcastMessage
    if (0 == _postid.reg.queue || 0 == _postid.seq) return false;

    ScopedLock lock(sg_messagequeue_mapksim_mutex);
    const MessageQueueksim_t& id = _postid.reg.queue;

    std::map<MessageQueueksim_t, MessageQueueksimContent>::iterator pos = sg_messagequeue_mapksim.find(id);
    if (sg_messagequeue_mapksim.end() == pos) {
        ASSERT2(false, "%" PRIu64, id);
        return false;
    }

    MessageQueueksimContent& content = pos->second;

    for (std::list<MessageWrapper*>::iterator it = content.lst_message.begin(); it != content.lst_message.end(); ++it) {
        if (_postid == (*it)->postid) {
            delete(*it);
            content.lst_message.erase(it);
            return true;
        }
    }

    return false;
}

void CancelMessage(const MessageHandler_tksim& _handlerid) {
    ASSERT(0 != _handlerid.queue);

    // 0==_handlerid.seq for BroadcastMessage
    if (0 == _handlerid.queue) return;

    ScopedLock lock(sg_messagequeue_mapksim_mutex);
    const MessageQueueksim_t& id = _handlerid.queue;

    std::map<MessageQueueksim_t, MessageQueueksimContent>::iterator pos = sg_messagequeue_mapksim.find(id);
    if (sg_messagequeue_mapksim.end() == pos) {
        //        ASSERT2(false, "%lu", id);
        return;
    }

    MessageQueueksimContent& content = pos->second;

    for (std::list<MessageWrapper*>::iterator it = content.lst_message.begin(); it != content.lst_message.end();) {
        if (_handlerid == (*it)->postid.reg) {
            delete(*it);
            it = content.lst_message.erase(it);
        } else {
            ++it;
        }
    }
}

void CancelMessage(const MessageHandler_tksim& _handlerid, const MessageTitle_t& _title) {
    ASSERT(0 != _handlerid.queue);

    // 0==_handlerid.seq for BroadcastMessage
    if (0 == _handlerid.queue) return;

    ScopedLock lock(sg_messagequeue_mapksim_mutex);
    const MessageQueueksim_t& id = _handlerid.queue;

    std::map<MessageQueueksim_t, MessageQueueksimContent>::iterator pos = sg_messagequeue_mapksim.find(id);
    if (sg_messagequeue_mapksim.end() == pos) {
        ASSERT2(false, "%" PRIu64, id);
        return;
    }

    MessageQueueksimContent& content = pos->second;

    for (std::list<MessageWrapper*>::iterator it = content.lst_message.begin(); it != content.lst_message.end();) {
        if (_handlerid == (*it)->postid.reg && _title == (*it)->message.title) {
            delete(*it);
            it = content.lst_message.erase(it);
        } else {
            ++it;
        }
    }
}
    
const Message& RunningMessage() {
    MessageQueueksim_t id = (MessageQueueksim_t)ThreadUtilksim::currentthreadid();
    ScopedLock lock(sg_messagequeue_mapksim_mutex);
    
    std::map<MessageQueueksim_t, MessageQueueksimContent>::iterator pos = sg_messagequeue_mapksim.find(id);
    if (sg_messagequeue_mapksim.end() == pos) {
        return KNullMessage;
    }
    
    Message* runing_message = pos->second.lst_runloop_info.back().runing_message;
    return runing_message? *runing_message: KNullMessage;
}
    
MessagePost_t RunningMessageID() {
    MessageQueueksim_t id = (MessageQueueksim_t)ThreadUtilksim::currentthreadid();
    return RunningMessageID(id);
}

MessagePost_t RunningMessageID(const MessageQueueksim_t& _id) {
    ScopedLock lock(sg_messagequeue_mapksim_mutex);

    std::map<MessageQueueksim_t, MessageQueueksimContent>::iterator pos = sg_messagequeue_mapksim.find(_id);
    if (sg_messagequeue_mapksim.end() == pos) {
        return KNullPost;
    }

    MessageQueueksimContent& content = pos->second;
    return content.lst_runloop_info.back().runing_message_id;
}

static void __AsyncInvokeHandler(const MessagePost_t& _id, Message& _message) {
    (*boost_ksim::any_cast<boost_ksim::shared_ptr<AsyncInvokeFunction> >(_message.body1))();
}

MessageHandler_tksim InstallAsyncHandler(const MessageQueueksim_t& id) {
    ASSERT(0 != id);
    return InstallMessageHandler(__AsyncInvokeHandler, false, id);
}
    

static MessageQueueksim_t __CreateMessageQueueksimInfo(boost_ksim::shared_ptr<RunloopCond>& _breaker, thread_tid _tid) {
    ScopedLock lock(sg_messagequeue_mapksim_mutex);

    MessageQueueksim_t id = (MessageQueueksim_t)_tid;

    if (sg_messagequeue_mapksim.end() == sg_messagequeue_mapksim.find(id)) {
        MessageQueueksimContent& content = sg_messagequeue_mapksim[id];
        HandlerWrapper* handler = new HandlerWrapper(&__AsyncInvokeHandler, false, id, __MakeSeq());
        content.lst_handler.push_back(handler);
        content.invoke_reg = handler->reg;
        if (_breaker)
            content.breaker = _breaker;
        else
            content.breaker = boost_ksim::make_shared<Cond>();
    }

    return id;
}
    
static void __ReleaseMessageQueueksimInfo() {

    MessageQueueksim_t id = (MessageQueueksim_t)ThreadUtilksim::currentthreadid();

    std::map<MessageQueueksim_t, MessageQueueksimContent>::iterator pos = sg_messagequeue_mapksim.find(id);
    if (sg_messagequeue_mapksim.end() != pos) {
        MessageQueueksimContent& content = pos->second;

        for (std::list<MessageWrapper*>::iterator it = content.lst_message.begin(); it != content.lst_message.end(); ++it) {
            delete(*it);
        }

        for (std::list<HandlerWrapper*>::iterator it = content.lst_handler.begin(); it != content.lst_handler.end(); ++it) {
            delete(*it);
        }

        sg_messagequeue_mapksim.erase(id);
    }
}

    
const static int kMQCallANRId = 110;
const static long kWaitANRTimeout = 15 * 1000;
static void __ANRAssert(bool _iOS_style, const marsksim::comm::check_content& _content, MessageHandler_tksim _mq_id) {
    if(MessageQueueksim2TID(_mq_id.queue) == 0) {
        xwarn2(TSF"messagequeue already destroy, handler:(%_,%_)", _mq_id.queue, _mq_id.seq);
        return;
    }
    
    __ASSERT2(_content.file.c_str(), _content.line, _content.func.c_str(), "anr dead lock", "timeout:%d, tid:%" PRIu64 ", runing time:%" PRIu64 ", real time:%" PRIu64 ", used_cpu_time:%" PRIu64 ", iOS_style:%d",
              _content.timeout, _content.tid, clock_app_monotonic() - _content.start_time, gettickcount() - _content.start_tickcount, _content.used_cpu_time, _iOS_style);
#ifdef ANDROID
    __FATAL_ASSERT2(_content.file.c_str(), _content.line, _content.func.c_str(), "anr dead lock", "timeout:%d, tid:%" PRIu64 ", runing time:%" PRIu64 ", real time:%" PRIu64 ", used_cpu_time:%" PRIu64 ", iOS_style:%s",
                    _content.timeout, _content.tid, clock_app_monotonic() - _content.start_time, gettickcount() - _content.start_tickcount, _content.used_cpu_time, _iOS_style?"true":"false");
#endif
}
    

static void __ANRCheckCallback(bool _iOS_style, const marsksim::comm::check_content& _content) {
    if (kMQCallANRId != _content.call_id) {
        return;
    }
    
    MessageHandler_tksim mq_id = *((MessageHandler_tksim*)_content.extra_info);
    xinfo2(TSF"anr check content:%_, handler:(%_,%_)", _content.call_id, mq_id.queue, mq_id.seq);
    
    boost_ksim::shared_ptr<Thread> thread(new Thread(boost_ksim::bind(__ANRAssert, _iOS_style, _content, mq_id)));
    thread->start_after(kWaitANRTimeout);
    
    MessageQueueksim::AsyncInvoke([=]() {
        if (thread->isruning()) {
            xinfo2(TSF"misjudge anr, timeout:%_, tid:%_, runing time:%_, real time:%_, used_cpu_time:%_, handler:(%_,%_)", _content.timeout,
                   _content.tid, clock_app_monotonic() - _content.start_time, gettickcount() - _content.start_tickcount, _content.used_cpu_time, mq_id.queue, mq_id.seq);
            thread->cancel_after();
        }
    }, MessageQueueksim::DefAsyncInvokeHandler(mq_id.queue), "__ANRCheckCallback");
}
#ifndef ANR_CHECK_DISABLE

static void __RgisterANRCheckCallback() {
    GetSignalksimCheckHit().connect(5, boost_ksim::bind(&__ANRCheckCallback, _1, _2));
}
static void __UnregisterANRCheckCallback() {
    GetSignalksimCheckHit().disconnect(5);
}
    
BOOT_RUN_STARTUP(__RgisterANRCheckCallback);
BOOT_RUN_EXIT(__UnregisterANRCheckCallback);
#endif
void RunLoop::Run() {
    MessageQueueksim_t id = CurrentThreadMessageQueueksim();
    ASSERT(0 != id);
    {
        ScopedLock lock(sg_messagequeue_mapksim_mutex);
        sg_messagequeue_mapksim[id].lst_runloop_info.push_back(RunLoopInfo());
    }
    
    xinfo_function(TSF"messagequeue id:%_", id);

    while (true) {
        ScopedLock lock(sg_messagequeue_mapksim_mutex);
        MessageQueueksimContent& content = sg_messagequeue_mapksim[id];
        content.lst_runloop_info.back().runing_message_id = KNullPost;
        content.lst_runloop_info.back().runing_message = NULL;
        content.lst_runloop_info.back().runing_handler.clear();
        content.lst_runloop_info.back().runing_cond->notifyAll(lock);
        
        if (duty_func_) duty_func_();

        if ((content.breakflag || (breaker_func_ && breaker_func_()))) {
            content.lst_runloop_info.pop_back();
            if (content.lst_runloop_info.empty())
                __ReleaseMessageQueueksimInfo();
            break;
        }

        int64_t wait_time = 10 * 60 * 1000;
        MessageWrapper* messagewrapper = NULL;
        bool delmessage = true;

        for (std::list<MessageWrapper*>::iterator it = content.lst_message.begin(); it != content.lst_message.end(); ++it) {
            if (kImmediately == (*it)->timing.type) {
                messagewrapper = *it;
                content.lst_message.erase(it);
                break;
            } else if (kAfter == (*it)->timing.type) {
                int64_t time_cost = ::gettickspan((*it)->record_time);

                if ((*it)->timing.after <= time_cost) {
                    messagewrapper = *it;
                    content.lst_message.erase(it);
                    break;
                } else {
                    wait_time = std::min(wait_time, (*it)->timing.after - time_cost);
                }
            } else if (kPeriod == (*it)->timing.type) {
                if (kAfter == (*it)->periodstatus) {
                    int64_t time_cost = ::gettickspan((*it)->record_time);

                    if ((*it)->timing.after <= time_cost) {
                        messagewrapper = *it;
                        (*it)->record_time = ::gettickcount();
                        (*it)->periodstatus = kPeriod;
                        delmessage = false;
                        break;
                    } else {
                        wait_time = std::min(wait_time, (*it)->timing.after - time_cost);
                    }
                } else if (kPeriod == (*it)->periodstatus) {
                    int64_t time_cost = ::gettickspan((*it)->record_time);

                    if ((*it)->timing.period <= time_cost) {
                        messagewrapper = *it;
                        (*it)->record_time = ::gettickcount();
                        delmessage = false;
                        break;
                    } else {
                        wait_time = std::min(wait_time, (*it)->timing.period - time_cost);
                    }
                } else {
                    ASSERT(false);
                }
            } else {
                ASSERT(false);
            }
        }

        if (NULL == messagewrapper) {
            content.breaker->Wait(lock, (long)wait_time);
            continue;
        }

        std::list<HandlerWrapper> fit_handler;

        for (std::list<HandlerWrapper*>::iterator it = content.lst_handler.begin(); it != content.lst_handler.end(); ++it) {
            if (messagewrapper->postid.reg == (*it)->reg || ((*it)->recvbroadcast && messagewrapper->postid.reg.isbroadcast())) {
                fit_handler.push_back(**it);
                content.lst_runloop_info.back().runing_handler.push_back((*it)->reg);
            }
        }

        content.lst_runloop_info.back().runing_message_id = messagewrapper->postid;
        content.lst_runloop_info.back().runing_message = &messagewrapper->message;
        int64_t anr_timeout = messagewrapper->message.anr_timeout;
        lock.unlock();

        messagewrapper->message.execute_time = ::gettickcount();
        for (std::list<HandlerWrapper>::iterator it = fit_handler.begin(); it != fit_handler.end(); ++it) {
            SCOPE_ANR_AUTO((int)anr_timeout, kMQCallANRId, &(*it).reg);
            uint64_t timestart = ::clock_app_monotonic();
            (*it).handler(messagewrapper->postid, messagewrapper->message);
            uint64_t timeend = ::clock_app_monotonic();
#if defined(DEBUG) && defined(__APPLE__)

            if (!isDebuggerPerforming())
#endif
                ASSERT2(0 >= anr_timeout || anr_timeout >= (int64_t)(timeend - timestart), "anr_timeout:%" PRId64 " < cost:%" PRIu64", timestart:%" PRIu64", timeend:%" PRIu64, anr_timeout, timeend - timestart, timestart, timeend);
        }

        if (delmessage) {
            delete messagewrapper;
        }
    }
}

boost_ksim::shared_ptr<RunloopCond> RunloopCond::CurrentCond() {
    ScopedLock lock(sg_messagequeue_mapksim_mutex);
    MessageQueueksim_t id = (MessageQueueksim_t)ThreadUtilksim::currentthreadid();

    std::map<MessageQueueksim_t, MessageQueueksimContent>::iterator pos = sg_messagequeue_mapksim.find(id);
    if (sg_messagequeue_mapksim.end() != pos) {
        MessageQueueksimContent& content = pos->second;
        return content.breaker;
    } else {
        return boost_ksim::shared_ptr<RunloopCond>();
    }
}

MessageQueueksimCreater::MessageQueueksimCreater(bool _iscreate, const char* _msg_queue_name)
    : MessageQueueksimCreater(boost_ksim::shared_ptr<RunloopCond>(), _iscreate, _msg_queue_name)
{}
    
MessageQueueksimCreater::MessageQueueksimCreater(boost_ksim::shared_ptr<RunloopCond> _breaker, bool _iscreate, const char* _msg_queue_name)
    : thread_(boost_ksim::bind(&MessageQueueksimCreater::__ThreadRunloop, this), _msg_queue_name)
	, messagequeue_id_(KInvalidQueueID), breaker_(_breaker) {
	if (_iscreate)
		CreateMessageQueueksim();
}

MessageQueueksimCreater::~MessageQueueksimCreater() {
    CancelAndWait();
}

void MessageQueueksimCreater::__ThreadRunloop() {
    ScopedLock lock(messagequeue_mutex_);
    lock.unlock();
    
    RunLoop().Run();
    
}

MessageQueueksim_t MessageQueueksimCreater::GetMessageQueueksim() {
    return messagequeue_id_;
}

MessageQueueksim_t MessageQueueksimCreater::CreateMessageQueueksim() {
    ScopedLock lock(messagequeue_mutex_);

    if (thread_.isruning()) return messagequeue_id_;

    if (0 != thread_.start()) { return KInvalidQueueID;}
    messagequeue_id_ = __CreateMessageQueueksimInfo(breaker_, thread_.tid());
    xinfo2(TSF"create messageqeue id:%_", messagequeue_id_);
    
    return messagequeue_id_;
}

void MessageQueueksimCreater::CancelAndWait() {
    ScopedLock lock(messagequeue_mutex_);

    if (KInvalidQueueID == messagequeue_id_) return;
    
    BreakMessageQueueksimRunloop(messagequeue_id_);
    messagequeue_id_ = KInvalidQueueID;
    lock.unlock();
    if(ThreadUtilksim::currentthreadid() != thread_.tid()) {
        thread_.join();
    }
}

MessageQueueksim_t MessageQueueksimCreater::CreateNewMessageQueueksim(boost_ksim::shared_ptr<RunloopCond> _breaker, thread_tid _tid) {
    return(__CreateMessageQueueksimInfo(_breaker, _tid));
}

MessageQueueksim_t MessageQueueksimCreater::CreateNewMessageQueueksim(boost_ksim::shared_ptr<RunloopCond> _breaker, const char* _messagequeue_name) {
    
    SpinLock* sp = new SpinLock;
    Thread thread(boost_ksim::bind(&__ThreadNewRunloop, sp), _messagequeue_name, true);
//    thread.outside_join();
    ScopedSpinLock lock(*sp);

    if (0 != thread.start()) {
        lock.unlock();
        delete sp;
        return KInvalidQueueID;
    }

    MessageQueueksim_t id = __CreateMessageQueueksimInfo(_breaker, thread.tid());
    return id;
}
    
MessageQueueksim_t MessageQueueksimCreater::CreateNewMessageQueueksim(const char* _messagequeue_name) {
    return CreateNewMessageQueueksim(boost_ksim::shared_ptr<RunloopCond>(), _messagequeue_name);
}

void MessageQueueksimCreater::ReleaseNewMessageQueueksim(MessageQueueksim_t _messagequeue_id) {

	if (KInvalidQueueID == _messagequeue_id) return;

	BreakMessageQueueksimRunloop(_messagequeue_id);
	WaitForRunningLockEnd(_messagequeue_id);
	ThreadUtilksim::join((thread_tid)_messagequeue_id);
}

void MessageQueueksimCreater::__ThreadNewRunloop(SpinLock* _sp) {
    ScopedSpinLock lock(*_sp);
    lock.unlock();
    delete _sp;

    RunLoop().Run();
}

MessageQueueksim_t GetDefMessageQueueksim() {
    static MessageQueueksimCreater* s_defmessagequeue = new MessageQueueksimCreater;
    return s_defmessagequeue->CreateMessageQueueksim();
}

MessageQueueksim_t GetDefTaskQueue() {
    static MessageQueueksimCreater* s_deftaskqueue = new MessageQueueksimCreater;
    return s_deftaskqueue->CreateMessageQueueksim();
}

MessageHandler_tksim DefAsyncInvokeHandler(const MessageQueueksim_t& _messagequeue) {
    ScopedLock lock(sg_messagequeue_mapksim_mutex);
    const MessageQueueksim_t& id = _messagequeue;

    std::map<MessageQueueksim_t, MessageQueueksimContent>::iterator pos = sg_messagequeue_mapksim.find(id);
    if (sg_messagequeue_mapksim.end() == pos) return KNullHandler;

    MessageQueueksimContent& content = pos->second;
    return content.invoke_reg;
}

ScopeRegisterksim::ScopeRegisterksim(const MessageHandler_tksim& _reg)
: m_regksim(new MessageHandler_tksim(_reg)) {}

ScopeRegisterksim::~ScopeRegisterksim() {
    Cancel();
    delete m_regksim;
}

const MessageHandler_tksim& ScopeRegisterksim::Get() const
{return *m_regksim;}

void ScopeRegisterksim::Cancel() const {
    UnInstallMessageHandler(*m_regksim);
    CancelMessage(*m_regksim);
}
void ScopeRegisterksim::CancelAndWait() const {
    Cancel();
    WaitForRunningLockEnd(*m_regksim);
}
}  // namespace MessageQueueksim
