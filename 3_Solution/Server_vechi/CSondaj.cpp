#include "CSondaj.h"
#include <iostream>
#include <sstream>

void CSondaj::afiseaza() const
{
    std::cout << "Sondaj: " << titlu << std::endl;
    for (const auto& intrebare : intrebari) {
        std::cout << " - " << intrebare << std::endl;
    }
}


std::string CSondaj::serialize() const
{
    std::stringstream ss;

    ss << "Sondaj: " << titlu << "\n";

    for (const auto& pair : intrebariSiRaspunsuri) {
        ss << "Intrebare: " << pair.first << "\n";

        for (const auto& raspuns : pair.second) {
            ss << "Raspuns: " << raspuns << "\n";
        }
    }

    return ss.str();
}
