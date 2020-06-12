//
//  KSMarsNetworkStatus.m
//  mytest
//
//  Created by lizihong on 2020/6/11.
//  Copyright © 2020 AL. All rights reserved.
//

#import "KSMarsNetworkStatus.h"
#import <SystemConfiguration/CaptiveNetwork.h>
#import <SystemConfiguration/SCNetworkReachability.h>
#import <netinet/in.h>

SCNetworkReachabilityRef ks_g_Reach;

static void ReachCallback(SCNetworkReachabilityRef target, SCNetworkConnectionFlags flags, void* info)
{
    @autoreleasepool {
        [(__bridge id)info performSelector:@selector(ChangeReach)];
    }
}

@implementation KSMarsNetworkStatus

static KSMarsNetworkStatus * sharedSingleton = nil;

+ (KSMarsNetworkStatus*)sharedInstance {
    @synchronized (self) {
        if (sharedSingleton == nil) {
            sharedSingleton = [[KSMarsNetworkStatus alloc] init];
        }
    }
    return sharedSingleton;
}

-(void)Start {
    if (ks_g_Reach == nil) {
        struct sockaddr_in zeroAddress;
        bzero(&zeroAddress, sizeof(zeroAddress));
        zeroAddress.sin_len = sizeof(zeroAddress);
        zeroAddress.sin_family = AF_INET;
        ks_g_Reach = SCNetworkReachabilityCreateWithAddress(kCFAllocatorDefault, (struct sockaddr *)&zeroAddress);
    }
  
    SCNetworkReachabilityContext context = {0, (__bridge void *)self, NULL, NULL, NULL};
    if(SCNetworkReachabilitySetCallback(ks_g_Reach, ReachCallback, &context)) {
        if(!SCNetworkReachabilityScheduleWithRunLoop(ks_g_Reach, CFRunLoopGetCurrent(), kCFRunLoopCommonModes)) {

            SCNetworkReachabilitySetCallback(ks_g_Reach, NULL, NULL);
            return;
        }
    }
}

-(void)Stop {
    if(ks_g_Reach != nil) {
        SCNetworkReachabilitySetCallback(ks_g_Reach, NULL, NULL);
        SCNetworkReachabilityUnscheduleFromRunLoop(ks_g_Reach, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
        CFRelease(ks_g_Reach);
        ks_g_Reach = nil;
    }
}

-(void)ChangeReach {
    SCNetworkConnectionFlags connFlags;

    if(!SCNetworkReachabilityGetFlags(ks_g_Reach, &connFlags)) {
        return;
    }
   
    [[NSNotificationCenter defaultCenter] postNotificationName:KSMarsNetworkStatusChangeNotificationName object:self userInfo:@{@"connFlags":@(connFlags)}];
}

@end
