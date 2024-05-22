#pragma once
#include <string>
#include <iostream>

class IExceptie
{
public:
	virtual void print() = 0;
};

class ExceptieTest :public IExceptie {
	std::string mesaj;
	int cod;
public:
	ExceptieTest(int c, std::string m) :mesaj{ m }, cod{ c } {}
	void print()override {
		std::cout << "Exceptie cu codul: " << cod << "." << mesaj << "\n";
	}
};


