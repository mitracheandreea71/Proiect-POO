#pragma once

#include <iostream>
#include <windows.h>
#include <mutex>
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>


#define SQL_RESULT_LEN 240
#define SQL_RETURN_CODE_LEN 1024

class BazaDeDate
{
private:
	static SQLHANDLE sqlConnHandle;
	static SQLHANDLE sqlStmtHandle;
	static SQLHANDLE sqlEnvHandle;
	static SQLWCHAR retconstring[SQL_RETURN_CODE_LEN];

	static std::mutex db_mutex;

	BazaDeDate();
	static BazaDeDate* instance;
	~BazaDeDate() {};

public:
	static BazaDeDate&		getInstance();
	static void				destroyInstance();
	void					insertData(const char* message);
	bool					userExists(const std::string& username);
	bool					addUser(const std::string& username, const std::string& hashedPassword, const std::string& role);
	bool					verifyPassword(const std::string& username, const std::string& hashedPassword);
};

