#pragma once
#include <iostream>

class Shell
{
public:
	Shell() {};
	Shell(std::string p) : prompt(p) {};
	//Program control
	void Display();
	//Receives user input
	std::string getInput();
	//executes the command returns an int to indicate continuing or ending of program
	int executeCMD(std::string cmd);
private:
	std::string prompt;
};