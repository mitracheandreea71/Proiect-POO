#pragma once
#include "IInregistrare.h"
#include "BazaDeDate.h"

class AInregistrare :
    public IInregistrare
{
public:
    virtual             ~AInregistrare() {};
    virtual bool        isUsernameAvailable(const std::string& username) override;
    virtual std::string hashPassword(const std::string& password); 
};

