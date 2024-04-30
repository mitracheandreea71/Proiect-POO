#include "AInregistrare.h"

bool AInregistrare::isUsernameAvailable(const std::string& username)
{
    BazaDeDate& db = BazaDeDate::getInstance();
    return !db.userExists(username);
}

std::string AInregistrare::hashPassword(const std::string& password)
{
    return std::to_string(std::hash<std::string>{}(password));
}
