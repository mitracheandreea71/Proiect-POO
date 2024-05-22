#pragma once
#include "IPersoana.h"
#include <iostream>

class APersoana :
    public IPersoana
{
protected:
    std::string nume;
    std::string parola;

public:
    APersoana(const std::string& nume, const std::string& parola) : nume(nume), parola(parola) {}
    std::string getNume() const override { return nume; }
    std::string getParola() const override { return parola; }
    virtual void afiseazaRol() const = 0;
};

