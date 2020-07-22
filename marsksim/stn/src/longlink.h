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
 * longlink.h
 *
 *  Created on: 2014-2-27
 *      Author: yerungui
 */

#ifndef STN_SRC_LONGLINK_H_
#define STN_SRC_LONGLINK_H_

#include <string>
#include <list>

#include "boost/signals2.hpp"
#include "boost/function.hpp"

#include "marsksim/comm/thread/mutex.h"
#include "marsksim/comm/thread/thread.h"
#include "marsksim/comm/alarm.h"
#include "marsksim/comm/tickcount.h"
#include "marsksim/comm/move_wrapper.h"
#include "marsksim/comm/messagequeue/message_queue.h"
#include "marsksim/comm/socket/socketselect.h"

#include "marsksim/stn/stn.h"
#include "marsksim/stn/task_profile.h"

#include "marsksim/stn/src/net_source.h"
#include "marsksim/stn/src/longlink_identify_checker.h"

class AutoBuffer;
class XLogger;
class WakeUpLock;

class SmartHeartbeatksim;

namespace marsksim {
    namespace comm {
        class ProxyInfo;
    }
    namespace stn {

class NetSourceksim;
class longlink_tracker;

struct LongLinkNWriteData {
    LongLinkNWriteData(ssize_t _writelen, const Task& _task)
    : writelen(_writelen), task(_task) {}
    
    ssize_t writelen;
    Task task;
};
        
struct StreamResp {
    StreamResp(const Task& _task = Task(Task::kInvalidTaskID))
    : task(_task), stream(KNullAtuoBuffer), extension(KNullAtuoBuffer) {}
    
    Task task;
    move_wrapper<AutoBuffer> stream;
    move_wrapper<AutoBuffer> extension;
};

class LongLink {
  public:
    enum TLongLinkStatus {
        kConnectIdle = 0,
        kConnecting = 1,
        kConnected,
        kDisConnected,
        kConnectFailed,
    };

    // Note: Never Delete Item!!!Just Add!!!
    enum TDisconnectInternalCode {
        kNone = 0,
        kReset = 10000,        // no use
        kRemoteClosed = 10001,
        kUnknownErr = 10002,
        kNoopTimeout = 10003,
        kDecodeError = 10004,
        kUnknownRead = 10005,
        kUnknownWrite = 10006,
        kDecodeErr = 10007,
        kTaskTimeout = 10008,
        kNetworkChange = 10009,
        kIDCChange = 10010,
        kNetworkLost = 10011,
        kSelectError = 10012,
        kPipeError = 10013,
        kHasNewDnsIP = 10014,
        kSelectException = 10015,
        kLinkCheckTimeout = 10016,
        kForceNewGetDns = 10017,
        kLinkCheckError = 10018,
        kTimeCheckSucc = 10019,
    };
  public:
    boost_ksim::signals2::signal<void (TLongLinkStatus _connectStatus)> SignalConnection;
    boost_ksim::signals2::signal<void (const ConnectProfile& _connprofile)> broadcast_linkstatus_signal_;
    
    boost_ksim::function< void (uint32_t _taskid)> OnSend;
    boost_ksim::function< void (uint32_t _taskid, size_t _cachedsize, size_t _package_size)> OnRecv;
    boost_ksim::function< void (ErrCmdType _error_type, int _error_code, uint32_t _cmdid, uint32_t _taskid, AutoBuffer& _body, AutoBuffer& _extension, const ConnectProfile& _info)> OnResponse;
    boost_ksim::function<void (int _line, ErrCmdType _errtype, int _errcode, const std::string& _ip, uint16_t _port)> fun_network_report_;

  public:
    LongLink(const mqksim::MessageQueueksim_t& _messagequeueid, NetSourceksim& _netsource);
    virtual ~LongLink();

    bool    Send(const AutoBuffer& _body, const AutoBuffer& _extension, const Task& _task);
    bool    SendWhenNoData(const AutoBuffer& _body, const AutoBuffer& _extension, uint32_t _cmdid, uint32_t _taskid);
    bool    Stop(uint32_t _taskid);

    bool            MakeSureConnected(bool* _newone = NULL);
    void            Disconnect(TDisconnectInternalCode _scene);
    TLongLinkStatus ConnectStatus() const;

    ConnectProfile  Profile() const   { return conn_profile_; }
    tickcount_t&    GetLastRecvTime() { return lastrecvtime_; }
    
  private:
    LongLink(const LongLink&);
    LongLink& operator=(const LongLink&);

  protected:
    void    __ConnectStatus(TLongLinkStatus _status);
    void    __UpdateProfile(const ConnectProfile& _conn_profile);
    void    __RunResponseError(ErrCmdType _type, int _errcode, ConnectProfile& _profile, bool _networkreport = true);

    bool    __SendNoopWhenNoData();
    bool    __NoopReq(XLogger& _xlog, Alarmksim& _alarm, bool need_active_timeout);
    bool    __NoopResp(uint32_t _cmdid, uint32_t _taskid, AutoBuffer& _buf, AutoBuffer& _extension, Alarmksim& _alarm, bool& _nooping, ConnectProfile& _profile);

    virtual void     __OnAlarmksim();
    virtual void     __Run();
    virtual SOCKET   __RunConnect(ConnectProfile& _conn_profile);
    virtual void     __RunReadWrite(SOCKET _sock, ErrCmdType& _errtype, int& _errcode, ConnectProfile& _profile);
  protected:
    
    uint32_t   __GetNextHeartbeatInterval();
    void       __NotifySmartHeartbeatksimConnectStatus(TLongLinkStatus _status);
    void       __NotifySmartHeartbeatksimHeartReq(ConnectProfile& _profile, uint64_t _internal, uint64_t _actual_internal);
    void       __NotifySmartHeartbeatksimHeartResult(bool _succes, bool _fail_of_timeout, ConnectProfile& _profile);
    void       __NotifySmartHeartbeatksimJudgeDozeStyle();
	
  protected:
    MessageQueueksim::ScopeRegisterksim     asyncreg_;
    NetSourceksim&                      netsource_;
    
    Mutex                           mutex_;
    Thread                          thread_;

    boost_ksim::scoped_ptr<longlink_tracker>         tracker_;
    NetSourceksim::DnsUtil                          dns_util_;
    SocketBreaker                               connectbreak_;
    TLongLinkStatus                             connectstatus_;
    ConnectProfile                              conn_profile_;
    TDisconnectInternalCode                     disconnectinternalcode_;
    
    SocketBreaker                                        readwritebreak_;
    LongLinkIdentifyCheckerksim                              identifychecker_;
    std::list<std::pair<Task, move_wrapper<AutoBuffer>>> lstsenddata_;
    tickcount_t                                          lastrecvtime_;
    
    SmartHeartbeatksim*                              smartheartbeat_;
    WakeUpLock*                                  wakelock_;
};
        
}}

#endif // STN_SRC_LONGLINK_H_
