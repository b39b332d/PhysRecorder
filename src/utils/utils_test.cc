#include <iostream>
#include <cstdlib>
#include <windows.h>
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

int main() {
    // Print all environment variables
    std::cout << "Environment Variables:\n";
    char* env = std::getenv("PATH");
    if (env) {
        std::cout << "PATH: " << env << std::endl;
    }

    // Get all environment variables and print them
    LPWCH lpEnv = GetEnvironmentStringsW();
    if (lpEnv != nullptr) {
        wchar_t* envVar = lpEnv;
        while (*envVar) {
            std::wcout << envVar << std::endl;
            envVar += wcslen(envVar) + 1;
        }
        FreeEnvironmentStringsW(lpEnv);
    }

    // Get and print the current directory using std::filesystem (C++17 and later)
    std::cout << "\nCurrent Directory: " << fs::current_path() << std::endl;

    // List files in the current directory
    std::cout << "\nFiles in the Current Directory:\n";
    try {
        for (const auto& entry : fs::directory_iterator(fs::current_path())) {
            std::cout << entry.path() << std::endl;
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error listing files: " << e.what() << std::endl;
    }
    int i;
    std::cin >> i;
    return 0;
}