#pragma once
#include <string>
#include "BazaDeDate.h"
#include "CInregistrare.h"

class Autentificare
{
public:
    bool     autentificareUser(const std::string& username, const std::string& password);
};

