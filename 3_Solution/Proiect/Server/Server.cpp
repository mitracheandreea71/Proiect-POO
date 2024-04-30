#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#include "Server.h"
#include <stdexcept>
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>
#include <sstream>
#include <cstdlib> 
#include <fstream>

#include "BazaDeDate.h"
#include "CInregistrare.h"
#include "Autentificare.h"

Server* Server::instance = nullptr;

Server::Server(int port):port(port)
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        WSACleanup();
        throw std::runtime_error("Eroare la crearea socketului serverului");
    }

    int reuseaddr = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuseaddr, sizeof(reuseaddr));

    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    bind(server_socket, (sockaddr*)&server_address, sizeof(server_address));
    listen(server_socket, SOMAXCONN);
}

Server& Server::getInstance(int port)
{
    if (instance == nullptr) {
        instance = new Server(port);

    }
    return *instance;
}

void Server::destroyInstance()
{
    if (instance == nullptr) {
        return;
    }
    delete instance;
    instance = nullptr;
}

void Server::start()
{
    while (true) {
        sockaddr_in client_address;
        int client_socket = accept(server_socket, (sockaddr*)&client_address, NULL);
        if (client_socket == INVALID_SOCKET) {
            printf("Eroare la acceptarea conexiunii!\n");
            continue;
        }

        printf("Client conectat de la %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

        std::thread client_thread(&Server::HandleClientConnection, this, client_socket);

        client_thread.detach();
    }
}

void Server::HandleClientConnection(int client_socket)
{
        while (true) {
            char message[1024];

            int bytes_received = recv(client_socket, message, sizeof(message), 0);
            if (bytes_received == SOCKET_ERROR) {
                printf("Eroare la receptionarea mesajului de la clientul %d!\n", client_socket);
                closesocket(client_socket);
                return;
            }

            if (bytes_received == 0) {
                printf("Clientul %d s-a deconectat!\n", client_socket);
                closesocket(client_socket);
                return;
            }

            message[bytes_received] = '\0';
            printf("Mesaj receptionat de la clientul %d: %s\n", client_socket, message);
            std::string buffer(message);
            std::string code = extractMessageCode(buffer);  // Extrage codul mesajului
            std::string content = extractMessageContent(buffer);

            if (code == "R") {  // Mesaj de Inregistrare
                HandleRegistration(client_socket, content);
            }
            else if (code == "A") {  // Mesaj de autentificare
                HandleAuthentication(client_socket, content);
            }
            else if (code == "ROLE") {  // Mesaj de selectie a rolului
                //HandleRoleSelection(client_socket, content);
            }
            else {
                // Cod de mesaj necunoscut
                printf("Mesaj necunoscut de la clientul %d!\n", client_socket);
                SendMessageToClient(client_socket, "Mesaj necunoscut.");
            }

        }

        closesocket(client_socket);
}

std::string readAdminPasswordFromFile(const std::string& filename) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Eroare la deschiderea fișierului " << filename << std::endl;
        return std::string();
    }

    std::string admin_password;
    std::getline(file, admin_password); // Citește prima linie din fișier

    file.close(); // Asigură-te că închizi fișierul
    return admin_password;
}

void Server::HandleRegistration(int client_socket, const std::string& content)
{
    std::string username = extractUsername(content);
    std::string password = extractPassword(content);
    std::string role     = extractRole(content);

    if (role == "admin") {
        // Citește parola de administrator din fișier
        std::string admin_password = readAdminPasswordFromFile("admin_password.txt");

        if (admin_password.empty()) {
            std::cerr << "Parola pentru administrator nu este disponibila. Contactati administratorul." << std::endl;
            SendMessageToClient(client_socket, "Eroare interna. Contactati administratorul.");
            return;
        }

        std::string entered_admin_password = extractAdminPassword(content);

        if (entered_admin_password != admin_password) {
            SendMessageToClient(client_socket, "Parola pentru administrator este incorecta.");
            printf("%s", "Parola pentru administrator este incorecta.");
            return;
        }
    }



    CInregistrare registration;
    if (registration.registerUser(username, password, role)) {
        printf("Utilizatorul %s a fost inregistrat.\n", username.c_str());
        SendMessageToClient(client_socket, "Inregistrare reusita.");
    }
    else {
        SendMessageToClient(client_socket, "Inregistrare esuata. Nume de utilizator indisponibil.");
    }
}

void Server::HandleAuthentication(int client_socket, const std::string& content)
{
    // Extrage detaliile necesare și autentifică utilizatorul
    std::string username = extractUsername(content);
    std::string password = extractPassword(content);

    Autentificare auth;
    if (auth.autentificareUser(username, password)) {
        SendMessageToClient(client_socket, "Autentificare reusita.");
    }
    else {
        SendMessageToClient(client_socket, "Autentificare esuata. Nume de utilizator sau parola incorecte.");
    }
}


void Server::SendMessageToClient(int client_socket, const char* message)
{
    int bytes_sent = send(client_socket, message, strlen(message), 0);
    if (bytes_sent == SOCKET_ERROR) {
        printf("Eroare la trimiterea mesajului catre clientul %d!\n", client_socket);
    }
}

std::string Server::extractUsername(const std::string& message)
{
    std::stringstream ss(message); 
    std::string token;

    std::getline(ss, token, '|'); // Ignoram prima parte ('R'/'A')
    //std::getline(ss, token, '|'); // Aici este numele de utilizator

    return token;
}

std::string Server::extractPassword(const std::string& message)
{
    std::stringstream ss(message);
    std::string token;

    std::getline(ss, token, '|'); // Ignoram prima parte ('R'/'A')
    std::getline(ss, token, '|'); // Ignoram numele de utilizator
    //std::getline(ss, token, '|'); // Aici este parola

    return token;
}

std::string Server::extractRole(const std::string& message)
{

    std::stringstream ss(message);
    std::string token;

    std::getline(ss, token, '|'); // Ignoram prima parte ('R'/'A')
    std::getline(ss, token, '|'); // Ignoram numele de utilizator
    std::getline(ss, token, '|'); // Ignaram parola
    //std::getline(ss, token, '|'); // Aici este rolul
     
    return token;
}

std::string Server::extractAdminPassword(const std::string& message)
{
    std::stringstream ss(message);
    std::string token;

    std::getline(ss, token, '|'); // Ignoră prima parte ('R'/'A')
    std::getline(ss, token, '|'); // Ignoră numele de utilizator
    std::getline(ss, token, '|'); // Ignoră parola obișnuită
    std::getline(ss, token, '|'); // Ignoră rolul
    //std::getline(ss, token, '|'); // Aici este parola specifică pentru admin

    return token;
}

std::string Server::extractMessageCode(const std::string& message)
{
    std::stringstream ss(message);
    std::string code;
    std::getline(ss, code, '|'); // Extrage partea dinaintea primei delimitări
    return code;
}

std::string Server::extractMessageContent(const std::string& message)
{
    std::stringstream ss(message);
    std::string content;
    std::getline(ss, content, '|'); // Ignoră codul
    std::getline(ss, content, '\0'); // Extrage partea de conținut
    return content;
}
