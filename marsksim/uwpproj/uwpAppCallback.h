#pragma once
#include "app/app_logic.h"

class uwpAppCallback : public marsksim::app::Callback {

public:
	virtual ~uwpAppCallback() {};

	virtual std::string GetAppFilePath();

	virtual marsksim::app::AccountInfo GetAccountInfo();

	virtual unsigned int GetClientVersion();

	virtual marsksim::app::DeviceInfo GetDeviceInfo();

};


