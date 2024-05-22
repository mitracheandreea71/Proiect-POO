#pragma once
#include "AFormular.h"
class CChestionar :
    public AFormular
{
public:
    CChestionar(const std::string& titlu) : AFormular(titlu) {};
    void adaugaIntrebare(const std::string& intrebare, const std::vector<std::string>& raspunsuri) {AFormular::adaugaIntrebare(intrebare + " " + getRaspunsuriString(raspunsuri));}
    void afiseaza() const override;
  
    std::vector<std::pair<std::string, std::vector<std::string>>> getIntrabariSiRaspunsuri() { return intrebariSiRaspunsuri; }
    virtual std::string serialize() const override;

private:
    
    std::string getRaspunsuriString(const std::vector<std::string>& raspunsuri);
};

