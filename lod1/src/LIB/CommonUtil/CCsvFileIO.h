#pragma once

#define SPDLOG_WCHAR_FILENAMES
#define SPDLOG_WCHAR_TO_UTF8_SUPPORT

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include "CTime.h"

enum class levels
{
	trace = 0,
	debug,
	info,
	warn,
	err,
	critical,
	off,
	n_levels
};

class CCsvFileIO
{
public:
	CCsvFileIO();
	~CCsvFileIO();
	static bool WriteA(
        std::vector<std::vector<std::string>> msgs,
        std::string outputFilepath,
        levels logLevel = levels::off,
        bool isOutputConsole = false,
        bool isNewFile = true);
	static bool WriteW(
        std::vector<std::vector<std::wstring>> msgs,
        std::string outputFilepath,
        levels logLevel = levels::off,
        bool isOutputConsole = false,
        bool isNewFile = true);

private:
};