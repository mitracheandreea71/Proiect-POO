#pragma once
#include <mutex>

class IDGenerator
{
private:
    static int lastID;
    static std::mutex idMutex;

    IDGenerator() {}
public:
    static IDGenerator& getInstance();
    int generateUniqueID();
};

