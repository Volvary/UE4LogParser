// UE4LogParser.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

//TODO:
//	- Support multiple Config files?

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include <windows.h>

std::string GetSelfPath() {
	char ownPth[MAX_PATH];
	std::string path = "";

	// When NULL is passed to GetModuleHandle, the handle of the exe itself is returned
	HMODULE hModule = GetModuleHandle(NULL);
	if (hModule != NULL)
	{
		// Use GetModuleFileName() with module handle to get the path
		GetModuleFileNameA(hModule, ownPth, (sizeof(ownPth)));

		path = std::string(ownPth);
		size_t lastSlash = path.find_last_of("\\");

		path.erase(lastSlash + 1, path.length() - lastSlash);
	}
	return path;
}
#endif

bool ParseLog(std::string FilePath) {

	std::string Line;
	std::string ConfigPath = "Config.txt";

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
	std::string SelfPath = GetSelfPath();
	ConfigPath = SelfPath + "Config.txt";

#endif

	std::ifstream ConfigFile(ConfigPath, std::ifstream::in);

	if (!ConfigFile.is_open()) {
		std::cout << "Couldn't find Config File!\n";

		return false;
	}

	std::vector<std::string> Ignores;
	std::vector<std::string> Bypasses;
	std::vector<std::string> BypassOverride;
	int BackWrap = 0;
	int ForwardWrap = 0;
	bool bIgnoreAll = false;

	while (std::getline(ConfigFile, Line)) {
		
		std::string TempLine = Line;
		unsigned int i = 0;

		while (i < TempLine.length()) {
			if (TempLine.at(i) == ' ') {
				TempLine.erase(i);
				continue;
			}
			i++;
		}

		if (TempLine.length() != 0) {
			if (Line == "*") {
				std::cout << "* found, every line will be considered ignored unless otherwise stated." << std::endl << std::endl;
				bIgnoreAll = true;
			}
			else if (Line.substr(0, 2) == "//") {
				//Ignore this line as it's a comment
			}
			else if (Line.at(0) == '!') {
				Bypasses.push_back(Line.substr(1));
			}
			else if (Line.at(0) == '$') {
				BypassOverride.push_back(Line.substr(1));
			}
			else if (Line.at(0) == '?') {
				std::stringstream ss;
				ss << Line.substr(1);
				ss >> BackWrap >> ForwardWrap;
			}
			else {
				Ignores.push_back(Line);
			}
		}
	}
	ConfigFile.close();

	size_t ExtensionPosition = FilePath.find_last_of(".");
	size_t ExtensionLength = FilePath.length() - ExtensionPosition;
	std::string OriginalExtension = FilePath.substr(ExtensionPosition, ExtensionLength);


	std::ifstream LogFile (FilePath, std::ifstream::in);
	std::ofstream OutFile(FilePath.replace(FilePath.end() - ExtensionLength, FilePath.end(), "_Parsed" + OriginalExtension));

	std::vector<std::string> BackStrings;
	int ForwardStringsRemaining = 0;
	bool BreakMarked = true;

	while (std::getline(LogFile, Line)) {

		bool bPrinting = true;
		bool bBypassed = false;
		size_t Found;

		if (bIgnoreAll) {
			bPrinting = false;
		}
		else {
			for (std::string Ignore : Ignores) {
				Found = Line.find(Ignore);
				if (Found != std::string::npos) {
					bPrinting = false;
					break;
				}
			}
		}

		if (!bPrinting) {
			for (std::string Bypass : Bypasses) {
				Found = Line.find(Bypass);
				if (Found != std::string::npos) {
					bPrinting = true;
					bBypassed = true;
					break;
				}
			}
		}

		if (bBypassed) {
			for (std::string Override : BypassOverride) {
				Found = Line.find(Override);
				if (Found != std::string::npos) {
					bPrinting = false;
					break;
				}
			}
		}

		if (bPrinting || ForwardStringsRemaining > 0) {
			for (std::string String : BackStrings) {
				OutFile << String << std::endl;
			}
			BackStrings.clear();

			OutFile << Line << std::endl;
			
			ForwardStringsRemaining -= (ForwardStringsRemaining > 0 ? 1 : 0);	//If we have forward strings to print, remove 1 from it when we print.

			if (bPrinting) {
				ForwardStringsRemaining = ForwardWrap;
				BreakMarked = false;
			}
		}
		else if (ForwardStringsRemaining <= 0) {	//If we aren't printing the line now, store it for later if we have space in the BackStrings.
			if (BackWrap > 0) {
				BackStrings.push_back(Line);
				if (BackStrings.size() > BackWrap) {
					BackStrings.erase(BackStrings.begin());
					if (!BreakMarked) {
						OutFile << "===================================" << std::endl;
						BreakMarked = true;
					}
				}
			}
		}
	}

	OutFile.close();

	return true;
}

int main(int argc, char* argv[])
{
	//Uncomment this line to debug. Start the Parser normally, attach to it while stopped, then press Enter.
	//std::cin.ignore();

	if (argc < 2) {
		std::cout << "Too few argument, was a log file passed? (Drag and drop the file(s) on the .exe)\n";
	}
	else {
		for (int i = 1; i < argc; i++) {
			if (ParseLog(argv[i])) {
				std::cout << argv[i] << " properly parsed :)" << std::endl;
			}
			else {
				std::cout << "Issue parsing file" << argv[i] << std::endl;
			}
		}	
	}

	std::cout << std::endl << std::endl << " Press enter to close...";
	std::cin.ignore();
}
