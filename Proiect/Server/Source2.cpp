#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif
#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>
#include <string>
#include <codecvt>
#include <thread>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment(lib, "ws2_32.lib") 

using namespace std;

#define SQL_RESULT_LEN 240
#define SQL_RETURN_CODE_LEN 1024

void SendMessageToClient(int client_socket, const char* message) {
    int bytes_sent = send(client_socket, message, strlen(message), 0);
    if (bytes_sent == SOCKET_ERROR) {
        printf("Eroare la trimiterea mesajului catre clientul %d!\n", client_socket);
    }
}

void InsertIntoDatabase(const char* message) {
    SQLHANDLE sqlConnHandle = NULL;
    SQLHANDLE sqlStmtHandle = NULL;
    SQLHANDLE sqlEnvHandle = NULL;
    SQLWCHAR retconstring[SQL_RETURN_CODE_LEN];

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
        (SQLWCHAR*)L"DRIVER={SQL Server};SERVER=LAPTOP-4TB37NLA\\MSSQLSERVER04;DATABASE=proiect;Trusted_Connection=yes;",
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

    std::wstring wmessage = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(message);
    SQLWCHAR* sqlQuery = (SQLWCHAR*)L"INSERT INTO tabela (nume) VALUES (?)";

    if (SQL_SUCCESS != SQLPrepare(sqlStmtHandle, sqlQuery, SQL_NTS)) {
        printf("Error preparing SQL query\n");
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        SQLDisconnect(sqlConnHandle);
        SQLFreeHandle(SQL_HANDLE_DBC, sqlConnHandle);
        SQLFreeHandle(SQL_HANDLE_ENV, sqlEnvHandle);
        return;
    }

    SQLLEN cbNume = SQL_NTS;
    SQLBindParameter(sqlStmtHandle, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 50, 0, (SQLPOINTER)wmessage.c_str(), 0, &cbNume);

    if (SQL_SUCCESS != SQLExecute(sqlStmtHandle)) {
        printf("Error executing SQL statement\n");
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        SQLDisconnect(sqlConnHandle);
        SQLFreeHandle(SQL_HANDLE_DBC, sqlConnHandle);
        SQLFreeHandle(SQL_HANDLE_ENV, sqlEnvHandle);
        return;
    }

    printf("Successfully inserted data into the database\n");

    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
    SQLDisconnect(sqlConnHandle);
    SQLFreeHandle(SQL_HANDLE_DBC, sqlConnHandle);
    SQLFreeHandle(SQL_HANDLE_ENV, sqlEnvHandle);
}

void HandleClientConnection(int client_socket) {
    while (true) {
        int bytes_received;
        char message[1024];

        bytes_received = recv(client_socket, message, sizeof(message), 0);
        if (bytes_received == SOCKET_ERROR) {
            printf("Eroare la receptionarea mesajului de la clientul %d!\n", client_socket);
            closesocket(client_socket);
            return;
        }
        else if (bytes_received == 0) {
            printf("Clientul %d s-a deconectat!\n", client_socket);
            closesocket(client_socket);
            return;
        }

        message[bytes_received] = '\0';
        printf("Mesaj receptionat de la clientul %d: %s\n", client_socket, message);

        InsertIntoDatabase(message);

        SendMessageToClient(client_socket, "Mesajul a fost adaugat in baza de date.");
        SendMessageToClient(client_socket, "Mesajul a fost primit de server.");
    }

    closesocket(client_socket);
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("Eroare la crearea socketului serverului!\n");
        WSACleanup();
        return 1;
    }

    int reuseaddr = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuseaddr, sizeof(reuseaddr));

    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(54000);
    server_address.sin_addr.s_addr = INADDR_ANY;

    bind(server_socket, (sockaddr*)&server_address, sizeof(server_address));
    listen(server_socket, SOMAXCONN);

    while (true) {
        sockaddr_in client_address;
        int client_socket = accept(server_socket, (sockaddr*)&client_address, NULL);
        if (client_socket == INVALID_SOCKET) {
            printf("Eroare la acceptarea conexiunii!\n");
            continue;
        }

        printf("Client conectat de la %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

        std::thread client_thread(HandleClientConnection, client_socket);
        client_thread.detach();
    }

    closesocket(server_socket);
    WSACleanup();

    return 0;
}
