#pragma once

#include <thread>
#include <winsock2.h>
#include <iostream>
#include <windows.h> 
#include <map>

class BazaDeDate;

class Server
{
private:
	Server(int port);
	static Server* instance;

	int server_socket;
	int port;

	~Server() {};

public:
	static Server&		getInstance(int port);
	static void			destroyInstance();
	void				start();
	void				HandleClientConnection(int client_socket);
	void				HandleRegistration(int client_socket, const std::string& message);  // Gestionarea înregistrării
	void			    HandleAuthentication(int client_socket, const std::string& message);  // Gestionarea autentificării
	//void				HandleRoleSelection(int client_socket, const std::string& message);
	void				SendMessageToClient(int client_socket, const char* message);
	std::string			extractUsername(const std::string& message);
	std::string			extractPassword(const std::string& message);
	std::string			extractRole(const std::string& message);
	std::string			extractAdminPassword(const std::string& message);
	std::string			extractMessageCode(const std::string& message);
	std::string			extractMessageContent(const std::string& message);
	
};
