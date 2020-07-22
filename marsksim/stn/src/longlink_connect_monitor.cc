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
 * longlink_connect_monitor.cc
 *
 *  Created on: 2014-2-26
 *      Author: yerungui
 */

#include "longlink_connect_monitor.h"

#include "boost/bind.hpp"

#include "marsksim/app/app.h"
#include "marsksim/baseevent/active_logic.h"
#include "marsksim/comm/thread/lock.h"
#include "marsksim/comm/xlogger/xlogger.h"
#include "marsksim/comm/time_utils.h"
#include "marsksim/comm/socket/unix_socket.h"
#include "marsksim/comm/platform_comm.h"
#include "marsksim/sdt/src/checkimpl/dnsquery.h"
#include "marsksim/stn/config.h"

#include "longlink_speed_test.h"
#include "net_source.h"

using namespace marsksim::stn;
using namespace marsksim::app;

static const unsigned int kTimeCheckPeriod = 10 * 1000;     // 10s
static const unsigned int kStartCheckPeriod = 3 * 1000;     // 3s

static const unsigned long kNoNetSaltRate = 3;
static const unsigned long kNoNetSaltRise = 600;
static const unsigned long kNoAccountInfoSaltRate = 2;
static const unsigned long kNoAccountInfoSaltRise = 300;

static const unsigned long kNoAccountInfoInactiveInterval = (7 * 24 * 60 * 60);  // s

enum {
    kTaskConnect,
    kLongLinkConnect,
    kNetworkChangeConnect,
};

enum {
    kForgroundOneMinute,
    kForgroundTenMinute,
    kForgroundActive,
    kBackgroundActive,
    kInactive,
};

static unsigned long const sg_interval[][5]  = {
    {5,  10, 20,  30,  300},
    {15, 30, 240, 300, 600},
    {0,  0,  0,   0,   0},
};

static int __CurActiveStateksim(const ActiveLogicksim& _activeLogic) {
    if (!_activeLogic.IsActive()) return kInactive;

    if (!_activeLogic.IsForeground()) return kBackgroundActive;

    if (10 * 60 * 1000 <= ::gettickcount() - _activeLogic.LastForegroundChangeTime()) return kForgroundActive;

    if (60 * 1000 <= ::gettickcount() - _activeLogic.LastForegroundChangeTime()) return kForgroundTenMinute;

    return kForgroundOneMinute;
}

static unsigned long __Interval(int _type, const ActiveLogicksim& _activelogic) {
    unsigned long interval = sg_interval[_type][__CurActiveStateksim(_activelogic)];

    if (kLongLinkConnect != _type) return interval;

    if (__CurActiveStateksim(_activelogic) == kInactive || __CurActiveStateksim(_activelogic) == kForgroundActive) {  // now - LastForegroundChangeTime>10min
        if (!_activelogic.IsActive() && GetAccountInfo().username.empty()) {
            interval = kNoAccountInfoInactiveInterval;
            xwarn2(TSF"no account info and inactive, interval:%_", interval);

        } else if (kNoNet == getNetInfo()) {
            interval = interval * kNoNetSaltRate + kNoNetSaltRise;
            xinfo2(TSF"no net, interval:%0", interval);

        } else if (GetAccountInfo().username.empty()) {
            interval = interval * kNoAccountInfoSaltRate + kNoAccountInfoSaltRise;
            xinfo2(TSF"no account info, interval:%0", interval);

        } else {
            // default value
			interval += rand() % (20);
        }
    }

    return interval;
}

#define AYNC_HANDLER asyncreg_.Get()

LongLinkConnectMonitorksim::LongLinkConnectMonitorksim(ActiveLogicksim& _activelogic, LongLink& _longlink, MessageQueueksim::MessageQueueksim_t _id)
    : asyncreg_(MessageQueueksim::InstallAsyncHandler(_id))
    , activelogic_(_activelogic), longlink_(_longlink), alarm_(boost_ksim::bind(&LongLinkConnectMonitorksim::__OnAlarmksim, this), _id)
    , status_(LongLink::kDisConnected)
    , last_connect_time_(0)
    , last_connect_net_type_(kNoNet)
    , thread_(boost_ksim::bind(&LongLinkConnectMonitorksim::__Run, this), XLOGGER_TAG"::con_mon")
    , conti_suc_count_(0)
    , isstart_(false) {
    xinfo2(TSF"handler:(%_,%_)", asyncreg_.Get().queue,asyncreg_.Get().seq);
    activelogic_.SignalActive.connect(boost_ksim::bind(&LongLinkConnectMonitorksim::__OnSignalActive, this, _1));
    activelogic_.SignalForeground.connect(boost_ksim::bind(&LongLinkConnectMonitorksim::__OnSignalForeground, this, _1));
    longlink_.SignalConnection.connect(boost_ksim::bind(&LongLinkConnectMonitorksim::__OnLongLinkStatuChanged, this, _1));
}

LongLinkConnectMonitorksim::~LongLinkConnectMonitorksim() {
#ifdef __APPLE__
    __StopTimer();
#endif
    longlink_.SignalConnection.disconnect(boost_ksim::bind(&LongLinkConnectMonitorksim::__OnLongLinkStatuChanged, this, _1));
    activelogic_.SignalForeground.disconnect(boost_ksim::bind(&LongLinkConnectMonitorksim::__OnSignalForeground, this, _1));
    activelogic_.SignalActive.disconnect(boost_ksim::bind(&LongLinkConnectMonitorksim::__OnSignalActive, this, _1));
    asyncreg_.CancelAndWait();
}

bool LongLinkConnectMonitorksim::MakeSureConnected() {
    __IntervalConnect(kTaskConnect);
    return LongLink::kConnected == longlink_.ConnectStatus();
}

bool LongLinkConnectMonitorksim::NetworkChange() {
    xinfo_function();
#ifdef __APPLE__
    __StopTimer();

    do {
        if (LongLink::kConnected != status_ || (::gettickcount() - last_connect_time_) <= 10 * 1000) break;

        if (kMobile != last_connect_net_type_) break;

        int netifo = getNetInfo();

        if (kNoNet == netifo) break;

        if (__StartTimer()) return false;
    } while (false);

#endif
    longlink_.Disconnect(LongLink::kNetworkChange);
    return 0 == __IntervalConnect(kNetworkChangeConnect);
}

uint64_t LongLinkConnectMonitorksim::__IntervalConnect(int _type) {
    if (LongLink::kConnecting == longlink_.ConnectStatus() || LongLink::kConnected == longlink_.ConnectStatus()) return 0;

    uint64_t interval =  __Interval(_type, activelogic_) * 1000ULL;
    uint64_t posttime = gettickcount() - longlink_.Profile().dns_time;

    if (posttime >= interval) {
        bool newone = false;
        bool ret = longlink_.MakeSureConnected(&newone);
        xinfo2(TSF"made interval connect interval:%0, posttime:%_, newone:%_, connectstatus:%_, ret:%_", interval, posttime, newone, longlink_.ConnectStatus(), ret);
        return 0;

    } else {
        return interval - posttime;
    }
}

uint64_t LongLinkConnectMonitorksim::__AutoIntervalConnect() {
    alarm_.Cancel();
    uint64_t remain = __IntervalConnect(kLongLinkConnect);

    if (0 == remain) return remain;

    xinfo2(TSF"start auto connect after:%0", remain);
    alarm_.Start((int)remain);
    return remain;
}

void LongLinkConnectMonitorksim::__OnSignalForeground(bool _isForeground) {
    ASYNC_BLOCK_START
#ifdef __APPLE__
    xinfo2(TSF"forground:%_ time:%_ tick:%_", _isForeground, timeMs(), gettickcount());

    if (_isForeground) {
        xinfo2(TSF"longlink:%_ time:%_ %_ %_", longlink_.ConnectStatus(), tickcount_t().gettickcount().get(), longlink_.GetLastRecvTime().get(), int64_t(tickcount_t().gettickcount() - longlink_.GetLastRecvTime()));
        
        if ((longlink_.ConnectStatus() == LongLink::kConnected) &&
                (tickcount_t().gettickcount() - longlink_.GetLastRecvTime() > tickcountdiff_t(4.5 * 60 * 1000))) {
            xwarn2(TSF"sock long time no send data, close it");
            __ReConnect();
        }
    }

#endif
    __AutoIntervalConnect();
    ASYNC_BLOCK_END
}

void LongLinkConnectMonitorksim::__OnSignalActive(bool _isactive) {
    ASYNC_BLOCK_START
    __AutoIntervalConnect();
    ASYNC_BLOCK_END
}

void LongLinkConnectMonitorksim::__OnLongLinkStatuChanged(LongLink::TLongLinkStatus _status) {
    alarm_.Cancel();

    if (LongLink::kConnectFailed == _status || LongLink::kDisConnected == _status) {
        alarm_.Start(500);
    } else if (LongLink::kConnected == _status) {
        xinfo2(TSF"cancel auto connect");
    }

    status_ = _status;
    last_connect_time_ = ::gettickcount();
    last_connect_net_type_ = ::getNetInfo();
}

void LongLinkConnectMonitorksim::__OnAlarmksim() {
    __AutoIntervalConnect();
}

#ifdef __APPLE__
bool LongLinkConnectMonitorksim::__StartTimer() {
    xdebug_function();

    conti_suc_count_ = 0;

    ScopedLock lock(testmutex_);
    isstart_ = true;

    if (thread_.isruning()) {
        return true;
    }

    int ret = thread_.start_periodic(kStartCheckPeriod, kTimeCheckPeriod);
    return 0 == ret;
}


bool LongLinkConnectMonitorksim::__StopTimer() {
    xdebug_function();

    ScopedLock lock(testmutex_);

    if (!isstart_) return true;

    isstart_ = false;

    if (!thread_.isruning()) {
        return true;
    }

    thread_.cancel_periodic();


    thread_.join();
    return true;
}
#endif


void LongLinkConnectMonitorksim::__Run() {
    int netifo = getNetInfo();

    if (LongLink::kConnected != status_ || (::gettickcount() - last_connect_time_) <= 12 * 1000
            || kMobile != last_connect_net_type_ || kMobile == netifo) {
        thread_.cancel_periodic();
        return;
    }

    struct socket_ipinfo_t dummyIpInfo;
    int ret = socket_gethostbyname(NetSourceksim::GetLongLinkHosts().front().c_str(), &dummyIpInfo, 0, NULL);

    if (ret == 0) {
        ++conti_suc_count_;
    } else {
        conti_suc_count_ = 0;
    }

    if (conti_suc_count_ >= 3) {
        __ReConnect();
        thread_.cancel_periodic();
    }
}

void LongLinkConnectMonitorksim::__ReConnect() {
    xinfo_function();
    xassert2(fun_longlink_reset_);
    fun_longlink_reset_();
}


