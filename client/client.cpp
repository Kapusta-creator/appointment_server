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

//��������� ����������� � �������
HTTPClientSession s("127.0.0.1", 8000);

int day_secs = 24 * 60 * 60;
int week_days = 7;
std::string client_login = "";
std::string client_password = "";
std::ofstream logging;
std::map<std::string, QString> names_of_weekdays = { {"1", QString::fromLocal8Bit("�����������")}, 
													 {"2", QString::fromLocal8Bit("�������")}, 
	                                                 {"3", QString::fromLocal8Bit("�����")}, 
	                                                 {"4", QString::fromLocal8Bit("�������")}, 
	                                                 {"5", QString::fromLocal8Bit("�������")}, 
	                                                 {"6", QString::fromLocal8Bit("�������")}, 
	                                                 {"7", QString::fromLocal8Bit("�����������")} 
												   };

/** @defgroup support_functions
*	@{
*/
/*!
	������������ ������ � �����
	\param value - �����, ���������� � ������
	\param number - ������ �� int, ���� ���������� �����
*/
void str_to_int(const std::string& value, int& number) {
	std::stringstream converter;
	converter << value;
	converter >> number;
}

/*!
����������� std::string � QString
\param data - std::string, ������� ���������� �������������
\return QString ����� std::string
*/
QString to_qstr(const std::string data) {
	return QString::fromStdString(data);
}

/*!
	��������� ������ �� ������� ������, �������� �����������
	\param s - ������, ������� ���� ���������
	\param delim - �����������
	\return ������ ���������� �����
*/
void split(const std::string& s, const char delim, std::vector<std::string>& elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
}

/*!
	�������� � ���� ����������� �����, ����� ��� ������� ��������� ������
	\param out - ������ �� �����, ���� ����� �������� �������� �������
*/
void print_query_time(std::ofstream& out) {
	LocalDateTime now;
	out << "\n������ " << DateTimeFormatter::format(now, "%d.%m.%y %H:%M:%S") << "\n";
}

/*!
* ���������, ���� �� ������ �������� �� ����� � ������
* \param data - ������, ������� ���������� ���������
* \return true, ���� ���� �������-�� �����, false, ���� ������ ������� �� ����.
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
	qDebug() << QString::fromStdString("������, ����� ������");
	std::cout << "\t��� ������� �������� �� ������������ �������, �������������� �������.\n\
	��� ����������� ������ ���������� ������� ��� �����(.txt) � ������ �������, ����������� � ���������� �������\n\
	������� �������:\n \
	1 - �������� ���� ������\n \
	2 - ��������� ���� ������ ������������ ����������\n \
	3 - �������� ��������\n \
	4 - ������� �� ���� ������\n \
	5 - �������� � ���� ������\n \
	6 - �������� ������ � ���� ������\n \
	7 - ���������� �������� ������\n \
	8 - �������� ���������� ������\n \
	9 - �������� ������ ������\n \
	10 - �������� ������ ���������\n \
	11 - �������� ������ ������� �� ������\n \
	12 - ��������� ����������� ������\n \
	13 - ��������������\n";
}

void cout_instructions_admin() {
	std::cout << "\t\
	������� �������:\n \
	1 - �������� ������ �������\n \
	2 - ��������������\n";
}
*/

/** @defgroup process_requests
*	@{
*/
/*!
* ���������� ������ ������� 
* \param req_data - ������-json - ���� �������
* \param method - ����� �������(/insert, /delete, ..)
* \param login - ����� ������������
* \param password - ������ ������������
* \return json-������ - ����� �������
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
	������� json-������ - ����� ������� ��� sql-������� �� ����� � �������
	\param inp - ������ �� �����, ����������� ���� � �������.
	\return ������� ��� �������� json-������ � ���������
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
	������� ������ json �� ������, ��� ����� ��������� �������.
	\param data - ������, ������� ���� ����������
	\return ����������� �� ���� � ������ ������ json
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
	���������� ������� ������ �� ���������� �������
	\param login - ����� ������ �������
	\param password - ������ ������ ������������
	\return ����� �������
*/
json add_new_client(const std::string& login, const std::string& password) {
	json req, resp;
	req["login"] = login;
	req["pass"] = password;
	resp = send_request(to_string(req), "/new_client", client_login, client_password);
	return resp;
}

/*!
	���������� ������� ������ �� �������� �������
	\param login - ����� �������
	\return ����� �������
*/
json delete_client(const std::string& login) {
	json req, resp;
	req["login"] = login;
	resp = send_request(to_string(req), "/delete_client", client_login, client_password);
	return resp;
}

/*!
    \brief ��������� update ������ �� �����
	�������, ��������� ���������� ������ ��� ������� update, ���������� �� ������������� ������� � ��������� ������ �� ������.
	\param inp - �����, ����������� ���� � ������� �������
	\return ����� �������
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
	\brief ��������� select ������ �� �����
	�������, ��������� ���������� ������ ��� select �������, ���������� �� ������������� ������� � ��������� ������ �� ������.
	\param inp - �����, ����������� ���� � ������� �������
	\return ����� �������
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
	\brief ��������� delete ������ �� �����
	�������, ��������� ���������� ������ ��� ������� delete, ���������� �� ������������� ������� � ��������� ������ �� ������.
	\param inp - �����, ����������� ���� � ������� �������
	\return ����� �������
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
	\brief ��������� insert ������ �� �����
	�������, ��������� ���������� ������ ��� ������� insert, ���������� �� ������������� ������� � ��������� ������ �� ������.
	\param inp - �����, ����������� ���� � ������� �������
	\return ����� �������
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
* ��������������� �������, ��������� ������� ����� � ��������� � ��������� ��� �������.
* \param filename - ��� ����� � ������� � ��������
* \return json-������ � �������� ������� �� ������ ������ �� �����
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
	��������� ������ ������� ������� � ���� ������
	\param table_name - ��� �������
	\return ����� �������
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
	�������� �������, ��������� ��������� ������� ������ � ����
	\return json-������ � ����������� � ������� �������
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
			logging << "������� " << std::string(x["patient_id"]) << " ";
			logging << "������� �� " << std::string(x["begin_time"]) << ' ' << std::string(x["date"]) << " � ������� " << std::string(x["doc_id"]);
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
	��������������� �������, ������������ ��������� ������ ���� �������� ����������� � ����
	\return ������ �������
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
			stream << QString::fromLocal8Bit("�������: ") << "\n";
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
	��������������� �������, ������������ ��������� ������ ���� ��������� ����������� � ����
	\return ������ �������
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
			stream << QString::fromLocal8Bit("��������: ") << "\n";
			
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
	��������������� �������, ������������ ��������� ���������� ���� �������� ����������� � ����
	\return ������ �������
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
		logging << "������ ����������: ���� ������, ������ ������, ����� ������, ������������ ������ ������ ��������\n";
		for (auto& x : schedule) {
			logging << "���������� ������� " << x.first << "\n";
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
	��������������� �������, ������������ ��������� ������ ��������� ������� �� ������ � ����
	\return ������ �������
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
		logging << "������: id, ������ ������, ����� ������, ����, id �������\n";
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
	������� ��������� ��������� ���������������� ������ � ��������� ��������� � ���� � ������
	\param fn - ��� ����� � ���������������� ��������
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
	������� ��������� ������������� ������ � ������������� ����� �� 1 ����.
	\param date - ����
	\param begin - ����� ������ �������� ������
	\param end - ����� ����� �������� ������
	\param max_time - ������������ ������ ������ ��������
	\param doc_id - id �������, ��� ������ ������������
	\return json - ������, ������� � �������� ������� ��� ���������� insert ������� � ������� � ��������
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
	�������, ��������� ������������� ������ �� <days> ���� ������, �� ������������� ��������� �������� days > 30
	\param days - �� ������� ���� ���� ������� �������
	\return ������ ������� �� �������� �������
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
	��������������� �������, ��������� ���� ������ ������������ ������������ ����������
	\param days - �� ������� ���� ���������� ������� �������
	\return ������� �������� �� ���������� ��.
*/
json fill_database(const int days) {
	json resp = parse_func("queries/fill_database.txt");
	resp["talons"] = make_talons(days);
	return resp;
}

/*!
	��������������� �������, ������� ��� ������� � ���� ������ �������
	\return ������� �������� �� ������� ��.
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
				std::cout << "������� �����!\n";
				continue;
			}
			str_to_int(cmd_str, cmd);
			switch (cmd) {
			case 1:
				std::cout << "�������� ������ �������(�������� ������ ������)\n������ ����� � ������ ������ �������\n";
				std::cin >> login >> password;
				std::cout << add_new_client(login, password).dump(2) << "\n";
				break;
			case 2:
				std::cout << "�������������:\n";
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
				std::cout << "������� �����!\n";
				continue;
			}
			str_to_int(cmd_str, cmd);
			
			switch (cmd) {
			case 1:
				std::cout << "����������� ������� ����:\n";
				std::cout << clear_database().dump(2) << "\n";
				break;
			case 2:
				std::cout << "����������� ���������� ����:\n";
				std::cout << "�������, �� ������� ���� ������ ������� ������(�� 0(������ �������) �� 30 ����):\n";
				std::cin >> days_str;
				if (has_alpha(days_str)) {
					std::cout << "������� �����!\n";
					continue;
				}
				str_to_int(days_str, days);
				if (days < 0 || days > 30) {
					std::cout << "����� ������ ���� � ��������� ���������\n";
				}
				else {
					std::cout << fill_database(days).dump(2) << "\n";
				}
				break;
			case 3:
				std::cout << "������ ������: \n";
				std::cout << parse_func("queries/make_appointment_test.txt").dump(2) << "\n";
				break;
			case 4:
				std::cout << "����������� �������� ���������:\n";
				std::cout << parse_func("queries/delete_test.txt").dump(2) << "\n";
				break;
			case 5:
				std::cout << "���������� ��������� � ���� ������: \n";
				std::cout << parse_func("queries/insert_test.txt").dump(2) << "\n";
				break;
			case 6:
				std::cout << "�������� ������ � ��������: \n";
				std::cout << parse_func("queries/update_test.txt").dump(2) << "\n";
				break;
			case 7:
				std::cout << "������ �������� � ����� log.txt\n";
				std::cout << check_talons().dump(2) << "\n";
				break;
			case 8:
				//qDebug() << to_qstr(to_string(print_schedules()));
				std::cout << "���������� ������ �������� � ����� log.txt\n";
				break;
			case 9:
				//������ ������
				print_doctors();
				std::cout << "������ �������� � ����� log.txt\n";
				break;
			case 10:
				qDebug() << QString::fromStdString(to_string(print_cards())).toCaseFolded();
				//qDebug() << to_qstr(to_string(print_cards())).toLocal8Bit();
				std::cout << "������ �������� � ����� log.txt\n";
				break;
			case 11:
				print_talons();
				std::cout << "������ �������� � ����� log.txt\n";
				break;
			case 12:
				std::cout << "������� ��� ����� � ��������\n";
				std::cin >> fn;
				make_user_query(fn);
				break;
			case 13:
				std::cout << "�������������:\n";
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