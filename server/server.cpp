#include<iostream>
#include<fstream>
#include<string>
#include<sstream>
#include<vector>
#include<map>
#include<conio.h>
#include<algorithm>

#include<nlohmann/json.hpp>

#include<QSql>
#include<QSqlDatabase>
#include<QSqlQuery>
#include<QSqlRecord>
#include<QDebug>
#include<QSqlError>
#include<qvariant.h>
#include<QFile>
#include"Server.h"

#include<Poco/Net/HTTPServerRequest.h>
#include<Poco/Net/HTTPRequestHandler.h>
#include<Poco/Net/HTTPServerResponse.h>
#include<Poco/Net/HTTPServer.h>
#include<Poco/Net/HTTPRequestHandlerFactory.h>
#include<Poco/Net/NameValueCollection.h>

#include<Poco/DateTimeParser.h>
#include<Poco/DateTimeFormat.h>
#include<Poco/Timestamp.h>
#include<Poco/Timespan.h>
#include<Poco/Data/Time.h>
#include<Poco/DateTime.h>

#include<Poco/JSON/Array.h>
#include<Poco/JSON/Parser.h>

#include<Poco/String.h>
#include<Poco/Dynamic/Var.h>


using namespace Poco::JSON;
using namespace Poco::Data;
using namespace Poco::Net;
using namespace Poco::Data::Keywords;
using namespace nlohmann;

// Назначение параметров по-умолчанию
bool session_created = false;
std::string clients_db_name = "clients.db";
std::string log_fn = "log.txt";
std::string admin_password = "1234";
std::ofstream logging;
int port = 8000;

//Map сессий, где ключ - логин пользователя, значение - подключение к базе данных этого пользователя
std::map<QString, QSqlDatabase> sessions;

/** @defgroup support_functions
*	@{
*/

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
std::vector<std::string> split(const std::string& s, char delim) {
	std::stringstream ss(s);
	std::string item;
	std::vector<std::string> elems;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

/*!
	Выполняет логирование запросов
	\param data - json с данными, который надо записать в файл с логами
*/
void write_logs(const json& data) {
	logging.open(log_fn, std::ios::app);
	LocalDateTime now;
	logging << "Запрос в " << DateTimeFormatter::format(now, "%d.%m.%y %H:%M:%S") << ":\n";
	logging << data.dump(2) << "\n";
	logging.close();
}

/*!
	Выставляет параметры сервера из файла config.txt
*/
void set_config() {
	std::ifstream config;
	config.open("config.txt");
	std::string data;
	while (std::getline(config, data)) {
		if (data == "clients_db:") {
			std::getline(config, clients_db_name);
		}
		if (data == "port:") {
			std::getline(config, data);
			std::stringstream converter;
			converter << data;
			converter >> port;
		}
		if (data == "log_fn:") {
			std::getline(config, data);
			log_fn = data;
		}
		if (data == "admin_password:") {
			std::getline(config, data);
			admin_password = data;
		}
	}
}
/** @} */

/*!
* @defgroup parse_functions
* @{
*/
/*!
	Создает умолчательную базу данных для нового клиента.
	\param sql_fn имя файла с sql-скриптом, создающим базу
	\param db_name имя файла - базы данных
*/
void create_new_database(std::string sql_fn, std::string db_name) {
	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "creating_database");
	db.setDatabaseName(to_qstr(db_name));
	db.open();
	QSqlQuery query(db);
	QFile scriptFile(to_qstr(sql_fn));
	if (scriptFile.open(QIODevice::ReadOnly))
	{
		QStringList scriptQueries = QTextStream(&scriptFile).readAll().split(';');
		for(auto& queryTxt : scriptQueries)
		{
			if (queryTxt.trimmed().isEmpty()) {
				continue;
			}
			if (!query.exec(queryTxt))
			{
				qDebug() << QString("One of the query failed to execute. Error detail: " + query.lastError().text()).toLocal8Bit();
			}
			query.finish();
		}
	}
}

/*!
	Составляет select-запрос для проверки наличия пользователя
	\param login - логин пользователя
*/
QString auth_query(const std::string login) {
	return QString::fromStdString("SELECT * FROM clients WHERE login = " + std::string("'") + login + "';");
}

/*!
	Создает строку - набор условий для sql-запроса из принятого json
	Формат json-объекта -> {{"условие", "условие"},{"условие", "условие"}} 
	Условия внутренних массивов соединяются оператором AND, между собой массивы условий соединяются оператором OR
	\param conditions - массив массивов json, содержащий условия.
	\return готовую строку условий, которую можно вставить в sql-запрос
*/
std::string get_conditions(const json& conditions) {
	std::string parsed;
	int i = 0;
	for (auto& and_cond : conditions) {
		parsed += "(";
		int j = 0;
		for (auto& cond : and_cond) {
			parsed += cond;
			if (j + 1 != and_cond.size()) {
				parsed += " AND ";
			}
			else {
				parsed += ")";
			}
			j++;
		}
		if (i + 1 != conditions.size()) {
			parsed += " OR ";
		}
		i++;
	}
	return parsed;
}

/*!
	Создает строку из элементов массива json, разделяя их запятой.
	\param fields массив json
	\return Строку элементов массива json, разделенных пробелами
*/
std::string get_elems(const json& fields) {
	std::string parsed;
	int i = 0;
	for (auto& field : fields) {
		parsed += field;
		if (i + 1 != fields.size()) {
			parsed += ", ";
		}
		i++;
	}
	return parsed;
}
/** @} */

/*!
* @defgroup db_interface
* @{
*/
/*!
	Реализует авторизацию пользователя на сервере
	Если пользователь уже подключался к серверу после старта его работы, проверяет логин и пароль пользователя,
	сохраненный в объекте сессии базы данных клиента
	Если пользователь подключается в первый раз, для проверки сервер обратится к базе данных клиентов
	\param auth_info - информация для авторизации(логин, пароль) в формате ключ - значение
	\return Значение true - автоизация выполнена, false - ошибка авторизации
*/
bool auth(const NameValueCollection& auth_info) {
	json log_data;
	log_data["operation"] = "auth";
	if (auth_info["login"] == "admin" && auth_info["pass"] == admin_password) {
		return true;
	}
	log_data["login"] = auth_info["login"];
	if (sessions.find(to_qstr(auth_info["login"])) != sessions.end()) {
		if (sessions[to_qstr(auth_info["login"])].password() == to_qstr(auth_info["pass"])) {
			log_data["status"] = "ok";
			write_logs(log_data);
			return true;
		}
	}
	else {
		QString query_text = auth_query(auth_info["login"]);
		std::cout << query_text.toStdString();
		QSqlQuery query(query_text, sessions["clients"]);
		query.next();
		QSqlRecord rec = query.record();
		if (to_qstr(auth_info["pass"]) == rec.value(to_qstr("pass")).toString()) {
			QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", to_qstr(auth_info["login"]));
			db.setDatabaseName(rec.value(to_qstr("db_name")).toString());
			db.setPassword(to_qstr(auth_info["pass"]));
			db.open();
			sessions.insert({ to_qstr(auth_info["login"]), db });
			log_data["status"] = "ok";
			write_logs(log_data);
			return true;
		}
		log_data["status"] = "failed";
		write_logs(log_data);
		return false;
	}
}

/*!
	Удаляет строки из базы данных
	\param name - имя таблицы
	\param conditions - массив условий
	\param session - сессия-подключение к базе данных
	\return Результат запроса
*/
json delete_row(const std::string name, const json& conditions, QSqlDatabase& session) {
	json resp, log_data;
	log_data["operation"] = "delete";
	log_data["database_name"] = session.databaseName().toStdString();
	try {
		std::string query_text = "DELETE FROM " + name;
		if (conditions.size() > 0)
			query_text += " WHERE " + get_conditions(conditions);
		query_text += ";";
		qDebug() << to_qstr(query_text);
		log_data["query"] = query_text;
		QSqlQuery query = QSqlQuery(to_qstr(query_text), session);
		qDebug() << query.lastError().text();
		if (!query.lastError().isValid()) {
			resp["status"] = "ok";
			log_data["status"] = "ok";
			write_logs(log_data);
			return resp;
		}
		else {
			resp["status"] = "error: " + query.lastError().text().toStdString();
			log_data["status"] = "failed";
			write_logs(log_data);
			return resp;
		}
	}
	catch (const std::exception& exc) {
		std::cerr << exc.what();
		resp["status"] = "error: " + std::string(exc.what());
		log_data["status"] = "failed";
		write_logs(log_data);
		return resp;
	}
}

/*!
	Обновляет строки в базе данных
	\param name - имя таблицы
	\param fields - массив действий(поле = значение, поле2 = значение2)
	\param conditions - массив условий
	\param session - сессия-подключение к базе данных
	\return Результат запроса
*/
json update(const std::string name, const json& fields, const json& conditions, QSqlDatabase& session) {
	json resp, log_data;
	log_data["operation"] = "update";
	log_data["database_name"] = session.databaseName().toStdString();
	try {
		std::string query_text;
		if(conditions.size() > 0)
			query_text = "UPDATE " + name + " SET " + get_elems(fields) + " WHERE " + get_conditions(conditions) + ";";
		else
		    query_text = "UPDATE " + name + " SET " + get_elems(fields);
		qDebug() << to_qstr(query_text);
		log_data["query"] = query_text;
		QSqlQuery query = QSqlQuery(to_qstr(query_text), session);
		if (query.exec()) {
			log_data["status"] = "ok";
			write_logs(log_data);
			resp["status"] = "ok";
			return resp;
		}
		else {
			log_data["status"] = "failed";
			write_logs(log_data);
			resp["status"] = "error: " + query.lastError().text().toStdString();
			return resp;
		}
	}
	catch (const std::exception& exc) {
		std::cerr << exc.what();
		resp["status"] = "error: " + std::string(exc.what());
		log_data["status"] = "failed";
		write_logs(log_data);
		return resp;
	}
}

/*!
	Получает строки из базы данных
	\param name - имя таблицы
	\param fields - массив полей
	\param conditions - массив условий
	\param group_by - по какому полю группировать
	\param order_by - по какому полю сортировать
	\param limit - сколько записей максимум получить
	\param join - объект json формата {"on" = "", "type" = ""}, type - тип join, on - с какой таблицей соединить
	\param session - сессия-подключение к базе данных
	\return Результат запроса
*/
json select(const std::string name, const json& fields, const json& conditions, const std::string& group_by, const std::string& order_by, const std::string& limit, QSqlDatabase& session, const json& join = {}) {
	json resp, log_data;
	log_data["operation"] = "select";
	log_data["database_name"] = session.databaseName().toStdString();
	try {
		std::string query_text;
		if (conditions.size() > 0)
			query_text = "SELECT " + get_elems(fields) + " FROM " + name + " WHERE " + get_conditions(conditions);
		else
			query_text = "SELECT " + get_elems(fields) + " FROM " + name;
		if (join.size() > 0) {
			for (auto& x : join) {
				for (auto& y : x) {
					query_text += " " + std::string(y["type"]) + " " + std::string(y["on"]) + std::string(" ON ") + get_conditions(y["conditions"]) + " ";
				}
				std::cout << query_text << "\n";
			}
		}
		if (group_by != "") {
			query_text += " GROUP BY " + group_by;
		}
		if (order_by != "") {
			query_text += " ORDER BY " + order_by;
		}
		if (limit != "") {
			query_text += " LIMIT " + limit;
		}
		query_text += ";";
		qDebug() << to_qstr(query_text);
		log_data["query"] = query_text;
		QSqlQuery query = QSqlQuery(to_qstr(query_text), session);
		resp["data"] = json::array();
		QSqlRecord rec;
		while (query.next()) {
			rec = query.record();
			json row;
			for (auto& field : fields) {
				std::string data = field;
				if (data.find("as", 0) != std::string::npos) {
					std::vector<std::string> splitted = split(data, ' ');
					data = splitted[splitted.size() - 1];
				}
				row[data] = rec.value(to_qstr(data)).toString().toStdString();
				
			}
			resp["data"].push_back(row);
		}
		if (query.lastError().isValid()) {
			log_data["status"] = "failed";
			resp["status"] = "error: " + query.lastError().text().toStdString();
		}
		else {
			log_data["status"] = "ok";
			resp["status"] = "ok";
		}
		write_logs(log_data);
		return resp;
	}
	catch (const std::exception& exc) {
		std::cerr << exc.what();
		resp["status"] = "error: " + std::string(exc.what());
		log_data["status"] = "failed";
		write_logs(log_data);
		return resp;
	}
}

/*!
	Вставляет строки в базу данных
	\param name - имя таблицы
	\param fields - массив полей
	\param values - json массив значений, которые надо вставить
	\param session - сессия-подключение к базе данных
	\return Результат запроса
*/
json insert(const std::string name, const json& fields, const json& values, QSqlDatabase& session) {
	json resp, log_data;
	log_data["operation"] = "insert";
	log_data["database_name"] = session.databaseName().toStdString();
	try {
		std::string query_text;
		query_text = "INSERT OR REPLACE INTO " + name + " (" + get_elems(fields) + ")" + " VALUES ";
		int i = 0;
		for (auto& row : values) {
			std::string cur_row = "(" + get_elems(row) + ")";
			query_text += cur_row;
			if (i + 1 != values.size()) {
				query_text += ", ";
			}
			else {
				query_text += ";";
			}
			i++;
		}
		QSqlQuery query = QSqlQuery(to_qstr(query_text), session);
		qDebug() << to_qstr(query_text);
		log_data["query"] = query_text;
		if (!query.lastError().isValid()) {
			log_data["status"] = "ok";
			write_logs(log_data);
			resp["status"] = "ok";
			return resp;
		}
		else {
			log_data["status"] = "failed";
			write_logs(log_data);
			resp["status"] = "error: " + query.lastError().text().toStdString();
			return resp;
		}
	}
	catch (const std::exception& exc) {
		std::cerr << exc.what();
		log_data["status"] = "failed";
		write_logs(log_data);
		resp["status"] = "error: " + std::string(exc.what());
		return resp;
	}
}

/*!
	Удаляет клиента из базы данных
	\param login - логин клиента, которого надо удалить
	\return Результат запроса
*/
json delete_client(const std::string& login) {
	json resp;
	json conditions = { {"login = " + std::string("'") + login + "'"}};
	resp = delete_row("clients", conditions, sessions[to_qstr("clients")]);
	return resp;
}

/*!
	Добавление нового клиента в базу данных
	\param login - логин клиента
	\param pass - пароль клиента
	\return Результат запроса
*/
json add_new_client(const std::string& login, const std::string& pass) {
	json resp;
	std::string query_text = "SELECT login FROM clients where login = '" + login + "';";
	std::cout << query_text << "\n";
	QSqlQuery query(to_qstr(query_text), sessions[to_qstr("clients")]);
	query.next();
	QSqlRecord rec = query.record();
	if (rec.value(to_qstr("login")).toString().toStdString() == login) {
		resp["status"] = "error: login already exists!";
		return resp;
	}
	else {
		json fields = { "login", "db_name", "pass" };
		json values = { {"'" + login + "'", "'" + login + ".db'", "'" + pass + "'"}};
		resp = insert("clients", fields, values, sessions[to_qstr("clients")]);
		create_new_database("create_query.sql", login + ".db");
	}
	return resp;
}
/** @} */

/*!
	\brief Класс - обработчик запросов от приложений - клиентов
*/
class AppRequestHandler : public HTTPRequestHandler
{
public:
	/*!
		Пользуясь ссылками на запрос и ответ получает потоки для передачи и приема данных от клиента, выполняет определенный запрос к базе данных, возвращает ответ. 
		\param req - ссылка на запрос
		\param resp - ссылка на ответ
	*/
	void handleRequest(HTTPServerRequest& req, HTTPServerResponse& resp) {
		NameValueCollection auth_cookie;
		req.getCookies(auth_cookie);
		if (!auth(auth_cookie)) {
			resp.setStatus(HTTPResponse::HTTP_FORBIDDEN);
			resp.setContentType("application/json");
			std::ostream& out = resp.send();
			Object status_obj;
			status_obj.set("status", "forbidden");
			status_obj.stringify(out);
			return;
		}
		else {
			std::istream& in = req.stream();
			std::string data;
			std::getline(in, data);
			in >> data;
			qDebug() << to_qstr(data) << to_qstr(req.getURI());
			json json_data = json::parse(data);
			std::ostream& out = resp.send();
			if (req.getMethod() == HTTPRequest::HTTP_POST) {
				if (req.getURI() == "/insert") {
					json resp = insert(json_data["table_name"], json_data["fields"], json_data["values"], sessions[to_qstr(auth_cookie["login"])]);
					out << to_string(resp);
				}
				if (req.getURI() == "/new_client" && auth_cookie["login"] == "admin") {
					json resp = add_new_client(json_data["login"], json_data["pass"]);
					out << to_string(resp);
				}
				if (req.getURI() == "/delete_client" && auth_cookie["login"] == "admin") {
					json resp = delete_client(json_data["login"]);
					out << to_string(resp);
				}
				if (req.getURI() == "/update") {
					json resp = update(json_data["table_name"], json_data["fields"], json_data["conditions"], sessions[to_qstr(auth_cookie["login"])]);
					out << to_string(resp);
				}
				if (req.getURI() == "/select") {
					std::string order_by = "";
					std::string limit = "";
					std::string group_by = "";
					json join;
					if (json_data.contains("order_by")) {
						order_by = json_data["order_by"];
					}
					if (json_data.contains("limit")) {
						limit = json_data["limit"];
					}
					if (json_data.contains("group_by")) {
						group_by = json_data["group_by"];
					}
					if (json_data.contains("join")) {
						join["join"] = json_data["join"];
						//join["join_conditions"] = json_data["join_conditions"];
					}
					json resp = select(json_data["table_name"], json_data["fields"], json_data["conditions"], group_by, order_by, limit, sessions[to_qstr(auth_cookie["login"])], join);
					out << to_string(resp);
				}
				if (req.getURI() == "/delete") {
					json resp = delete_row(json_data["table_name"], json_data["conditions"], sessions[to_qstr(auth_cookie["login"])]);
					out << to_string(resp);
				}
			}
		}
	}
};

/*!
	\brief Класс - генератор обработчиков
*/
class AppRequestHandlerFactory : public HTTPRequestHandlerFactory
{
public:
	virtual HTTPRequestHandler* createRequestHandler(const HTTPServerRequest&) {
		return new AppRequestHandler;
	}
};

/*!
	\brief Запускает сервер
*/
int AppointmentServer::main(const std::vector<std::string>&) {
	setlocale(LC_ALL, "ru-RU");
	set_config();
	HTTPServer s(new AppRequestHandlerFactory, ServerSocket(port), new HTTPServerParams);
	int k;
	s.start();
	sessions.insert({ to_qstr("clients"), QSqlDatabase::addDatabase("QSQLITE", "admin")});
	sessions[to_qstr("clients")].setDatabaseName(to_qstr(clients_db_name));
	sessions[to_qstr("clients")].open();
	for(;;)
	{
		std::cout << "I'm alive\n";
		_getch();
	}
	s.stop();
	return Application::EXIT_OK;
}