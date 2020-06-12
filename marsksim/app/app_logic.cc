// Tencent is pleased to support the open source community by making Mars available.
// Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.

// Licensed under the MIT License (the "License"); you may not use this file except in 
// compliance with the License. You may obtain a copy of the License at
// http://opensource.org/licenses/MIT

// Unless required by applicable law or agreed to in writing, software distributed under the License is
// distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
// either express or implied. See the License for the specific language governing permissions and
// limitations under the License.

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
 * app_logic.cc
 *
 *  Created on: 2016/3/3
 *      Author: caoshaokun
 */

#include "marsksim/app/app_logic.h"

#ifdef __APPLE__
#include <TargetConditionals.h>
#include "marsksim/comm/objc/data_protect_attr.h"
#endif

#include "marsksim/comm/xlogger/xlogger.h"
#include "marsksim/comm/bootrun.h"
#include "marsksim/comm/thread/mutex.h"
#include "marsksim/comm/thread/lock.h"
#include "marsksim/comm/thread/thread.h"
#include "marsksim/comm/time_utils.h"
#include "marsksim/comm/dns/dns.h"
#include "marsksim/baseevent/baseprjevent.h"

namespace marsksim {
namespace app {

static Callback* sg_callback = NULL;

void SetCallback(Callback* const callback) {
	sg_callback = callback;
}

#ifndef ANDROID
    
    static marsksim::comm::ProxyInfo sg_proxyInfo;
    static bool sg_gotProxy = false;
    static Mutex sg_slproxymutex;
    static Thread sg_slproxyThread;
    static uint64_t sg_slporxytimetick = gettickcount();
    static int sg_slproxycount = 0;
    
    static void __ClearProxyInfo() {
        ScopedLock lock(sg_slproxymutex);
        sg_slporxytimetick = gettickcount();
        sg_slproxycount = 0;
        sg_gotProxy = false;
        sg_proxyInfo.type = marsksim::comm::kProxyNone;
    }
    
    static void __GetProxyInfo(const std::string& _host, uint64_t _timetick) {
        xinfo_function(TSF"timetick:%_, host:%_", _timetick, _host);
    
        marsksim::comm::ProxyInfo proxy_info;
        if (!sg_callback->GetProxyInfo(_host, proxy_info)) {
            ScopedLock lock(sg_slproxymutex);
            if (_timetick != sg_slporxytimetick) {
                return;
            }
            ++ sg_slproxycount;
            return;
        }
        
        ScopedLock lock(sg_slproxymutex);
        if (_timetick != sg_slporxytimetick) {
            return;
        }
        
        ++ sg_slproxycount;
        
        sg_proxyInfo = proxy_info;
        
        if (marsksim::comm::kProxyNone == sg_proxyInfo.type || !sg_proxyInfo.ip.empty() || sg_proxyInfo.host.empty()) {
            sg_gotProxy = true;
            return;
        }
        
        std::string host = sg_proxyInfo.host;
        lock.unlock();
        
        static DNS s_dns;
        std::vector<std::string> ips;
        s_dns.GetHostByName(host, ips);
        
        if (ips.empty()) {
            return;
        }
        
        lock.lock();
        sg_proxyInfo.ip = ips.front();
        sg_gotProxy = true;
        
    }
    
    static void __InitbindBaseprjevent() {
        GetSignalksimOnNetworkChange().connect(&__ClearProxyInfo);
    }

    
#if TARGET_OS_IPHONE
    BOOT_RUN_STARTUP(__InitbindBaseprjevent);
#endif
    
    marsksim::comm::ProxyInfo GetProxyInfo(const std::string& _host) {
        xassert2(sg_callback != NULL);
        
#if !TARGET_OS_IPHONE
        marsksim::comm::ProxyInfo proxy_info;
        sg_callback->GetProxyInfo(_host, proxy_info);
        return proxy_info;
#endif
        
        if (sg_gotProxy) {
            return sg_proxyInfo;
        }
        
        ScopedLock lock(sg_slproxymutex, false);
        if (!lock.timedlock(500))   return marsksim::comm::ProxyInfo();
        
        if (sg_slproxycount < 3 || 5 * 1000 > gettickspan(sg_slporxytimetick)) {
            sg_slproxyThread.start(boost_ksim::bind(&__GetProxyInfo, _host, sg_slporxytimetick));
        }
        
        if (sg_gotProxy) {
            return sg_proxyInfo;
        }
        
        return marsksim::comm::ProxyInfo();

    }

    std::string GetAppFilePath() {
        xassert2(sg_callback != NULL);

        std::string path = sg_callback->GetAppFilePath();
#ifdef __APPLE__
        setAttrProtectionNone(path.c_str());
#endif

        return path;
    }
    
	AccountInfo GetAccountInfo() {
		xassert2(sg_callback != NULL);
		return sg_callback->GetAccountInfo();
	}

	std::string GetUserName() {
		xassert2(sg_callback != NULL);
		AccountInfo info = sg_callback->GetAccountInfo();
		return info.username;
	}

	std::string GetRecentUserName() {
		xassert2(sg_callback != NULL);
		return GetUserName();
	}

	unsigned int GetClientVersion() {
		xassert2(sg_callback != NULL);
		return sg_callback->GetClientVersion();
	}


	DeviceInfo GetDeviceInfo() {
		xassert2(sg_callback != NULL);
        
    static DeviceInfo device_info;
    if (!device_info.devicename.empty() || !device_info.devicetype.empty()) {
        return device_info;
    }
    
    device_info = sg_callback->GetDeviceInfo();
    return device_info;
	}

#endif

}
}


