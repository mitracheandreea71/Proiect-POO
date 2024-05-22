#pragma once
#include <string>
#include <vector>

class IFormular
{
public:
	virtual void		adaugaIntrebare(const std::string& intrebare) = 0;
	virtual void		adaugaRaspunsuri(const std::vector<std::string>& raspunsuri) = 0;
	virtual void		afiseaza() const = 0;
	virtual				~IFormular() = default;
	virtual std::string getTitlu() = 0;
	virtual std::vector<std::string> getIntrebari() = 0;
	virtual std::string serialize() const = 0;
};

