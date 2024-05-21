#pragma once

#include <thread>
#include <winsock2.h>
#include <iostream>
#include <windows.h> 
#include <map>
#include <vector>

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
	static Server&				getInstance(int port);
	static void					destroyInstance();
	void						start();
	void						HandleClientConnection(int client_socket);
	void						HandleRegistration(int client_socket, const std::string& message); 
	void						HandleAuthentication(int client_socket, const std::string& message);  
	void						SendMessageToClient(int client_socket, const char* message);
	void						HandleAddUserToOrganization(int client_socket, const std::string& content);
	void						HandleJoinOrganization(int client_socket, const std::string& content);
	void						HandleCreateForm(int client_socket, const std::string& content);
	void						HandleGetType(int client_socket, const std::string& username);
	void						HandleGetUncompletedForms(int client_socket, const std::string& username);
	void						HandleGetCompletedForms(int client_socket, const std::string& username);
	void						HandleGetCreatedForms(int client_socket, const std::string& username);
	void						HandleGetForm(int client_socket, const std::string& titlu);
	void						HandleGetFormByMe(int client_socket, const std::string& content);
	void						HandleCompleteForm(int client_socket, const std::string& content);
	void						HandleGetStatistic(int client_socket, const std::string& content);
	void						HandleArchive(int client_socket, const std::string& content);
	void						HandleAllowArchive(int client_socket, const std::string& content);
	void						HandleViewArchive(int client_socket, const std::string& content);
	void						HandleDelete(int client_socket, const std::string& content);
	void						HandleAdminApproveDelete(int client_socket, const std::string& content);
	void						HandleAdminApproveDeleteRequest(int client_socket, const std::string& content);
	void						HandleRecover(int client_socket, const std::string& content);
	void						PrintClientSockets();
	void						SendFormToOrganizationMembers(const std::string& formContent, const std::vector<std::string>& members);						
	int							GetClientSocketByUsername(const std::string& username);
	std::string					extractUsername(const std::string& message);
	std::string					extractUsernameT(const std::string& message);
	std::string					extractPassword(const std::string& message);
	std::string					extractRole(const std::string& message);
	std::string					extractAdminPassword(const std::string& message);
	std::string					extractMessageCode(const std::string& message);
	std::string					extractMessageContent(const std::string& message);
	std::string					extractTargetUsername(const std::string& message);
	std::string					extractOrganizationName(const std::string& message);
	std::string					extractFormType(const std::string& content);
	std::string					extractFormTitle(const std::string& content);
	std::string					extractFormTitleC(const std::string& content);
	std::string					GetUserType(const std::string& username);
	std::vector<std::string>	extractQuestions(const std::string& content);
	std::vector<std::string>	GetOrganizationMembersForManager(const std::string& managerUsername);
	std::vector<std::pair<std::string, std::vector<std::string>>> extractQuestionsAndResponses(const std::string& content);
	std::vector<std::pair<std::string, std::vector<std::string>>> extractQuestionsSondaje(const std::string& content);
	std::vector<std::pair<int, std::string>>					  extractResponses(const std::string& content);
	
};
