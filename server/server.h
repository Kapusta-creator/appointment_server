#pragma once

#include"Poco/Mutex.h"
#include"Poco/Util/ServerApplication.h"
#include"Poco/Data/Session.h"

using namespace Poco;
using namespace Poco::Util;

/*!
	\brief Основной класс сервера
	
	Данный класс реализует прием и отправку запросов от приложения-клиента.
*/
class AppointmentServer : public ServerApplication
{
protected:
	int main(const std::vector<std::string>&);
};