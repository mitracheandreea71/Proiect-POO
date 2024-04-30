#pragma once
#include "AInregistrare.h"

class CInregistrare: public AInregistrare
{
public:
    bool    registerUser(const std::string& username, const std::string& password, const std::string& role) override;
};

