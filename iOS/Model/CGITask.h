// Tencent is pleased to support the open source community by making Mars available.
// Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.

// Licensed under the MIT License (the "License"); you may not use this file except in 
// compliance with the License. You may obtain a copy of the License at
// http://opensource.org/licenses/MIT

// Unless required by applicable law or agreed to in writing, software distributed under the License is
// distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
// either express or implied. See the License for the specific language governing permissions and
// limitations under the License.

//
//  CGITask.h
//  iOSDemo
//
//  Created by caoshaokun on 16/11/23.
//  Copyright © 2016 Tencent. All rights reserved.
//

#import <Foundation/Foundation.h>

//返回0是成功，-1是失敗，會傳入接收到的body
typedef NSInteger(^CGIResponseBlock)(NSData*data);
//第一個是errtype，第二個是errcode(對應的看stn.h里的ErrCmdType)
typedef void(^CGIEndBlock)(int errorType, int errorCode);

typedef enum : int32_t {
    ChannelType_ShortConn = 1,
    ChannelType_LongConn = 2,
    ChannelType_All = 3
} ChannelType;

@interface CGITask : NSObject

- (id)init;

- (id)initAll:(ChannelType)ChannelType AndCmdId:(uint32_t)cmdId AndCGIUri:(NSString*)cgiUri AndHost:(NSString*)host;

@property(nonatomic) uint32_t taskid;
//必须设置channel_select，如果channel_select为0，会直接走taskend方法
@property(nonatomic) ChannelType channel_select;
//具體有什麼值請看CommandID.h
@property(nonatomic) uint32_t cmdid;
//是否只發送，如果為YES，那麼發送完後便會回調taskend，不需要服務器回復
@property(nonatomic) BOOL sendOnly;
//是否需要登錄（默認NO）
@property(nonatomic, assign) BOOL needAuthed;
@property(nonatomic, copy) NSString *cgi;
@property(nonatomic, copy) NSString *host;
@property(nonatomic, assign) int32_t retryCount;

@property (nonatomic, strong) NSData *requestData;

@property (nonatomic, copy) CGIResponseBlock responseBlock;
@property (nonatomic, copy) CGIEndBlock endBlock;

@end
