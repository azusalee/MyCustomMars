//
//  KSMarsNetworkStatus.h
//  mytest
//
//  Created by lizihong on 2020/6/11.
//  Copyright © 2020 AL. All rights reserved.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// @{@"connFlags":@(connFlags)}
static NSNotificationName KSMarsNetworkStatusChangeNotificationName = @"KSMarsNetworkStatusChangeNotificationName";

@interface KSMarsNetworkStatus : NSObject

+ (KSMarsNetworkStatus*)sharedInstance;

- (void)Start;
- (void)Stop;
- (void)ChangeReach;

@end

NS_ASSUME_NONNULL_END
