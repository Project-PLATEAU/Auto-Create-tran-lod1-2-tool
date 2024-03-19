#include "pch.h"
#include "CCsvFileIO.h"

CCsvFileIO::CCsvFileIO()
{
}

CCsvFileIO::~CCsvFileIO()
{
}

bool CCsvFileIO::WriteA(
    std::vector<std::vector<std::string>> msgs, std::string outputFilepath,
    levels logLevel, bool isOutputConsole, bool isNewFile)
{

	// check filename
	if (outputFilepath.find(".csv") == std::string::npos)
	{
		return false;
	}

    // open mode
    std::ios_base::openmode mode = std::ios::app;
    if (isNewFile)
    {
        mode = std::ios::trunc;
    }

    // initialize
	std::ofstream ofs;
	ofs.open(outputFilepath, mode);
			
	if (ofs.is_open() == false)
	{
		return false;
	}

	for (std::vector<std::string> line : msgs)
	{
		std::string output;

		switch (logLevel)
		{
		case levels::trace:
			output = "[TRACE],";
			break;
		case levels::debug:
			output = "[DEBUG],";
			break;
		case levels::info:
			output = "[INFO],";
			break;
		case levels::warn:
			output = "[WARNING],";
			break;
		case levels::err:
			output = "[ERROR],";
			break;
		case levels::critical:
			output = "[CRITICAL],";
			break;
		case levels::off:
		case levels::n_levels:
		default:
			output = "";
			break;
		}

		for (std::string msg : line)
		{
			output += msg;
			output += ",";
		}

		ofs << output << std::endl;

		if (isOutputConsole == true)
		{
			std::cout << output << std::endl;
		}
	}
			
	// close stream
	ofs.close();

	return true;
}

bool CCsvFileIO::WriteW(
    std::vector<std::vector<std::wstring>> msgs, std::string outputFilepath,
    levels logLevel, bool isOutputConsole, bool isNewFile)
{
	// check filename
	if (outputFilepath.find(".csv") == std::string::npos)
	{
		return false;
	}

    // open mode
    std::ios_base::openmode mode = std::ios::app;
    if (isNewFile)
    {
        mode = std::ios::trunc;
    }

	// initialize
	std::wofstream ofs;
	ofs.open(outputFilepath, mode);

	if (ofs.is_open() == false)
	{
		return false;
	}

	for (std::vector<std::wstring> line : msgs)
	{
		std::wstring output;

		switch (logLevel)
		{
		case levels::trace:
			output = L"[TRACE],";
			break;
		case levels::debug:
			output = L"[DEBUG],";
			break;
		case levels::info:
			output = L"[INFO],";
			break;
		case levels::warn:
			output = L"[WARNING],";
			break;
		case levels::err:
			output = L"[ERROR],";
			break;
		case levels::critical:
			output = L"[CRITICAL],";
			break;
		case levels::off:
		case levels::n_levels:
		default:
			output = L"";
			break;
		}

		for (std::wstring msg : line)
		{
			output += msg;
			output += L",";
		}

		ofs << output << std::endl;

		if (isOutputConsole == true)
		{
			std::wcout << output << std::endl;
		}
	}

	// close stream
	ofs.close();

	return true;
}