#pragma once

#include <iostream>
#include <windows.h>
#include <mutex>
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>
#include <vector>
#include <map>


#define SQL_RESULT_LEN 240
#define SQL_RETURN_CODE_LEN 1024

class BazaDeDate
{
private:
	static SQLHANDLE sqlConnHandle;
	static SQLHANDLE sqlStmtHandle;
	static SQLHANDLE sqlEnvHandle;
	static SQLWCHAR retconstring[SQL_RETURN_CODE_LEN];

	static std::mutex db_mutex;

	BazaDeDate();
	static BazaDeDate* instance;
	~BazaDeDate() {};

	bool getRoleID(const std::wstring& role, int& roleID);


public:
	static BazaDeDate&		getInstance();
	static void				destroyInstance();
	void					insertData(const char* message);
	void					execute(const std::string& query, const std::vector<std::string>& params);
	void					bindParameters(SQLHSTMT sqlStmtHandle, const std::vector<std::wstring>& params);
	bool					createSondaj(const std::string& form_title, const std::vector<std::string>& questions);
	bool					addUser(const std::string& username, const std::string& hashedPassword, const std::string& role, const std::string& organization);
	bool					verifyPassword(const std::string& username, const std::string& hashedPassword);
	bool					addUserToOrganization(const std::string& username, const std::string& org_name);
	bool					organizationExists(const std::string& org_name);
	bool					isManager(const std::string& username);
	bool					createOrganization(const std::string& org_name, int manager_id);
	bool					setUserOrganization(const std::string& username, const std::string& organization);
	bool					addUserToOrganizationMapping(const std::string& username, const std::string& organization);
	bool					addRaspuns(int intrebare_id, const std::string& raspuns);
	bool					addFormToUser(int form_id, int user_id);
	bool					isFormTitleUnique(const std::string& form_title);
	bool					addUserResponse(int id_utilizator, int id_intrebare, int raspuns, int id_formular);
	bool					setFormCompleted(int id_formular, int id_utilizator);
	bool					hasUserCompletedForm(int id_utilizator, int id_formular);
	bool					updateFormType(int form_id, const std::string& new_type);
	bool					setArchiveRequestStatus(int form_id, int status);
	bool					archive(const std::string& titlu);
	bool					addDeleteRequest(int form_id, int user_id);
	bool					approveDeleteRequest(int form_id);
	bool					deleteRequestExists(int form_id);
	bool					isFormArchived(int form_id);
	bool					recoverForm(int form_id);
	std::string				getOrganizationByManager(const std::string& manager_username);
	std::string				GetUserType(const std::string& username);
	std::string				GetFormAndAnswersFromDatabase(const std::string& form_name);
	std::string				getStatistics(const std::string& nume_formular);
	std::string				getIntrebareText(SQLINTEGER id_intrebare);
	std::string				getRaspunsText(SQLINTEGER id_intrebare);
	int						GetFormIDFromDatabase(const std::string& form_name);
	int						getUserID(const std::string& username);
	int						userExists(const std::string& username);
	int						addIntrebare(int form_id, const std::string& intrebare);
	int						createForm(const std::string& titlu, const std::string& tip_formular, const std::string& stare, int id_manager);
	int						getOrganizationIDByManager(const std::string& managerUsername);
	int						getIntrebareID(const std::string& text_intrebare);
	int						getResponseID(const std::string& text_raspuns);
	int						getResponseID(int id_intrebare, const std::string& raspuns);
	int						getResponseID(int id_intrebare, int id_utilizator, int id_formular);
	std::vector<int>        getSondajStatistics(int sondaj_id);
	std::vector<int>		getOrganizationMembersForManager(int manager_id);
	std::vector<int>		getUnarchivedFormIDs();
	std::vector<std::string>getOrganizationMembers(int organizationID);
	std::vector<std::string>getUncompletedForms(const std::string& username);
	std::vector<std::string>getCompletedForms(const std::string& username);
	std::vector<std::string>getCreatedForms(const std::string& username);
	std::vector<std::string>getFormTitlesByIDs(const std::vector<int>& form_ids);
	std::vector<std::pair<std::string, std::string>>getUserResponsesForForm(int id_utilizator, int id_formular);
	std::vector<std::pair<int, std::string>>		getPendingDeleteRequests();
	std::vector<std::map<std::string, std::string>> query(const std::string& query, const std::vector<std::string>& params);
	std::pair<int, int>     getIDs(const std::string& titlu);
};

