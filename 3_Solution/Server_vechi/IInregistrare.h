#pragma once
#include <string>

class IInregistrare
{
public:
    virtual          ~IInregistrare() {}; 

    virtual int      registerUser(const std::string& username, const std::string& password, const std::string& role, const std::string& organization) = 0;
    virtual bool     isUsernameAvailable(const std::string& username) = 0;
};

