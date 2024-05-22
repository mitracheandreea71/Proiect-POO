#pragma once
#include "IFormular.h"
#include <vector>

class AFormular :
    public IFormular
{
protected:
    std::string titlu;
    std::vector<std::string> intrebari;
    std::vector<std::vector<std::string>> raspunsuri;
    std::vector<std::pair<std::string, std::vector<std::string>>> intrebariSiRaspunsuri;

public:
    AFormular(const std::string& titlu) : titlu(titlu) {};
    virtual void adaugaIntrebare (const std::string& intrebare) override{ intrebari.push_back(intrebare); }
    virtual void afiseaza() const override;
    std::string  getTitlu() override{ return this->titlu; };
    virtual std::vector<std::string> getIntrebari() override { return this->intrebari; };
    virtual void adaugaRaspunsuri(const std::vector<std::string>& raspunsuri) override { this->raspunsuri.push_back(raspunsuri); };
    void adaugaIntrebareSiRaspunsuri(const std::string& intrebare, const std::vector<std::string>& raspunsuri) {
        intrebariSiRaspunsuri.push_back(std::make_pair(intrebare, raspunsuri));
    }

};

