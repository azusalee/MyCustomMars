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
 * shortlink_task_manager.h
 *
 *  Created on: 2012-8-24
 *      Author: zhouzhijie
 */

#ifndef STN_SRC_SHORTLINK_TASK_MANAGER_H_
#define STN_SRC_SHORTLINK_TASK_MANAGER_H_

#include <list>
#include <stdint.h>

#include "boost/function.hpp"

#include "marsksim/comm/messagequeue/message_queue.h"
#include "marsksim/comm/alarm.h"
#include "marsksim/stn/stn.h"
#include "marsksim/stn/task_profile.h"

#include "shortlink.h"

class AutoBuffer;

#ifdef ANDROID
class WakeUpLock;
#endif

namespace marsksim {
    namespace stn {

class DynamicTimeout;

class ShortLinkTaskManager {
  public:
    boost_ksim::function<int (ErrCmdType _err_type, int _err_code, int _fail_handle, const Task& _task, unsigned int _taskcosttime)> fun_callback_;
    boost_ksim::function<void (int _line, ErrCmdType _err_type, int _err_code, const std::string& _ip, const std::string& _host, uint16_t _port)> fun_notify_network_err_;
    boost_ksim::function<bool (const Task& _task, const void* _buffer, int _len)> fun_anti_avalanche_check_;
    boost_ksim::function<void (int _status_code)> fun_shortlink_response_;
    boost_ksim::function<void (ErrCmdType _err_type, int _err_code, int _fail_handle, uint32_t _src_taskid)> fun_notify_retry_all_tasks;

  public:
    ShortLinkTaskManager(marsksim::stn::NetSourceksim& _netsource, DynamicTimeout& _dynamictimeout, MessageQueueksim::MessageQueueksim_t _messagequeueid);
    virtual ~ShortLinkTaskManager();

    bool StartTask(const Task& _task);
    bool StopTask(uint32_t _taskid);
    bool HasTask(uint32_t _taskid) const;
    void ClearTasks();
    void RedoTasks();
    void RetryTasks(ErrCmdType _err_type, int _err_code, int _fail_handle, uint32_t _src_taskid);

    unsigned int GetTasksContinuousFailCount();

    ConnectProfile GetConnectProfile(uint32_t _taskid) const;
  private:
    void __RunLoop();
    void __RunOnTimeout();
    void __RunOnStartTask();

    void __OnResponse(ShortLinkInterface* _worker, ErrCmdType _err_type, int _status, AutoBuffer& _body, AutoBuffer& _extension, bool _cancel_retry, ConnectProfile& _conn_profile);
    void __OnSend(ShortLinkInterface* _worker);
    void __OnRecv(ShortLinkInterface* _worker, unsigned int _cached_size, unsigned int _total_size);

    void __BatchErrorRespHandle(ErrCmdType _err_type, int _err_code, int _fail_handle, uint32_t _src_taskid, bool _callback_runing_task_only = true);
    bool __SingleRespHandle(std::list<TaskProfile>::iterator _it, ErrCmdType _err_type, int _err_code, int _fail_handle, size_t _resp_length, const ConnectProfile& _connect_profile);

    std::list<TaskProfile>::iterator __LocateBySeq(intptr_t _running_id);

    void __DeleteShortLink(intptr_t& _running_id);

  private:
    MessageQueueksim::ScopeRegisterksim     asyncreg_;
    NetSourceksim&                      net_source_;
    
    std::list<TaskProfile>          lst_cmd_;
    
    bool                            default_use_proxy_;
    unsigned int                    tasks_continuous_fail_count_;
    DynamicTimeout&                 dynamic_timeout_;
#ifdef ANDROID
    WakeUpLock*                     wakeup_lock_;
#endif
};
        
    }
}


#endif // STN_SRC_SHORTLINK_TASK_MANAGER_H_
