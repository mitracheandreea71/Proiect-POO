#pragma once
#include "AInregistrare.h"

class CInregistrare: public AInregistrare
{
public:
    int    registerUser(const std::string& username, const std::string& password, const std::string& role, const std::string& organization) override;
};

