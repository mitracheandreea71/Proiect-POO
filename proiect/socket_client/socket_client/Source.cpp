#include <iostream>
#include <windows.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>

using namespace std;

#define SQL_RESULT_LEN 240
#define SQL_RETURN_CODE_LEN 1024

int main() {
    // Define handles and variables
    SQLHANDLE sqlConnHandle = NULL;
    SQLHANDLE sqlStmtHandle = NULL;
    SQLHANDLE sqlEnvHandle = NULL;
    SQLWCHAR retconstring[SQL_RETURN_CODE_LEN];

    // Allocate environment handle
    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnvHandle)) {
        wcout << L"Error allocating environment handle" << endl;
        return -1;
    }

    // Set environment attributes
    if (SQL_SUCCESS != SQLSetEnvAttr(sqlEnvHandle, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0)) {
        wcout << L"Error setting environment attributes" << endl;
        return -1;
    }

    // Allocate connection handle
    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_DBC, sqlEnvHandle, &sqlConnHandle)) {
        wcout << L"Error allocating connection handle" << endl;
        return -1;
    }

    // Connect to SQL Server
    switch (SQLDriverConnect(sqlConnHandle,
        NULL,
        (SQLWCHAR*)L"DRIVER={SQL Server};SERVER=LAPTOP-4TB37NLA\\MSSQLSERVER04;DATABASE=proiect;Trusted_Connection=yes;",
        SQL_NTS,
        retconstring,
        1024,
        NULL,
        SQL_DRIVER_NOPROMPT)) {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
        wcout << L"Successfully connected to SQL Server" << endl;
        break;

    default:
        wcout << L"Could not connect to SQL Server" << endl;
        wcout << retconstring;
        return -1;
    }

    // Allocate statement handle
    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
        wcout << L"Error allocating statement handle" << endl;
        return -1;
    }

    wstring Nume;
    wcout << L"Enter Nume: ";
    wcin >> Nume;

    // Define SQL query with parameters
    SQLWCHAR* sqlQuery = (SQLWCHAR*)L"INSERT INTO tabela (nume) VALUES (?)";

    // Execute SQL INSERT query
    if (SQL_SUCCESS != SQLPrepare(sqlStmtHandle, sqlQuery, SQL_NTS)) {
        wcout << L"Error preparing SQL query" << endl;
        return -1;
    }

    // Bind parameters
    SQLLEN cbNume = SQL_NTS;
    SQLBindParameter(sqlStmtHandle, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 50, 0, (SQLPOINTER)Nume.c_str(), 0, &cbNume);

    // Execute the statement
    if (SQL_SUCCESS == SQLExecute(sqlStmtHandle)) {
        wcout << L"Successfully executed SQL statement" << endl;
    }
    else {
        wcout << L"Error executing SQL statement" << endl;
    }

    // Free resources
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
    SQLDisconnect(sqlConnHandle);
    SQLFreeHandle(SQL_HANDLE_DBC, sqlConnHandle);
    SQLFreeHandle(SQL_HANDLE_ENV, sqlEnvHandle);

    return 0;
}

