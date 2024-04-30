#pragma once
#include <string>

class IInregistrare
{
public:
    virtual          ~IInregistrare() {}; 

    virtual bool     registerUser(const std::string& username, const std::string& password, const std::string& role) = 0;
    virtual bool     isUsernameAvailable(const std::string& username) = 0;
};

