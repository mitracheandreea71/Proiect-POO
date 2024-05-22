#include "AFormular.h"
#include <iostream>

void AFormular::afiseaza() const
{
    std::cout << "Formular: " << titlu << std::endl;
    for (const auto& intrebare : intrebari) {
        std::cout << " - " << intrebare << std::endl;
    }
}
