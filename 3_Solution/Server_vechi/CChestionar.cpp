#include "CChestionar.h"
#include <iostream>
#include <sstream>

void CChestionar::afiseaza() const
{
    std::cout << "Chestionar: " << titlu << std::endl;
    for (const auto& intrebare : intrebari) {
        std::cout << " - " << intrebare << std::endl;
    }
}

std::string CChestionar::serialize() const
{
    std::stringstream ss;

    ss << "Chestionar: " << titlu << "\n";

    for (const auto& pair : intrebariSiRaspunsuri) {
        ss << "Intrebare : " << pair.first << "\n";

        for (const auto& raspuns : pair.second) {
            ss << "Raspuns: " << raspuns << "\n";
        }
    }

    return ss.str();
}

std::string CChestionar::getRaspunsuriString(const std::vector<std::string>& raspunsuri)
{
    std::string result = "[";
    for (const auto& rsp : raspunsuri) {
        if (result.size() > 1) {
            result += ", ";
        }
        result += rsp;
    }
    result += "]";
    return result;
}
