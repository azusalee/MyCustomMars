//
//  KSMarsManager.h
//  mytest
//
//  Created by lizihong on 2020/6/11.
//  Copyright © 2020 AL. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "KSMarsProtocol.h"

NS_ASSUME_NONNULL_BEGIN

@interface KSMarsManager : NSObject <MarsNetworkServiceDelegate>

///单例获取
+ (instancetype)sharedInstance;

@end

NS_ASSUME_NONNULL_END
