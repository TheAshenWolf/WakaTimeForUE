// Copyright Epic Games, Inc. All Rights Reserved.

#include "WakaTimeForUE4.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include <string>
#include <array>
#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <chrono>

using namespace std;
using namespace EAppMsgType;

#define LOCTEXT_NAMESPACE "FWakaTimeForUE4Module"

bool isDeveloper(true);
bool isDesigner(false);
string devCategory("coding");

void SetDeveloper()
{
	isDeveloper = true;
	isDesigner = false;
	devCategory = "coding";
}

void SetDesigner()
{
	isDeveloper = false;
	isDesigner = true;
	devCategory = "designing";
}

string GetTime()
{
	const auto now = std::chrono::system_clock::now();

	return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
}

string GetProjectName()
{
	stringstream projectPath = FPaths::GetProjectFilePath();

	std::string segment;
	std::vector<std::string> seglist;

	while (std::getline(test, segment, '\\'))
	{
		seglist.push_back(segment);
	}

	return seglist.back();
}

string BuildJson(string currentCategory, string savedFile)
{
	string entity("\"Unreal Engine\"");				// "Unreal Engine"
	string type("\"app\"");							// "app"
	string category("\"" + currentCategory + "\""); // eg. "coding"
	string time(GetTime());							// 123456789
	string project("\"" + GetProjectName() + "\""); // "MyProject"
	string language("\"Unreal Engine\"");			// "Unreal Engine"
	string isWrite(savedFile);						// true
}

void CheckForPython()
{
	UE_LOG(LogTemp, Warning, TEXT("Checking Python installation..."));
	char buffer[128];
	string result = "";
	string winPy = "not recognized";
	string unixPy = "not found";
	FILE *pipe;

	pipe = _popen("python --version", "r");

	while (!feof(pipe))
	{

		if (fgets(buffer, 128, pipe) != NULL)
			result += buffer;
	}

	if (result.find(winPy) == std::string::npos && result.find(unixPy) == std::string::npos)
	{
		UE_LOG(LogTemp, Warning, TEXT("Python found."));
	}
	else
	{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__) // If windows
		UE_LOG(LogTemp, Error, TEXT("Python not found. Please, install it and restart the editor."));
		_pclose(_popen("python", "r")); // Opens Microsoft Store
		FMessageDialog::Open(Ok, FText::FromString("Python not found. Please, install it and restart the editor."));
		FGenericPlatformMisc::RequestExit(false); // Quits the editor
#endif

#ifdef __unix__ // If Mac or Linux
		UE_LOG(LogTemp, Error, TEXT("Python not found. Please, install it and restart the editor."));
		FMessageDialog::Open(Ok, FText::FromString("Python not found. Please, install it and restart the editor."));
		FGenericPlatformMisc::RequestExit(false);															 // Quits the editor
		system("gnome-terminal -x sh -c 'echo sudo apt-get install python3; sudo apt-get install python3'"); // Starts terminal and runs a command prompting user for password
#else																										 // Else...
		UE_LOG(LogTemp, Error, TEXT("Python not found. Please, install it and restart the editor."));
		FMessageDialog::Open(Ok, FText::FromString("Python not found. Please, install it and restart the editor."));
		FGenericPlatformMisc::RequestExit(false); // Quits the editor
#endif
	}

	_pclose(pipe);
}

void CheckForPip()
{
	UE_LOG(LogTemp, Warning, TEXT("Checking Pip installation..."));
	char buffer[128];
	string result = "";
	string winPip = "not recognized";
	string unixPip = "not found";
	FILE *pipe;

	pipe = _popen("pip help", "r");

	while (!feof(pipe))
	{

		if (fgets(buffer, 128, pipe) != NULL)
			result += buffer;
	}

	if (result.find(winPip) == std::string::npos && result.find(unixPip) == std::string::npos)
	{
		UE_LOG(LogTemp, Warning, TEXT("Pip found."));
	}
	else
	{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__) // If windows
		UE_LOG(LogTemp, Warning, TEXT("Installing Pip..."));
		_pclose(_popen("python ../../../Resources/get-pip.py", "r"));
#endif

#ifdef __unix__ // If Mac or Linux
		UE_LOG(LogTemp, Warning, TEXT("Installing Pip..."));
		system("gnome-terminal -x sh -c 'echo sudo apt-get install python3-pip; sudo apt-get install python3-pip'");
#else // Else...
		UE_LOG(LogTemp, Warning, TEXT("Installing Pip..."));
		_pclose(_popen("python ../../../Resources/get-pip.py", "r"));
#endif
	}

	_pclose(pipe);
}

void CheckForWakaTimeCLI()
{
	UE_LOG(LogTemp, Warning, TEXT("Checking WakaTimeCLI installation..."));

	bool cliPresent = false;

	FILE *file;
	if (file = fopen("../../../Resources/wakatime/cli.py", "r"))
	{
		fclose(file);
		cliPresent = true;
	}
	else
	{
		cliPresent = false;
	}

	if (cliPresent)
	{
		UE_LOG(LogTemp, Warning, TEXT("CLI found."));
	}
	else
	{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__) // If windows
		UE_LOG(LogTemp, Warning, TEXT("Installing WakaTimeCLI..."));
		_pclose(_popen("pip install wakatime", "r"));
#endif

#ifdef __unix__ // If Mac or Linux
		UE_LOG(LogTemp, Warning, TEXT("Installing WakaTimeCLI..."));
		system("gnome-terminal -x sh -c 'pip install wakatime'");
#else // Else...
		UE_LOG(LogTemp, Warning, TEXT("Installing WakaTimeCLI..."));
		_pclose(_popen("pip install wakatime", "r"));
#endif
	}

	_pclose(pipe);
}

void SendHeartbeat(bool fileSave, string filePath)
{
	UE_LOG(LogTemp, Warning, TEXT("Sending Heartbeat"));

	char buffer[128];
	string result = "";
	FILE *pipe;

	pipe = _popen("", "r");

	while (!feof(pipe))
	{

		if (fgets(buffer, 128, pipe) != NULL)
			result += buffer;
	}
}

void FWakaTimeForUE4Module::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	CheckForPython();
}

void FWakaTimeForUE4Module::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FWakaTimeForUE4Module, WakaTimeForUE4)