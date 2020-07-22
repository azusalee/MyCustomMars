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
 * active_logic.cc
 *
 *  Created on: 2012-8-22
 *      Author: yerungui
 */

#include "boost/bind.hpp"

#include "marsksim/baseevent/active_logic.h"
#include "marsksim/baseevent/baseprjevent.h"
#include "marsksim/comm/singleton.h"
#include "marsksim/comm/xlogger/xlogger.h"
#include "marsksim/comm/thread/lock.h"
#include "marsksim/comm/time_utils.h"
#include "marsksim/comm/bootrun.h"
#include "marsksim/comm/messagequeue/message_queue.h"

static void onForeground(bool _isforeground) {
    ActiveLogicksim::Singletonksim::Instance()->OnForeground(_isforeground);
}

static void __initbind_baseprjevent() {
    GetSignalksimOnForeground().connect(&onForeground);
}

BOOT_RUN_STARTUP(__initbind_baseprjevent);

#define INACTIVE_TIMEOUT (10*60*1000) //ms

ActiveLogicksim::ActiveLogicksim()
: isforeground_(false), isactive_(true)
, alarm_(boost_ksim::bind(&ActiveLogicksim::__OnInActive, this), false)
, lastforegroundchangetime_(::gettickcount())
{
    xinfo_function();
#ifndef __APPLE__
        if (!alarm_.Start(INACTIVE_TIMEOUT))
       	{
            xerror2(TSF"m_alarm.Start false");
    	}
#endif
}

ActiveLogicksim::~ActiveLogicksim()
{
    xinfo_function();
	MessageQueueksim::CancelMessage(MessageQueueksim::DefAsyncInvokeHandler(MessageQueueksim::GetDefMessageQueueksim()), (MessageQueueksim::MessageTitle_t)this);
	MessageQueueksim::WaitForRunningLockEnd(MessageQueueksim::DefAsyncInvokeHandler(MessageQueueksim::GetDefMessageQueueksim()));
}

void ActiveLogicksim::OnForeground(bool _isforeground)
{
	if (MessageQueueksim::GetDefMessageQueueksim()!=MessageQueueksim::CurrentThreadMessageQueueksim())
	{
        MessageQueueksim::AsyncInvoke(boost_ksim::bind(&ActiveLogicksim::OnForeground, this, _isforeground), (MessageQueueksim::MessageTitle_t)this, mqksim::DefAsyncInvokeHandler(mqksim::GetDefMessageQueueksim()), "ActiveLogicksim::OnForeground");
		return;
	}

    xgroup2_define(group);
    xinfo2(TSF"OnForeground:%0, change:%1, ", _isforeground, _isforeground!=isforeground_) >> group;

    if (_isforeground == isforeground_) return;

    bool oldisactive = isactive_;
    isactive_ = true;
    isforeground_ = _isforeground;
    lastforegroundchangetime_ = ::gettickcount();
    alarm_.Cancel();

    if (!isforeground_)
    {
#ifndef __APPLE__
        if (!alarm_.Start(INACTIVE_TIMEOUT))
       	{
            xerror2(TSF"m_alarm.Start false") >> group;
    	}
#endif
    }

    bool isnotify = oldisactive!=isactive_;
    SignalForeground(isforeground_);

    if (isnotify)
    {
    	xinfo2(TSF"active change:%0", isactive_) >> group;
    	SignalActive(isactive_);
    }
}

bool ActiveLogicksim::IsActive() const
{
    return isactive_;
}

bool ActiveLogicksim::IsForeground() const
{
	return isforeground_;
}

uint64_t ActiveLogicksim::LastForegroundChangeTime() const
{
	return lastforegroundchangetime_;
}

void ActiveLogicksim::__OnInActive()
{
    xdebug_function();
    if (!isforeground_) isactive_ = false;

    bool  isactive = isactive_;
    xinfo2(TSF"active change:%0", isactive_);
    SignalActive(isactive);
}
