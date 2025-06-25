#include "utils.hpp"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>

namespace coinpoker {
namespace utils {

bool attachConsole() {
	if (!AllocConsole()) {
		return false;
	}
	FILE* fp;
	freopen_s(&fp, "CONOUT$", "w", stdout);
	FILE* fp2;
	freopen_s(&fp2, "CONIN$", "r", stdin);

	setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stdin, nullptr, _IONBF, 0);

	return true;
}

void detachConsole() {
	FILE* fp;
	freopen_s(&fp, "NUL", "r", stdin);
	freopen_s(&fp, "NUL", "w", stdout);
	FreeConsole();
}

std::string hexdump(char* buffer, int buflen) {
	unsigned char* buf = (unsigned char*)buffer;
	std::ostringstream output;
	for (int i = 0; i < buflen; i += 16) {
		output << std::hex << std::setw(6) << std::setfill('0') << i << ": ";
		for (int j = 0; j < 16; j++) {
			if (i + j < buflen)
				output << std::setw(2) << std::setfill('0') << std::hex << (buf[i + j] & 0xFF) << " ";
			else
				output << "   ";
		}
		output << " ";
		for (int j = 0; j < 16; j++)
			if (i + j < buflen) output << (std::isprint(buf[i + j]) ? (char)buf[i + j] : '.');
		output << "\n";
	}
	return output.str();
}

std::vector<std::string> splitString(const std::string& str, char delimiter) {
	std::vector<std::string> tokens;
	size_t start = 0, end;

	while ((end = str.find(delimiter, start)) != std::string::npos) {
		tokens.push_back(str.substr(start, end - start));
		start = end + 1;
	}

	tokens.push_back(str.substr(start));
	return tokens;
}

std::vector<std::string> splitCards(const std::string& cards) {
	std::vector<std::string> result;
	if (cards.size() % 2 != 0) {
		throw std::invalid_argument("Invalid string length");
	}

	for (size_t i = 0; i < cards.size(); i += 2) {
		result.emplace_back(cards.substr(i, 2));
	}
	return result;
}


bool parseMac(const std::string& macStr, unsigned char mac[6]) {
    std::istringstream stream(macStr);
    std::string byte_str;
    int i = 0;
    while (std::getline(stream, byte_str, ':')) {
        if (byte_str.length() != 2 || i >= 6) return false;

        std::istringstream byte_stream(byte_str);
        int byte;
        byte_stream >> std::hex >> byte;

        if (byte_stream.fail()) return false;

        mac[i++] = static_cast<unsigned char>(byte);
    }
    return i == 6;
}

std::string joinStrings(const std::vector<std::string>& strings, const std::string& delimiter) {
    std::string result;
    for (size_t i = 0; i < strings.size(); ++i) {
        result += strings[i];
        if (i < strings.size() - 1) {
            result += delimiter;
        }
    }
    return result;
}


}
}

