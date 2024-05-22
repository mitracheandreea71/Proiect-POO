#include "Autentificare.h"

bool Autentificare::autentificareUser(const std::string& username, const std::string& password)
{
    BazaDeDate& db = BazaDeDate::getInstance();

    if (!db.userExists(username)) {
        return false; 
    }

    CInregistrare registration;
    std::string hashedPassword = registration.hashPassword(password);

    return db.verifyPassword(username, hashedPassword); 
}
