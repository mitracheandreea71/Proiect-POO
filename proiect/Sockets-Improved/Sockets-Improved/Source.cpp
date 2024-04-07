#undef UNICODE
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN


#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string>

#include "BazaDeDate.h"

int main() {
    WSADATA wsaData;
    int iResult;

    // Initializare Winsock
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    
    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in coming_addr;

    // Structura sockaddr_in e folosita pentru a specifica adresa IP
    // si portul pe care ascultam
    struct sockaddr_in saServer;
    saServer.sin_family = AF_INET;
    // 0.0.0.0 inseamna orice IP 
    inet_pton(AF_INET, "0.0.0.0", &saServer.sin_addr);
    printf("Starting server \n");
    saServer.sin_port = htons(12345);

    bind(listen_sock, (sockaddr*)&saServer, sizeof(sockaddr_in));
    listen(listen_sock, SOMAXCONN);

    int size = sizeof(coming_addr);
    SOCKET client_sock = accept(listen_sock,(sockaddr*) &coming_addr, &size);
    if (client_sock == INVALID_SOCKET)
    {
		printf("connect function failed with error: %ld\n", WSAGetLastError());
        return -1;
    }
    else {
        char buffer[36];
        inet_ntop(AF_INET, &coming_addr.sin_addr.s_addr, buffer, 36);
        printf("%s connected!\n", buffer);
    }

    BazaDeDate& bazaDeDate = BazaDeDate::getInstance();

    wchar_t ID[1024], Nume[1024], Prenume[1024], Salariu[1024];
    std::wstring message;

    message = L"Enter ID: ";
    send(client_sock, reinterpret_cast<const char*>(message.c_str()), message.size() * sizeof(wchar_t), 0);
    recv(client_sock, reinterpret_cast<char*>(ID), sizeof(ID), 0);

    message = L"Enter Nume: ";
    send(client_sock, reinterpret_cast<const char*>(message.c_str()), message.size() * sizeof(wchar_t), 0);
    recv(client_sock, reinterpret_cast<char*>(Nume), sizeof(Nume), 0);

    message = L"Enter Prenume: ";
    send(client_sock, reinterpret_cast<const char*>(message.c_str()), message.size() * sizeof(wchar_t), 0);
    recv(client_sock, reinterpret_cast<char*>(Prenume), sizeof(Prenume), 0);

    message = L"Enter Salariu: ";
    send(client_sock, reinterpret_cast<const char*>(message.c_str()), message.size() * sizeof(wchar_t), 0);
    recv(client_sock, reinterpret_cast<char*>(Salariu), sizeof(Salariu), 0);

    std::wstring wID(ID);
    std::wstring wNume(Nume);
    std::wstring wPrenume(Prenume);
    std::wstring wSalariu(Salariu);

    int id = std::stoi(wID);
    double salariu = std::stod(wSalariu);

    bazaDeDate.insertData(id, wNume, wPrenume, salariu);

    
    return 0;
}