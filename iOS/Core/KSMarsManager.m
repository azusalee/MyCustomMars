//
//  KSMarsManager.m
//  mytest
//
//  Created by lizihong on 2020/6/11.
//  Copyright © 2020 AL. All rights reserved.
//

#import "KSMarsManager.h"

@implementation KSMarsManager

+ (instancetype)sharedInstance
{
    static dispatch_once_t onceToken;
    static id Instance;
    dispatch_once(&onceToken, ^{
        Instance = [[self alloc] init];
    });
    return Instance;
}

/// 收到socket push
- (void)marsNetworkService:(id<MarsNetworkService>)service didReceivePushWithCmdid:(NSInteger)cid data:(NSData *)data{

}

/// socket连接状态边更
- (void)marsNetworkService:(id<MarsNetworkService>)service onConnectionStatusChange:(KSMarsNetServiceStatus)status{

}

/// 获取验证字典
- (NSDictionary *)marsNetworkServiceGetLonglinkIndetifyDict:(id<MarsNetworkService>)service{
    return @{@"token":@"cFE1QWxNYk0xc3RhQkowdGM3ZVE4NE1GUzBUUTFqeUp1SUJ3YmVSSXVvb200bzllbzBGL3ZKOFZqLzhnY1RrakhYcm95dVFhNktDVjcrV1BWN09VTEdzYnA5aEdjQzFROXVWaUV4NjFzdk1vN0xOaXNEdGhESHkzWE1BN05SZFE",
            @"role_id":@(3)
             };
}

/// 验证回调
- (BOOL)marsNetworkService:(id<MarsNetworkService>)service onLonglinkIdentifyResponse:(NSData *)recvData{
    NSDictionary *dict = [NSJSONSerialization JSONObjectWithData:recvData options:0 error:nil];
    NSInteger errorCode = [dict[@"error"] integerValue];
    //error为0表示成功
    if (errorCode == 0) {
        return YES;
    }else{
        return NO;
    }
}

@end
