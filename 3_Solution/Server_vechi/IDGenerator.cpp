#include "IDGenerator.h"

int IDGenerator::lastID = 2;
std::mutex IDGenerator::idMutex;

IDGenerator& IDGenerator::getInstance()
{
    static IDGenerator instance;
    return instance;
}

int IDGenerator::generateUniqueID()
{
    std::lock_guard<std::mutex> lock(idMutex);
    return ++lastID;
}
