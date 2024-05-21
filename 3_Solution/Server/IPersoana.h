#pragma once
#include <string>

class IPersoana
{
public:
    virtual std::string getNume() const = 0;
    virtual std::string getParola() const = 0;
    virtual ~IPersoana() {}
};

