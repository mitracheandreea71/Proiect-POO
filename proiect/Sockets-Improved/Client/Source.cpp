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

int main() {
	WSADATA wsaData;
	int iResult;

	// Initializare Winsock
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKET client_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	sockaddr_in clientService;
	clientService.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &clientService.sin_addr.s_addr);
	clientService.sin_port = htons(12345);
	int ret = connect(client_sock, (sockaddr*)&clientService, sizeof(clientService));
	if (ret == SOCKET_ERROR) {
		printf("connect function failed with error: %ld\n", WSAGetLastError());
		
	}

	char buffer[1024];
	for (;;) {
		fgets(buffer, 1024, stdin);
		send(client_sock, buffer, strlen(buffer) + 1, 0);
	}


	return 0;
}