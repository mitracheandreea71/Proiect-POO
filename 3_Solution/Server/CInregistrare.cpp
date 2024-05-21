#include "CInregistrare.h"

int CInregistrare::registerUser(const std::string& username, const std::string& password, const std::string& role, const std::string& organization)
{
    if (!isUsernameAvailable(username)) 
    {
        return false;
    }

    BazaDeDate& db = BazaDeDate::getInstance();
    if (db.addUser(username, password, role, organization)) {
        int user_id = db.getUserID(username);  
        return user_id;
    }

    return -1;
}
