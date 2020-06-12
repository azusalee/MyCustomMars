//
//  ksim_stn_callback.m
//  mytest
//
//  Created by lizihong on 2020/6/11.
//  Copyright © 2020 AL. All rights reserved.
//

#import "ksim_stn_callback.h"

#import <marsksim/comm/autobuffer.h>
#import <marsksim/xlog/xlogger.h>
#import <marsksim/stn/stn.h>

#include "KSIMMarsNetworkService.h"

namespace marsksim {
    namespace stn {
        
StnCallBack_ksim* StnCallBack_ksim::instance_ = NULL;
        
StnCallBack_ksim* StnCallBack_ksim::Instance() {
    if(instance_ == NULL) {
        instance_ = new StnCallBack_ksim();
    }
    
    return instance_;
}
        
void StnCallBack_ksim::Release() {
    delete instance_;
    instance_ = NULL;
}
        
bool StnCallBack_ksim::MakesureAuthed() {
    return [[KSIMMarsNetworkService sharedInstance] isAuthed];
}


void StnCallBack_ksim::TrafficData(ssize_t _send, ssize_t _recv) {
    xdebug2(TSF"send:%_, recv:%_", _send, _recv);
}
        
std::vector<std::string> StnCallBack_ksim::OnNewDns(const std::string& _host) {
    std::vector<std::string> vector;
    //vector.push_back("118.89.24.72");
    
    vector.push_back([[[KSIMMarsNetworkService sharedInstance] ipString] UTF8String]);
    return vector;
}

void StnCallBack_ksim::OnPush(uint64_t _channel_id, uint32_t _cmdid, uint32_t _taskid, const AutoBuffer& _body, const AutoBuffer& _extend) {
    if (_body.Length() > 0) {
        NSData* recvData = [NSData dataWithBytes:(const void *) _body.Ptr() length:_body.Length()];
        [[KSIMMarsNetworkService sharedInstance] OnPushWithCmd:_cmdid data:recvData];
    }
    
}

bool StnCallBack_ksim::Req2Buf(uint32_t _taskid, void* const _user_context, AutoBuffer& _outbuffer, AutoBuffer& _extend, int& _error_code, const int _channel_select) {
    NSData* requestData =  [[KSIMMarsNetworkService sharedInstance] Request2BufferWithTaskID:_taskid userContext:_user_context];
    if (requestData == nil) {
        requestData = [[NSData alloc] init];
    }
    _outbuffer.AllocWrite(requestData.length);
    _outbuffer.Write(requestData.bytes,requestData.length);
    return requestData.length > 0;
}

int StnCallBack_ksim::Buf2Resp(uint32_t _taskid, void* const _user_context, const AutoBuffer& _inbuffer, const AutoBuffer& _extend, int& _error_code, const int _channel_select) {
    
    int handle_type = marsksim::stn::kTaskFailHandleNormal;
    NSData* responseData = [NSData dataWithBytes:(const void *) _inbuffer.Ptr() length:_inbuffer.Length()];
    NSInteger errorCode = [[KSIMMarsNetworkService sharedInstance] Buffer2ResponseWithTaskID:_taskid ResponseData:responseData userContext:_user_context];
    if (errorCode != 0) {
        handle_type = marsksim::stn::kTaskFailHandleDefault;
    }
    
    return handle_type;
}

int StnCallBack_ksim::OnTaskEnd(uint32_t _taskid, void* const _user_context, int _error_type, int _error_code) {
    
    return (int)[[KSIMMarsNetworkService sharedInstance] OnTaskEndWithTaskID:_taskid userContext:_user_context errType:_error_type errCode:_error_code];

}

void StnCallBack_ksim::ReportConnectStatus(int _status, int longlink_status) {
    [[KSIMMarsNetworkService sharedInstance] OnConnectionStatusChange:_status longConnStatus:longlink_status];
//    switch (longlink_status) {
//        case mars::stn::kServerFailed:
//        case mars::stn::kServerDown:
//        case mars::stn::kGateWayFailed:
//            break;
//        case mars::stn::kConnecting:
//            break;
//        case mars::stn::kConnected:
//            break;
//        case mars::stn::kNetworkUnkown:
//            return;
//        default:
//            return;
//    }

}

// synccheck：长链成功后由网络组件触发
// 需要组件组包，发送一个req过去，网络成功会有resp，但没有taskend，处理事务时要注意网络时序
// 不需组件组包，使用长链做一个sync，不用重试
// _identify_buffer传入验证的数据, cmdid就是包头的cmdid，buffer_hash用于本地验证
int  StnCallBack_ksim::GetLonglinkIdentifyCheckBuffer(AutoBuffer& _identify_buffer, AutoBuffer& _buffer_hash, int32_t& _cmdid) {
    NSDictionary *dict = [[KSIMMarsNetworkService sharedInstance] getLonglinkIndetifyDict];
    if (dict) {
        NSData *requestData = [NSJSONSerialization dataWithJSONObject:dict options:0 error:nil];
        _cmdid = 9;
        _identify_buffer.AllocWrite(requestData.length);
        _identify_buffer.Write(requestData.bytes,requestData.length);
        return IdentifyMode::kCheckNow;
    }else{
        return IdentifyMode::kCheckNext;
    }
}

// 验证返回检验_response_buffer，返回true表示验证成功
// _identify_buffer_hash其实就是GetLonglinkIdentifyCheckBuffer里_buffer_hash，用于本地自己检验的，并不会发给服务器
bool StnCallBack_ksim::OnLonglinkIdentifyResponse(const AutoBuffer& _response_buffer, const AutoBuffer& _identify_buffer_hash) {
    if (_response_buffer.Length() > 0) {
        NSData* recvData = [NSData dataWithBytes:(const void *) _response_buffer.Ptr() length:_response_buffer.Length()];
        return [[KSIMMarsNetworkService sharedInstance] onLonglinkIdentifyResponse:recvData];
    }
    
    return false;
}
//
void StnCallBack_ksim::RequestSync() {

}
        
        
    }
}
