//
//  ksim_app_callback.h
//  mytest
//
//  Created by lizihong on 2020/6/11.
//  Copyright © 2020 AL. All rights reserved.
//

#ifndef appcomm_callback_h_ksim
#define appcomm_callback_h_ksim

#import <marsksim/app/app.h>
#import <marsksim/app/app_logic.h>

namespace marsksim {
    namespace app {


class AppCallBack_ksim : public Callback {
    
private:
    AppCallBack_ksim() {}
    ~AppCallBack_ksim() {}
    AppCallBack_ksim(AppCallBack_ksim&);
    AppCallBack_ksim& operator = (AppCallBack_ksim&);
    
    
public:
    static AppCallBack_ksim* Instance();
    static void Release();
    
    virtual std::string GetAppFilePath();
    
    virtual AccountInfo GetAccountInfo();
    
    virtual unsigned int GetClientVersion();
    
    virtual DeviceInfo GetDeviceInfo();
    
private:
    static AppCallBack_ksim* instance_;
};
        
}}

#endif
