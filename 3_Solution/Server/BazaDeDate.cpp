#include "BazaDeDate.h"
#include "IDGenerator.h"
#include "LogareActiuni.h"

#include <codecvt>
#include <windows.h>
#include <wchar.h>
#include <sstream>
#include <locale>
#include <string>
#include <map>

std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

BazaDeDate* BazaDeDate::instance = nullptr;
SQLHANDLE BazaDeDate::sqlConnHandle = NULL;
SQLHANDLE BazaDeDate::sqlStmtHandle = NULL;
SQLHANDLE BazaDeDate::sqlEnvHandle = NULL;
SQLWCHAR BazaDeDate::retconstring[SQL_RETURN_CODE_LEN];

std::mutex BazaDeDate::db_mutex; //pentru a proteja resursele partajate pe mai multe fire de executie 

BazaDeDate::BazaDeDate()
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnvHandle)) {
        logger.log("Error allocating environment handle\n");
        printf("Error allocating environment handle\n");
        return;
    }

    if (SQL_SUCCESS != SQLSetEnvAttr(sqlEnvHandle, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0)) {
        logger.log("Error setting environment attributes\n");
        printf("Error setting environment attributes\n");
        SQLFreeHandle(SQL_HANDLE_ENV, sqlEnvHandle);
        return;
    }

    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_DBC, sqlEnvHandle, &sqlConnHandle)) {
        logger.log("Error allocating connection handle\n");
        printf("Error allocating connection handle\n");
        SQLFreeHandle(SQL_HANDLE_ENV, sqlEnvHandle);
        return;
    }

    switch (SQLDriverConnect(sqlConnHandle,
        NULL,
        (SQLWCHAR*)L"DRIVER={SQL Server};SERVER=DESKTOP-T3V5KN7\\SQLEXPRESS04;DATABASE=Sondaje;Trusted_Connection=yes;",
        SQL_NTS,
        retconstring,
        1024,
        NULL,
        SQL_DRIVER_NOPROMPT)) {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
        logger.log("Successfully connected to SQL Server\n");
        printf("Successfully connected to SQL Server\n");
        break;
    default:
        logger.log("Could not connect to SQL Server\n");
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

bool BazaDeDate::getRoleID(const std::wstring& role, int& roleID)
{
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    SQLWCHAR sqlQuery[SQL_RESULT_LEN];
    swprintf(sqlQuery, SQL_RESULT_LEN, L"SELECT ID FROM Rol WHERE Descriere = '%s'", role.c_str());

    if (SQL_SUCCESS != SQLExecDirect(sqlStmtHandle, sqlQuery, SQL_NTS)) {
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    SQLINTEGER id;
    if (SQLFetch(sqlStmtHandle) == SQL_SUCCESS) {
        SQLGetData(sqlStmtHandle, 1, SQL_C_SLONG, &id, sizeof(id), nullptr);
        roleID = id;
        return roleID;
    }
    else {
        return  -1; 
    }

    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
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

    SQLCloseCursor(sqlStmtHandle);

    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
}

std::wstring s2ws(const std::string& str) {
    //converteste stringuri pentru a putea fi introduse in baza de date
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

void BazaDeDate::execute(const std::string& query, const std::vector<std::string>& params)
{
    SQLHSTMT sqlStmtHandle;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    std::wstring wquery = s2ws(query);  
    SQLPrepareW(sqlStmtHandle, (SQLWCHAR*)wquery.c_str(), SQL_NTS);

    std::vector<std::wstring> wparams(params.size());
    for (size_t i = 0; i < params.size(); ++i) {
        wparams[i] = s2ws(params[i]);
    }

    bindParameters(sqlStmtHandle, wparams);

    SQLExecute(sqlStmtHandle);
    SQLCloseCursor(sqlStmtHandle);

    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
}

void BazaDeDate::bindParameters(SQLHSTMT sqlStmtHandle, const std::vector<std::wstring>& params)
{
    for (size_t i = 0; i < params.size(); ++i) {
        SQLBindParameter(sqlStmtHandle, i + 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 0, 0, (SQLPOINTER)params[i].c_str(), 0, NULL);
    }
}

bool BazaDeDate::addRaspuns(int intrebare_id, const std::string& raspuns)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    SQLHANDLE sqlStmtHandle;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wraspuns = converter.from_bytes(raspuns);

    SQLWCHAR sqlQuery[SQL_RESULT_LEN];
    swprintf(sqlQuery, SQL_RESULT_LEN, L"INSERT INTO Raspunsuri (IDintrebare, Text) "
        L"OUTPUT INSERTED.ID " 
        L"VALUES (%d, N'%s')",
        intrebare_id, wraspuns.c_str());

    SQLRETURN ret = SQLExecDirect(sqlStmtHandle, sqlQuery, SQL_NTS);

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        SQLWCHAR sqlState[6], message[SQL_MAX_MESSAGE_LENGTH];
        SQLINTEGER nativeError;
        SQLSMALLINT textLength;
        SQLGetDiagRec(SQL_HANDLE_STMT, sqlStmtHandle, 1, sqlState, &nativeError, message, sizeof(message), &textLength);
        std::wcerr << L"Eroare SQL: " << message << L" SQLState: " << sqlState << L" ErrorCode: " << nativeError << std::endl;
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    SQLINTEGER id_raspuns;
    ret = SQLFetch(sqlStmtHandle); 
    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
        SQLBindCol(sqlStmtHandle, 1, SQL_C_SLONG, &id_raspuns, sizeof(id_raspuns), NULL);
    }
    else {
        SQLWCHAR sqlState[6], message[SQL_MAX_MESSAGE_LENGTH];
        SQLINTEGER nativeError;
        SQLSMALLINT textLength;
        SQLGetDiagRec(SQL_HANDLE_STMT, sqlStmtHandle, 1, sqlState, &nativeError, message, sizeof(message), &textLength);
        std::wcerr << L"Eroare SQL: " << message << L" SQLState: " << sqlState << L" ErrorCode: " << nativeError << std::endl;
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    if (SQLFetch(sqlStmtHandle) != SQL_SUCCESS) {
        SQLWCHAR sqlState[6], message[SQL_MAX_MESSAGE_LENGTH];
        SQLINTEGER nativeError;
        SQLSMALLINT textLength;
        SQLGetDiagRec(SQL_HANDLE_STMT, sqlStmtHandle, 1, sqlState, &nativeError, message, sizeof(message), &textLength);
        std::wcerr << L"Eroare SQL: " << message << L" SQLState: " << sqlState << L" ErrorCode: " << nativeError << std::endl;
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
    return true;
}

bool BazaDeDate::addFormToUser(int form_id, int user_id)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    SQLHANDLE sqlStmtHandle;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    SQLWCHAR sqlQuery[SQL_RESULT_LEN];
    swprintf(sqlQuery, SQL_RESULT_LEN, L"INSERT INTO Formulare_Distribuite (IDformular, IDutilizator) VALUES (%d, %d)", form_id, user_id);

    SQLRETURN ret = SQLExecDirect(sqlStmtHandle, sqlQuery, SQL_NTS);

    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    return (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO);
}

bool BazaDeDate::isFormTitleUnique(const std::string& form_title)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    SQLHSTMT sqlStmtHandle;
    SQLRETURN retcode;

    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
        logger.log("Error allocating statement handle\n");
        printf("Error allocating statement handle\n");
        return false; 
    }

    SQLWCHAR sql_query[255];
    swprintf(sql_query, 255, L"SELECT COUNT(*) FROM Formulare WHERE Titlu = '%S'", form_title.c_str());
    retcode = SQLExecDirect(sqlStmtHandle, sql_query, SQL_NTS);
    if (retcode == SQL_SUCCESS) {
        SQLINTEGER count;
        SQLLEN indicator;
        retcode = SQLBindCol(sqlStmtHandle, 1, SQL_C_SLONG, &count, sizeof(count), &indicator);
        retcode = SQLFetch(sqlStmtHandle);
        if (retcode == SQL_SUCCESS) {
            SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
            return (count == 0);
        }
    }

    SQLCloseCursor(sqlStmtHandle);

    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
    return false;
}

bool BazaDeDate::addUserResponse(int id_utilizator, int id_intrebare, int id_raspuns, int id_formular)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    SQLHSTMT sqlStmtHandle;
    SQLRETURN retcode;

    SQLWCHAR sql_query[255];
    swprintf(sql_query, 255, L"SELECT ID FROM Raspunsuri_Completate WHERE IDraspuns = %d", id_raspuns);

    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
        logger.log("Error allocating statement handle\n");
        printf("Error allocating statement handle\n");
        return false;
    }

    retcode = SQLExecDirect(sqlStmtHandle, sql_query, SQL_NTS);
    bool response_exists = false;

    if (retcode == SQL_SUCCESS && SQLFetch(sqlStmtHandle) == SQL_SUCCESS) {
        response_exists = true;
    }

    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    if (!response_exists) {
        swprintf(sql_query, 255, L"INSERT INTO Raspunsuri_Completate (IDraspuns, Contor) VALUES (%d, 1)", id_raspuns);
        if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
            logger.log("Error allocating statement handle\n");
            printf("Error allocating statement handle\n");
            return false;
        }
        retcode = SQLExecDirect(sqlStmtHandle, sql_query, SQL_NTS);
        if (retcode != SQL_SUCCESS) {
            logger.log("Error executing SQL query\n");
            printf("Error executing SQL query\n");
            SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
            return false;
        }
    }
    else {
        swprintf(sql_query, 255, L"UPDATE Raspunsuri_Completate SET Contor = Contor + 1 WHERE IDraspuns = %d", id_raspuns);
        if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
            printf("Error allocating statement handle\n");
            return false;
        }
        retcode = SQLExecDirect(sqlStmtHandle, sql_query, SQL_NTS);
        if (retcode != SQL_SUCCESS) {
            printf("Error executing SQL query\n");
            SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
            return false;
        }
    }

    swprintf(sql_query, 255, L"INSERT INTO Raspunsuri_User (IDintrebare, IDraspuns, IDutilizator, IdFormular) VALUES (%d, %d, %d, %d)", id_intrebare, id_raspuns, id_utilizator, id_formular);
    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
        printf("Error allocating statement handle\n");
        return false;
    }
    retcode = SQLExecDirect(sqlStmtHandle, sql_query, SQL_NTS);
    if (retcode != SQL_SUCCESS) {
        printf("Error executing SQL query\n");
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }
    SQLCloseCursor(sqlStmtHandle);

    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
    return true;
}

bool BazaDeDate::setFormCompleted(int id_formular, int id_utilizator)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    SQLHSTMT sqlStmtHandle;
    SQLRETURN retcode;

    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
        printf("Error allocating statement handle\n");
        return false;
    }

    const wchar_t* sql_query = L"UPDATE Formulare_Distribuite SET Completat = 1 WHERE IDformular = ? AND IDutilizator = ?";
    retcode = SQLPrepareW(sqlStmtHandle, (SQLWCHAR*)sql_query, SQL_NTS);
    if (retcode != SQL_SUCCESS) {
        printf("Error preparing SQL query\n");
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    retcode = SQLBindParameter(sqlStmtHandle, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &id_formular, 0, NULL);
    if (retcode != SQL_SUCCESS) {
        logger.log("Error binding parameter 1\n");
        printf("Error binding parameter 1\n");
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    retcode = SQLBindParameter(sqlStmtHandle, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &id_utilizator, 0, NULL);
    if (retcode != SQL_SUCCESS) {
        logger.log("Error binding parameter 2\n");
        printf("Error binding parameter 2\n");
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    retcode = SQLExecute(sqlStmtHandle);
    if (retcode != SQL_SUCCESS) {
        printf("Error executing SQL query\n");
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }
    SQLCloseCursor(sqlStmtHandle);

    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
    return true; 
}

void printSQLError(SQLHANDLE handle, SQLSMALLINT type) {
    SQLINTEGER i = 0;
    SQLINTEGER native;
    SQLWCHAR state[7];
    SQLWCHAR text[256];
    SQLSMALLINT len;
    SQLRETURN ret;
    while ((ret = SQLGetDiagRec(type, handle, ++i, state, &native, text, sizeof(text), &len)) != SQL_NO_DATA) {
        wprintf(L"Message %d: %s, SQLSTATE: %s\n", i, text, state);
    }
}

bool BazaDeDate::hasUserCompletedForm(int id_utilizator, int id_formular)
{
    SQLHSTMT sqlStmtHandle;
    SQLRETURN retcode;

    SQLWCHAR sql_query[255];
    swprintf(sql_query, 255, L"SELECT Completat FROM Formulare_Distribuite WHERE IDutilizator = %d AND IDformular = %d", id_utilizator, id_formular);

    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
        printf("Error allocating statement handle\n");
        return false;
    }

    retcode = SQLExecDirect(sqlStmtHandle, sql_query, SQL_NTS);
    if (retcode != SQL_SUCCESS) {
        printf("Error executing SQL query\n");

        printSQLError(sqlStmtHandle, SQL_HANDLE_STMT);
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    bool completed = false;
    SQLINTEGER completat;
    if (SQLFetch(sqlStmtHandle) == SQL_SUCCESS) {
        SQLGetData(sqlStmtHandle, 1, SQL_C_LONG, &completat, sizeof(completat), NULL);
        if (completat == 1) {
            completed = true;
        }
    }

    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
    return completed;
}

bool BazaDeDate::updateFormType(int form_id, const std::string& new_type)
{
    SQLHSTMT sqlStmtHandle;
    SQLRETURN retcode;

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wnew_type = converter.from_bytes(new_type);
    SQLWCHAR sql_query[255];
    swprintf(sql_query, 255, L"UPDATE Formulare SET TipFormular = '%ls' WHERE ID = %d", wnew_type.c_str(), form_id);

    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
        printf("Error allocating statement handle\n");
        return false;
    }

    retcode = SQLExecDirect(sqlStmtHandle, sql_query, SQL_NTS);
    if (retcode != SQL_SUCCESS) {
        printf("Error executing SQL query\n");
        printSQLError(sqlStmtHandle, SQL_HANDLE_STMT);
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
    return true;
}

bool BazaDeDate::setArchiveRequestStatus(int form_id, int status)
{
    SQLHSTMT sqlStmtHandle;
    SQLRETURN retcode;

    SQLWCHAR sql_query[255];
    swprintf(sql_query, 255, L"UPDATE CererideArhivare SET Arhivat = %d WHERE IDformular = %d", status, form_id);

    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
        printf("Error allocating statement handle\n");
        return false;
    }

    retcode = SQLExecDirect(sqlStmtHandle, sql_query, SQL_NTS);

    if (retcode != SQL_SUCCESS) {
        printf("Error executing SQL query\n");
        printSQLError(sqlStmtHandle, SQL_HANDLE_STMT);
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
    return true;
}

std::pair<int, int> BazaDeDate::getIDs(const std::string& titlu) {
    std::pair<int, int> ids = std::make_pair(-1, -1);  

    SQLHSTMT sqlStmtHandle;
    SQLRETURN ret;

    ret = SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);
    if (ret != SQL_SUCCESS) {
        std::wcerr << L"Eroare la alocarea pentru declaratie." << std::endl;
        return ids;
    }

    std::wstring wquery = L"SELECT ID, IDmanager FROM Formulare WHERE Titlu = ?";
    ret = SQLPrepareW(sqlStmtHandle, (SQLWCHAR*)wquery.c_str(), SQL_NTS);
    if (ret != SQL_SUCCESS) {
        std::wcerr << L"Eroare la pregatirea interogarii SQL." << std::endl;
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return ids;
    }

    std::wstring wtitlu = s2ws(titlu);
    ret = SQLBindParameter(sqlStmtHandle, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, wtitlu.length(), 0, (SQLPOINTER)wtitlu.c_str(), 0, NULL);
    if (ret != SQL_SUCCESS) {
        std::wcerr << L"Eroare la legarea parametrului." << std::endl;
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return ids;
    }

    ret = SQLExecute(sqlStmtHandle);
    if (ret != SQL_SUCCESS) {
        std::wcerr << L"Eroare la executarea interogarii SQL." << std::endl;
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return ids;
    }

    SQLINTEGER idFormular, idManager;
    ret = SQLFetch(sqlStmtHandle);
    if (ret == SQL_SUCCESS) {
        SQLGetData(sqlStmtHandle, 1, SQL_C_SLONG, &idFormular, sizeof(idFormular), nullptr);
        SQLGetData(sqlStmtHandle, 2, SQL_C_SLONG, &idManager, sizeof(idManager), nullptr);
        ids = std::make_pair(idFormular, idManager);
    }

    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    return ids;
}

bool BazaDeDate::archive(const std::string& titlu)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    SQLHSTMT sqlStmtHandle;
    SQLRETURN retcode;

    retcode = SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        std::cout << "Error allocating statement handle" << std::endl;
        return false;
    }

    std::wstring wtitlu = s2ws(titlu);

    std::wstring checkQuery = L"SELECT Arhivat FROM CererideArhivare WHERE IDformular = (SELECT ID FROM Formulare WHERE Titlu = ?)";
    retcode = SQLPrepare(sqlStmtHandle, (SQLWCHAR*)checkQuery.c_str(), SQL_NTS);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        std::cout << "Error preparing SQL query for checking if the survey is already archived" << std::endl;
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    retcode = SQLBindParameter(sqlStmtHandle, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, wtitlu.length(), 0, (SQLPOINTER)wtitlu.c_str(), 0, NULL);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        std::cout << "Error binding parameter for the check query" << std::endl;
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    retcode = SQLExecute(sqlStmtHandle);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        std::cout << "Error executing SQL query for checking if the survey is already archived" << std::endl;
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    SQLINTEGER arhivat;
    if (SQLFetch(sqlStmtHandle) == SQL_SUCCESS) {
        SQLGetData(sqlStmtHandle, 1, SQL_C_SLONG, &arhivat, sizeof(arhivat), nullptr);
        if (arhivat == 1) {
            logger.log("Sondajul este deja arhivat");
            std::cout << "Sondajul este deja arhivat" << std::endl;
            SQLCloseCursor(sqlStmtHandle);
            SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
            return false;
        }
    }

    SQLCloseCursor(sqlStmtHandle);

    std::wstring insertQuery = L"INSERT INTO CererideArhivare (IDformular, IDmanager, DataCerere, Arhivat) VALUES (?, ?, GETDATE(), 0)";
    std::wcout << L"Query: " << insertQuery << std::endl;

    retcode = SQLPrepare(sqlStmtHandle, (SQLWCHAR*)insertQuery.c_str(), SQL_NTS);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        std::cout << "Error preparing SQL query for inserting into CererideArhivare table" << std::endl;
        printSQLError(sqlStmtHandle, SQL_HANDLE_STMT);
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    std::pair<int, int> ids = getIDs(titlu);
    SQLINTEGER idFormular = ids.first;
    SQLINTEGER idManager = ids.second;
    std::wcout << L"IDformular: " << idFormular << L", IDmanager: " << idManager << std::endl;

    retcode = SQLBindParameter(sqlStmtHandle, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &idFormular, 0, NULL);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        std::cout << "Error binding parameter 1 for inserting into CererideArhivare table" << std::endl;
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }
    retcode = SQLBindParameter(sqlStmtHandle, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &idManager, 0, NULL);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        std::cout << "Error binding parameter 2 for inserting into CererideArhivare table" << std::endl;
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    retcode = SQLExecute(sqlStmtHandle);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        std::cout << "Error executing SQL query for inserting into CererideArhivare table" << std::endl;
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }
    SQLCloseCursor(sqlStmtHandle);

    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
    return true;
}

bool BazaDeDate::addDeleteRequest(int form_id, int user_id)
{
    SQLHSTMT sqlStmtHandle;
    SQLRETURN retcode;

    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
        printf("Error allocating statement handle\n");
        return false;
    }

    SQLWCHAR sql_query[255];
    swprintf(sql_query, 255, L"INSERT INTO CererideStergere (IDformular, IDmanager) VALUES (%d, %d)", form_id, user_id);

    retcode = SQLExecDirect(sqlStmtHandle, sql_query, SQL_NTS);

    if (retcode != SQL_SUCCESS) {
        printf("Error executing SQL query\n");
        printSQLError(sqlStmtHandle, SQL_HANDLE_STMT);
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
    return true;
}

bool BazaDeDate::approveDeleteRequest(int form_id)
{
    SQLHSTMT sqlStmtHandle;
    SQLRETURN retcode;

    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
        printf("Error allocating statement handle\n");
        return false;
    }

    SQLWCHAR sql_query[255];
    swprintf(sql_query, 255, L"UPDATE CererideStergere SET Sters = 1 WHERE IDformular = %d", form_id);

    retcode = SQLExecDirect(sqlStmtHandle, sql_query, SQL_NTS);

    if (retcode != SQL_SUCCESS) {
        printf("Error executing SQL query\n");
        printSQLError(sqlStmtHandle, SQL_HANDLE_STMT);
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    SQLCloseCursor(sqlStmtHandle);

    swprintf(sql_query, 255, L"UPDATE Formulare SET TipFormular = 'sters' WHERE ID = %d", form_id);

    retcode = SQLExecDirect(sqlStmtHandle, sql_query, SQL_NTS);

    if (retcode != SQL_SUCCESS) {
        printf("Error executing SQL query\n");
        printSQLError(sqlStmtHandle, SQL_HANDLE_STMT);
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    return true;
}

bool BazaDeDate::deleteRequestExists(int form_id)
{
    SQLHSTMT sqlStmtHandle;
    SQLRETURN retcode;

    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
        printf("Error allocating statement handle\n");
        return false;
    }

    SQLWCHAR sql_query[255];
    swprintf(sql_query, 255, L"SELECT COUNT(*) FROM CererideStergere WHERE IDformular = %d AND Sters = 0", form_id);

    retcode = SQLExecDirect(sqlStmtHandle, sql_query, SQL_NTS);

    if (retcode != SQL_SUCCESS) {
        printf("Error executing SQL query\n");
        printSQLError(sqlStmtHandle, SQL_HANDLE_STMT);
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    SQLLEN count = 0;
    SQLBindCol(sqlStmtHandle, 1, SQL_C_SLONG, &count, 0, NULL);
    retcode = SQLFetch(sqlStmtHandle);

    bool exists = (count > 0);

    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    return exists;
}

bool BazaDeDate::isFormArchived(int form_id)
{
    SQLHSTMT sqlStmtHandle;
    SQLRETURN retcode;
    bool isArchived = false;

    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
        printf("Error allocating statement handle\n");
        return false;
    }

    SQLWCHAR sql_query[255];
    swprintf(sql_query, 255, L"SELECT Arhivat FROM CererideArhivare WHERE IDformular = %d", form_id);

    retcode = SQLExecDirect(sqlStmtHandle, sql_query, SQL_NTS);
    if (retcode == SQL_SUCCESS) {
        SQLINTEGER arhivat;
        SQLLEN indicator;
        if (SQLFetch(sqlStmtHandle) == SQL_SUCCESS) {
            SQLGetData(sqlStmtHandle, 1, SQL_C_LONG, &arhivat, 0, &indicator);
            if (indicator != SQL_NULL_DATA && arhivat == 1) {
                isArchived = true;
            }
        }
    }
    else {
        printf("Error executing SQL query\n");
        printSQLError(sqlStmtHandle, SQL_HANDLE_STMT);
    }

    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
    return isArchived;
}

bool BazaDeDate::recoverForm(int form_id)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    SQLHSTMT sqlStmtHandle;
    SQLRETURN retcode;

    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
        printf("Error allocating statement handle\n");
        return false;
    }

    SQLSetConnectAttr(sqlConnHandle, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);

    SQLWCHAR sql_update_form[255];
    swprintf(sql_update_form, 255, L"UPDATE Formulare SET TipFormular = 'arhivat' WHERE ID = %d", form_id);

    retcode = SQLExecDirect(sqlStmtHandle, sql_update_form, SQL_NTS);
    if (retcode != SQL_SUCCESS) {
        printf("Error updating Formulare table\n");
        printSQLError(sqlStmtHandle, SQL_HANDLE_STMT);
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        SQLSetConnectAttr(sqlConnHandle, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
        return false;
    }

    SQLWCHAR sql_delete_request[255];
    swprintf(sql_delete_request, 255, L"DELETE FROM CererideStergere WHERE IDformular = %d", form_id);

    retcode = SQLExecDirect(sqlStmtHandle, sql_delete_request, SQL_NTS);
    if (retcode != SQL_SUCCESS) {
        logger.log("Error deleting from CererideStergere table\n");
        printf("Error deleting from CererideStergere table\n");
        printSQLError(sqlStmtHandle, SQL_HANDLE_STMT);
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        SQLSetConnectAttr(sqlConnHandle, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
        return false;
    }

    retcode = SQLEndTran(SQL_HANDLE_DBC, sqlConnHandle, SQL_COMMIT);
    if (retcode != SQL_SUCCESS) {
        printf("Error committing transaction\n");
        printSQLError(sqlStmtHandle, SQL_HANDLE_STMT);
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        SQLSetConnectAttr(sqlConnHandle, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
        return false;
    }

    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
    SQLSetConnectAttr(sqlConnHandle, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
    return true;
}

bool BazaDeDate::createSondaj(const std::string& form_title, const std::vector<std::string>& questions)
{
    SQLHANDLE stmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &stmt);

    SQLWCHAR sqlQuery[SQL_RESULT_LEN];
    swprintf(sqlQuery, SQL_RESULT_LEN, L"INSERT INTO Formulare (Titlu, Stare) VALUES ('%s', 'Sondaj')", form_title.c_str());
    SQLExecDirect(stmt, sqlQuery, SQL_NTS);

 
    SQLCloseCursor(sqlStmtHandle);

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);

    return true;  
}

int BazaDeDate::userExists(const std::string& username) {
    std::lock_guard<std::mutex> lock(db_mutex);

    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wusername = converter.from_bytes(username);

    SQLWCHAR sqlQuery[SQL_RESULT_LEN];
    swprintf(sqlQuery, SQL_RESULT_LEN, L"SELECT COUNT(*) FROM Utilizator WHERE Nume = '%s'", wusername.c_str());

    if (SQL_SUCCESS != SQLExecDirect(sqlStmtHandle, sqlQuery, SQL_NTS)) {
        printf("Eroare la executarea interogarii SQL\n");
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    SQLLEN count = 0;
    if (SQL_SUCCESS != SQLFetch(sqlStmtHandle)) {
        printf("Eroare la preluarea rezultatelor\n");
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    if (SQL_SUCCESS != SQLGetData(sqlStmtHandle, 1, SQL_C_SLONG, &count, 0, NULL)) {
        printf("Eroare la obtinerea datelor\n");
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }
    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    printf("Numar de utilizatori gasiti: %ld\n", count);

    return count;
}

std::wstring stringToWstring(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(str);
}

int BazaDeDate::createForm(const std::string& titlu, const std::string& tip_formular, const std::string& stare, int id_manager)
{
    std::wstring w_tip_formular = stringToWstring(tip_formular);
    std::wstring w_titlu = stringToWstring(titlu);
    std::wstring w_stare = stringToWstring(stare);

    SQLHANDLE sqlStmtHandle;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    SQLRETURN ret;
    SQLWCHAR sqlQuery[512]; 

    std::wstringstream queryStream;
    queryStream << L"INSERT INTO Formulare (Titlu, TipFormular, Stare, IDmanager) VALUES (N'" << w_titlu << L"', N'" << w_tip_formular << L"', N'" << w_stare << L"', " << id_manager << L");";
    std::wstring query = queryStream.str();

    wcscpy_s(sqlQuery, sizeof(sqlQuery) / sizeof(SQLWCHAR), query.c_str());

    ret = SQLExecDirect(sqlStmtHandle, sqlQuery, SQL_NTS);

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        SQLSMALLINT recNumber = 1;
        SQLINTEGER nativeError;
        SQLWCHAR sqlState[6];
        SQLWCHAR errMsg[SQL_MAX_MESSAGE_LENGTH];
        SQLSMALLINT errMsgLen;

        while (SQLGetDiagRec(SQL_HANDLE_STMT, sqlStmtHandle, recNumber, sqlState, &nativeError, errMsg, SQL_MAX_MESSAGE_LENGTH, &errMsgLen) != SQL_NO_DATA) {
            std::wcerr << L"Eroare SQL: " << errMsg << std::endl;
            recNumber++;
        }

        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return -1;  
    }

    SQLINTEGER form_id;
    ret = SQLExecDirect(sqlStmtHandle, (SQLWCHAR*)L"SELECT @@IDENTITY;", SQL_NTS);

    if (ret == SQL_SUCCESS) {
        SQLFetch(sqlStmtHandle);
        SQLGetData(sqlStmtHandle, 1, SQL_C_SLONG, &form_id, sizeof(form_id), nullptr);
    }
    else {
        std::wcerr << L"Eroare la obținerea ID-ului formularului." << std::endl;
        form_id = -1;  
    }

    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    return form_id;
}

int BazaDeDate::getOrganizationIDByManager(const std::string& username)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    SQLHANDLE sqlStmtHandle;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wusername = converter.from_bytes(username);

    SQLWCHAR sqlQuery[SQL_RESULT_LEN];
    swprintf(sqlQuery, SQL_RESULT_LEN, L"SELECT IDorganizatie FROM Utilizator WHERE Nume = '%s'", wusername.c_str());

    SQLRETURN ret = SQLExecDirect(sqlStmtHandle, sqlQuery, SQL_NTS);

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return -1;
    }

    SQLINTEGER organization_id;
    SQLFetch(sqlStmtHandle);
    SQLGetData(sqlStmtHandle, 1, SQL_C_SLONG, &organization_id, sizeof(organization_id), NULL);
    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    return organization_id;
}

int BazaDeDate::getIntrebareID(const std::string& text_intrebare)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    SQLHANDLE sqlStmtHandle;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wtext_intrebare = converter.from_bytes(text_intrebare);

    SQLWCHAR sqlQuery[SQL_RESULT_LEN];
    swprintf(sqlQuery, SQL_RESULT_LEN, L"SELECT ID FROM Intrebari WHERE Text = '%s'", wtext_intrebare.c_str());

    SQLRETURN ret = SQLExecDirect(sqlStmtHandle, sqlQuery, SQL_NTS);

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return -1;
    }

    int intrebare_id = -1;
    SQLLEN indicator;
    ret = SQLFetch(sqlStmtHandle);
    if (ret == SQL_SUCCESS) {
        SQLGetData(sqlStmtHandle, 1, SQL_C_SLONG, &intrebare_id, sizeof(intrebare_id), &indicator);
        if (indicator == SQL_NULL_DATA) {
            intrebare_id = -1;
        }
    }
    else {
        intrebare_id = -1; 
    }
    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    return intrebare_id;
}

int BazaDeDate::getResponseID(const std::string& text_raspuns)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    SQLHANDLE sqlStmtHandle;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wtext_raspuns = converter.from_bytes(text_raspuns);

    SQLWCHAR sqlQuery[SQL_RESULT_LEN];
    swprintf(sqlQuery, SQL_RESULT_LEN, L"SELECT ID FROM Raspunsuri WHERE Text = '%s'", wtext_raspuns.c_str());

    SQLRETURN ret = SQLExecDirect(sqlStmtHandle, sqlQuery, SQL_NTS);

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return -1;
    }

    int raspuns_id = -1;
    SQLLEN indicator;
    ret = SQLFetch(sqlStmtHandle);
    if (ret == SQL_SUCCESS) {
        SQLGetData(sqlStmtHandle, 1, SQL_C_SLONG, &raspuns_id, sizeof(raspuns_id), &indicator);
        if (indicator == SQL_NULL_DATA) {
            raspuns_id = -1;
        }
    }
    else {
        raspuns_id = -1; 
    }
    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    return raspuns_id;
}

int BazaDeDate::getResponseID(int id_intrebare, const std::string& raspuns)
{
    int id_raspuns = -1;

    SQLWCHAR sql_query[255];
    swprintf(sql_query, 255, L"SELECT ID FROM Raspunsuri WHERE IDintrebare = %d AND Text = '%s'", id_intrebare, raspuns.c_str());

    SQLHSTMT sqlStmtHandle;
    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
        printf("Error allocating statement handle\n");
        return id_raspuns;
    }

    SQLRETURN retcode = SQLExecDirect(sqlStmtHandle, sql_query, SQL_NTS);
    if (retcode != SQL_SUCCESS) {
        printf("Error executing SQL query\n");
        printSQLError(sqlStmtHandle, SQL_HANDLE_STMT);
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return id_raspuns;
    }

    if (SQLFetch(sqlStmtHandle) == SQL_SUCCESS) {
        SQLINTEGER id;
        SQLGetData(sqlStmtHandle, 1, SQL_C_LONG, &id, sizeof(id), NULL);
        id_raspuns = id;
    }
    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
    return id_raspuns;
}

int BazaDeDate::getResponseID(int id_intrebare, int id_utilizator, int id_formular)
{
    SQLHSTMT sqlStmtHandle;
    SQLRETURN retcode;

    SQLWCHAR sql_query[255];
    swprintf(sql_query, 255, L"SELECT IDraspuns FROM Raspunsuri_User WHERE IDintrebare = %d AND IDutilizator = %d AND IdFormular = %d", id_intrebare, id_utilizator, id_formular);

    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
        printf("Error allocating statement handle\n");
        return -1;
    }

    retcode = SQLExecDirect(sqlStmtHandle, sql_query, SQL_NTS);
    if (retcode != SQL_SUCCESS) {
        printf("Error executing SQL query\n");

        printSQLError(sqlStmtHandle, SQL_HANDLE_STMT);
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return -1;
    }

    int id_raspuns = -1;
    if (SQLFetch(sqlStmtHandle) == SQL_SUCCESS) {
        SQLGetData(sqlStmtHandle, 1, SQL_C_SLONG, &id_raspuns, sizeof(id_raspuns), NULL);
    }

    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    return id_raspuns;
}

std::vector<int> BazaDeDate::getSondajStatistics(int sondaj_id)
{
    std::vector<int> stats;

    SQLHANDLE stmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &stmt);

    SQLWCHAR sqlQuery[SQL_RESULT_LEN];
    swprintf(sqlQuery, SQL_RESULT_LEN, L"SELECT Raspunsuri.Text FROM Raspunsuri JOIN Intrebari ON Raspunsuri.IDintrebare = Intrebari.ID WHERE Intrebari.IDformular = %d", sondaj_id);

    SQLExecDirect(stmt, sqlQuery, SQL_NTS);

    while (SQLFetch(stmt) == SQL_SUCCESS) {
        wchar_t buffer[SQL_RESULT_LEN];
        SQLGetData(stmt, 1, SQL_C_WCHAR, buffer, sizeof(buffer), nullptr);

        int response = std::stoi(buffer); 
        stats.push_back(response);
    }
    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);

    return stats;
}

std::vector<int> BazaDeDate::getOrganizationMembersForManager(int manager_id)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    std::vector<int> members;
    SQLHANDLE sqlStmtHandle;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    SQLWCHAR sqlQuery[SQL_RESULT_LEN];
    swprintf(sqlQuery, SQL_RESULT_LEN, L"SELECT IDutilizator FROM Organizatii_Utilizatori WHERE IDorganizatie = (SELECT ID FROM Organizatii WHERE Manager = %d)", manager_id);

    SQLRETURN ret = SQLExecDirect(sqlStmtHandle, sqlQuery, SQL_NTS);

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return members;
    }

    SQLINTEGER user_id;
    SQLRETURN fetchRet;
    while ((fetchRet = SQLFetch(sqlStmtHandle)) == SQL_SUCCESS || fetchRet == SQL_SUCCESS_WITH_INFO) {
        SQLGetData(sqlStmtHandle, 1, SQL_C_LONG, &user_id, 0, NULL);
        members.push_back(user_id);
    }
    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    return members;
}

std::vector<int> BazaDeDate::getUnarchivedFormIDs()
{
    std::vector<int> form_ids;
    SQLHSTMT sqlStmtHandle;
    SQLRETURN retcode;

    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
        printf("Error allocating statement handle\n");
        return form_ids;
    }

    SQLWCHAR sql_query[] = L"SELECT IDformular FROM CererideArhivare WHERE Arhivat = 0";
    retcode = SQLExecDirect(sqlStmtHandle, sql_query, SQL_NTS);

    if (retcode != SQL_SUCCESS) {
        printf("Error executing SQL query\n");
        printSQLError(sqlStmtHandle, SQL_HANDLE_STMT);
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return form_ids;
    }

    SQLINTEGER id_formular;
    while (SQLFetch(sqlStmtHandle) == SQL_SUCCESS) {
        SQLGetData(sqlStmtHandle, 1, SQL_C_SLONG, &id_formular, 0, NULL);
        form_ids.push_back(id_formular);
    }

    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
    return form_ids;
}

std::vector<std::string> BazaDeDate::getOrganizationMembers(int organizationID)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    std::vector<std::string> organizationMembers;

    SQLHANDLE sqlStmtHandle;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    SQLWCHAR sqlQuery[SQL_RESULT_LEN];
    swprintf(sqlQuery, SQL_RESULT_LEN, L"SELECT Nume FROM Utilizator WHERE IDorganizatie = %d", organizationID);

    SQLRETURN ret = SQLExecDirect(sqlStmtHandle, sqlQuery, SQL_NTS);

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return organizationMembers;
    }

    SQLWCHAR username[SQL_RESULT_LEN];
    SQLLEN indicator;

    while (SQLFetch(sqlStmtHandle) == SQL_SUCCESS) {
        SQLGetData(sqlStmtHandle, 1, SQL_C_WCHAR, username, SQL_RESULT_LEN, &indicator);
        if (indicator != SQL_NULL_DATA) {
            std::wstring wUsername(username);
            std::string usernameUTF8 = converter.to_bytes(wUsername);
            organizationMembers.push_back(usernameUTF8);
        }
    }
    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    return organizationMembers;
}

std::vector<std::string> BazaDeDate::getUncompletedForms(const std::string& username)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    std::lock_guard<std::mutex> lock(db_mutex);

    SQLHANDLE sqlStmtHandle;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wusername = converter.from_bytes(username);

    SQLWCHAR sqlQuery[SQL_RESULT_LEN];
    swprintf(sqlQuery, SQL_RESULT_LEN,
        L"SELECT F.Titlu "
        L"FROM Formulare F "
        L"JOIN Formulare_Distribuite FD ON F.ID = FD.IDformular "
        L"JOIN Utilizator U ON FD.IDutilizator = U.ID "
        L"WHERE U.Nume = '%s' AND (FD.Completat IS NULL OR FD.Completat = 0) AND F.TipFormular != 'sters'",
        wusername.c_str());

    std::cout << "Interogarea SQL: " << converter.to_bytes(sqlQuery) << std::endl;

    SQLRETURN ret = SQLExecDirect(sqlStmtHandle, sqlQuery, SQL_NTS);

    std::vector<std::string> forms;

    if (ret != SQL_SUCCESS) {
        SQLSMALLINT recNumber = 1;
        SQLINTEGER nativeError;
        SQLWCHAR sqlState[6];
        SQLWCHAR errMsg[SQL_MAX_MESSAGE_LENGTH];
        SQLSMALLINT errMsgLen;

        while (SQLGetDiagRec(SQL_HANDLE_STMT, sqlStmtHandle, recNumber, sqlState, &nativeError, errMsg, SQL_MAX_MESSAGE_LENGTH, &errMsgLen) != SQL_NO_DATA) {
            std::wcerr << L"Eroare SQL: " << errMsg << std::endl;
            recNumber++;
        }
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return forms;
    }

    wchar_t form_title[SQL_RESULT_LEN];
    SQLLEN indicator;
    while (SQLFetch(sqlStmtHandle) == SQL_SUCCESS) {
        SQLGetData(sqlStmtHandle, 1, SQL_C_WCHAR, form_title, sizeof(form_title), &indicator);
        if (indicator == SQL_NULL_DATA) {
            continue;
        }
        std::wstring wform_title(form_title);
        std::string form_title_str = converter.to_bytes(wform_title);
        forms.push_back(form_title_str);
        logger.log(form_title_str);
        std::cout << "Titlu formular: " << form_title_str << std::endl;
    }

    logger.log("Rezultatele interogrii din baza de date: ");
    std::cout << "Rezultatele interogrii din baza de date: ";
    for (const auto& form : forms) {
        logger.log(form);
        std::cout << form << " ";
    }
    std::cout << std::endl;
    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
    return forms;
}

std::vector<std::string> BazaDeDate::getCompletedForms(const std::string& username)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    std::lock_guard<std::mutex> lock(db_mutex);

    SQLHANDLE sqlStmtHandle;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wusername = converter.from_bytes(username);

    SQLWCHAR sqlQuery[SQL_RESULT_LEN];
    swprintf(sqlQuery, SQL_RESULT_LEN,
        L"SELECT F.Titlu "
        L"FROM Formulare F "
        L"JOIN Formulare_Distribuite FD ON F.ID = FD.IDformular "
        L"JOIN Utilizator U ON FD.IDutilizator = U.ID "
        L"WHERE U.Nume = '%s' AND FD.Completat = 1 AND F.TipFormular != 'sters'",
        wusername.c_str());

    std::cout << "Interogarea SQL: " << converter.to_bytes(sqlQuery) << std::endl;

    SQLRETURN ret = SQLExecDirect(sqlStmtHandle, sqlQuery, SQL_NTS);

    std::vector<std::string> forms;

    if (ret != SQL_SUCCESS) {
        SQLSMALLINT recNumber = 1;
        SQLINTEGER nativeError;
        SQLWCHAR sqlState[6];
        SQLWCHAR errMsg[SQL_MAX_MESSAGE_LENGTH];
        SQLSMALLINT errMsgLen;

        while (SQLGetDiagRec(SQL_HANDLE_STMT, sqlStmtHandle, recNumber, sqlState, &nativeError, errMsg, SQL_MAX_MESSAGE_LENGTH, &errMsgLen) != SQL_NO_DATA) {
            std::wcerr << L"Eroare SQL: " << errMsg << std::endl;
            recNumber++;
        }
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return forms;
    }

    wchar_t form_title[SQL_RESULT_LEN];
    SQLLEN indicator;
    while (SQLFetch(sqlStmtHandle) == SQL_SUCCESS) {
        SQLGetData(sqlStmtHandle, 1, SQL_C_WCHAR, form_title, sizeof(form_title), &indicator);
        if (indicator == SQL_NULL_DATA) {
            continue;
        }
        std::wstring wform_title(form_title);
        std::string form_title_str = converter.to_bytes(wform_title);
        forms.push_back(form_title_str);
        std::cout << "Titlu formular: " << form_title_str << std::endl;
    }

    std::cout << "Rezultatele interogarii din baza de date: ";
    for (const auto& form : forms) {
        logger.log(form);
        std::cout << form << " ";
    }
    std::cout << std::endl;
    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
    return forms;
}

std::vector<std::string> BazaDeDate::getCreatedForms(const std::string& username)
{
    SQLHSTMT sqlStmtHandle;
    SQLRETURN retcode;
    std::vector<std::string> createdForms;

    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
        printf("Error allocating statement handle\n");
        return createdForms;
    }

    const wchar_t* sql_query = L"SELECT Titlu FROM Formulare WHERE IDmanager = (SELECT ID FROM Utilizator WHERE Nume = ?) AND TipFormular != 'sters'";
    retcode = SQLPrepareW(sqlStmtHandle, (SQLWCHAR*)sql_query, SQL_NTS);
    if (retcode != SQL_SUCCESS) {
        printf("Error preparing SQL query\n");
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return createdForms;
    }

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wusername = converter.from_bytes(username);

    retcode = SQLBindParameter(sqlStmtHandle, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, wusername.size(), 0, (SQLWCHAR*)wusername.c_str(), 0, NULL);
    if (retcode != SQL_SUCCESS) {
        printf("Error binding parameter: %d\n", retcode);
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return createdForms;
    }

    retcode = SQLExecute(sqlStmtHandle);
    if (retcode != SQL_SUCCESS) {
        printf("Error executing SQL query: %d\n", retcode);
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return createdForms;
    }

    SQLWCHAR title[256];
    while (SQLFetch(sqlStmtHandle) == SQL_SUCCESS) {
        SQLGetData(sqlStmtHandle, 1, SQL_C_WCHAR, title, sizeof(title), NULL);
        std::wstring wtitle(title);
        std::string stitle = converter.to_bytes(wtitle);
        createdForms.push_back(stitle);
    }

    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    return createdForms;
}

std::vector<std::string> BazaDeDate::getFormTitlesByIDs(const std::vector<int>& form_ids)
{
    std::vector<std::string> form_titles;
    if (form_ids.empty()) {
        return form_titles;
    }

    SQLHSTMT sqlStmtHandle;
    SQLRETURN retcode;

    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
        printf("Error allocating statement handle\n");
        return form_titles;
    }

    std::wstring ids;
    for (size_t i = 0; i < form_ids.size(); ++i) {
        ids += std::to_wstring(form_ids[i]);
        if (i < form_ids.size() - 1) {
            ids += L",";
        }
    }

    SQLWCHAR sql_query[512];
    swprintf(sql_query, 512, L"SELECT Titlu FROM Formulare WHERE ID IN (%ls)", ids.c_str());
    retcode = SQLExecDirect(sqlStmtHandle, sql_query, SQL_NTS);

    if (retcode != SQL_SUCCESS) {
        printf("Error executing SQL query\n");
        printSQLError(sqlStmtHandle, SQL_HANDLE_STMT);
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return form_titles;
    }

    SQLWCHAR title[256];
    while (SQLFetch(sqlStmtHandle) == SQL_SUCCESS) {
        SQLGetData(sqlStmtHandle, 1, SQL_C_WCHAR, title, sizeof(title), NULL);
        std::wstring wtitle(title);
        std::string str_title(wtitle.begin(), wtitle.end());
        form_titles.push_back(str_title);
    }

    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
    return form_titles;
}

std::vector<std::pair<std::string, std::string>> BazaDeDate::getUserResponsesForForm(int id_utilizator, int id_formular)
{
    std::vector<std::pair<std::string, std::string>> user_responses;

    SQLHSTMT sqlStmtHandle;
    SQLRETURN retcode;

    SQLWCHAR sql_query[255];
    swprintf(sql_query, 255, L"SELECT IDintrebare FROM Raspunsuri_User WHERE IDutilizator = %d AND IdFormular = %d", id_utilizator, id_formular);

    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
        printf("Error allocating statement handle\n");
        return user_responses;
    }

    retcode = SQLExecDirect(sqlStmtHandle, sql_query, SQL_NTS);
    if (retcode != SQL_SUCCESS) {
        printf("Error executing SQL query\n");
        printSQLError(sqlStmtHandle, SQL_HANDLE_STMT);
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return user_responses;
    }

    SQLINTEGER id_intrebare;
    while (SQLFetch(sqlStmtHandle) == SQL_SUCCESS) {
        SQLGetData(sqlStmtHandle, 1, SQL_C_LONG, &id_intrebare, sizeof(id_intrebare), NULL);

        std::string intrebare_text = getIntrebareText(id_intrebare);


        int id_raspuns = getResponseID( id_intrebare, id_utilizator,  id_formular);
        std::string raspuns_text = getRaspunsText(id_raspuns);

        user_responses.push_back({ intrebare_text, raspuns_text });
    }

    if (SQL_SUCCESS != SQLCloseCursor(sqlStmtHandle)) {
        printf("Error closing cursor\n");
        printSQLError(sqlStmtHandle, SQL_HANDLE_STMT);
    }
    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
    return user_responses;
}

std::vector<std::pair<int, std::string>> BazaDeDate::getPendingDeleteRequests()
{
    std::vector<std::pair<int, std::string>> requests;
    SQLHSTMT sqlStmtHandle;
    SQLRETURN retcode;

    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
        printf("Error allocating statement handle\n");
        return requests;
    }

    SQLWCHAR sql_query[] = L"SELECT IDformular, Titlu FROM CererideStergere JOIN Formulare ON CererideStergere.IDformular = Formulare.ID WHERE Sters = 0";
    retcode = SQLExecDirect(sqlStmtHandle, sql_query, SQL_NTS);

    if (retcode != SQL_SUCCESS) {
        printf("Error executing SQL query\n");
        printSQLError(sqlStmtHandle, SQL_HANDLE_STMT);
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return requests;
    }

    SQLINTEGER id_formular;
    SQLWCHAR titlu[256];
    while (SQLFetch(sqlStmtHandle) == SQL_SUCCESS) {
        SQLGetData(sqlStmtHandle, 1, SQL_C_SLONG, &id_formular, 0, NULL);
        SQLGetData(sqlStmtHandle, 2, SQL_C_WCHAR, titlu, sizeof(titlu), NULL);
        std::wstring wtitle(titlu);
        std::string str_title(wtitle.begin(), wtitle.end());
        requests.emplace_back(id_formular, str_title);
    }

    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
    return requests;
}

std::vector<std::map<std::string, std::string>> BazaDeDate::query(const std::string& query, const std::vector<std::string>& params)
{
    SQLHSTMT sqlStmtHandle;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    std::wstring wquery = s2ws(query);  
    SQLPrepareW(sqlStmtHandle, (SQLWCHAR*)wquery.c_str(), SQL_NTS);

    std::vector<std::wstring> wparams(params.size());
    for (size_t i = 0; i < params.size(); ++i) {
        wparams[i] = s2ws(params[i]);
    }

    bindParameters(sqlStmtHandle, wparams);

    SQLExecute(sqlStmtHandle);

    SQLSMALLINT columns;
    SQLNumResultCols(sqlStmtHandle, &columns);

    std::vector<std::map<std::string, std::string>> results;
    while (SQLFetch(sqlStmtHandle) == SQL_SUCCESS) {
        std::map<std::string, std::string> row;
        for (SQLUSMALLINT i = 1; i <= columns; i++) {
            SQLWCHAR columnName[128];
            SQLWCHAR columnValue[256];
            SQLLEN indicator;
            SQLDescribeColW(sqlStmtHandle, i, columnName, sizeof(columnName) / sizeof(SQLWCHAR), NULL, NULL, NULL, NULL, NULL);
            SQLGetData(sqlStmtHandle, i, SQL_C_WCHAR, columnValue, sizeof(columnValue), &indicator);

            std::wstring wColumnName(columnName);
            std::wstring wColumnValue(columnValue);

            std::string columnNameStr(wColumnName.begin(), wColumnName.end());
            std::string columnValueStr(wColumnValue.begin(), wColumnValue.end());

            row[columnNameStr] = columnValueStr;
        }
        results.push_back(row);
    }

    std::cout << "Number of columns retrieved: " << columns << std::endl;
    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
    return results;
}

bool BazaDeDate::addUser(const std::string& username, const std::string& hashedPassword, const std::string& role, const std::string& organization) {
   
    std::lock_guard<std::mutex> lock(db_mutex);

    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wusername = converter.from_bytes(username);
    std::wstring whashedPassword = converter.from_bytes(hashedPassword);
    std::wstring wrole = converter.from_bytes(role);
    std::wstring worganization = converter.from_bytes(organization);

    int roleID;
    if (!getRoleID(wrole, roleID)) {
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    SQLWCHAR sqlQuery[256];
    const wchar_t* query = L"INSERT INTO Utilizator (Nume, Parola, IDrol) VALUES (?, ?, ?)";
    errno_t error = wcsncpy_s(sqlQuery, sizeof(sqlQuery) / sizeof(SQLWCHAR), query, _TRUNCATE);

    if (error != 0) {
        printf("Error during string copy. Error code: %d\n", error);
        return false;
    }
  
    

    SQLRETURN allocRet = SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);
    if (allocRet != SQL_SUCCESS && allocRet != SQL_SUCCESS_WITH_INFO) {
        printf("Failed to allocate SQL statement handle.\n");
        return false;
    }

    SQLRETURN ret = SQLPrepare(sqlStmtHandle, sqlQuery, SQL_NTS);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        printf("Eroare la pregatirea interogarii SQL.\n");
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    SQLBindParameter(sqlStmtHandle, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 30, 0, (SQLPOINTER)wusername.c_str(), 0, NULL);
    SQLBindParameter(sqlStmtHandle, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 30, 0, (SQLPOINTER)whashedPassword.c_str(), 0, NULL);
    SQLBindParameter(sqlStmtHandle, 3, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &roleID, 0, NULL);

    ret = SQLExecute(sqlStmtHandle);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        printf("Eroare la executarea interogarii SQL.\n");
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }
    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    return true;
}

bool BazaDeDate::verifyPassword(const std::string& username, const std::string& hashedPassword)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    SQLHANDLE sqlStmtHandle;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wusername = converter.from_bytes(username);

    SQLWCHAR sqlQuery[SQL_RESULT_LEN];
    swprintf(sqlQuery, SQL_RESULT_LEN, L"SELECT Parola FROM Utilizator WHERE Nume = '%s'", wusername.c_str());

    if (SQL_SUCCESS != SQLExecDirect(sqlStmtHandle, sqlQuery, SQL_NTS)) {
        logger.log("Eroare la executarea interogarii SQL\n");
        std::cerr << "Eroare la executarea interogarii SQL\n";
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    std::wstring storedHashedPassword;
    SQLLEN indicator;
    wchar_t buffer[SQL_RESULT_LEN];

    if (SQL_SUCCESS != SQLFetch(sqlStmtHandle)) {
        logger.log("Niciun utilizator gasit\n");
        std::cerr << "Niciun utilizator gasit\n";
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    if (SQL_SUCCESS != SQLGetData(sqlStmtHandle, 1, SQL_C_WCHAR, buffer, sizeof(buffer), &indicator)) {
        logger.log("Eroare la obtinerea datelor\n");
        std::cerr << "Eroare la obtinerea datelor\n";
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    storedHashedPassword = buffer;
    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    if (storedHashedPassword == converter.from_bytes(hashedPassword)) {
        logger.log(" Utilizatorul a fost gasit cu parola corecta.\n");
        std::cout << "Utilizatorul " << username << " a fost gasit cu parola corecta.\n";
        return true;
    }
    else {
        logger.log("Parola pentru utilizatorul este incorecta.\n");
        std::cout << "Parola pentru utilizatorul " << username << " este incorecta.\n";
        return false;
    }
}

bool BazaDeDate::addUserToOrganization(const std::string& username, const std::string& org_name)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    SQLHANDLE sqlStmtHandle;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wusername = converter.from_bytes(username);
    std::wstring worg_name = converter.from_bytes(org_name);

    SQLWCHAR sqlQuery[SQL_RESULT_LEN];
    swprintf(sqlQuery, SQL_RESULT_LEN, L"UPDATE Utilizator SET IDorganizație = (SELECT ID FROM Organizații WHERE Nume = '%s') WHERE Nume = '%s'", worg_name.c_str(), wusername.c_str());

    SQLRETURN ret = SQLExecDirect(sqlStmtHandle, sqlQuery, SQL_NTS);
    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    return ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO;
}

bool BazaDeDate::organizationExists(const std::string& org_name)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    SQLHANDLE sqlStmtHandle;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring worg_name = converter.from_bytes(org_name);

    SQLWCHAR sqlQuery[SQL_RESULT_LEN];
    swprintf(sqlQuery, SQL_RESULT_LEN, L"SELECT COUNT(*) FROM Organizatii WHERE Nume = '%s'", worg_name.c_str());

    SQLRETURN ret = SQLExecDirect(sqlStmtHandle, sqlQuery, SQL_NTS);

    SQLLEN count = 0;
    SQLFetch(sqlStmtHandle);
    SQLGetData(sqlStmtHandle, 1, SQL_C_SLONG, &count, sizeof(count), NULL);
    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    return count > 0;
}

bool BazaDeDate::isManager(const std::string& username)
{
    std::lock_guard<std::mutex> lock(db_mutex);  

    SQLHANDLE sqlStmtHandle;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wusername = converter.from_bytes(username);

    SQLWCHAR sqlQuery[SQL_RESULT_LEN];
    swprintf(sqlQuery, SQL_RESULT_LEN, L"SELECT IDrol FROM Utilizator WHERE Nume = '%s'", wusername.c_str());

    SQLRETURN ret = SQLExecDirect(sqlStmtHandle, sqlQuery, SQL_NTS);

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }

    int roleID;
    SQLFetch(sqlStmtHandle);
    SQLGetData(sqlStmtHandle, 1, SQL_C_SLONG, &roleID, sizeof(roleID), NULL);
    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    return roleID == 2;
}

bool BazaDeDate::createOrganization(const std::string& org_name, int manager_id)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    SQLHANDLE sqlStmtHandle;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring worg_name = converter.from_bytes(org_name);

    SQLWCHAR sqlQuery[SQL_RESULT_LEN];
    swprintf(sqlQuery, SQL_RESULT_LEN, L"INSERT INTO Organizatii (Nume, Manager) VALUES ('%s', %d)", worg_name.c_str(), manager_id);

    SQLRETURN ret = SQLExecDirect(sqlStmtHandle, sqlQuery, SQL_NTS);
    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    return ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO;
}

bool BazaDeDate::setUserOrganization(const std::string& username, const std::string& organization)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    SQLHANDLE sqlStmtHandle;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wusername = converter.from_bytes(username);
    std::wstring worganization = converter.from_bytes(organization);

    SQLWCHAR sqlQuery[SQL_RESULT_LEN];
    swprintf(sqlQuery, SQL_RESULT_LEN, L"UPDATE Utilizator SET IDorganizatie = (SELECT ID FROM Organizatii WHERE Nume = '%s') WHERE Nume = '%s'", worganization.c_str(), wusername.c_str());

    SQLRETURN ret = SQLExecDirect(sqlStmtHandle, sqlQuery, SQL_NTS);
    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    return ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO;
}

bool BazaDeDate::addUserToOrganizationMapping(const std::string& username, const std::string& organization)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    SQLHANDLE sqlStmtHandle;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wusername = converter.from_bytes(username);
    std::wstring worganization = converter.from_bytes(organization);

    SQLWCHAR sqlQuery[SQL_RESULT_LEN];
    swprintf(sqlQuery, SQL_RESULT_LEN, L"INSERT INTO Organizatii_Utilizatori (IDorganizatie, IDutilizator) "
        L"VALUES ((SELECT ID FROM Organizatii WHERE Nume = '%s'), "
        L"(SELECT ID FROM Utilizator WHERE Nume = '%s'))",
        worganization.c_str(), wusername.c_str());

    SQLRETURN ret = SQLExecDirect(sqlStmtHandle, sqlQuery, SQL_NTS);
    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    return ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO;
}

int BazaDeDate::addIntrebare(int form_id, const std::string& intrebare)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    SQLHANDLE sqlStmtHandle;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wintrebare = converter.from_bytes(intrebare);

    SQLWCHAR sqlQuery[SQL_RESULT_LEN];
    swprintf(sqlQuery, SQL_RESULT_LEN, L"INSERT INTO Intrebari ( IDformular, Text) "
        L"VALUES (%d, N'%s')",
         form_id, wintrebare.c_str());

    SQLRETURN ret = SQLExecDirect(sqlStmtHandle, sqlQuery, SQL_NTS);

    if (ret != SQL_SUCCESS) {
        SQLSMALLINT recNumber = 1;
        SQLINTEGER nativeError;
        SQLWCHAR sqlState[6];
        SQLWCHAR errMsg[SQL_MAX_MESSAGE_LENGTH];
        SQLSMALLINT errMsgLen;

        while (SQLGetDiagRec(SQL_HANDLE_STMT, sqlStmtHandle, recNumber, sqlState, &nativeError, errMsg, SQL_MAX_MESSAGE_LENGTH, &errMsgLen) != SQL_NO_DATA) {
            std::wcerr << L"Eroare SQL: " << errMsg << std::endl;
            recNumber++;
        }
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return -1;  
    }

     int intrebare_id = -1;
    ret = SQLExecDirect(sqlStmtHandle, (SQLWCHAR*)L"SELECT SCOPE_IDENTITY();", SQL_NTS);

    if (ret == SQL_SUCCESS) {
        SQLINTEGER id;
        SQLLEN indicator;
        SQLBindCol(sqlStmtHandle, 1, SQL_C_SLONG, &id, sizeof(id), &indicator);
        SQLFetch(sqlStmtHandle);
        if (indicator != SQL_NULL_DATA) {
            intrebare_id = id;
        }
    }
    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    return intrebare_id;
}

std::string BazaDeDate::getOrganizationByManager(const std::string& manager_username)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    SQLHANDLE sqlStmtHandle;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wmanager_username = converter.from_bytes(manager_username);

    SQLWCHAR sqlQuery[SQL_RESULT_LEN];
    swprintf(sqlQuery, SQL_RESULT_LEN, L"SELECT Nume FROM Organizatii WHERE Manager = (SELECT ID FROM Utilizator WHERE Nume = '%s')", wmanager_username.c_str());

    SQLRETURN ret = SQLExecDirect(sqlStmtHandle, sqlQuery, SQL_NTS);

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return "";  
    }

    std::wstring org_name;
    wchar_t buffer[SQL_RESULT_LEN]; 


    SQLRETURN fetchRet = SQLFetch(sqlStmtHandle);
    if (fetchRet != SQL_SUCCESS && fetchRet != SQL_SUCCESS_WITH_INFO) {
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return ""; 
    }


    SQLFetch(sqlStmtHandle);
    SQLRETURN getDataRet = SQLGetData(sqlStmtHandle, 1, SQL_C_WCHAR, buffer, sizeof(buffer), NULL);

    if (getDataRet == SQL_SUCCESS || getDataRet == SQL_SUCCESS_WITH_INFO) {
        org_name = buffer; 
    }
    else {
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return "";
    }

    std::string org_name_str = converter.to_bytes(org_name);

    return org_name_str;
}

std::string BazaDeDate::GetUserType(const std::string& username)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    SQLHANDLE sqlStmtHandle;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wusername = converter.from_bytes(username);

    SQLWCHAR sqlQuery[SQL_RESULT_LEN];
    swprintf(sqlQuery, SQL_RESULT_LEN, L"SELECT R.Descriere FROM Utilizator U JOIN Rol R ON U.IDrol = R.ID WHERE U.Nume = '%s'", wusername.c_str());

    SQLRETURN ret = SQLExecDirect(sqlStmtHandle, sqlQuery, SQL_NTS);

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return "";
    }

    std::wstring userType;
    wchar_t buffer[SQL_RESULT_LEN]; 

    SQLRETURN fetchRet = SQLFetch(sqlStmtHandle);
    if (fetchRet != SQL_SUCCESS && fetchRet != SQL_SUCCESS_WITH_INFO) {
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return ""; 
    }

    SQLRETURN getDataRet = SQLGetData(sqlStmtHandle, 1, SQL_C_WCHAR, buffer, sizeof(buffer), NULL);

    if (getDataRet == SQL_SUCCESS || getDataRet == SQL_SUCCESS_WITH_INFO) {
        userType = buffer; 
    }
    else {
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return "";
    }

    std::string userType_str = converter.to_bytes(userType);
    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    return userType_str;
}

std::string BazaDeDate::GetFormAndAnswersFromDatabase(const std::string& form_name)
{
    std::string form;
    SQLHSTMT sqlStmtHandle;
    SQLRETURN retcode;

    retcode = SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);
    if (retcode != SQL_SUCCESS) {
        printf("Error allocating statement handle\n");
        return "";
    }

    SQLWCHAR sql_query[255];
    swprintf(sql_query, 255, L"SELECT Intrebari.Text, Raspunsuri.Text FROM Intrebari JOIN Raspunsuri ON Intrebari.ID = Raspunsuri.IDintrebare WHERE Intrebari.IDformular = (SELECT ID FROM Formulare WHERE Titlu = '%S')", form_name.c_str());
    printf("SQL Query: %ls\n", sql_query); 
    retcode = SQLExecDirect(sqlStmtHandle, sql_query, SQL_NTS);
    if (retcode != SQL_SUCCESS) {
        printf("Error executing SQL query\n");
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return "";
    }

    SQLWCHAR question_text[255];
    SQLWCHAR answer_text[255];
    SQLLEN indicator;
    while (SQLFetch(sqlStmtHandle) == SQL_SUCCESS) {
        retcode = SQLGetData(sqlStmtHandle, 1, SQL_C_WCHAR, question_text, sizeof(question_text), &indicator);
        if (retcode == SQL_SUCCESS) {
            std::wstring w_question(question_text);
            std::string question(w_question.begin(), w_question.end());
            form += question;
            form += "*";
        }
        retcode = SQLGetData(sqlStmtHandle, 2, SQL_C_WCHAR, answer_text, sizeof(answer_text), &indicator);
        if (retcode == SQL_SUCCESS) {
            std::wstring w_answer(answer_text);
            std::string answer(w_answer.begin(), w_answer.end());
            form += answer;
            form += "|";
        }
    }

    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    return form;
}

std::string BazaDeDate::getStatistics(const std::string& nume_formular)
{
    SQLHSTMT sqlStmtHandle;
    SQLRETURN retcode;
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

    int id_formular = 0;
    {
        const wchar_t* sql_query = L"SELECT ID FROM Formulare WHERE Titlu = ?";
        if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
            printf("Error allocating statement handle\n");
            return "";
        }
        std::wstring w_nume_formular = converter.from_bytes(nume_formular);
        SQLPrepareW(sqlStmtHandle, (SQLWCHAR*)sql_query, SQL_NTS);
        SQLBindParameter(sqlStmtHandle, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, w_nume_formular.size(), 0, (SQLWCHAR*)w_nume_formular.c_str(), 0, NULL);
        SQLExecute(sqlStmtHandle);
        SQLBindCol(sqlStmtHandle, 1, SQL_C_SLONG, &id_formular, sizeof(id_formular), NULL);
        if (SQLFetch(sqlStmtHandle) != SQL_SUCCESS) {
            printf("Formular not found\n");
            SQLCloseCursor(sqlStmtHandle);
            SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
            return "";
        }
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
    }

    std::vector<int> intrebari_ids;
    {
        const wchar_t* sql_query = L"SELECT ID FROM Intrebari WHERE IDformular = ?";
        if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
            printf("Error allocating statement handle\n");
            return "";
        }
        SQLPrepareW(sqlStmtHandle, (SQLWCHAR*)sql_query, SQL_NTS);
        SQLBindParameter(sqlStmtHandle, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &id_formular, 0, NULL);
        SQLExecute(sqlStmtHandle);
        int id_intrebare;
        SQLBindCol(sqlStmtHandle, 1, SQL_C_SLONG, &id_intrebare, sizeof(id_intrebare), NULL);
        while (SQLFetch(sqlStmtHandle) == SQL_SUCCESS) {
            intrebari_ids.push_back(id_intrebare);
        }
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
    }

    std::string result;
    for (int id_intrebare : intrebari_ids) {
        std::string text_intrebare;
        {
            const wchar_t* sql_query = L"SELECT Text FROM Intrebari WHERE ID = ?";
            if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
                printf("Error allocating statement handle\n");
                return "";
            }
            SQLPrepareW(sqlStmtHandle, (SQLWCHAR*)sql_query, SQL_NTS);
            SQLBindParameter(sqlStmtHandle, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &id_intrebare, 0, NULL);
            SQLExecute(sqlStmtHandle);
            SQLWCHAR w_text_intrebare[256];
            SQLBindCol(sqlStmtHandle, 1, SQL_C_WCHAR, w_text_intrebare, sizeof(w_text_intrebare), NULL);
            if (SQLFetch(sqlStmtHandle) == SQL_SUCCESS) {
                text_intrebare = converter.to_bytes(w_text_intrebare);
            }
            SQLCloseCursor(sqlStmtHandle);
            SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        }

        std::map<std::string, int> raspunsuri_contori;
        {
            const wchar_t* sql_query = L"SELECT R.Text, ISNULL(RC.Contor, 0) "
                L"FROM Raspunsuri R LEFT JOIN Raspunsuri_Completate RC ON R.ID = RC.IDraspuns "
                L"WHERE R.IDintrebare = ?";
            if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
                printf("Error allocating statement handle\n");
                return "";
            }
            SQLPrepareW(sqlStmtHandle, (SQLWCHAR*)sql_query, SQL_NTS);
            SQLBindParameter(sqlStmtHandle, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &id_intrebare, 0, NULL);
            SQLExecute(sqlStmtHandle);
            SQLWCHAR w_text_raspuns[256];
            int contor;
            SQLBindCol(sqlStmtHandle, 1, SQL_C_WCHAR, w_text_raspuns, sizeof(w_text_raspuns), NULL);
            SQLBindCol(sqlStmtHandle, 2, SQL_C_SLONG, &contor, sizeof(contor), NULL);
            while (SQLFetch(sqlStmtHandle) == SQL_SUCCESS) {
                std::string text_raspuns = converter.to_bytes(w_text_raspuns);
                raspunsuri_contori[text_raspuns] = contor;
            }
            SQLCloseCursor(sqlStmtHandle);
            SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        }

        int total = 0;
        for (const auto& pair : raspunsuri_contori) {
            total += pair.second;
        }

        for (const auto& pair : raspunsuri_contori) {
            float procentaj = (total > 0) ? (float(pair.second) / total) * 100 : 0;
            result += "|" + text_intrebare + "*";
            result += pair.first + "+" + std::to_string(procentaj) + "%";
        }

    }
    if (!result.empty()) {
        result.pop_back(); 
    }

    return result;
}

std::string BazaDeDate::getIntrebareText(SQLINTEGER id_intrebare)
{
    std::string intrebare_text;

    SQLWCHAR sql_query[255];
    swprintf(sql_query, 255, L"SELECT Text FROM Intrebari WHERE ID = %d", id_intrebare);

    SQLHSTMT sqlStmtHandle;
    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
        printf("Error allocating statement handle\n");
        return intrebare_text;
    }

    SQLRETURN retcode = SQLExecDirect(sqlStmtHandle, sql_query, SQL_NTS);
    if (retcode != SQL_SUCCESS) {
        printf("Error executing SQL query\n");
        printSQLError(sqlStmtHandle, SQL_HANDLE_STMT);
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return intrebare_text;
    }

    SQLCHAR intrebare[255];
    if (SQLFetch(sqlStmtHandle) == SQL_SUCCESS) {
        SQLGetData(sqlStmtHandle, 1, SQL_C_CHAR, intrebare, sizeof(intrebare), NULL);
        intrebare_text = std::string((char*)intrebare);
    }

    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
    return intrebare_text;
}

std::string BazaDeDate::getRaspunsText(SQLINTEGER id_intrebare)
{
    std::string raspuns_text;

    SQLWCHAR sql_query[255];
    swprintf(sql_query, 255, L"SELECT Text FROM Raspunsuri WHERE ID = %d", id_intrebare);

    SQLHSTMT sqlStmtHandle;
    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
        printf("Error allocating statement handle\n");
        return raspuns_text;
    }

    SQLRETURN retcode = SQLExecDirect(sqlStmtHandle, sql_query, SQL_NTS);
    if (retcode != SQL_SUCCESS) {
        printf("Error executing SQL query\n");
        printSQLError(sqlStmtHandle, SQL_HANDLE_STMT);
        SQLCloseCursor(sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return raspuns_text;
    }

    SQLCHAR raspuns[255];
    if (SQLFetch(sqlStmtHandle) == SQL_SUCCESS) {
        SQLGetData(sqlStmtHandle, 1, SQL_C_CHAR, raspuns, sizeof(raspuns), NULL);
        raspuns_text = std::string((char*)raspuns);
    }

    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
    return raspuns_text;
}

int BazaDeDate::GetFormIDFromDatabase(const std::string& form_name)
{
    int form_id = -1;
    SQLHSTMT sqlStmtHandle;
    SQLRETURN retcode;

    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle)) {
        printf("Error allocating statement handle\n");
        return -1;
    }

    wchar_t wform_name[255];
    size_t convertedChars = 0;
    mbstowcs_s(&convertedChars, wform_name, sizeof(wform_name) / sizeof(wchar_t), form_name.c_str(), _TRUNCATE);
    SQLWCHAR sql_query[255];
    swprintf((wchar_t*)sql_query, 255, L"SELECT ID FROM Formulare WHERE Titlu = '%s'", wform_name);

    retcode = SQLExecDirect(sqlStmtHandle, sql_query, SQL_NTS);
    if (retcode == SQL_SUCCESS) {
        SQLINTEGER id;
        SQLLEN indicator;
        retcode = SQLBindCol(sqlStmtHandle, 1, SQL_C_SLONG, &id, sizeof(id), &indicator);
        retcode = SQLFetch(sqlStmtHandle);
        if (retcode == SQL_SUCCESS) {
            form_id = static_cast<int>(id);
        }
    }

    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    return form_id;
}

int BazaDeDate::getUserID(const std::string& username)
{
    std::lock_guard<std::mutex> lock(db_mutex);

    SQLHANDLE sqlStmtHandle;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wusername = converter.from_bytes(username);

    SQLWCHAR sqlQuery[SQL_RESULT_LEN];
    swprintf(sqlQuery, SQL_RESULT_LEN, L"SELECT ID FROM Utilizator WHERE Nume = '%s'", wusername.c_str());

    SQLRETURN ret = SQLExecDirect(sqlStmtHandle, sqlQuery, SQL_NTS);

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return -1;
    }

    SQLINTEGER user_id;
    SQLFetch(sqlStmtHandle);
    SQLGetData(sqlStmtHandle, 1, SQL_C_SLONG, &user_id, sizeof(user_id), NULL);
    SQLCloseCursor(sqlStmtHandle);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);

    return user_id;
}

