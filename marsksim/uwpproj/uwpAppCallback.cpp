#include "uwpAppCallback.h"
#include "runtime2cs.h"


using namespace marsksim;


std::string uwpAppCallbackksim::GetAppFilePath()
{
	ICallback_Comm^ callback = Runtime2Cs_Comm::Singletonksim()->GetCallBack();

	if (nullptr == callback)
	{
		return std::string();
	}

	Platform::String^ strPath = callback->GetAppFilePath();
	std::wstring wsFilePath(strPath->Data());

	std::string strRetFilePath;
	strRetFilePath.assign(wsFilePath.begin(), wsFilePath.end());
	return strRetFilePath;
}

marsksim::app::AccountInfo uwpAppCallbackksim::GetAccountInfo()
{
	marsksim::app::AccountInfo info;


	ICallback_Comm^ callback = Runtime2Cs_Comm::Singletonksim()->GetCallBack();
	if (nullptr == callback)
	{
		return info;
	}

	AccountInfoRet^ infoPlatfrom = callback->GetAccountInfo();
	info.uin = infoPlatfrom->uin;


	std::wstring wsUsrName(infoPlatfrom->username->Data());
	info.username.assign(wsUsrName.begin(), wsUsrName.end());

	return info;
}

unsigned int uwpAppCallbackksim::GetClientVersion()
{
	ICallback_Comm^ callback = Runtime2Cs_Comm::Singletonksim()->GetCallBack();
	if (nullptr == callback)
	{
		return 0;
	}

	return callback->GetClientVersion();
}

marsksim::app::DeviceInfo uwpAppCallbackksim::GetDeviceInfo()
{
	marsksim::app::DeviceInfo info;


	ICallback_Comm^ callback = Runtime2Cs_Comm::Singletonksim()->GetCallBack();
	if (nullptr == callback)
	{
		return info;
	}

	DeviceInfoRet^ infoPlatfrom = callback->GetDeviceInfo();



	std::wstring wsDeviceName(infoPlatfrom->devicename->Data());
	info.devicename.assign(wsDeviceName.begin(), wsDeviceName.end());


	std::wstring wsDeviceType(infoPlatfrom->devicetype->Data());
	info.devicetype.assign(wsDeviceType.begin(), wsDeviceType.end());

	return info;
}
