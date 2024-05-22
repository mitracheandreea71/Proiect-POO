#pragma once
#include <fstream>

class LogareActiuni
{
private:
	LogareActiuni(const std::string& filename);
	static LogareActiuni* instance;
	std::ofstream logfile;
	~LogareActiuni() {};

public:
	static LogareActiuni&	getInstance(const std::string& filename);
	static void				destroyInstance();
	void					log(const std::string& message);
};

