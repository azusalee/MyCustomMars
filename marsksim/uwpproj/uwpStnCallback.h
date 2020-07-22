#pragma once
#include "stn/stn_logic.h"

class uwpStnCallbackksim :public marsksim::stn::Callback
{
public:
	virtual ~uwpStnCallbackksim() {}
	virtual bool MakesureAuthed();

	//����ͳ��
	virtual void TrafficData(ssize_t _send, ssize_t _recv);

	//�ײ�ѯ���ϲ��host��Ӧ��ip�б�
	virtual std::vector<std::string> OnNewDns(const std::string& host);
	//������յ�push��Ϣ�ص�
	virtual void OnPush(int32_t cmdid, const AutoBuffer& msgpayload);
	//�ײ��ȡtaskҪ���͵�����
	virtual bool Req2Buf(int32_t taskid, void* const user_context, AutoBuffer& outbuffer, int& error_code, const int channel_select);
	//�ײ�ذ����ظ��ϲ����
	virtual int Buf2Resp(int32_t taskid, void* const user_context, const AutoBuffer& inbuffer, int& error_code, const int channel_select);
	//����ִ�н���
	virtual int  OnTaskEnd(int32_t taskid, void* const user_context, int error_type, int error_code);

	//�ϱ���������״̬
	virtual void ReportConnectStatus(int status, int longlink_status);
	//��������У�� ECHECK_NOW = 0, ECHECK_NEXT = 1, ECHECK_NEVER = 2
	virtual int  GetLonglinkIdentifyCheckBuffer(AutoBuffer& identify_buffer, AutoBuffer& buffer_hash, int32_t& cmdid);
	//��������У��ذ�
	virtual bool OnLonglinkIdentifyResponse(const AutoBuffer& response_buffer, const AutoBuffer& identify_buffer_hash);


	virtual void RequestSync();

	//��֤�Ƿ��ѵ�¼
	virtual bool IsLogoned();
};


