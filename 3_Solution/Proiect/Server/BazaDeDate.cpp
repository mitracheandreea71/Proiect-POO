#include "BazaDeDate.h"
#include <codecvt>
#include <windows.h>

BazaDeDate* BazaDeDate::instance = nullptr;
SQLHANDLE BazaDeDate::sqlConnHandle = NULL;
SQLHANDLE BazaDeDate::sqlStmtHandle = NULL;
SQLHANDLE BazaDeDate::sqlEnvHandle = NULL;
SQLWCHAR BazaDeDate::retconstring[SQL_RETURN_CODE_LEN];

std::mutex BazaDeDate::db_mutex;

BazaDeDate::BazaDeDate()
{
    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnvHandle)) {
          printf("Error allocating environment handle\n");
          return;
    }

    if (SQL_SUCCESS != SQLSetEnvAttr(sqlEnvHandle, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0)) {
        printf("Error setting environment attributes\n");
        SQLFreeHandle(SQL_HANDLE_ENV, sqlEnvHandle);
        return;
    }

    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_DBC, sqlEnvHandle, &sqlConnHandle)) {
        printf("Error allocating connection handle\n");
        SQLFreeHandle(SQL_HANDLE_ENV, sqlEnvHandle);
        return;
    }

     switch (SQLDriverConnect(sqlConnHandle,
        NULL,
        (SQLWCHAR*)L"DRIVER={SQL Server};SERVER=DESKTOP-T3V5KN7\\SQLEXPRESS04;DATABASE=ex;Trusted_Connection=yes;",
         SQL_NTS,
         retconstring,
         1024,
         NULL,
       SQL_DRIVER_NOPROMPT)) {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
        printf("Successfully connected to SQL Server\n");
        break;
    default:
        printf("Could not connect to SQL Server\n");
        printf("%ls\n", retconstring);
       SQLFreeHandle(SQL_HANDLE_DBC, sqlConnHandle);
       SQLFreeHandle(SQL_HANDLE_ENV, sqlEnvHandle);
        return;
    }
     if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
             printf("Error allocating statement handle\n");
                SQLDisconnect(sqlConnHandle);
              SQLFreeHandle(SQL_HANDLE_DBC, sqlConnHandle);
                SQLFreeHandle(SQL_HANDLE_ENV, sqlEnvHandle);
              return;
     }
}

BazaDeDate& BazaDeDate::getInstance()
{
    if (instance == nullptr) {
        instance = new BazaDeDate;

    }
    return *instance;
}

void BazaDeDate::destroyInstance()
{
    if (instance == nullptr) {
        return;
    }
    delete instance;
    instance = nullptr;
}

void BazaDeDate::insertData(const char* message)
{
    std::lock_guard<std::mutex> lock(db_mutex);  

    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    std::wstring wmessage = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(message);
    SQLWCHAR* sqlQuery = (SQLWCHAR*)L"INSERT INTO tabela (nume) VALUES (?)";

    if (SQL_SUCCESS != SQLPrepare(sqlStmtHandle, sqlQuery, SQL_NTS)) {
        printf("Error preparing SQL query\n");
        return;
    }

    SQLLEN cbNume = SQL_NTS;
    SQLBindParameter(sqlStmtHandle, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 50, 0, (SQLPOINTER)wmessage.c_str(), 0, &cbNume);

    if (SQL_SUCCESS != SQLExecute(sqlStmtHandle)) {
        printf("Error executing SQL statement\n");
        return;
    }

    printf("Successfully inserted data into the database\n");

    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

}

bool BazaDeDate::userExists(const std::string& username) {
    std::lock_guard<std::mutex> lock(db_mutex);  

    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wusername = converter.from_bytes(username);

    SQLWCHAR sqlQuery[SQL_RESULT_LEN];
    swprintf(sqlQuery, SQL_RESULT_LEN, L"SELECT COUNT(*) FROM utilizatori WHERE nume = '%s'", wusername.c_str());

    if (SQL_SUCCESS != SQLExecDirect(sqlStmtHandle, sqlQuery, SQL_NTS)) {
        printf("Eroare la executarea interogarii SQL\n");
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;  
    }

    SQLLEN count = 0;
    if (SQL_SUCCESS != SQLFetch(sqlStmtHandle)) {
        printf("Eroare la preluarea rezultatelor\n");
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    if (SQL_SUCCESS != SQLGetData(sqlStmtHandle, 1, SQL_C_SLONG, &count, 0, NULL)) {
        printf("Eroare la obtinerea datelor\n");
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    printf("Numar de utilizatori gasiti: %ld\n", count); 

    return count > 0; 
}


bool BazaDeDate::addUser(const std::string& username, const std::string& hashedPassword, const std::string& role) {
    std::lock_guard<std::mutex> lock(db_mutex); 

    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    std::wstring wusername = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(username);
    std::wstring whashedPassword = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(hashedPassword);
    std::wstring wrole = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(role);

    SQLWCHAR sqlQuery[SQL_RESULT_LEN];
    swprintf(sqlQuery, SQL_RESULT_LEN, L"INSERT INTO utilizatori (nume, parola, rol) VALUES ('%s', '%s', '%s')", wusername.c_str(), whashedPassword.c_str(), wrole.c_str());


    if (SQL_SUCCESS != SQLExecDirect(sqlStmtHandle, sqlQuery, SQL_NTS)) {
        printf("Eroare la executarea interogarii SQL\n");
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false; 
    }

    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    return true; 
}

bool BazaDeDate::verifyPassword(const std::string& username, const std::string& hashedPassword)
{
    SQLHANDLE sqlStmtHandle;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wusername = converter.from_bytes(username);

    SQLWCHAR sqlQuery[SQL_RESULT_LEN];
    swprintf(sqlQuery, SQL_RESULT_LEN, L"SELECT parola FROM utilizatori WHERE nume = '%s'", wusername.c_str());

    if (SQL_SUCCESS != SQLExecDirect(sqlStmtHandle, sqlQuery, SQL_NTS)) {
        std::cerr << "Eroare la executarea interogarii SQL\n";
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    std::wstring storedHashedPassword;
    SQLLEN indicator;
    wchar_t buffer[SQL_RESULT_LEN];

    if (SQL_SUCCESS != SQLFetch(sqlStmtHandle)) {
        std::cerr << "Niciun utilizator gasit\n";
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false; 
    }

    if (SQL_SUCCESS != SQLGetData(sqlStmtHandle, 1, SQL_C_WCHAR, buffer, sizeof(buffer), &indicator)) {
        std::cerr << "Eroare la obtinerea datelor\n";
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false; 
    }

    storedHashedPassword = buffer;

    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    if (storedHashedPassword == converter.from_bytes(hashedPassword)) {
        std::cout << "Utilizatorul " << username << " a fost gasit cu parola corecta.\n";
        return true; 
    }
    else {
        std::cout << "Parola pentru utilizatorul " << username << " este incorecta.\n";
        return false; 
    }
}

