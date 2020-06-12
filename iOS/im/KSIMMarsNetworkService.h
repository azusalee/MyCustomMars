//
//  KSIMMarsNetworkService.h
//  mytest
//
//  Created by lizihong on 2020/6/11.
//  Copyright © 2020 AL. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "KSMarsProtocol.h"

NS_ASSUME_NONNULL_BEGIN

@class CGITask;
@interface KSIMMarsNetworkService : NSObject <MarsNetworkService>

///ip地址
@property (nonatomic, readonly) NSString *ipString;
///端口号
@property (nonatomic, readonly) NSInteger port;
///mars是否已经准备好
@property (nonatomic, assign, readonly) BOOL isSetup;
///mars是否已验证token
@property (nonatomic, assign, readonly) BOOL isVerifiedToken;

///是否加密
@property (nonatomic, assign) BOOL isEncrypt;
///加密器
@property (nonatomic, strong) id<MarsEncryptor> encryptor;
///是否壓縮(未实现)
//@property (nonatomic, assign) BOOL isCompress;

///代理
@property (nonatomic, weak) id<MarsNetworkServiceDelegate> delegate;

///单例获取
+ (instancetype)sharedInstance;

///初始化
- (void)setup;
- (void)setupWithIp:(NSString*)ipString port:(NSInteger)port clientVersion:(NSInteger)version;
- (void)setCallBack;

///立即嘗試長鏈接
- (void)makesureLongLinkConnect;
///返回是否長連接已鏈接上
- (BOOL)longLinkIsConnected;
///釋放mars，在applicationWillTerminate里調用
- (void)destroyMars;

///连接ip，（如果ip为@""或nil会断开连接）
- (void)connectIP:(NSString *)ipString port:(const unsigned short)port;

/**
 開始請求任務

 @param task 任務對象
 @return 任務的id
 */
- (int)startTask:(CGITask *)task;
/// 停止任务
- (void)stopTask:(NSInteger)taskID;

@end

NS_ASSUME_NONNULL_END
