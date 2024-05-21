#pragma once
#include "APersoana.h"


class Manager :
    public APersoana
{
public:
    Manager(const std::string& nume, const std::string& parola): APersoana(nume, parola) {}

    void afiseazaRol() const override { std::cout << "Rol: Manager" << std::endl; }
};

