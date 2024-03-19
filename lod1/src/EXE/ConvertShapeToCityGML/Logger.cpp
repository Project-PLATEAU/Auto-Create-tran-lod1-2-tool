#include "Logger.h"

Logger::Logger()
{
	Initialize("outputLog.txt");
}

Logger::Logger(std::string outputFilePath)
{
	Initialize(outputFilePath);
}

Logger::~Logger()
{
}

void Logger::Log2File(std::string& log, levels logLevel)
{
	std::ofstream ofs;
	ofs.open(filePath, std::ios::app);

	switch (logLevel)
	{
	case levels::trace:
		ofs << "[TRACE] " << log << std::endl;
		break;
	case levels::debug:
		ofs << "[DEBUG] " << log << std::endl;
		break;
	case levels::info:
		ofs << "[INFO] " << log << std::endl;
		break;
	case levels::warn:
		ofs << "[WARNING] " << log << std::endl;
		break;
	case levels::err:
		ofs << "[ERROR] " << log << std::endl;
		break;
	case levels::critical:
		ofs << "[CRITICAL] " << log << std::endl;
		break;
	case levels::off:
	case levels::n_levels:
		break;
	default:
		break;
	}

	ofs.close();
}

void Logger::Log2Console(std::string& log, levels logLevel)
{
	switch (logLevel)
	{
	case levels::trace:
		std::cout << "[TRACE] " << log << std::endl;
		break;
	case levels::debug:
		std::cout << "[DEBUG] " << log << std::endl;
		break;
	case levels::info:
		std::cout << "[INFO] " << log << std::endl;
		break;
	case levels::warn:
		std::cout << "[WARNING] " << log << std::endl;
		break;
	case levels::err:
		std::cout << "[ERROR] " << log << std::endl;
		break;
	case levels::critical:
		std::cout << "[CRITICAL] " << log << std::endl;
		break;
	case levels::off:
	case levels::n_levels:
		break;
	default:
		break;
	}
}

void Logger::Initialize(std::string outputFilePath)
{
	filePath = outputFilePath;

	// initialize output file
	std::ofstream ofs;
	ofs.open(filePath, std::ios::out);
	ofs.close();
}
