//
//  KSMarsProtocol.h
//  mytest
//
//  Created by lizihong on 2020/6/11.
//  Copyright © 2020 AL. All rights reserved.
//

#ifndef KSMarsProtocol_h
#define KSMarsProtocol_h

#import <Foundation/Foundation.h>


typedef enum : NSUInteger {
    //其他
    KSMarsNetServiceStatusOther = 0,
    //连接中
    KSMarsNetServiceStatusConnecting = 1,
    //已连接
    KSMarsNetServiceStatusConnected = 2,
} KSMarsNetServiceStatus;

@protocol MarsNetworkService <NSObject>

// callbacks，以下方法为给stn_callback用的方法
///是否已登錄
- (BOOL)isAuthed;
- (NSArray *)OnNewDns:(NSString *)address;
///收到socket通知（服務器端主動推push回調）
- (void)OnPushWithCmd:(NSInteger)cid data:(NSData *)data;
///請求的Data
- (NSData*)Request2BufferWithTaskID:(uint32_t)tid userContext:(const void *)context;
///返回的Data
- (NSInteger)Buffer2ResponseWithTaskID:(uint32_t)tid ResponseData:(NSData *)data userContext:(const void *)context;
///任務結束回調
- (NSInteger)OnTaskEndWithTaskID:(uint32_t)tid userContext:(const void *)context errType:(int)errtype errCode:(int)errcode;
///長連接狀態改變
- (void)OnConnectionStatusChange:(int32_t)status longConnStatus:(int32_t)longConnStatus;
///获取验证的内容(token)，如果没有返回nil
- (NSDictionary*)getLonglinkIndetifyDict;
///服务器返回验证结果
- (BOOL)onLonglinkIdentifyResponse:(NSData *)recvData;

@end

@protocol MarsNetworkServiceDelegate <NSObject>

/// 收到socket push
- (void)marsNetworkService:(id<MarsNetworkService>)service didReceivePushWithCmdid:(NSInteger)cid data:(NSData *)data;

/// socket连接状态边更
- (void)marsNetworkService:(id<MarsNetworkService>)service onConnectionStatusChange:(KSMarsNetServiceStatus)status;

/// 获取验证字典
- (NSDictionary *)marsNetworkServiceGetLonglinkIndetifyDict:(id<MarsNetworkService>)service;

/// 验证回调
- (BOOL)marsNetworkService:(id<MarsNetworkService>)service onLonglinkIdentifyResponse:(NSData *)recvData;

@end

@protocol MarsEncryptor <NSObject>

///加密data，加密失敗返回nil
- (NSData *)marsNetEncrypt:(NSData*)data;
///解密data，解密失敗返回nil
- (NSData *)marsNetDecrypt:(NSData*)data;

- (BOOL)isNeedEncryptCmdId:(uint32_t)cmdid;
- (BOOL)isNeedDecryptCmdId:(uint32_t)cmdid;

@end


#endif /* KSMarsProtocol_h */
