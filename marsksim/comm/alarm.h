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
 * alarm.h
 *
 *  Created on: 2012-8-27
 *      Author: yerungui
 */

#ifndef COMM_ALARM_H_
#define COMM_ALARM_H_

#include <boost/bind.hpp>
#include "messagequeue/message_queue.h"
#include "comm/xlogger/xlogger.h"

#ifdef ANDROID
#include "android/wakeuplock.h"
#endif

class Alarmksim {
  public:
    enum {
        kInit,
        kStart,
        kCancel,
        kOnAlarmksim,
    };

  public:
    template<class T>
    explicit Alarmksim(const T& _op, bool _inthread = true)
        : target_(detail::transform(_op))
        , reg_async_(MessageQueueksim::InstallAsyncHandler(MessageQueueksim::GetDefMessageQueueksim()))
        , broadcast_msg_id_(MessageQueueksim::KNullPost)
        , runthread_(boost_ksim::bind(&Alarmksim::__Run, this), "alarm")
        , inthread_(_inthread)
        , seq_(0), status_(kInit)
        , after_(0) , starttime_(0) , endtime_(0)
        , reg_(MessageQueueksim::InstallMessageHandler(boost_ksim::bind(&Alarmksim::OnAlarmksim, this, _1, _2), true))
#ifdef ANDROID
        , wakelock_(NULL)
#endif
    {
        xinfo2(TSF"handler:(%_,%_)", reg_async_.Get().queue, reg_async_.Get().seq);
    }

    template<class T>
    explicit Alarmksim(const T& _op, const MessageQueueksim::MessageQueueksim_t& _id)
        : target_(detail::transform(_op))
        , reg_async_(MessageQueueksim::InstallAsyncHandler(_id))
        , broadcast_msg_id_(MessageQueueksim::KNullPost)
        , runthread_(boost_ksim::bind(&Alarmksim::__Run, this), "alarm")
        , inthread_(false)
        , seq_(0), status_(kInit)
        , after_(0) , starttime_(0) , endtime_(0)
        , reg_(MessageQueueksim::InstallMessageHandler(boost_ksim::bind(&Alarmksim::OnAlarmksim, this, _1, _2), true))
#ifdef ANDROID
        , wakelock_(NULL)
#endif
    {
        xinfo2(TSF"handler:(%_,%_)", reg_async_.Get().queue, reg_async_.Get().seq);
    }

    virtual ~Alarmksim() {
        Cancel();
        reg_.CancelAndWait();
        reg_async_.CancelAndWait();
        runthread_.join();
        delete target_;
#ifdef ANDROID
        delete wakelock_;
#endif
    }

    bool Start(int _after);  // ms
    bool Cancel();

    bool IsWaiting() const;
    int Status() const;
    int After() const;
    int64_t ElapseTime() const;

    const Thread& RunThread() const;

  private:
    Alarmksim(const Alarmksim&);
    Alarmksim& operator=(const Alarmksim&);

    void OnAlarmksim(const MessageQueueksim::MessagePost_t& _id, MessageQueueksim::Message& _message);
    virtual void    __Run();

  private:
    Runnable*                   target_;
    MessageQueueksim::ScopeRegisterksim reg_async_;
    MessageQueueksim::MessagePost_t broadcast_msg_id_;
    Thread                      runthread_;
    bool                        inthread_;

    int64_t                  	seq_;
    int                         status_;

    int                         after_;
    uint64_t          			starttime_;
    uint64_t          			endtime_;

    MessageQueueksim::ScopeRegisterksim reg_;
#ifdef ANDROID
    WakeUpLock*                 wakelock_;
#endif
};

#endif /* COMM_ALARM_H_ */
