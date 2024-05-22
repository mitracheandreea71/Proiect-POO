#pragma once
#include "APersoana.h"
class Admin :
    public APersoana
{
public:
    Admin(const std::string& nume, const std::string& parola): APersoana(nume, parola) {}

    void afiseazaRol() const override {std::cout << "Rol: Admin" << std::endl;}
};

