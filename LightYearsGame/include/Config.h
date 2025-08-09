#pragma once
#include <string>

std::string getResourceDir()
{
#ifdef NDEBUG
	return "assets/";
#else
	return "C:/LightYears/LightYearsGame/assets/";
#endif
}
