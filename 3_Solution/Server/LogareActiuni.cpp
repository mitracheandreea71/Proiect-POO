#include "LogareActiuni.h"

LogareActiuni* LogareActiuni::instance = nullptr;

LogareActiuni& LogareActiuni::getInstance(const std::string& filename)
{
    if (instance == nullptr) {
        instance = new LogareActiuni(filename);

    }
    return *instance;
}

LogareActiuni::LogareActiuni(const std::string& filename)
{
    logfile.open(filename, std::ios::app);
}

void LogareActiuni::destroyInstance()
{
    if (instance == nullptr) {
        return;
    }
    delete instance;
    instance = nullptr;
}

void LogareActiuni::log(const std::string& message)
{
    logfile << message << "     ";
}
