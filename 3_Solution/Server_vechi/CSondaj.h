#pragma once
#include "AFormular.h"
class CSondaj :
    public AFormular
{
public:
    CSondaj(const std::string& titlu) : AFormular(titlu) {}

    void adaugaIntrebare(const std::string& intrebare) override { AFormular::adaugaIntrebare(intrebare + " (1-5)"); }
    void afiseaza() const override;
    virtual std::string serialize() const override;
};

