//#include "DatabaseConnector.h"
//
//DatabaseConnector::DatabaseConnector()
//{
//    // Allocate environment handle
//    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnvHandle)) {
//        std::wcout << L"Error allocating environment handle" << std::endl;
//        throw std::runtime_error("Error allocating environment handle");
//    }
//
//    // Set environment attributes
//    if (SQL_SUCCESS != SQLSetEnvAttr(sqlEnvHandle, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0)) {
//        std::wcout << L"Error setting environment attributes" << std::endl;
//        throw std::runtime_error("Error setting environment attributes");
//    }
//
//    // Allocate connection handle
//    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_DBC, sqlEnvHandle, &sqlConnHandle)) {
//        std::wcout << L"Error allocating connection handle" << std::endl;
//        throw std::runtime_error("Error allocating connection handle");
//    }
//
//    // Connect to SQL Server
//    switch (SQLDriverConnect(sqlConnHandle,
//        NULL,
//        (SQLWCHAR*)L"DRIVER={SQL Server};SERVER=DESKTOP-T3V5KN7\\SQLEXPRESS04,50003;DATABASE=ex;Trusted_Connection=yes;",
//        SQL_NTS,
//        retconstring,
//        1024,
//        NULL,
//        SQL_DRIVER_NOPROMPT)) {
//    case SQL_SUCCESS:
//        break;
//    case SQL_SUCCESS_WITH_INFO:
//        std::wcout << L"Successfully connected to SQL Server" << std::endl;
//        break;
//    default:
//        std::wcout << L"Could not connect to SQL Server" << std::endl;
//        std::wcout << retconstring;
//        throw std::runtime_error("Could not connect to SQL Server");
//    }
//
//    // Allocate statement handle
//    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
//        std::wcout << L"Error allocating statement handle" << std::endl;
//        throw std::runtime_error("Error allocating statement handle");
//    }
//}
//
//void DatabaseConnector::insertData(int ID, const std::wstring& Nume, const std::wstring& Prenume, double Salariu)
//{
//    // Define SQL query with parameters
//    SQLWCHAR* sqlQuery = (SQLWCHAR*)L"INSERT INTO Angajati (ID, Nume, Prenume, Salariu) VALUES (?, ?, ?, ?)";
//
//    // Execute SQL INSERT query
//    if (SQL_SUCCESS != SQLPrepare(sqlStmtHandle, sqlQuery, SQL_NTS)) {
//        std::wcout << L"Error preparing SQL query" << std::endl;
//        throw std::runtime_error("Error preparing SQL query");
//    }
//
//    // Bind parameters
//    SQLLEN cbID = 0, cbNume = SQL_NTS, cbPrenume = SQL_NTS, cbSalariu = 0;
//    SQLBindParameter(sqlStmtHandle, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &ID, 0, &cbID);
//    SQLBindParameter(sqlStmtHandle, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 50, 0, (SQLPOINTER)Nume.c_str(), 0, &cbNume);
//    SQLBindParameter(sqlStmtHandle, 3, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 50, 0, (SQLPOINTER)Prenume.c_str(), 0, &cbPrenume);
//    SQLBindParameter(sqlStmtHandle, 4, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_DECIMAL, 10, 2, &Salariu, 0, &cbSalariu);
//
//    // Execute the statement
//    SQLRETURN retCode = SQLExecute(sqlStmtHandle);
//    if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO) {
//        SQLWCHAR errorMsg[SQL_RETURN_CODE_LEN];
//        SQLSMALLINT msgLength;
//        SQLGetDiagField(SQL_HANDLE_STMT, sqlStmtHandle, 1, SQL_DIAG_MESSAGE_TEXT, errorMsg, SQL_RETURN_CODE_LEN, &msgLength);
//        std::wcout << L"Error executing SQL statement: " << errorMsg << std::endl;
//        throw std::runtime_error("Error executing SQL statement");
//    }
//
//}
