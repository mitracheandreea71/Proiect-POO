#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#include "Server.h"
#include "LogareActiuni.h"
#include "BazaDeDate.h"
#include "CInregistrare.h"
#include "Autentificare.h"
#include "IFormular.h"
#include "CChestionar.h"
#include "CSondaj.h"
#include "Admin.h"
#include "Manager.h"
#include "User.h"
#include "IExceptie.h"

#include <stdexcept>
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>
#include <sstream>
#include <cstdlib> 
#include <fstream>
#include <vector>
#include <utility>
#include <memory>
#include <unordered_map>

Server* Server::instance = nullptr;

using ClientSocketMap = std::unordered_map<std::string, int>; // socket-ul clientului este reprezentat printr-un int

ClientSocketMap clientSockets;

Server::Server(int port) :port(port)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    try {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            logger.log("Eroare la initializarea WSA");
            throw ExceptieTest(WSAGetLastError(), "Eroare la initializarea WSA");
        }

        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == INVALID_SOCKET) {
            WSACleanup();
            logger.log("Eroare la crearea socketului serverului");
            throw ExceptieTest(WSAGetLastError(), "Eroare la crearea socketului serverului");
        }

        int reuseaddr = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuseaddr, sizeof(reuseaddr));

        sockaddr_in server_address;
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(port);
        server_address.sin_addr.s_addr = INADDR_ANY;

        int bind_result = bind(server_socket, (sockaddr*)&server_address, sizeof(server_address));
        if (bind_result == SOCKET_ERROR) {
            int error_code = WSAGetLastError();
            logger.log("Eroare la bind! Cod eroare: " + std::to_string(error_code));
            throw ExceptieTest(error_code, "Eroare la bind!");
        }

        int listen_result = listen(server_socket, SOMAXCONN);
        if (listen_result == SOCKET_ERROR) {
            int error_code = WSAGetLastError();
            logger.log("Eroare la listen! Cod eroare: " + std::to_string(error_code));
            throw ExceptieTest(error_code, "Eroare la listen!");
        }
    }
    catch (ExceptieTest& e) {
        e.print();
        
        throw; 
    }
    catch (std::exception& e) {
        logger.log("Standard exception: " + std::string(e.what()));
        throw;
    }
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
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    while (true) {
        sockaddr_in client_address;
        int client_socket = accept(server_socket, (sockaddr*)&client_address, NULL);
        if (client_socket == INVALID_SOCKET) {
            int error_code = WSAGetLastError(); 
            logger.log("Eroare la acceptarea conexiunii!\n");
            printf("Eroare la acceptarea conexiunii! Cod eroare: %d\n", error_code);
            continue;
        }
        logger.log("Client conectat de la ");
        logger.log(inet_ntoa(client_address.sin_addr));
        printf("Client conectat de la %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

        std::thread client_thread(&Server::HandleClientConnection, this, client_socket);

        client_thread.detach();
    }
}

void Server::HandleClientConnection(int client_socket)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    while (true) {
        char message[1024];

        int bytes_received = recv(client_socket, message, sizeof(message), 0);
        if (bytes_received == SOCKET_ERROR) {
            logger.log("Eroare la receptionarea mesajului de la client\n");
            printf("Eroare la receptionarea mesajului de la client\n", client_socket);
            closesocket(client_socket);
            return;
        }

        if (bytes_received == 0) {
            logger.log(std::to_string(client_socket));
            logger.log("Clientul s-a deconectat!\n");
            printf("Clientul %d s-a deconectat!\n", client_socket);
            closesocket(client_socket);
            return;
        }

        message[bytes_received] = '\0';
        logger.log("Mesaj receptionat de la clientul");
        logger.log(std::to_string(client_socket));
        logger.log(message);
        logger.log("\n");
        printf("Mesaj receptionat de la clientul %d: %s\n", client_socket, message);
        std::string buffer(message);
        std::string code = extractMessageCode(buffer);
        std::string content = extractMessageContent(buffer);
        std::string username = extractUsername(buffer);
        std::string usernameT = extractUsernameT(buffer);
        
        clientSockets[username] = client_socket;

        if (code == "R") {  // Mesaj de Inregistrare
            HandleRegistration(client_socket, content);
        }
        else if (code == "A") {  // Mesaj de autentificare
            HandleAuthentication(client_socket, content);
        }
        else if (code == "C") {
            HandleCreateForm(client_socket, content);
        }
        else if (code == "T") { //Mesaj pentru a intoarce clientului tipul de utilizator
            HandleGetType(client_socket, usernameT);
        }
        else if (code == "FormulareNecompletate") {  // Mesaj de formulare necompletate
            HandleGetUncompletedForms(client_socket, usernameT);
        }
        else if (code == "FormulareCompletate") {  
            HandleGetCompletedForms(client_socket, usernameT);
        }
        else if (code == "FormulareCreate") {  
            HandleGetCreatedForms(client_socket, usernameT);
        }
        else if (code == "FormularCompletatDeMine") { 
            HandleGetFormByMe(client_socket, content);
        }
        else if (code == "Vizualizareformular") { //Mesaj pentru a vedea formularul complet
            std::string titlu = extractUsernameT(buffer);
            HandleGetForm(client_socket, titlu);
        }
        else if (code == "Completeaza") {  
            HandleCompleteForm(client_socket, content);
        }
        else if (code == "Statistica") { 
            HandleGetStatistic(client_socket, content);
        }
        else if (code == "CerereArhivare") {
            HandleArchive(client_socket, content);
        }
        else if (code == "AprobaArhivare") { 
            HandleAllowArchive(client_socket, content);
        }
        else if (code == "VizualizareCereriArhivare") {  
            HandleViewArchive(client_socket, content);
        }
        else if (code == "CerereStergere") {  
            HandleDelete(client_socket, content);
        }
        else if (code == "VizualizareStergere") {  
            HandleAdminApproveDelete(client_socket, content);
        }
        else if (code == "AprobaStergere") {  
            HandleAdminApproveDeleteRequest(client_socket, content);
        }
        else if (code == "Recupereaza") {  
            HandleRecover(client_socket, content);
        }
        else {
            logger.log("Mesaj necunoscut de la clientul");
            logger.log(std::to_string(client_socket));
            logger.log("\n");
            printf("Mesaj necunoscut de la clientul %d!\n", client_socket);
            SendMessageToClient(client_socket, "Mesaj necunoscut.");
        }
    }
    closesocket(client_socket);
}

std::string readAdminPasswordFromFile(const std::string& filename) {

    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    std::ifstream file(filename);

    if (!file.is_open()) {
        logger.log("Eroare la deschiderea fisierului pentru administratori.\n");
        std::cerr << "Eroare la deschiderea fisierului " << filename << std::endl;
        return std::string();
    }

    std::string admin_password;
    std::getline(file, admin_password);

    file.close();
    return admin_password;
}

void Server::HandleRegistration(int client_socket, const std::string& content)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    std::string username = extractUsername(content);
    std::string password = extractPassword(content);
    std::string role = extractRole(content);
    std::string organization = extractOrganizationName(content);

    std::unique_ptr<IPersoana> persoana;

    CInregistrare registration;
    BazaDeDate& db = BazaDeDate::getInstance();

    std::string hashedPassword = registration.hashPassword(password);

    if (role == "admin") {
        std::string admin_password = readAdminPasswordFromFile("admin_password.txt");

        if (admin_password.empty()) {
            std::cerr << "Parola pentru administrator nu este disponibila. Contactati administratorul." << std::endl;
            logger.log(std::to_string(client_socket));
            logger.log("Eroare interna. Contactati administratorul.\n");
            SendMessageToClient(client_socket, "Eroare interna. Contactati administratorul.");
            return;
        }

        std::string entered_admin_password = extractAdminPassword(content);

        if (entered_admin_password != admin_password) {
            logger.log(std::to_string(client_socket));
            logger.log("Parola pentru administrator este incorecta.\n");
            SendMessageToClient(client_socket, "Parola pentru administrator este incorecta.");
            printf("%s", "Parola pentru administrator este incorecta.");
            return;
        }else {
            if (db.addUser(username, hashedPassword, role, "administratori")) {
                logger.log(std::to_string(client_socket));
                logger.log("Inregistrare admin reusita\n");
                SendMessageToClient(client_socket, "Inregistrare admin reusita");
            }
            else {
                logger.log(std::to_string(client_socket));
                logger.log("Inregistrare esuata\n");
                SendMessageToClient(client_socket, "Inregistrare esuata");
            }
          
        }
        persoana = std::make_unique<Admin>(username, password);
    }else if (role == "manager") {
        int user_id = registration.registerUser(username, hashedPassword, role, organization);

        if (user_id >= 0) {  
            if (!db.organizationExists(organization)) {
                if (!db.createOrganization(organization, user_id)) {
                    logger.log(std::to_string(client_socket));
                    logger.log("Eroare la crearea organizatiei.\n");
                    SendMessageToClient(client_socket, "Eroare la crearea organizatiei.");
                    return;
                }
            }

            db.setUserOrganization(username, organization);
            db.addUserToOrganizationMapping(username, organization);
            logger.log("Managerul ");
            logger.log(username.c_str());
            logger.log("a fost inregistrat si organizatia a fost creata.\n");
            printf("Managerul %s a fost inregistrat si organizatia %s a fost creata.\n", username.c_str(), organization.c_str());
            SendMessageToClient(client_socket, "Inregistrare reusita si organizatie creata.");

            persoana = std::make_unique<Manager>(username, password);
            
        }
        else {
            logger.log(std::to_string(client_socket));
            logger.log("Inregistrare esuata. Nume de utilizator indisponibil\n");
            SendMessageToClient(client_socket, "Inregistrare esuata. Nume de utilizator indisponibil.");
        }
    }
    else {
        if (!db.organizationExists(organization)) {
            logger.log(std::to_string(client_socket));
            logger.log("Numele organizatiei nu exista.\n");
            SendMessageToClient(client_socket, "Numele organizatiei nu exista.");
            return;
        }
        else if(registration.registerUser(username, hashedPassword, role, organization)) {
            db.setUserOrganization(username, organization);
            db.addUserToOrganizationMapping(username, organization);
            logger.log("Utilizatorul ");
            logger.log(username.c_str());
            logger.log("a fost inregistrat.\n");
            printf("Utilizatorul %s a fost inregistrat.\n", username.c_str());
            SendMessageToClient(client_socket, "Inregistrare reusita.");

            persoana = std::make_unique<User>(username, password);
        }
        else {
            logger.log("Inregistrare esuata. Nume de utilizator indisponibil.\n");
            SendMessageToClient(client_socket, "Inregistrare esuata. Nume de utilizator indisponibil.");
        }
    }
}

void Server::HandleAuthentication(int client_socket, const std::string& content)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    std::string username = extractUsername(content);
    std::string password = extractPassword(content);

    Autentificare auth;
    if (auth.autentificareUser(username, password)) {
        logger.log(std::to_string(client_socket));
        logger.log("Autentificare reusita.");
        SendMessageToClient(client_socket, "Autentificare reusita.");
    }
    else {
        logger.log(std::to_string(client_socket));
        logger.log("Autentificare esuata. Nume de utilizator sau parola incorecte.");
        SendMessageToClient(client_socket, "Autentificare esuata. Nume de utilizator sau parola incorecte.");
    }
}

void Server::SendMessageToClient(int client_socket, const char* message)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    int bytes_sent = send(client_socket, message, strlen(message), 0);
    if (bytes_sent == SOCKET_ERROR) {
        logger.log("Eroare la trimiterea mesajului catre clientul");
        logger.log(std::to_string(client_socket));
        printf("Eroare la trimiterea mesajului catre clientul %d!\n", client_socket);
    }
}

void Server::HandleAddUserToOrganization(int client_socket, const std::string& content)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    std::string manager_username = extractUsername(content);
    std::string user_to_add = extractTargetUsername(content);

    BazaDeDate& db = BazaDeDate::getInstance();

    if (!db.isManager(manager_username)) {
        logger.log(std::to_string(client_socket));
        logger.log("Numai managerii pot adauga utilizatori.");
        SendMessageToClient(client_socket, "Numai managerii pot adauga utilizatori.");
        return;
    }

    std::string org_name = db.getOrganizationByManager(manager_username);

    if (org_name.empty()) {
        logger.log(std::to_string(client_socket));
        logger.log("Managerul nu are o organizatie asociata.");
        SendMessageToClient(client_socket, "Managerul nu are o organizatie asociata.");
        return;
    }

    if (db.addUserToOrganization(user_to_add, org_name)) {
        logger.log(std::to_string(client_socket));
        logger.log("Utilizator adaugat la organizatie.");
        SendMessageToClient(client_socket, "Utilizator adaugat la organizatie.");
    }
    else {
        logger.log(std::to_string(client_socket));
        logger.log("Eroare la adaugarea utilizatorului.");
        SendMessageToClient(client_socket, "Eroare la adaugarea utilizatorului.");
    }
}

void Server::HandleJoinOrganization(int client_socket, const std::string& content)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    std::string username = extractUsername(content);
    std::string org_name = extractOrganizationName(content);

    BazaDeDate& db = BazaDeDate::getInstance();

    if (!db.organizationExists(org_name)) {
        logger.log(std::to_string(client_socket));
        logger.log("Organizatia nu exista.\n");
        SendMessageToClient(client_socket, "Organizatia nu exista.");
        return;
    }

    if (db.addUserToOrganization(username, org_name)) {
        logger.log(std::to_string(client_socket));
        logger.log("Alaturare reusita.\n");
        SendMessageToClient(client_socket, "Alaturare reusita.");
    }
    else {
        logger.log(std::to_string(client_socket));
        logger.log("Eroare la alaturare.\n");
        SendMessageToClient(client_socket, "Eroare la alaturare.");
    }
}

void Server::HandleCreateForm(int client_socket, const std::string& content)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    std::string username = extractUsername(content);
    std::string tip_formular = extractFormType(content);
    std::string titlu = extractFormTitle(content);
    std::string stare = "activ";

    BazaDeDate& db = BazaDeDate::getInstance();
    int id_manager = db.getUserID(username);
    int form_id;

    if (db.isFormTitleUnique(titlu)) {
       
         form_id = db.createForm(titlu, stare, tip_formular, id_manager);
         logger.log(std::to_string(client_socket));
         logger.log("Formular creat cu succes.\n");
        SendMessageToClient(client_socket, "Formular creat cu succes.");
    }
    else {
        logger.log(std::to_string(client_socket));
        logger.log("Eroare: Numele formularului este deja folosit. Te rog să alegi alt nume.\n");
        SendMessageToClient(client_socket, "Eroare: Numele formularului este deja folosit. Te rog să alegi alt nume.");
        std::cout << "nume formular invalid";
        return;
    }

    std::vector<int> organizationMembers = db.getOrganizationMembersForManager(id_manager);

    IFormular* formular = nullptr;

    if (tip_formular == "chestionar") {

        formular = new CChestionar(titlu);
        CChestionar* chestionar = dynamic_cast<CChestionar*>(formular);
        if (chestionar) {
            std::vector<std::pair<std::string, std::vector<std::string>>> intrebariSiRaspunsuri = extractQuestionsAndResponses(content);

            for (const auto& pair : intrebariSiRaspunsuri) {
                int intrebare_id = db.addIntrebare(form_id, pair.first);

                for (const auto& raspuns : pair.second) {
                    db.addRaspuns(intrebare_id, raspuns);
                }

                chestionar->adaugaIntrebareSiRaspunsuri(pair.first, pair.second);
            }
           

            for (int user_id : organizationMembers) {
                db.addFormToUser(form_id, user_id);
            }
            logger.log(std::to_string(client_socket));
            logger.log("Chestionar realizat cu succes. Asteapta raspunsurile organizatiei pentru a le evalua.\n");
            SendMessageToClient(client_socket, "Chestionar realizat cu succes. Asteapta raspunsurile organizatiei pentru a le evalua.");
        }
    }
    else {
        formular = new CSondaj(titlu);
        CSondaj* sondaj = dynamic_cast<CSondaj*>(formular);
        if (sondaj) {
            std::vector<std::pair<std::string, std::vector<std::string>>> intrebariSiRaspunsuri = extractQuestionsSondaje(content);

            for (const auto& pair : intrebariSiRaspunsuri) {
                int intrebare_id = db.addIntrebare(form_id, pair.first);

                for (const auto& raspuns : pair.second) {
                    db.addRaspuns(intrebare_id, raspuns);
                }

                sondaj->adaugaIntrebareSiRaspunsuri(pair.first, pair.second);
            }

            for (int user_id : organizationMembers) {
                db.addFormToUser(form_id, user_id);
            }
            logger.log(std::to_string(client_socket));
            logger.log("Sondaj realizat cu succes.Asteapta raspunsurile organizatiei  pentru a evalua.\n");
            SendMessageToClient(client_socket, "Sondaj realizat cu succes.Asteapta raspunsurile organizatiei  pentru a evalua.");
        }
    }
}

void Server::HandleGetType(int client_socket, const std::string& username)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    BazaDeDate& db = BazaDeDate::getInstance();
    std::string userType = db.GetUserType(username);
    if (userType == "user") {
        SendMessageToClient(client_socket, "user");
    }
    else if (userType == "manager") {
        SendMessageToClient(client_socket, "manager");
    }
    else {
        SendMessageToClient(client_socket, "admin");
    }
}

void Server::HandleGetUncompletedForms(int client_socket, const std::string& username)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    BazaDeDate& db = BazaDeDate::getInstance();
    std::vector<std::string> forms = db.getUncompletedForms(username);

    if (forms.empty()) {
        logger.log(std::to_string(client_socket));
        logger.log("Nu ai formulare necompletate.\n");
        SendMessageToClient(client_socket, "Nu ai formulare necompletate.");
    }
    else {
        std::ostringstream oss;
        for (size_t i = 0; i < forms.size(); ++i) {
            oss << forms[i];
            if (i < forms.size() - 1) {
                oss << "|"; 
            }
        }
        logger.log(std::to_string(client_socket));
        logger.log(oss.str().c_str());
        logger.log("\n");
        SendMessageToClient(client_socket, oss.str().c_str());
    }
}

void Server::HandleGetCompletedForms(int client_socket, const std::string& username)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    BazaDeDate& db = BazaDeDate::getInstance();
    std::vector<std::string> forms = db.getCompletedForms(username);

    if (forms.empty()) {
        logger.log(std::to_string(client_socket));
        logger.log("Nu ai formulare necompletate.\n");
        SendMessageToClient(client_socket, "Nu ai formulare necompletate.");
    }
    else {
        std::ostringstream oss;
        for (size_t i = 0; i < forms.size(); ++i) {
            oss << forms[i];
            if (i < forms.size() - 1) {
                oss << "|"; 
            }
        }
        logger.log(std::to_string(client_socket));
        logger.log(oss.str().c_str());
        logger.log("\n");
        SendMessageToClient(client_socket, oss.str().c_str());
    }
}

void Server::HandleGetCreatedForms(int client_socket, const std::string& username)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    BazaDeDate& db = BazaDeDate::getInstance();
    std::vector<std::string> forms = db.getCreatedForms(username);

    if (forms.empty()) {
        logger.log(std::to_string(client_socket));
        logger.log("Nu ai formulare create.\n");
        SendMessageToClient(client_socket, "Nu ai formulare create.");
    }
    else {
        std::ostringstream oss;
        for (size_t i = 0; i < forms.size(); ++i) {
            oss << forms[i];
            if (i < forms.size() - 1) {
                oss << "|"; 
            }
        }
        logger.log(std::to_string(client_socket));
        logger.log(oss.str().c_str());
        logger.log("\n");
        SendMessageToClient(client_socket, oss.str().c_str());
    }
}

void Server::HandleGetForm(int client_socket, const std::string& form_name)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    BazaDeDate& db = BazaDeDate::getInstance();

    int form_id = db.GetFormIDFromDatabase(form_name);
    if (form_id != -1) {
        std::string full_form = db.GetFormAndAnswersFromDatabase(form_name);
        std::cout << full_form;
        logger.log(std::to_string(client_socket));
        logger.log(full_form.c_str());
        SendMessageToClient(client_socket, full_form.c_str());
    }
    else {
        logger.log(std::to_string(client_socket));
        logger.log("Formularul nu a fost gasit.\n");
        SendMessageToClient(client_socket, "Formularul nu a fost gasit.");
    }
}

void Server::HandleGetFormByMe(int client_socket, const std::string& content)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    BazaDeDate& db = BazaDeDate::getInstance();
    std::string username = extractUsername(content);
    std::string titlu = extractFormTitleC(content);
    int id_utilizator = db.getUserID(username);

    int id_formular = db.GetFormIDFromDatabase(titlu);

    std::vector<std::pair<std::string, std::string>> user_responses = db.getUserResponsesForForm(id_utilizator, id_formular);

    std::string response_string;
    for (const auto& response : user_responses) {
        response_string += response.first + "*" + response.second + "|";
    }
    std::cout << response_string;
    if (!response_string.empty()) {
        logger.log(std::to_string(client_socket));
        logger.log(response_string.c_str());
        logger.log("\n");
        SendMessageToClient(client_socket, response_string.c_str());
    }
    else {
        logger.log(std::to_string(client_socket));
        logger.log("Formular gresit\n");
        SendMessageToClient(client_socket, "Formular gresit");
    }
}

void Server::HandleCompleteForm(int client_socket, const std::string& content)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    BazaDeDate& db = BazaDeDate::getInstance();
    std::string username = extractUsername(content);
    std::string titlu = extractFormTitleC(content);
    int id_utilizator = db.getUserID(username);
    int id_formular = db.GetFormIDFromDatabase(titlu);

    if (db.hasUserCompletedForm(id_utilizator, id_formular)) {
        std::cout << "Ai completat deja acest formular.";
        logger.log(std::to_string(client_socket));
        logger.log("Ai completat deja acest formular.\n");
        SendMessageToClient(client_socket, "Ai completat deja acest formular.");
        return;
    }
    else {
        std::vector<std::pair<int, std::string>> responses = extractResponses(content); 

        bool success = true;
        for (const auto& response : responses) {
            int id_intrebare = response.first;
            std::string raspuns = response.second;
            int id_raspuns = db.getResponseID(id_intrebare, raspuns);
            if (!db.addUserResponse(id_utilizator, id_intrebare, id_raspuns, id_formular)) {
                success = false;
                break;
            }
        }

        if (success) {
            int id_formular = db.GetFormIDFromDatabase(titlu); 
            if (id_formular != -1) {
                db.setFormCompleted(id_formular, id_utilizator); 
            }
            SendMessageToClient(client_socket, " ");
        }
        else {
            logger.log(std::to_string(client_socket));
            logger.log("Eroare la completarea formularului.\n");
            SendMessageToClient(client_socket, "Eroare la completarea formularului.");
        }
    }  
}

void Server::HandleGetStatistic(int client_socket, const std::string& content)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    std::string titlu = extractUsername(content);

    BazaDeDate& db = BazaDeDate::getInstance();
    std::string statistici = db.getStatistics(titlu);
    std::cout << statistici;
    logger.log(std::to_string(client_socket));
    logger.log(statistici.c_str());
    SendMessageToClient(client_socket, statistici.c_str());
}

void Server::HandleArchive(int client_socket, const std::string& content)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    std::string titlu = extractUsername(content);

    BazaDeDate& db = BazaDeDate::getInstance();
    bool success = db.archive(titlu);
    if (success) {
        logger.log(std::to_string(client_socket));
        logger.log("Cererea de arhivare a fost inregistrata.\n");
        SendMessageToClient(client_socket, "Cererea de arhivare a fost inregistrata.");
    }
    else {
        logger.log(std::to_string(client_socket));
        logger.log("Eroare la inregistrarea cererii de arhivare. Formularul nu a fost gasit sau e deja arhivat.\n");
        SendMessageToClient(client_socket, "Eroare la inregistrarea cererii de arhivare. Formularul nu a fost gasit sau e deja arhivat.");
    }
}

void Server::HandleAllowArchive(int client_socket, const std::string& content)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    std::string titlu = extractUsername(content);
    BazaDeDate& db = BazaDeDate::getInstance();
    int form_id = db.GetFormIDFromDatabase(titlu);

    bool ok= db.updateFormType(form_id, "arhivat");
    bool ok1= db.setArchiveRequestStatus(form_id, 1);
    if (ok && ok1)
    {
        logger.log(std::to_string(client_socket));
        logger.log("Formularul a fost arhivat cu succes.\n");
        std::cout << "Formularul a fost arhivat cu succes.";
        SendMessageToClient(client_socket, "Formularul a fost arhivat cu succes.");
    }
    else
    {
        logger.log(std::to_string(client_socket));
        logger.log("Eroare la arhivarea formularului.\n");
        std::cout << "Eroare la arhivarea formularului.";
        SendMessageToClient(client_socket, "Eroare la arhivarea formularului.");
    }
}

void Server::HandleViewArchive(int client_socket, const std::string& content)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    BazaDeDate& db = BazaDeDate::getInstance();
    std::vector<int> form_ids = db.getUnarchivedFormIDs();
    std::vector<std::string> form_titles = db.getFormTitlesByIDs(form_ids);

    std::string response;
    if (form_titles.empty()) {
        response = "Nu exista formulare ne-arhivate.";
    }
    else {
        for (const auto& title : form_titles) {
            response += title + "|";
        }
    }
    std::cout << response;

    logger.log(std::to_string(client_socket));
    logger.log(response.c_str());
    SendMessageToClient(client_socket, response.c_str());
}

void Server::HandleDelete(int client_socket, const std::string& content)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    std::string username = extractUsername(content);
    std::string titlu = extractUsernameT(content);

    BazaDeDate& db = BazaDeDate::getInstance();
    int form_id = db.GetFormIDFromDatabase(titlu);
    int user_id = db.getUserID(username); 

    if (form_id == -1) {
        std::cout << "Formularul nu a fost gasit.";
        logger.log(std::to_string(client_socket));
        logger.log("Formularul nu a fost gasit.\n");
        SendMessageToClient(client_socket, "Formularul nu a fost gasit.");
        return;
    }

    if (db.deleteRequestExists(form_id)) {
        std::cout << "Cererea de stergere pentru acest formular exista deja.";
        logger.log(std::to_string(client_socket));
        logger.log("Cererea de stergere pentru acest formular exista deja.\n");
        SendMessageToClient(client_socket, "Cererea de stergere pentru acest formular exista deja.");
        return;
    }

    if (db.addDeleteRequest(form_id, user_id)) {
        logger.log(std::to_string(client_socket));
        logger.log("Cererea de stergere a fost adaugata cu succes.\n");
        SendMessageToClient(client_socket, "Cererea de stergere a fost adaugata cu succes.");
    }
    else {
        logger.log(std::to_string(client_socket));
        logger.log("Eroare la adaugarea cererii de stergere.\n");
        SendMessageToClient(client_socket, "Eroare la adaugarea cererii de stergere.");
    }
}

void Server::HandleAdminApproveDelete(int client_socket, const std::string& content)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    BazaDeDate& db = BazaDeDate::getInstance();
    std::vector<std::pair<int, std::string>> requests = db.getPendingDeleteRequests();

    std::string response;
    if (requests.empty()) {
        response = "Nu exista cereri de stergere.";
    }
    else {
        for (const auto& request : requests) {
            response += request.second + "|";
        }
    }
    std::cout << response;
    logger.log(std::to_string(client_socket));
    logger.log(response.c_str());
    SendMessageToClient(client_socket, response.c_str());
}

void Server::HandleAdminApproveDeleteRequest(int client_socket, const std::string& content)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    std::string titlu = extractUsername(content);

    BazaDeDate& db = BazaDeDate::getInstance();
    int form_id = db.GetFormIDFromDatabase(titlu);  
    

    if (db.approveDeleteRequest(form_id)) {
        logger.log(std::to_string(client_socket));
        logger.log("Cererea de stergere a fost aprobata și formularul a fost sters.\n");
        SendMessageToClient(client_socket, "Cererea de stergere a fost aprobata și formularul a fost sters.");
    }
    else {
        logger.log(std::to_string(client_socket));
        logger.log("Eroare la aprobarea cererii de stergere.\n");
        SendMessageToClient(client_socket, "Eroare la aprobarea cererii de stergere.");
    }
}

void Server::HandleRecover(int client_socket, const std::string& content)
{
    LogareActiuni& logger = LogareActiuni::getInstance("log.txt");

    std::string username = extractUsername(content);
    std::string titlu = extractUsernameT(content); 

    BazaDeDate& db = BazaDeDate::getInstance();

    int form_id = db.GetFormIDFromDatabase(titlu);
    if (form_id == -1) {
        logger.log(std::to_string(client_socket));
        logger.log("Formularul nu a fost gasit.\n");
        SendMessageToClient(client_socket, "Formularul nu a fost gasit.");
        return;
    }

    if (db.isFormArchived(form_id)) {
        if (db.recoverForm(form_id)) {
            logger.log(std::to_string(client_socket));
            logger.log("Formularul a fost recuperat cu succes.\n");
            SendMessageToClient(client_socket, "Formularul a fost recuperat cu succes.");
        }
        else {
            logger.log(std::to_string(client_socket));
            logger.log("Eroare la recuperarea formularului.\n");
            SendMessageToClient(client_socket, "Eroare la recuperarea formularului.");
        }
    }
    else {
        logger.log(std::to_string(client_socket));
        logger.log("Formularul nu poate fi recuperat, deoarece nu are un backup.\n");
        SendMessageToClient(client_socket, "Formularul nu poate fi recuperat, deoarece nu are un backup.");
    }
}

void Server::PrintClientSockets()
{
    for (const auto& pair : clientSockets) {
        std::cout << "Username: " << pair.first << ", Socket: " << pair.second << std::endl;
    }
}

void Server::SendFormToOrganizationMembers(const std::string& formContent, const std::vector<std::string>& members)
{
    for (const std::string& member : members) {
        SendMessageToClient(GetClientSocketByUsername(member), formContent.c_str());
    }
}

int Server::GetClientSocketByUsername(const std::string& username)
{
    auto it = clientSockets.find(username);
    if (it != clientSockets.end()) {
        
        return it->second; 
    }
    else {
      
        return -1; 
    }
}

std::string Server::extractUsernameT(const std::string& message)
{
    std::stringstream ss(message);
    std::string token;

    std::getline(ss, token, '|'); 
    std::getline(ss, token, '|');// Ignoram prima parte 

    return token;
}

std::string Server::extractUsername(const std::string& message)
{
    std::stringstream ss(message);
    std::string token;

    std::getline(ss, token, '|'); 

    return token;
}

std::string Server::extractPassword(const std::string& message)
{
    std::stringstream ss(message);
    std::string token;

    std::getline(ss, token, '|'); 
    std::getline(ss, token, '|'); // Ignoram numele de utilizator

    return token;
}

std::string Server::extractRole(const std::string& message)
{

    std::stringstream ss(message);
    std::string token;

    std::getline(ss, token, '|'); // Ignoram prima parte 
    std::getline(ss, token, '|'); // Ignoram numele de utilizator
    std::getline(ss, token, '|'); // Ignaram parola

    return token;
}

std::string Server::extractAdminPassword(const std::string& message)
{
    std::stringstream ss(message);
    std::string token;

    std::getline(ss, token, '|'); // Ignora prima parte 
    std::getline(ss, token, '|'); // Ignora numele de utilizator
    std::getline(ss, token, '|'); // Ignora parola obisnuit
    std::getline(ss, token, '|'); // Ignora rolul

    return token;
}

std::string Server::extractMessageCode(const std::string& message)
{
    std::stringstream ss(message);
    std::string code;
    std::getline(ss, code, '|'); // Extrage partea dinaintea primei delimitari
    return code;
}

std::string Server::extractMessageContent(const std::string& message)
{
    std::stringstream ss(message);
    std::string content;
    std::getline(ss, content, '|'); // Ignora codul
    std::getline(ss, content, '\0'); // Extrage partea de conținut
    return content;
}

std::string Server::extractTargetUsername(const std::string& message)
{
    std::stringstream ss(message);
    std::string token;

    std::getline(ss, token, '|');  // Ignora codul mesajului
    std::getline(ss, token, '|');  // Ignora numele managerului
    std::getline(ss, token, '|');  // Obbine numele utilizatorului

    return token;
}

std::string Server::extractOrganizationName(const std::string& message)
{
    std::stringstream ss(message);
    std::string token;

    std::getline(ss, token, '|');  // Ignora codul mesajului
    std::getline(ss, token, '|');  // Ignora numele utilizatorului
    std::getline(ss, token, '|');
    std::getline(ss, token, '|');// Obtine numele organizației

    return token;
}

std::string Server::extractFormType(const std::string& content)
{
    std::stringstream ss(content);
    std::string token;
    std::getline(ss, token, '|'); 
    std::getline(ss, token, '|'); // Obtine tipul formularului
    return token;
}

std::string Server::extractFormTitle(const std::string& content)
{
    std::stringstream ss(content); 
    std::string token;

    std::getline(ss, token, '|'); 
    std::getline(ss, token, '|'); 
    std::getline(ss, token, '|'); // Extrage titlul formularului

    return token;
}

std::string Server::extractFormTitleC(const std::string& content)
{
    std::stringstream ss(content);
    std::string token;

    std::getline(ss, token, '|');
    std::getline(ss, token, '|'); // Extrage titlul formularului

    return token;
}

std::string Server::GetUserType(const std::string& username)
{
    return std::string();
}

std::vector<std::string> Server::extractQuestions(const std::string& content)
{
    std::stringstream ss(content);
    std::vector<std::string> questions;

    std::string token;
    std::getline(ss, token, '|');
    std::getline(ss, token, '|');
    std::getline(ss, token, '|');
    while (std::getline(ss, token, '|')) {
        questions.push_back(token);
    }
    return questions;
}

std::vector<std::string> Server::GetOrganizationMembersForManager(const std::string& managerUsername)
{
    std::vector<std::string> organizationMembers;

    BazaDeDate& db = BazaDeDate::getInstance();
    int organizationID = db.getOrganizationIDByManager(managerUsername);

    organizationMembers = db.getOrganizationMembers(organizationID);

    return organizationMembers;
}

std::vector<std::pair<std::string, std::vector<std::string>>> Server::extractQuestionsAndResponses(const std::string& content)
{
    std::vector<std::pair<std::string, std::vector<std::string>>> questionsAndAnswers;

    std::stringstream ss(content);
    std::string token;

    std::getline(ss, token, '|'); // Ignoram username-ul
    std::getline(ss, token, '|'); // Ignoram tipul formularului
    std::getline(ss, token, '|'); // Ignoram titlul formularului

    while (std::getline(ss, token, '*')) {
        std::string intrebare = token; // Extragem întrebarea

        std::getline(ss, token, '|');

        std::vector<std::string> raspunsuri;
        std::stringstream ss_raspunsuri(token); 
        while (std::getline(ss_raspunsuri, token, '*')) {
            raspunsuri.push_back(token); 
        }

        questionsAndAnswers.push_back(std::make_pair(intrebare, raspunsuri));
    }

    return questionsAndAnswers;
}

std::vector<std::pair<std::string, std::vector<std::string>>> Server::extractQuestionsSondaje(const std::string& content)
{
    std::vector<std::pair<std::string, std::vector<std::string>>> questionsAndAnswers;

    std::stringstream ss(content);
    std::string token;

 
    std::getline(ss, token, '|'); // Ignoram username-ul
    std::getline(ss, token, '|'); // Ignoram tipul formularului
    std::getline(ss, token, '|'); // Ignoram titlul formularului

    while (std::getline(ss, token, '|')) {
        std::vector<std::string> raspunsuri;
        for (int i = 1; i <= 5; ++i) {
            raspunsuri.push_back(std::to_string(i));
        }
        questionsAndAnswers.push_back(std::make_pair(token, raspunsuri));
    }

    return questionsAndAnswers;
}

std::vector<std::pair<int, std::string>> Server::extractResponses(const std::string& content)
{
    std::vector<std::pair<int, std::string>> responses;
    std::istringstream iss(content);
    std::string token;

    std::getline(iss, token, '|');
    std::getline(iss, token, '|');

    BazaDeDate& db = BazaDeDate::getInstance();

    while (std::getline(iss, token, '|')) {
        size_t pos = token.find('*');
        if (pos != std::string::npos) {
            std::string text_intrebare = token.substr(0, pos);
            std::string raspuns = token.substr(pos + 1);

            int id_intrebare = db.getIntrebareID(text_intrebare); 
            responses.push_back({ id_intrebare, raspuns });
        }
    }

    return responses;
}
