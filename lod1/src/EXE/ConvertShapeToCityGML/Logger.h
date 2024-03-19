#pragma once

#define SPDLOG_WCHAR_FILENAMES
#define SPDLOG_WCHAR_TO_UTF8_SUPPORT

#include <iostream>
#include <fstream>

enum levels
{
	trace,
	debug,
	info,
	warn,
	err,
	critical,
	off,
	n_levels
};

class Logger
{
public:
	Logger();
	Logger(std::string outputFilePath);
	~Logger();
	void Log2File(std::string& log, levels logLevel = levels::trace);
	void Log2Console(std::string& log, levels logLevel = levels::trace);
	
private:
	std::string filePath;

	void Initialize(std::string outputFilePath);
};