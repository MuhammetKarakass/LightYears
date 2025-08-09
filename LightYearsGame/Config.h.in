#pragma once
#include <string>

std::string getResourceDir()
{
#ifdef NDEBUG
	return "@RESOURCE_FOLDER_NAME@/";
#else
	return "@RESOURCE_SRC_DIR@/";
#endif
}