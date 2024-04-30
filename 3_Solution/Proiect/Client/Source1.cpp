#define _WINSOCK_DEPRECATED_NO_WARNINGS
#undef UNICODE
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <string>
#include <sstream>
#pragma comment(lib, "ws2_32.lib")
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")



int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) {
        printf("Eroare la crearea socketului clientului!\n");
        return 1;
    }

    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(54000);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    int connect_result = connect(client_socket, (sockaddr*)&server_address, sizeof(server_address));
    if (connect_result == SOCKET_ERROR) {
        printf("Eroare la conectarea la server!\n");
        return 1;
    }

    while (true) {
        char message[200];

        printf("Introduceti numele (sau 'quit' pentru a iesi): ");
        gets_s(message);
        if (strcmp(message, "quit") == 0) {
            break;
        }

        int bytes_sent = send(client_socket, message, strlen(message), 0);
        if (bytes_sent == SOCKET_ERROR) {
            printf("Eroare la trimiterea mesajului!\n");
            break;
        }


        char response[1200];
        int bytes_received = recv(client_socket, response, sizeof(response) - 1, 0);
        if (bytes_received > 0) {
            response[bytes_received] = '\0';
            // Imparte răspunsul în funcție de delimitatorul '\n'
            std::istringstream iss(response);
            std::string line;
            while (std::getline(iss, line, '\n')) {
                printf("Raspuns de la server: %s\n", line.c_str());
            }
        }


    }

    closesocket(client_socket);
    WSACleanup();
    return 0;
}
