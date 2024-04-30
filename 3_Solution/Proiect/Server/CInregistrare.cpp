#include "CInregistrare.h"

bool CInregistrare::registerUser(const std::string& username, const std::string& password, const std::string& role)
{
    if (!isUsernameAvailable(username)) {
        return false;
    }

    std::string registrationMessage = "R|"; // incepem cu 'R' pentru înregistrare
    registrationMessage += username + "|"; // adaugam username-ul
    registrationMessage += hashPassword(password) + "|"; // adaugam parola hashuită
    registrationMessage += role;

    BazaDeDate& db = BazaDeDate::getInstance();
    db.addUser(username, hashPassword(password), role); 

    return true; 
}
