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
 * longlink_task_manager.h
 *
 *  Created on: 2012-7-17
 *      Author: yerungui
 */

#ifndef STN_SRC_LONGLINK_TASK_MANAGER_H_
#define STN_SRC_LONGLINK_TASK_MANAGER_H_

#include <list>
#include <stdint.h>

#include "boost/function.hpp"

#include "marsksim/comm/messagequeue/message_queue.h"
#include "marsksim/comm/alarm.h"
#include "marsksim/stn/stn.h"

#include "longlink.h"
#include "longlink_connect_monitor.h"

class AutoBuffer;
class ActiveLogicksim;

struct STChannelResp;

#ifdef ANDROID
class WakeUpLock;
#endif

namespace marsksim {
    namespace stn {

struct TaskProfile;
class DynamicTimeout;
class LongLinkConnectMonitorksim;

class LongLinkTaskManager {
  public:
    boost_ksim::function<int (ErrCmdType _err_type, int _err_code, int _fail_handle, const Task& _task, unsigned int _taskcosttime)> fun_callback_;

    boost_ksim::function<void (ErrCmdType _err_type, int _err_code, int _fail_handle, uint32_t _src_taskid)> fun_notify_retry_all_tasks;
    boost_ksim::function<void (int _line, ErrCmdType _err_type, int _err_code, const std::string& _ip, uint16_t _port)> fun_notify_network_err_;
    boost_ksim::function<bool (const Task& _task, const void* _buffer, int _len)> fun_anti_avalanche_check_;
    
    boost_ksim::function<void (uint64_t _channel_id, uint32_t _cmdid, uint32_t _taskid, const AutoBuffer& _body, const AutoBuffer& _extend)> fun_on_push_;
    

  public:
    LongLinkTaskManager(marsksim::stn::NetSourceksim& _netsource, ActiveLogicksim& _activelogic, DynamicTimeout& _dynamictimeout, MessageQueueksim::MessageQueueksim_t  _messagequeueid);
    virtual ~LongLinkTaskManager();

    bool StartTask(const Task& _task);
    bool StopTask(uint32_t _taskid);
    bool HasTask(uint32_t _taskid) const;
    void ClearTasks();
    void RedoTasks();
    void RetryTasks(ErrCmdType _err_type, int _err_code, int _fail_handle, uint32_t _src_taskid);

    LongLink& LongLinkChannel() { return *longlink_; }
    LongLinkConnectMonitorksim& getLongLinkConnectMonitorksim() { return *longlinkconnectmon_; }

    unsigned int GetTaskCount();
    unsigned int GetTasksContinuousFailCount();

  private:
    // from ILongLinkObserver
    void __OnResponse(ErrCmdType _error_type, int _error_code, uint32_t _cmdid, uint32_t _taskid, AutoBuffer& _body, AutoBuffer& _extension, const ConnectProfile& _connect_profile);
    void __OnSend(uint32_t _taskid);
    void __OnRecv(uint32_t _taskid, size_t _cachedsize, size_t _totalsize);
    void __SignalConnection(LongLink::TLongLinkStatus _connect_status);

    void __RunLoop();
    void __RunOnTimeout();
    void __RunOnStartTask();

    void __BatchErrorRespHandle(ErrCmdType _err_type, int _err_code, int _fail_handle, uint32_t _src_taskid, const ConnectProfile& _connect_profile, bool _callback_runing_task_only = true);
    bool __SingleRespHandle(std::list<TaskProfile>::iterator _it, ErrCmdType _err_type, int _err_code, int _fail_handle, const ConnectProfile& _connect_profile);

    std::list<TaskProfile>::iterator __Locate(uint32_t  _taskid);

  private:
    MessageQueueksim::ScopeRegisterksim     asyncreg_;
    std::list<TaskProfile>          lst_cmd_;
    uint64_t                        lastbatcherrortime_;   // ms
    unsigned long                   retry_interval_;	//ms
    unsigned int                    tasks_continuous_fail_count_;

    LongLink*                       longlink_;
    LongLinkConnectMonitorksim*         longlinkconnectmon_;
    DynamicTimeout&                 dynamic_timeout_;

#ifdef ANDROID
    WakeUpLock*                     wakeup_lock_;
#endif
};
    }
}

#endif // STN_SRC_LONGLINK_TASK_MANAGER_H_
