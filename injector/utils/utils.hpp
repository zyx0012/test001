#pragma once
#include <Windows.h>
#include <iostream>
#include <vector>


namespace coinpoker {

namespace utils {
bool attachConsole();
void detachConsole();
std::string hexdump(char* buf, int len);
std::vector<std::string> splitString(const std::string& str, char delimiter);
std::string joinStrings(const std::vector<std::string>& strings, const std::string& delimiter);
std::vector<std::string> splitCards(const std::string& cards);
bool parseMac(const std::string& macStr, unsigned char mac[6]);
}
}


