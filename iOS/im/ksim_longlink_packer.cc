// Tencent is pleased to support the open source community by making Mars available.
// Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.

// Licensed under the MIT License (the "License"); you may not use this file except in 
// compliance with the License. You may obtain a copy of the License at
// http://opensource.org/licenses/MIT

// Unless required by applicable law or agreed to in writing, software distributed under the License is
// distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
// either express or implied. See the License for the specific language governing permissions and
// limitations under the License.


/*
 * longlink_packer.cc
 *
 *  Created on: 2012-7-18
 *      Author: yerungui, caoshaokun
 */

#include "ksim_longlink_packer.h"

#ifndef WIN32
#include <arpa/inet.h>
#endif // !WIN32

#ifdef __APPLE__
#include "marsksim/xlog/xlogger.h"
#else
#include "marsksim/comm/xlogger/xlogger.h"
#endif
#include "marsksim/comm/autobuffer.h"
#include "marsksim/stn/stn.h"

static uint32_t sg_client_version = 0;

#pragma pack(push, 1)
struct __STNetMsgXpHeader {
    uint32_t    head_length;
    uint32_t    client_version;
    uint32_t    cmdid;
    uint32_t    seq;
    uint32_t	body_length;
};
#pragma pack(pop)

namespace marsksim {
namespace stn {
longlink_tracker* (*longlink_tracker::Create)()
= []() {
    return new longlink_tracker;
};
    
void SetClientVersion(uint32_t _client_version)  {
    sg_client_version = _client_version;
}


static int __unpack_test(const void* _packed, size_t _packed_len, uint32_t& _cmdid, uint32_t& _seq, size_t& _package_len, size_t& _body_len) {
    //包頭的結構體
    __STNetMsgXpHeader st = {0};
    //判斷收到的信息長度是否比定義的包頭大，小於則認為收到的信息不完全，繼續接收
    if (_packed_len < sizeof(__STNetMsgXpHeader)) {
        _package_len = 0;
        _body_len = 0;
        return LONGLINK_UNPACK_CONTINUE;
    }
    
//    long count = _packed_len;
//    uint8_t v[_packed_len];
//    memcpy(&v, _packed, _packed_len);
//    for (int i = 0; i < count; i++) {
//        printf("%02X", *(v+i));
//    }
    

    //_seq就是上層的task_id，另外task_id為PUSH_DATA_TASKID，則認為是push，走的是OnPush的邏輯
    
    //獲取包頭信息，賦值到st
    memcpy(&st, _packed, sizeof(__STNetMsgXpHeader));
    
    //ntohl大端轉小端
    uint32_t head_len = ntohl(st.head_length);
//    uint32_t client_version = ntohl(st.client_version);
//    //判斷version是否一致，不一致則認為無效信息
//    if (client_version != sg_client_version) {
//        _package_len = 0;
//        _body_len = 0;
//        return LONGLINK_UNPACK_FALSE;
//    }
    _cmdid = ntohl(st.cmdid);
    _seq = ntohl(st.seq);
    _body_len = ntohl(st.body_length);
    _package_len = head_len + _body_len;
    
    //打印包头
//    time_t tt = time(NULL);//这句返回的只是一个时间cuo
//    tm* t= localtime(&tt);
//    printf("receivePacket: %d-%02d-%02d %02d:%02d:%02d, headlen:%u, clientVersion:%u, seq:%u, cmdid:%u, body_len:%lu\n",
//           t->tm_year + 1900,
//           t->tm_mon + 1,
//           t->tm_mday,
//           t->tm_hour,
//           t->tm_min,
//           t->tm_sec,
//           head_len,
//           client_version,
//           _seq,
//           _cmdid,
//           _body_len
//           );
//    printf("包头:%X%X%X%X%X\n", head_len, client_version, _seq, _cmdid, _body_len);
    
//    [[NSNotificationCenter defaultCenter] postNotificationName:@"SocketReceivePacket" object:[NSString stringWithFormat:@"收到包：%02X%02X%02X%02X%02X, headlen:%u, clientVersion:%u, seq:%u, cmdid:%u, body_len:%lu\n", head_len, client_version, _seq, _cmdid, _body_len, head_len, client_version, _seq,  _cmdid, _body_len]];

    //判斷包的長度，大於1024*1024認為是無效的包
    if (_package_len > 1024*1024) { return LONGLINK_UNPACK_FALSE; }
    //判斷這次包的信息是否接收完，未接受完則繼續接收
    if (_package_len > _packed_len) { return LONGLINK_UNPACK_CONTINUE; }
    
    return LONGLINK_UNPACK_OK;
}

void (*longlink_pack)(uint32_t _cmdid, uint32_t _seq, const AutoBuffer& _body, const AutoBuffer& _extension, AutoBuffer& _packed, longlink_tracker* _tracker)
= [](uint32_t _cmdid, uint32_t _seq, const AutoBuffer& _body, const AutoBuffer& _extension, AutoBuffer& _packed, longlink_tracker* _tracker) {
    __STNetMsgXpHeader st = {0};
    st.head_length = htonl(sizeof(__STNetMsgXpHeader));
    st.client_version = htonl(sg_client_version);
    st.cmdid = htonl(_cmdid);
    st.seq = htonl(_seq);
    st.body_length = htonl(_body.Length());

    //不做大小端转换
//    st.head_length = sizeof(__STNetMsgXpHeader);
//    st.client_version = sg_client_version;
//    st.cmdid = _cmdid;
//    st.seq = _seq;
//    st.body_length = _body.Length();

    
    _packed.AllocWrite(sizeof(__STNetMsgXpHeader) + _body.Length());
    _packed.Write(&st, sizeof(st));
    
    if (NULL != _body.Ptr()) _packed.Write(_body.Ptr(), _body.Length());
    
    _packed.Seek(0, AutoBuffer::ESeekStart);
};


int (*longlink_unpack)(const AutoBuffer& _packed, uint32_t& _cmdid, uint32_t& _seq, size_t& _package_len, AutoBuffer& _body, AutoBuffer& _extension, longlink_tracker* _tracker)
= [](const AutoBuffer& _packed, uint32_t& _cmdid, uint32_t& _seq, size_t& _package_len, AutoBuffer& _body, AutoBuffer& _extension, longlink_tracker* _tracker) {
   size_t body_len = 0;
   int ret = __unpack_test(_packed.Ptr(), _packed.Length(), _cmdid,  _seq, _package_len, body_len);
    
    if (LONGLINK_UNPACK_OK != ret) return ret;
    
    
    //把接收到的數據寫入body
    _body.Write(AutoBuffer::ESeekCur, _packed.Ptr(_package_len-body_len), body_len);
    //_body.Write(AutoBuffer::ESeekCur, _packed.Ptr(0), _packed.Length());
    
    
    
    return ret;
};


#define NOOP_CMDID 6
#define SIGNALKEEP_CMDID 243
#define PUSH_DATA_TASKID 0

uint32_t (*longlink_noop_cmdid)()
= []() -> uint32_t {
    return NOOP_CMDID;
};

bool  (*longlink_noop_isresp)(uint32_t _taskid, uint32_t _cmdid, uint32_t _recv_seq, const AutoBuffer& _body, const AutoBuffer& _extend)
= [](uint32_t _taskid, uint32_t _cmdid, uint32_t _recv_seq, const AutoBuffer& _body, const AutoBuffer& _extend) {
    return Task::kNoopTaskID == _taskid && NOOP_CMDID == _cmdid;
};

uint32_t (*signal_keep_cmdid)()
= []() -> uint32_t {
    return SIGNALKEEP_CMDID;
};

void (*longlink_noop_req_body)(AutoBuffer& _body, AutoBuffer& _extend)
= [](AutoBuffer& _body, AutoBuffer& _extend) {
    
};
    
void (*longlink_noop_resp_body)(const AutoBuffer& _body, const AutoBuffer& _extend)
= [](const AutoBuffer& _body, const AutoBuffer& _extend) {
    
};

uint32_t (*longlink_noop_interval)()
= []() -> uint32_t {
    //return 30*1000;
	return 0;
};

bool (*longlink_complexconnect_need_verify)()
= []() {
    return false;
};

bool (*longlink_ispush)(uint32_t _cmdid, uint32_t _taskid, const AutoBuffer& _body, const AutoBuffer& _extend)
= [](uint32_t _cmdid, uint32_t _taskid, const AutoBuffer& _body, const AutoBuffer& _extend) {
    return PUSH_DATA_TASKID == _taskid;
};
    
bool (*longlink_identify_isresp)(uint32_t _sent_seq, uint32_t _cmdid, uint32_t _recv_seq, const AutoBuffer& _body, const AutoBuffer& _extend)
= [](uint32_t _sent_seq, uint32_t _cmdid, uint32_t _recv_seq, const AutoBuffer& _body, const AutoBuffer& _extend) {
    return _sent_seq == _recv_seq && 0 != _sent_seq;
};

}
}
