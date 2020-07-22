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
 * netsource_timercheck.h
 *
 *  Created on: 2013-5-16
 *      Author: yanguoyue
 */

#ifndef STN_SRC_NETSOURCE_TIMERCHECK_H_
#define STN_SRC_NETSOURCE_TIMERCHECK_H_

#include "boost/signals2.hpp"

#include "marsksim/comm/thread/thread.h"
#include "marsksim/baseevent/active_logic.h"
#include "marsksim/comm/socket/socketselect.h"
#include "marsksim/comm/messagequeue/message_queue.h"

#include "net_source.h"

class CommFrequencyLimitksim;

namespace marsksim {
    namespace stn {
        
class LongLink;

class NetSourceksimTimerCheck {
  public:
    NetSourceksimTimerCheck(NetSourceksim* _net_source, ActiveLogicksim& _active_logic, LongLink& _longlink, MessageQueueksim::MessageQueueksim_t  _messagequeue_id);
    ~NetSourceksimTimerCheck();
    void CancelConnect();

  public:
    boost_ksim::function<void ()> fun_time_check_suc_;

  private:
    void __Run(const std::string& _host);
    bool __TryConnnect(const std::string& _host);
    void __OnActiveChanged(bool _is_active);
    void __StartCheck();
    void __Check();
    void __StopCheck();

  private:
    Thread thread_;
    boost_ksim::signals2::scoped_connection active_connection_;
    NetSourceksim* net_source_;
    SocketBreaker breaker_;
    SocketSelect seletor_;
    CommFrequencyLimitksim* frequency_limit_;
    LongLink& longlink_;

    MessageQueueksim::ScopeRegisterksim asyncreg_;
    MessageQueueksim::MessagePost_t asyncpost_;
    NetSourceksim::DnsUtil dns_util_;
};
        
    }
}


#endif // STN_SRC_NETSOURCE_TIMERCHECK_H_
