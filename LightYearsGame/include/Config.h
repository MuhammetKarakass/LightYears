#pragma once
#include <string>
#include <filesystem>

#if defined(_WIN32)
#include <windows.h>
#endif

inline std::string getResourceDir()
{
#if defined(_WIN32)
 char pathBuf[MAX_PATH] = {0};
 DWORD len = GetModuleFileNameA(NULL, pathBuf, MAX_PATH);
 if (len ==0)
 {
 return "assets/"; // fallback
 }

 std::filesystem::path exePath(pathBuf);
 std::filesystem::path assetsPath = exePath.parent_path() / "assets";
 std::string result = assetsPath.string();

 // Ensure trailing separator
 if (!result.empty())
 {
 char last = result.back();
 if (last != '/' && last != '\\')
 {
 result += std::filesystem::path::preferred_separator;
 }
 }
 return result;
#else
 // Non-Windows fallback: assume assets folder is next to executable or use relative path
 return "assets/";
#endif
}
