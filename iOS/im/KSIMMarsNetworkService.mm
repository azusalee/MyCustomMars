//
//  KSIMMarsNetworkService.m
//  mytest
//
//  Created by lizihong on 2020/6/11.
//  Copyright © 2020 AL. All rights reserved.
//

#import "KSIMMarsNetworkService.h"
#import "KSMarsNetworkStatus.h"

#import <UIKit/UIKit.h>
#import <SystemConfiguration/SCNetworkReachability.h>

#import "CGITask.h"

#import <marsksim/app/app_logic.h>
#import <marsksim/baseevent/base_logic.h>
#import <marsksim/stn/stn_logic.h>

#import "ksim_stn_callback.h"
#import "ksim_app_callback.h"

namespace marsksim {
namespace stn {

void SetClientVersion(uint32_t _client_version);

}}

using namespace marsksim::stn;

@interface KSIMMarsNetworkService ()
{
    NSMutableDictionary *tasks;
}

///ip地址
@property (nonatomic, strong, readwrite) NSString *ipString;
///端口号
@property (nonatomic, assign, readwrite) NSInteger port;

@end

@implementation KSIMMarsNetworkService

+ (instancetype)sharedInstance
{
    static dispatch_once_t onceToken;
    static id Instance;
    dispatch_once(&onceToken, ^{
        Instance = [[self alloc] init];
    });
    return Instance;
}

- (void)setupWithIp:(NSString*)ipString port:(NSInteger)port clientVersion:(NSInteger)version{
    [self setCallBack];
    [self createMars];
    // 設置版本號，可以判斷發過來的包頭中的版本號，與本地設置版本號是否一致，來判斷包是否有效 （暫時沒有用到，這部分代碼屏蔽了）
    [self setClientVersion:(unsigned int)version];
    [self setLongLinkAddress:ipString port:port];
    [self setShortLinkPort:8080];
    [self reportEvent_OnForeground:YES];
    [self makesureLongLinkConnect];
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(ReachabilityChange:) name:KSMarsNetworkStatusChangeNotificationName object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(appWillForeground:) name:UIApplicationWillEnterForegroundNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(appDidBackground:) name:UIApplicationDidEnterBackgroundNotification object:nil];
    _isSetup = YES;
}

- (void)setup{
    [self setupWithIp:@"" port:0 clientVersion:1];
}

- (instancetype)init{
    if (self = [super init]) {
        tasks = [[NSMutableDictionary alloc] init];
    }
    return self;
}

- (void)appWillForeground:(NSNotification*)notification{
    [self reportEvent_OnForeground:YES];
}

- (void)appDidBackground:(NSNotification*)notification{
    [self reportEvent_OnForeground:NO];
}

- (void)setCallBack {
    marsksim::stn::SetCallback(marsksim::stn::StnCallBack_ksim::Instance());
    marsksim::app::SetCallback(marsksim::app::AppCallBack_ksim::Instance());
}

- (void) createMars {
    marsksim::baseevent::OnCreate();
}

- (void)setClientVersion:(UInt32)clientVersion {
    marsksim::stn::SetClientVersion(clientVersion);
}

- (void)setShortLinkDebugIP:(NSString *)IP port:(const unsigned short)port {
    std::string ipAddress([IP UTF8String]);
    marsksim::stn::SetShortlinkSvrAddr(port, ipAddress);
}

- (void)setShortLinkPort:(const unsigned short)port {
    marsksim::stn::SetShortlinkSvrAddr(port, "");
}

- (void)setLongLinkAddress:(NSString *)string port:(const unsigned short)port debugIP:(NSString *)IP {
    std::string ipAddress([string UTF8String]);
    std::string debugIP([IP UTF8String]);
    std::vector<uint16_t> ports;
    ports.push_back(port);
    marsksim::stn::SetLonglinkSvrAddr(ipAddress,ports,debugIP);
}

- (void)setLongLinkAddress:(NSString *)string port:(const unsigned short)port {
    _ipString = string;
    _port = port;
    std::string ipAddress([string UTF8String]);
    std::vector<uint16_t> ports;
    ports.push_back(port);
    marsksim::stn::SetLonglinkSvrAddr(ipAddress, ports, "");
}

- (void)makesureLongLinkConnect {
    marsksim::stn::MakesureLonglinkConnected();
}

- (BOOL)longLinkIsConnected {
    return marsksim::stn::LongLinkIsConnected();
}

- (void)disConnectLonglink {
    //断开连接，但断开后会自动重新连接(可以通过设置空ip地址来阻止重连)
    marsksim::stn::DisConnectLonglink();
//    [self setLongLinkAddress:@"" port:0];
//    [self setShortLinkPort:8080];
//    _isSetup = NO;
}

- (void)connectIP:(NSString *)ipString port:(const unsigned short)port{
    if (ipString.length > 0) {
        [self setLongLinkAddress:ipString port:port];
        [self makesureLongLinkConnect];
    }else{
        //断开连接，但断开后会自动重新连接(可以通过设置空ip地址来阻止重连)
        marsksim::stn::DisConnectLonglink();
        [self setLongLinkAddress:@"" port:0];
        [self setShortLinkPort:8080];
    }
}

- (void)destroyMars {
    _isSetup = NO;
    marsksim::baseevent::OnDestroy();
}

- (int)startTask:(CGITask *)task{
    if (self.isSetup == NO) {
        //mrs没初始化，不能发消息，直接回调错误block
        if (task.endBlock) {
            task.endBlock(kEctLocal, kEctLocalStartTaskFail);
        }
        return 0;
    }
    Task ctask;
    ctask.cmdid = task.cmdid;
    ctask.channel_select = task.channel_select;
    ctask.send_only = task.sendOnly;
    ctask.need_authed = task.needAuthed;
    ctask.retry_count = task.retryCount;
    ctask.cgi = std::string(task.cgi.UTF8String);
    ctask.shortlink_host_list.push_back(std::string(task.host.UTF8String));
    ctask.user_context = (__bridge void*)task;
    
    marsksim::stn::StartTask(ctask);
    
    NSString *taskIdKey = [NSString stringWithFormat:@"%d", ctask.taskid];
    tasks[taskIdKey] = task;
    
    return ctask.taskid;
}

- (void)stopTask:(NSInteger)taskID {
    marsksim::stn::StopTask((uint32_t)taskID);
}

// event reporting
- (void)reportEvent_OnForeground:(BOOL)isForeground {
    marsksim::baseevent::OnForeground(isForeground);
}

- (void)reportEvent_OnNetworkChange {
    marsksim::baseevent::OnNetworkChange();
}

// callbacks
- (BOOL)isAuthed {
    return _isVerifiedToken;
}

- (NSArray *)OnNewDns:(NSString *)address {
    return NULL;
}

//push回調
- (void)OnPushWithCmd:(NSInteger)cid data:(NSData *)data {
    NSData *resultData = nil;
    if (self.isEncrypt) {
        //解密
        resultData = [self.encryptor marsNetDecrypt:data];
    }else{
        resultData = data;
    }
    
    if (self.delegate) {
        [self.delegate marsNetworkService:self didReceivePushWithCmdid:cid data:resultData];
    }
}

- (NSData*)Request2BufferWithTaskID:(uint32_t)tid userContext:(const void *)context {
    CGITask *task = (__bridge CGITask *)context;
    if (task.requestData) {
        if (self.isEncrypt && [self.encryptor isNeedEncryptCmdId:task.cmdid] == YES) {
            //加密
            return [self.encryptor marsNetEncrypt:task.requestData];
        }else{
            return task.requestData;
        }
    }
    return nil;
}

- (NSInteger)Buffer2ResponseWithTaskID:(uint32_t)tid ResponseData:(NSData *)data userContext:(const void *)context {
    CGITask *task = (__bridge CGITask *)context;
    if (task.responseBlock) {
        if (self.isEncrypt && [self.encryptor isNeedDecryptCmdId:task.cmdid] == YES) {
            //解密
            return task.responseBlock([self.encryptor marsNetDecrypt:data]);
        }else{
            return task.responseBlock(data);
        }
    }
    return -1;
}

- (NSInteger)OnTaskEndWithTaskID:(uint32_t)tid userContext:(const void *)context errType:(int)errtype errCode:(int)errcode {
    CGITask *task = (__bridge CGITask *)context;
    if (task.endBlock) {
        task.endBlock(errtype, errcode);
    }
    
    NSString *taskIdKey = [NSString stringWithFormat:@"%d", tid];
    [tasks removeObjectForKey:taskIdKey];
    
    return 0;
}

- (void)OnConnectionStatusChange:(int32_t)status longConnStatus:(int32_t)longConnStatus {
    KSMarsNetServiceStatus serviceStatus = KSMarsNetServiceStatusOther;
    if (longConnStatus == kConnected) {
        //已連接
        serviceStatus = KSMarsNetServiceStatusConnected;
    }else if (longConnStatus == kConnecting) {
        //连接中
        serviceStatus = KSMarsNetServiceStatusConnecting;
    }
    [self.delegate marsNetworkService:self onConnectionStatusChange:serviceStatus];
}

- (NSDictionary*)getLonglinkIndetifyDict{
    //返回验证的token
    _isVerifiedToken = NO;
    return [self.delegate marsNetworkServiceGetLonglinkIndetifyDict:self];
}

- (BOOL)onLonglinkIdentifyResponse:(NSData *)recvData{
    BOOL flag = [self.delegate marsNetworkService:self onLonglinkIdentifyResponse:recvData];
    if (flag) {
        _isVerifiedToken = YES;
    }else{
        [self disConnectLonglink];
    }
    return flag;
}

#pragma mark NetworkStatusDelegate
-(void) ReachabilityChange:(NSNotification *)notification {
    UInt32 uiFlags = [notification.userInfo[@"connFlags"] unsignedIntValue];
    if ((uiFlags & kSCNetworkReachabilityFlagsConnectionRequired) == 0) {
        marsksim::baseevent::OnNetworkChange();
    }
}

///根据code返回错误文字
- (NSString *)errorStringWithCode:(int)code{
    if (code == kEctLocalTaskTimeout || code == kEctLongFirstPkgTimeout || code == kEctHttpPkgPkgTimeout || code == kEctHttpReadWriteTimeout) {
        return @"超时";
    }
    if (code == kEctLocalTaskRetry) {
        return @"重试";
    }
    if (code == kEctLocalStartTaskFail) {
        return @"失败";
    }
    if (code == kEctLocalAntiAvalanche) {
        return @"雪崩";
    }
    if (code == kEctLocalNoNet) {
        return @"无网络";
    }
    
    return [NSString stringWithFormat:@"%d", code];
}


@end
