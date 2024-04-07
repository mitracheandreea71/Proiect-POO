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

	wchar_t buffer[1024];
	wchar_t input[1024];  

	for (;;) {
		int bytesReceived = recv(client_sock, reinterpret_cast<char*>(buffer), 1024 * sizeof(wchar_t), 0);
		if (bytesReceived > 0) {
			buffer[bytesReceived / sizeof(wchar_t)] = L'\0'; // Null-terminate the received message
			wprintf(L"Received from server: %s\n", buffer);
		}

		fgetws(input, 1024, stdin); 
		send(client_sock, reinterpret_cast<char*>(input), wcslen(input) * sizeof(wchar_t) + sizeof(wchar_t), 0);
	}



	return 0;
}