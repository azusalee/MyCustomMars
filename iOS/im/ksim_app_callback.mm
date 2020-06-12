//
//  ksim_app_callback.m
//  mytest
//
//  Created by lizihong on 2020/6/11.
//  Copyright © 2020 AL. All rights reserved.
//

#import "ksim_app_callback.h"
#import <marsksim/comm/autobuffer.h>

namespace marsksim {
    namespace app {

AppCallBack_ksim* AppCallBack_ksim::instance_ = NULL;

AppCallBack_ksim* AppCallBack_ksim::Instance() {
    if(instance_ == NULL) {
        instance_ = new AppCallBack_ksim();
    }
    
    return instance_;
}

void AppCallBack_ksim::Release() {
    delete instance_;
    instance_ = NULL;
}

// return your app path
std::string AppCallBack_ksim::GetAppFilePath(){
    return "";
}
        
AccountInfo AppCallBack_ksim::GetAccountInfo() {
    AccountInfo info;
    info.username = "Tester";
    
    return info;
}

unsigned int AppCallBack_ksim::GetClientVersion() {
    
    return 0;
}

DeviceInfo AppCallBack_ksim::GetDeviceInfo() {
    DeviceInfo info;

    info.devicename = "";
    info.devicetype = 1;
    
    return info;
}

}}
