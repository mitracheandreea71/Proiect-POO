
#include <iostream>
#include <string>

#include "Server.h"
#include "BazaDeDate.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment(lib, "ws2_32.lib") 

using namespace std;

int main() {

    Server& server = Server::getInstance(8080);
    server.start();

    WSACleanup();

    return 0;
}