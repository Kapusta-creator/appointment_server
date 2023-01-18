#include<Poco/Net/HTTPClientSession.h>
#include<Poco/Net/HTTPServerRequest.h>
#include<Poco/Net/HTTPResponse.h>
#include<Poco/Foundation.h>
#include<Poco/Net/HTTPStreamFactory.h>
#include<Poco/StreamCopier.h>
#include<Poco/Uri.h>
#include<Poco/JSON/Parser.h>
#include<Poco/DateTimeParser.h>
#include<Poco/DateTimeFormat.h>
#include<Poco/Timestamp.h>
#include<Poco/Timespan.h>
#include<Poco/Data/Time.h>
#include<Poco/DateTime.h>
#include<iostream>
#include<fstream>
#include<sstream>
#include<string>
#include<Poco/Net/HTTPCredentials.h>
#include<nlohmann/json.hpp>
#include<QFile>
#include<QDebug>

#include<fcntl.h>
#include<io.h>

using namespace Poco::Net;
using namespace Poco;
using Poco::DateTime;
using Poco::DateTimeParser;
using Poco::DateTimeFormat;
using namespace nlohmann;

//Создается подключение к серверу
HTTPClientSession s("127.0.0.1", 8000);

int day_secs = 24 * 60 * 60;
int week_days = 7;
std::string client_login = "";
std::string client_password = "";
std::ofstream logging;
std::map<std::string, QString> names_of_weekdays = { {"1", QString::fromLocal8Bit("Понедельник")}, 
													 {"2", QString::fromLocal8Bit("Вторник")}, 
	                                                 {"3", QString::fromLocal8Bit("Среда")}, 
	                                                 {"4", QString::fromLocal8Bit("Четверг")}, 
	                                                 {"5", QString::fromLocal8Bit("Пятница")}, 
	                                                 {"6", QString::fromLocal8Bit("Суббота")}, 
	                                                 {"7", QString::fromLocal8Bit("Воскресенье")} 
												   };

/** @defgroup support_functions
*	@{
*/
/*!
	Конвертирует строку в число
	\param value - число, записанное в строке
	\param number - ссылка на int, куда сохранится число
*/
void str_to_int(const std::string& value, int& number) {
	std::stringstream converter;
	converter << value;
	converter >> number;
}

/*!
Преобразует std::string в QString
\param data - std::string, которую необходимо преобразовать
\return QString копию std::string
*/
QString to_qstr(const std::string data) {
	return QString::fromStdString(data);
}

/*!
	Разделяет строку на меньшие строки, учитывая разделитель
	\param s - строка, которую надо разделить
	\param delim - разделитель
	\return Вектор полученных строк
*/
void split(const std::string& s, const char delim, std::vector<std::string>& elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
}

/*!
	Печатает в файл логирования время, когда был получен очередной запрос
	\param out - Ссылка на поток, куда стоит записать значение времени
*/
void print_query_time(std::ofstream& out) {
	LocalDateTime now;
	out << "\nЗапрос " << DateTimeFormatter::format(now, "%d.%m.%y %H:%M:%S") << "\n";
}

/*!
* Проверяет, есть ли символ отличный от цифры в строке
* \param data - строка, которую необходимо проверить
* \return true, если есть символы-не цифры, false, если строка состоит из цифр.
*/
bool has_alpha(const std::string& data) {
	std::string numbers = "1234567890";
	for (int i = 0; i < data.size(); i++) {
		bool flag = true;
		for (int j = 0; j < numbers.size(); j++) {
			if (data[i] == numbers[j]) {
				flag = false;
				break;
			}
		}
		if (flag) {
			return true;
		}
	}
	return false;
}
/** @} */
/*
void cout_instructions() {
	qDebug() << QString::fromStdString("Привет, гнида ебаная");
	std::cout << "\tВсе команды работают со стандартными данными, заготовленными заранее.\n\
	Для собственных тестов необходимо указать имя файла(.txt) с Вашими данными, записанными в правильном формате\n\
	Введите команду:\n \
	1 - Очистить базу данных\n \
	2 - Заполнить базу данных стандартными значениями\n \
	3 - Записать пациента\n \
	4 - Удалить из базы данных\n \
	5 - Добавить в базу данных\n \
	6 - Обновить данные в базе данных\n \
	7 - Посмотреть активные записи\n \
	8 - Получить расписание врачей\n \
	9 - Получить список врачей\n \
	10 - Получить список пациентов\n \
	11 - Получить список талонов на неделю\n \
	12 - Выполнить собственный запрос\n \
	13 - Авторизоваться\n";
}

void cout_instructions_admin() {
	std::cout << "\t\
	Введите команду:\n \
	1 - Добавить нового клиента\n \
	2 - Авторизоваться\n";
}
*/

/** @defgroup process_requests
*	@{
*/
/*!
* Отправляет запрос серверу 
* \param req_data - строка-json - тело запроса
* \param method - метод запроса(/insert, /delete, ..)
* \param login - логин пользователя
* \param password - пароль пользователя
* \return json-объект - ответ сервера
*/
json send_request(const std::string& req_data, const std::string method, const std::string& login, const std::string& password) {
	URI uri(method);
	HTTPRequest request(Poco::Net::HTTPRequest::HTTP_POST, uri.getPathAndQuery());
	NameValueCollection auth_cookie;
	auth_cookie.add("login", login);
	auth_cookie.add("pass", password);
	request.setCookies(auth_cookie);
	std::ostream& os = s.sendRequest(request);
	os << req_data;
	HTTPResponse response;
	std::istream& rs = s.receiveResponse(response);
	std::string data;
	std::getline(rs, data);
	return json::parse(data);
}

/*!
	Создает json-объект - набор условий для sql-запроса из файла с данными
	\param inp - ссылка на поток, открывающий файл с данными.
	\return готовый для отправки json-объект с условиями
*/
json get_conditions(std::ifstream& inp) {
	std::vector<std::string> elems;
	json cond = json::array();
	std::string and_condition;
	json and_cond = json::array();
	while (std::getline(inp, and_condition)) {
		if (and_condition == "end") {
			cond.push_back(and_cond);
			break;
		}
		if (and_condition == "#") {
			cond.push_back(and_cond);
			and_cond = json::array();
		}
		else {
			elems.clear();
			split(and_condition, ',', elems);
			for (int i = 0; i < elems.size(); i++) {
				and_cond.push_back(elems[i]);
			}
		}
	}
	return cond;
}
/*!
	Создает массив json из строки, где слова разделены запятой.
	\param data - строка, которую надо распарсить
	\return построенный из слов в строке массив json
*/
json get_fields(const std::string& data) {
	std::vector<std::string> elems;
	split(data, ',', elems);
	json fields = json::array();
	for (int i = 0; i < elems.size(); i++) {
		fields.push_back(elems[i]);
	}
	return fields;
}
/** @} */

/** @defgroup for_requests
	@{
*/
/*!
	Отправляет серверу запрос на добавление клиента
	\param login - логин нового клиента
	\param password - пароль нового пользователя
	\return ответ сервера
*/
json add_new_client(const std::string& login, const std::string& password) {
	json req, resp;
	req["login"] = login;
	req["pass"] = password;
	resp = send_request(to_string(req), "/new_client", client_login, client_password);
	return resp;
}

/*!
	Отправляет серверу запрос на удаление клиента
	\param login - логин клиента
	\return ответ сервера
*/
json delete_client(const std::string& login) {
	json req, resp;
	req["login"] = login;
	resp = send_request(to_string(req), "/delete_client", client_login, client_password);
	return resp;
}

/*!
    \brief Выполняет update запрос из файла
	Функция, способная распарсить данные для запроса update, записанные по определенному шаблону и отправить запрос на сервер.
	\param inp - Поток, открывающий файл с данными запроса
	\return ответ сервера
*/
json make_update_query(std::ifstream& inp) {
	json resp, req;
	std::string fields, table_name, data;
	std::getline(inp, data);
	while (data != "eoq") {
		if (data == "table_name:") {
			std::getline(inp, table_name);
			req["table_name"] = table_name;
		}
		if (data == "fields:") {
			std::getline(inp, fields);
			req["fields"] = get_fields(fields);
		}
		if (data == "conditions:") {
			req["conditions"] = get_conditions(inp);
		}
		std::getline(inp, data);
	}

	resp = send_request(to_string(req), "/update", client_login, client_password);
	return resp;
}

/*!
	\brief Выполняет select запрос из файла
	Функция, способная распарсить данные для select запроса, записанные по определенному шаблону и отправить запрос на сервер.
	\param inp - Поток, открывающий файл с данными запроса
	\return ответ сервера
*/
json make_select_query(std::ifstream& inp) {
	json resp, req;
	std::string fields, table_name, limit, order_by, group_by, data;
	req["conditions"] = json::array();
	req["join"] = json::array();
	std::getline(inp, data);
	while (data != "eoq") {
		if (data == "table_name:") {
			std::getline(inp, table_name);
			req["table_name"] = table_name;
		}
		if (data == "fields:") {
			std::getline(inp, fields);
			req["fields"] = get_fields(fields);
		}
		if (data == "limit:") {
			std::getline(inp, limit);
			req["limit"] = limit;
		}
		if (data == "order_by:") {
			std::getline(inp, order_by);
			req["order_by"] = order_by;
		}
		if (data == "group_by:") {
			std::getline(inp, group_by);
			req["group_by"] = group_by;
		}
		if (data == "conditions:") {
			req["conditions"] = get_conditions(inp);
		}
		if (data == "join:") {
			json join;
			std::string on, type;
			std::getline(inp, on);
			std::getline(inp, type);
			join["on"] = on;
			join["type"] = type;
			join["conditions"] = get_conditions(inp);
			req["join"].push_back(join);
		}
		std::getline(inp, data);
	}
	resp = send_request(to_string(req), "/select", client_login, client_password);
	return resp;

}


/*!
	\brief Выполняет delete запрос из файла
	Функция, способная распарсить данные для запроса delete, записанные по определенному шаблону и отправить запрос на сервер.
	\param inp - Поток, открывающий файл с данными запроса
	\return ответ сервера
*/
json make_delete_query(std::ifstream& inp) {
	json resp, req;
	std::string fields, table_name, data;
	std::getline(inp, data);
	while (data != "eoq") {
		if (data == "table_name:") {
			std::getline(inp, table_name);
			req["table_name"] = table_name;
		}
		if (data == "conditions:") {
			req["conditions"] = get_conditions(inp);
		}
		std::getline(inp, data);
	}
	resp = send_request(to_string(req), "/delete", client_login, client_password);
	return resp;
}

/*!
	\brief Выполняет insert запрос из файла
	Функция, способная распарсить данные для запроса insert, записанные по определенному шаблону и отправить запрос на сервер.
	\param inp - Поток, открывающий файл с данными запроса
	\return ответ сервера
*/
json make_insert_query(std::ifstream& inp) {
	json resp, req, join;
	std::string fields, table_name, values, data;
	req["values"] = json::array();
	std::getline(inp, data);
	while (data != "eoq") {
		if (data == "table_name:") {
			std::getline(inp, table_name);
			req["table_name"] = table_name;
		}
		if (data == "fields:") {
			std::getline(inp, fields);
			req["fields"] = get_fields(fields);
		}
		if (data == "values:") {
			std::getline(inp, values);
			while (values != "end") {
				std::vector<std::string> elems;
				split(values, ',', elems);
				json elems_json;
				for (auto& x : elems) {
					elems_json.push_back(x);
				}
				req["values"].push_back(elems_json);
				std::getline(inp, values);
			}
		}
		std::getline(inp, data);
	}
	resp = send_request(to_string(req), "/insert", client_login, client_password);
	return resp;
}

/*!
* Вспомогательная функция, способная парсить файлы с запросами и выполнять эти запросы.
* \param filename - имя файла с данными к запросам
* \return json-объект с ответами сервера на каждый запрос из файла
*/
json parse_func(const std::string filename) {
	std::ifstream inp;
	inp.open(filename);
	std::string command;
	json resp;
	while (std::getline(inp, command)) {
		if (command == "insert") {
			resp["insert"].push_back(make_insert_query(inp));
			continue;
		}
		if (command == "select") {
			resp["select"].push_back(make_select_query(inp));
			continue;
		}
		if (command == "delete") {
			resp["delete"].push_back(make_delete_query(inp));
			continue;
		}
		if (command == "update") {
			resp["update"].push_back(make_update_query(inp));
			continue;
		}
	}
	return resp;
}
/** @} */

/** @defgroup for_tests
	@{
*/
/*!
	Выполняет полную очистку таблицы в базе данных
	\param table_name - имя таблицы
	\return ответ сервера
*/
json clear_table(const std::string& table_name) {
	json req, resp;
	req["table_name"] = "sqlite_sequence";
	req["conditions"].push_back(json::array({ "name='" + table_name + "'" }));
	resp["cache"] = send_request(to_string(req), "/delete", client_login, client_password);
	req["table_name"] = table_name;
	req["conditions"] = json::array();
	resp["data"] = send_request(to_string(req), "/delete", client_login, client_password);
	return resp;
}

/*!
	Тестовая функция, позволяет выгрузить занятые талоны в файл
	\return json-объект с инофрмацией о занятых талонах
*/
json check_talons() {
	json resp, req, req_pat, req_doc;
	req["table_name"] = "talons";
	req["fields"] = json::array({ "patient_id", "begin_time", "date", "doc_id"});
	req["conditions"] = json::array();
	req["conditions"].push_back({ "status_id > 0" });
	resp = send_request(to_string(req), "/select", client_login, client_password);
	logging.open("log.txt", std::ios::app);
	print_query_time(logging);
	if (resp["status"] == "ok") {
		std::string row;
		for (auto& x : resp["data"]) {
			logging << "Пациент " << std::string(x["patient_id"]) << " ";
			logging << "Записан на " << std::string(x["begin_time"]) << ' ' << std::string(x["date"]) << " к доктору " << std::string(x["doc_id"]);
			logging << "\n";
		}
	}
	else {
		logging << resp["status"] << "\n";
	}
	logging.close();
	return resp;
}

/*!
	Вспомогательная функция, позволяющаяя выгрузить список всех докторов поликлиники в файл
	\return Статус запроса
*/
json print_doctors() {
	json resp;
	logging.open("log.txt", std::ios::app);
	print_query_time(logging);
	logging.close();
	QFile file("log.txt");
	resp = parse_func("queries/select_doctors_test.txt")["select"][0];
	if (resp["status"] == "ok") {
		if (file.open(QIODevice::Append)) {
			QTextStream stream(&file);
			stream << QString::fromLocal8Bit("Доктора: ") << "\n";
			for (auto& doc : resp["data"]) {
				stream << "id: " << to_qstr(doc["id"]) << ", " << to_qstr(doc["first_name"]) << " " << to_qstr(doc["second_name"]) << " " << to_qstr(doc["branch"]) << " " << to_qstr(doc["type"]) << "\n";
			}
		}
	}
	else {
		logging.open("log.txt", std::ios::app);
		logging << resp["status"] << "\n";
	}
	return resp["status"];
}

/*!
	Вспомогательная функция, позволяющаяя выгрузить список всех пациентов поликлиники в файл
	\return Статус запроса
*/
json print_cards() {
	json resp;
	logging.open("log.txt", std::ios::app);
	print_query_time(logging);
	logging.close();
	QFile file("log.txt");
	resp = parse_func("queries/select_cards_test.txt")["select"][0];
	if (resp["status"] == "ok") {
		if (file.open(QIODevice::Append)) {
			QTextStream stream(&file);
			stream << QString::fromLocal8Bit("Пациенты: ") << "\n";
			
			for (auto& card : resp["data"]) {
				stream << "id: " << to_qstr(card["id"]) << ", " << to_qstr(card["first_name"]) << " " << to_qstr(card["last_name"]) << " " << to_qstr(card["passport"]) << " " << to_qstr(card["phone_number"]) << " " << to_qstr(card["birthdate"]) << "\n";
			}
		}
	}
	else {
		logging.open("log.txt", std::ios::app);
		logging << resp["status"] << "\n";
	}
	return resp["status"];
}

/*!
	Вспомогательная функция, позволяющаяя выгрузить расписание всех докторов поликлиники в файл
	\return Статус запроса
*/
std::string print_schedules() {
	json resp;
	logging.open("log.txt", std::ios::app);
	print_query_time(logging);
	resp = parse_func("queries/select_doc.txt")["select"][0];
	std::string status = resp["status"];
	if (resp["status"] == "ok") {
		resp = resp["data"];
		std::map<std::string, std::vector<std::vector<std::string>>> schedule;
		for (auto& x : resp) {
			std::vector<std::string> day_schedule = { x["weekday"], x["begin_time"], x["end_time"], x["max_time"] };
			schedule[x["id"]].push_back(day_schedule);
		}
		logging << "Формат расписания: день недели, начало приема, конец приема, длительность приема одного пациента\n";
		for (auto& x : schedule) {
			logging << "Расписание доктора " << x.first << "\n";
			for (auto& y : x.second) {
				logging << names_of_weekdays[y[0]].toLocal8Bit().toStdString() << ", " << y[1] << ", " << y[2] << ", " << y[3] << "\n";
			}
		}
	}
	else {
		logging << resp["status"] << "\n";
	}
	logging.close();
	return status;
}

/*!
	Вспомогательная функция, позволяющаяя выгрузить список свободных талонов на неделю в файл
	\return Статус запроса
*/
json print_talons() {
	json resp, req, req_pat, req_doc;
	req["table_name"] = "talons";
	req["fields"] = json::array({ "id", "begin_time", "end_time", "date", "doc_id" });
	req["conditions"] = json::array();
	LocalDateTime now;
	std::string date = DateTimeFormatter::format(now.timestamp() + Timespan(week_days * day_secs, 0), "'%y%m%d'");
	req["conditions"].push_back({ "status_id = 0", "(substr(date,7)||substr(date,4,2)||substr(date,1,2) < " + date + ")"});
	resp = send_request(to_string(req), "/select", client_login, client_password);
	logging.open("log.txt", std::ios::app);
	print_query_time(logging);
	if (resp["status"] == "ok") {
		std::string row;
		logging << "Формат: id, начало приема, конец приема, дата, id доктора\n";
		for (auto& talon : resp["data"]) {
			logging << talon["id"] << " " << talon["begin_time"] << " " << talon["end_time"] << " " << talon["date"] << " " << talon["doc_id"];
			logging << "\n";
		}
	}
	else {
		logging << resp["status"] << "\n";
	}
	logging.close();
	return resp["status"];
}

/*!
	Функция позволяет выполнить пользовательский запрос и выгружает результат в файл с логами
	\param fn - имя файла с пользовательским запросом
*/
void make_user_query(const std::string& fn) {
	json resp;
	resp = parse_func(fn);
	print_query_time(logging);
	logging << resp.dump(2) << "\n";
}
/** @} */
/** @defgroup generation_data 
	@{
*/
/*!
	Функция позволяет сгенерировать талоны к определенному врачу на 1 день.
	\param date - дата
	\param begin - время начала дневного приема
	\param end - время конца дневного приема
	\param max_time - длительность приема одного пациента
	\param doc_id - id доктора, чьи талоны генерируются
	\return json - объект, готовый к отправке серверу для выполнения insert запроса в таблицу с талонами
*/
json make_talon(const DateTime date, const DateTime& begin, const DateTime& end, const DateTime& max_time, const std::string& doc_id) {
	json request;
	request["table_name"] = "talons";
	request["fields"] = json::array({ "begin_time", "end_time", "status_id", "date", "doc_id", "patient_id" });
	request["values"] = json::array();
	for (DateTime st = begin; st < end; st += Timespan(max_time.minute() * 60, 0)) {
		DateTime end_out = st + Timespan(max_time.minute() * 60, 0);
		request["values"].push_back(json::array({ DateTimeFormatter::format(Timespan(st.day(), st.hour(), st.minute(), st.second(), 0), "'%H:%M:%S'"), 
												  DateTimeFormatter::format(Timespan(end_out.day(), end_out.hour(), end_out.minute(), end_out.second(), 0), "'%H:%M:%S'"), 
												  "0", DateTimeFormatter::format(date, "'%d-%m-%y'"), doc_id, "-1" }));
	}
	return request;
}

/*!
	Функция, способная сгенерировать талоны на <days> дней вперед, не рекомендуется указывать параметр days > 30
	\param days - на сколько дней надо сделать талонов
	\return Статус запроса на создание талонов
*/
json make_talons(const int days) {
	json req, resps = json::array();
	req = parse_func("queries/select_doc.txt")["select"][0];
	if (req["status"] == "ok") {
		for (int i = 0; i < req["data"].size(); i++) {	
			DateTime max_time, begin_time, end_time;
			LocalDateTime today, last_day;
			int tzd = 0;
			last_day = DateTime(today.timestamp() + Poco::Timespan(days * 24 * 60 * 60, 0));
			DateTimeParser::parse("%H:%M:%S", req["data"][i]["max_time"], max_time, tzd);
			DateTimeParser::parse("%H:%M:%S", req["data"][i]["begin_time"], begin_time, tzd);
			DateTimeParser::parse("%H:%M:%S", req["data"][i]["end_time"], end_time, tzd);
			
			int weekday;
			std::string value = req["data"][i]["weekday"];
			str_to_int(value, weekday);

			LocalDateTime right_day = LocalDateTime(today);
			if (right_day.dayOfWeek() > weekday % week_days) {
				right_day = DateTime(right_day.timestamp() + Timespan(day_secs * (week_days - right_day.dayOfWeek() + weekday), 0));
			}
			else {
				if (right_day.dayOfWeek() < weekday % week_days) {
					right_day = DateTime(right_day.timestamp() + Timespan((day_secs - 3 * 60 * 60) * (weekday - right_day.dayOfWeek()), 0));
				}
			}

			Timespan week = Timespan(week_days * day_secs, 0);
			std::string doc_id = req["data"][i]["id"];
			for (Timestamp st = right_day.timestamp(); st <= last_day.timestamp(); st += week) {
				resps.push_back(send_request(to_string(make_talon(DateTime(st), begin_time, end_time, max_time, doc_id)), "/insert", client_login, client_password));
			}
		}
	}
	else {
		return req;
	}
	return resps;
}
/** @} */

/** @defgroup put_def_data
	@{	
*/
/*!
	Вспомогательная функция, заполняет базу данных пользователя стандартными значениями
	\param days - на сколько дней необходимо создать талонов
	\return Статусы запросов на заполнение бд.
*/
json fill_database(const int days) {
	json resp = parse_func("queries/fill_database.txt");
	resp["talons"] = make_talons(days);
	return resp;
}

/*!
	Вспомогательная функция, очищает все таблицы в базе данных клиента
	\return Статусы запросов на очистку бд.
*/
json clear_database() {
	json resp;
	resp["delete_talons"] = clear_table("talons");
	resp["delete_cards"] = clear_table("cards");
	resp["delete_doctors"] = clear_table("doctors");
	resp["delete_branches"] = clear_table("branches");
	resp["delete_types"] = clear_table("types");
	resp["delete_schedules"] = clear_table("schedules");
	return resp;
}
/** @} */
int main() {
	AllocConsole();
	freopen("conin$", "r", stdin);
	freopen("conout$", "w", stdout);
	freopen("conout$", "w", stderr);
	setlocale(LC_ALL, "ru-RU");
	std::string fn, login, password, cmd_str, days_str;
	std::string cmd;
	int days = 0;
	bool correct_cmd = false;
	std::cout << "login: ";
	std::cin >> client_login;
	std::cout << "password: ";
	std::cin >> client_password;
	for (;;) {
		std::cout << client_login << ">";
		std::cin >> cmd;
		if (cmd == "clear") {
			std::cout << clear_database().dump(2) << "\n";
			correct_cmd = true;
		}
		if (cmd == "fill") {
			std::cin >> days_str;
			if (has_alpha(days_str)) {
				std::cout << "error: wrong data\n";
				continue;
			}
			str_to_int(days_str, days);
			if (days < 0 || days > 30) {
				std::cout << "error: count should be between 0 and 30\n";
			}
			else {
				std::cout << fill_database(days).dump(2) << "\n";
			}
			correct_cmd = true;
		}
		if (cmd == "make_appointment") {
			std::cout << parse_func("queries/make_appointment_test.txt").dump(2) << "\n";
			correct_cmd = true;
		}
		if (cmd == "delete") {
			std::cout << parse_func("queries/delete_test.txt").dump(2) << "\n";
			correct_cmd = true;
		}
		if (cmd == "update") {
			std::cout << parse_func("queries/update_test.txt").dump(2) << "\n";
			correct_cmd = true;
		}
		if (cmd == "insert") {
			std::cout << parse_func("queries/insert_test.txt").dump(2) << "\n";
			correct_cmd = true;
		}
		if (cmd == "check_appointments") {
			std::cout << check_talons().dump(2) << "\n";
			correct_cmd = true;
		}
		if (cmd == "check_schedules") {
			std::cout << print_schedules() << "\n";
			correct_cmd = true;
		}
		if (cmd == "check_doctors") {
			std::cout << print_doctors() << "\n";
			correct_cmd = true;
		}
		if (cmd == "check_patients") {
			std::cout << print_cards() << "\n";
			correct_cmd = true;
		}
		if (cmd == "check_talons") {
			std::cout << print_talons() << "\n";
			correct_cmd = true;
		}
		if (cmd == "add_client") {
			if (client_login != "admin") {
				std::cout << "forbidden\n";
			}
			else {
				std::cout << "login: ";
				std::cin >> login;
				std::cout << "password: ";
				std::cin >> password;
				std::cout << add_new_client(login, password).dump(2) << "\n";
			}
			correct_cmd = true;
		}
		if (cmd == "make_own_query") {
			std::cin >> fn;
			make_user_query("queries/" + fn);
			correct_cmd = true;
		}
		if (cmd == "auth") {
			std::cout << "login: ";
			std::cin >> client_login;
			std::cout << "password: ";
			std::cin >> client_password;
			correct_cmd = true;
		}
		if (cmd == "delete_client") {
			std::cout << "login: ";
			std::cin >> login;
			std::cout << delete_client(login).dump(2) << "\n";
		}
		if (!correct_cmd) {
			std::cout << "unknown command: " << cmd << "\n";
		}
		correct_cmd = false;
	}
	/*
	for (;;) {
		if (client_login == "admin") {
			std::cin >> cmd_str;
			if (has_alpha(cmd_str)) {
				std::cout << "Введите число!\n";
				continue;
			}
			str_to_int(cmd_str, cmd);
			switch (cmd) {
			case 1:
				std::cout << "Добавить нового клиента(Доступно только админу)\nВедите логин и пароль нового клиента\n";
				std::cin >> login >> password;
				std::cout << add_new_client(login, password).dump(2) << "\n";
				break;
			case 2:
				std::cout << "Авторизуйтесь:\n";
				std::cout << "login: ";
				std::cin >> client_login;
				std::cout << "password: ";
				std::cin >> client_password;
				break;
			}
		}
		else {
			cout_instructions();
			std::cin >> cmd_str;
			if (has_alpha(cmd_str)) {
				std::cout << "Введите число!\n";
				continue;
			}
			str_to_int(cmd_str, cmd);
			
			switch (cmd) {
			case 1:
				std::cout << "Выполняется очистка базы:\n";
				std::cout << clear_database().dump(2) << "\n";
				break;
			case 2:
				std::cout << "Выполняется заполнение базы:\n";
				std::cout << "Введите, на сколько дней вперед создать талоны(от 0(только сегодня) до 30 дней):\n";
				std::cin >> days_str;
				if (has_alpha(days_str)) {
					std::cout << "Введите число!\n";
					continue;
				}
				str_to_int(days_str, days);
				if (days < 0 || days > 30) {
					std::cout << "Число должно быть в указанном диапазоне\n";
				}
				else {
					std::cout << fill_database(days).dump(2) << "\n";
				}
				break;
			case 3:
				std::cout << "Статус записи: \n";
				std::cout << parse_func("queries/make_appointment_test.txt").dump(2) << "\n";
				break;
			case 4:
				std::cout << "Выполняется удаление пациентов:\n";
				std::cout << parse_func("queries/delete_test.txt").dump(2) << "\n";
				break;
			case 5:
				std::cout << "Добавление пациентов в базу данных: \n";
				std::cout << parse_func("queries/insert_test.txt").dump(2) << "\n";
				break;
			case 6:
				std::cout << "Обновить данные о пациенте: \n";
				std::cout << parse_func("queries/update_test.txt").dump(2) << "\n";
				break;
			case 7:
				std::cout << "Данные смотрите в файле log.txt\n";
				std::cout << check_talons().dump(2) << "\n";
				break;
			case 8:
				//qDebug() << to_qstr(to_string(print_schedules()));
				std::cout << "Расписание врачей смотрите в файле log.txt\n";
				break;
			case 9:
				//список врачей
				print_doctors();
				std::cout << "Данные смотрите в файле log.txt\n";
				break;
			case 10:
				qDebug() << QString::fromStdString(to_string(print_cards())).toCaseFolded();
				//qDebug() << to_qstr(to_string(print_cards())).toLocal8Bit();
				std::cout << "Данные смотрите в файле log.txt\n";
				break;
			case 11:
				print_talons();
				std::cout << "Данные смотрите в файле log.txt\n";
				break;
			case 12:
				std::cout << "Введите имя файла с запросом\n";
				std::cin >> fn;
				make_user_query(fn);
				break;
			case 13:
				std::cout << "Авторизуйтесь:\n";
				std::cout << "login: ";
				std::cin >> client_login;
				std::cout << "password: ";
				std::cin >> client_password;
				break;
			}
		}
		
	}
	*/
}