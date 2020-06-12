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
 * net_core.h
 *
 *  Created on: 2012-7-18
 *      Author: yerungui
 */

#ifndef STN_SRC_NET_CORE_H_
#define STN_SRC_NET_CORE_H_

#include "marsksim/comm/singleton.h"
#include "marsksim/comm/messagequeue/message_queue.h"

#include "marsksim/stn/stn.h"
#include "marsksim/stn/config.h"
#ifdef USE_LONG_LINK
#include "marsksim/stn/src/longlink.h"
#endif

namespace marsksim {
    
    namespace stn {

class NetSourceksim;

    
class ShortLinkTaskManager;
        
#ifdef USE_LONG_LINK
class LongLinkTaskManager;
class TimingSync;
class ZombieTaskManager;
class NetSourceksimTimerCheck;
#endif
        
class SignallingKeeper;
class NetCheckLogic;
class DynamicTimeout;
class AntiAvalanche;

enum {
    kCallFromLong,
    kCallFromShort,
    kCallFromZombie,
};

class NetCoreksim {
  public:
    SINGLETON_INTRUSIVE_ksim(NetCoreksim, new NetCoreksim, __Release);

  public:
    boost_ksim::function<void (Task& _task)> task_process_hook_;
    boost_ksim::function<int (int _from, ErrCmdType _err_type, int _err_code, int _fail_handle, const Task& _task)> task_callback_hook_;
    boost_ksim::signals2::signal<void (uint32_t _cmdid, const AutoBuffer& _buffer)> push_preprocess_signal_;

  public:
    MessageQueueksim::MessageQueueksim_t GetMessageQueueksimId() { return messagequeue_creater_.GetMessageQueueksim(); }
    NetSourceksim& GetNetSourceksimRef() {return *net_source_;}
    
    void    CancelAndWait() { messagequeue_creater_.CancelAndWait(); }
    
    void    StartTask(const Task& _task);
    void    StopTask(uint32_t _taskid);
    bool    HasTask(uint32_t _taskid) const;
    void    ClearTasks();
    void    RedoTasks();
    void    RetryTasks(ErrCmdType _err_type, int _err_code, int _fail_handle, uint32_t _src_taskid);

    void    MakeSureLongLinkConnect();
    bool    LongLinkIsConnected();
    void    DisConnectLonglink();
    void    OnNetworkChange();

    void	KeepSignal();
    void	StopSignal();

    ConnectProfile GetConnectProfile(uint32_t _taskid, int _channel_select);
    void AddServerBan(const std::string& _ip);
    
#ifdef USE_LONG_LINK
    LongLink& Longlink();
#endif

  private:
    NetCoreksim();
    virtual ~NetCoreksim();
    static void __Release(NetCoreksim* _instance);
    
  private:
    int     __CallBack(int _from, ErrCmdType _err_type, int _err_code, int _fail_handle, const Task& _task, unsigned int _taskcosttime);
    void    __OnShortLinkNetworkError(int _line, ErrCmdType _err_type, int _err_code, const std::string& _ip, const std::string& _host, uint16_t _port);

    void    __OnShortLinkResponse(int _status_code);

#ifdef USE_LONG_LINK
    void    __OnLongLinkNetworkError(int _line, ErrCmdType _err_type, int _err_code, const std::string& _ip, uint16_t _port);
    void    __OnLongLinkConnStatusChange(LongLink::TLongLinkStatus _status);
    void    __ResetLongLink();
#endif
    
    void    __ConnStatusCallBack();
    void    __OnTimerCheckSuc();
    
    void    __OnSignalActive(bool _isactive);

    void    __OnPush(uint64_t _channel_id, uint32_t _cmdid, uint32_t _taskid, const AutoBuffer& _body, const AutoBuffer& _extend);
  private:
    NetCoreksim(const NetCoreksim&);
    NetCoreksim& operator=(const NetCoreksim&);

  private:
    MessageQueueksim::MessageQueueksimCreater   messagequeue_creater_;
    MessageQueueksim::ScopeRegister         asyncreg_;
    NetSourceksim*                          net_source_;
    NetCheckLogic*                      netcheck_logic_;
    AntiAvalanche*                      anti_avalanche_;
    
    DynamicTimeout*                     dynamic_timeout_;
    ShortLinkTaskManager*               shortlink_task_manager_;
    int                                 shortlink_error_count_;

#ifdef USE_LONG_LINK
    ZombieTaskManager*                  zombie_task_manager_;
    LongLinkTaskManager*                longlink_task_manager_;
    SignallingKeeper*                   signalling_keeper_;
    NetSourceksimTimerCheck*                netsource_timercheck_;
    TimingSync*                         timing_sync_;
#endif

    bool                                shortlink_try_flag_;

};
        
}}

#endif // STN_SRC_NET_CORE_H_
