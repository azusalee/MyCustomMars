//
//  objc_timer.cpp
//  MicroMessenger
//
//  Created by yerungui on 12-12-10.
//

#include "comm/objc/objc_timer.h"
#import <Foundation/Foundation.h>
#include <list>

#include "comm/thread/lock.h"
#include "comm/alarm.h"

void onAlarmksimksim(long long _id)
{
//    Alarmksim::OnAlarmksim(reinterpret_cast<Alarmksim*>(_id));
}

@interface TimerRunerksim : NSObject
-(void) Run:(NSTimer*)timer;
@end

struct TimerRecord
{
    TimerRecord()
    {
        id_ = 0;
        timer = nil;
        timer_runer = nil;
    }
    
    long long id_;
    NSTimer* timer;
    TimerRunerksim* timer_runer;
};

static std::list<TimerRecord> gs_lst_timer_record;
static Mutex gs_mutex;

@implementation TimerRunerksim

-(void) Run:(NSTimer*)timer
{
    ScopedLock lock(gs_mutex);
    for (std::list<TimerRecord>::iterator it = gs_lst_timer_record.begin(); it!=gs_lst_timer_record.end(); ++it)
    {
        if (timer==it->timer)
        {
            if (nil!=it->timer)
            {
                [it->timer invalidate];
                [it->timer release];
                it->timer = nil;
            }
            if (nil!=it->timer_runer)
            {
                [it->timer_runer release];
                it->timer_runer = nil;
            }
            
            long long id_ = it->id_;
            gs_lst_timer_record.erase(it);

            onAlarmksimksim(id_);
            break;
        }
    }
}

@end

bool StartksimAlarmksim(long long _id, int after)
{
    ScopedLock lock(gs_mutex);
    for (std::list<TimerRecord>::iterator it = gs_lst_timer_record.begin(); it!=gs_lst_timer_record.end(); ++it)
    {
        if (_id==it->id_) return false;
    }
    
    NSTimeInterval  interval = (NSTimeInterval)after/1000;
    NSRunLoop *runLoop = [NSRunLoop mainRunLoop];
    TimerRecord tr;
    tr.id_ = _id;
    tr.timer_runer = [[TimerRunerksim alloc] init];
    tr.timer = [NSTimer timerWithTimeInterval:interval target:tr.timer_runer selector:@selector(Run:) userInfo:tr.timer repeats:NO];
    [tr.timer retain];
    [runLoop addTimer:tr.timer forMode:NSDefaultRunLoopMode];
    gs_lst_timer_record.push_back(tr);
    return true;
}

bool StopksimAlarmksim(long long _id)
{
    ScopedLock lock(gs_mutex);
    for (std::list<TimerRecord>::iterator it = gs_lst_timer_record.begin(); it!=gs_lst_timer_record.end(); ++it)
    {
        if (_id==it->id_)
        {
            if (nil!=it->timer)
            {
                [it->timer invalidate];
                [it->timer release];
                it->timer = nil;
            }
            if (nil!=it->timer_runer)
            {
                [it->timer_runer release];
                it->timer_runer = nil;
            }
            gs_lst_timer_record.erase(it);
            return true;
        }
    }
    return false;
}

void comm_export_symbolsksim_0(){}
