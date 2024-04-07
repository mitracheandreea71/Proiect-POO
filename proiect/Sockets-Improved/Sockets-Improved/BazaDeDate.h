#pragma once
#include <iostream>
#include <windows.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>

using namespace std;

#define SQL_RESULT_LEN 240
#define SQL_RETURN_CODE_LEN 1024

class BazaDeDate
{
private:
	static SQLHANDLE sqlConnHandle;
	static SQLHANDLE sqlStmtHandle ;
	static SQLHANDLE sqlEnvHandle  ;
	static SQLWCHAR retconstring[SQL_RETURN_CODE_LEN];

	BazaDeDate();
	static BazaDeDate* instance;
	~BazaDeDate() {};

public:
	static BazaDeDate&		getInstance();
	static void				destroyInstance();
	void					insertData(int ID, const std::wstring& Nume, const std::wstring& Prenume, double Salariu);
};

