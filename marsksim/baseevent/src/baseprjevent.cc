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
 * baseprjevent.cpp
 *
 *  Created on: 2014-7-7
 *      Author: yerungui
 */

#include "marsksim/baseevent/baseprjevent.h"

boost_ksim::signals2::signal<void ()>& GetSignalksimOnCreate()
{
	static boost_ksim::signals2::signal<void ()> SignalOnCreate;
	return SignalOnCreate;
}

boost_ksim::signals2::signal<void ()>& GetSignalksimOnDestroy()
{
	static boost_ksim::signals2::signal<void ()> SignalOnDestroy;
	return SignalOnDestroy;
}

boost_ksim::signals2::signal<void (int _sig)>& GetSignalksimOnSingalCrash()
{
	static boost_ksim::signals2::signal<void (int _sig)> SignalOnSingalCrash;
	return SignalOnSingalCrash;
}

boost_ksim::signals2::signal<void ()>& GetSignalksimOnExceptionCrash()
{
	static boost_ksim::signals2::signal<void ()> SignalOnExceptionCrash;
	return SignalOnExceptionCrash;
}

boost_ksim::signals2::signal<void (bool _isForeground)>& GetSignalksimOnForeground()
{
	static boost_ksim::signals2::signal<void (bool _isForeground)> SignalOnForeground;
	return SignalOnForeground;
}

boost_ksim::signals2::signal<void ()>& GetSignalksimOnNetworkChange()
{
	static boost_ksim::signals2::signal<void ()> SignalOnNetworkChange;
	return SignalOnNetworkChange;
}


boost_ksim::signals2::signal<void (const char* _tag, ssize_t _send, ssize_t _recv)>& GetSignalksimOnNetworkDataChange() {
    static boost_ksim::signals2::signal<void (const char* _tag, ssize_t _send, ssize_t _recv)> SignalOnNetworkDataChange;
    return SignalOnNetworkDataChange;
}
