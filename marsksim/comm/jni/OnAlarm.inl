/*
 * OnAlarmksim.cpp
 *
 *  Created on: 2014-1-13
 *      Author: yerungui
 */


#include <jni.h>

extern "C" JNIEXPORT void JNICALL Java_com_tencent_marsksim_comm_Alarmksim_onAlarmksim(JNIEnv *, jclass, jlong id)
{
    xdebug2(TSF"BroadcastMessage seq:%_", (int64_t)id);
    MessageQueueksim::BroadcastMessage(MessageQueueksim::GetDefMessageQueueksim(), MessageQueueksim::Message(KALARM_MESSAGETITLE, (int64_t)id, 0, "KALARM_MESSAGETITLE.id"));
}
