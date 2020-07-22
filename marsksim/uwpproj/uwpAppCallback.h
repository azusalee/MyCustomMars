#pragma once
#include "app/app_logic.h"

class uwpAppCallbackksim : public marsksim::app::Callback {

public:
	virtual ~uwpAppCallbackksim() {};

	virtual std::string GetAppFilePath();

	virtual marsksim::app::AccountInfo GetAccountInfo();

	virtual unsigned int GetClientVersion();

	virtual marsksim::app::DeviceInfo GetDeviceInfo();

};


