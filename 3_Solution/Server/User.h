#pragma once
#include "APersoana.h"


class User :
    public APersoana
{
public:
    User(const std::string& nume, const std::string& parola) : APersoana(nume, parola) {}

    void afiseazaRol() const override { std::cout << "Rol: User" << std::endl; }
};

