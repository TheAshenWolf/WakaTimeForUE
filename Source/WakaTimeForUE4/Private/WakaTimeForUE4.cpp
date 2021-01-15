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

string GetTime() {
	const auto now = std::chrono::system_clock::now();

	return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
}

string GetProjectName() {
	stringstream projectPath = FPaths::GetProjectFilePath();

	std::string segment;
	std::vector<std::string> seglist;

	while(std::getline(test, segment, '\\'))
	{
   		seglist.push_back(segment);
	}

	return seglist.back();
}

string BuildJson(string devCategory, string savedFile) {
	string entity("\"Unreal Engine\""); // "Unreal Engine"
	string type("\"app\""); // "app"
	string category("\"" + devCategory + "\""); // eg. "coding"
	string time(GetTime()); // 123456789
	string project("\"" + GetProjectName() + "\""); // "MyProject"
	string language("\"Unreal Engine\""); // "Unreal Engine"
	string isWrite(savedFile); // true

	string json = "{";								// {
	json.append("\"entity\":" + entity + ","; 		// 	   "entity":"Unreal Engine",
	json.append("\"type\":" + type + ",");			// 	   "type":"app",
	json.append("\"category\":" + category + ",");	// 	   "category":"coding",
	json.append("\"time\":" + time + ",");			//     "time":123456789,
	json.append("\"project\":" + project + ",");	//     "project":"MyProject",
	json.append("\"language\":" + language + ",");  //     "language":"Unreal Engine",
	json.append("\"is_write\":" + isWrite);			//     "is_write":true
	json.append("}");								// }
}

void CheckForPython() {
	UE_LOG(LogTemp, Warning, TEXT("Checking Python installation..."));
	char buffer[128];
	string result = "";
	string py = "Python";
	FILE* pipe;

	pipe = _popen("python --version", "r");

	while (!feof(pipe)) {

		if (fgets(buffer, 128, pipe) != NULL)
			result += buffer;
	}

	if (result.find(py) != std::string::npos) {
		UE_LOG(LogTemp, Warning, TEXT("Python found."));
	}
	else {
		UE_LOG(LogTemp, Error, TEXT("Python not found. Please, install it and restart the editor."));
		_pclose(_popen("python", "r")); // Opens Microsoft Store
		FMessageDialog::Open(Ok, FText::FromString("Python not found. Please, install it and restart the editor."));
		FGenericPlatformMisc::RequestExit(false); // Quits the editor
	}

	_pclose(pipe);
}

void SendHeartbeatCoding(bool fileSave) {
	FILE* pipe;
	pipe = _popen("curl -d '" + BuildJson("coding", fileSave ? "true" : "false") + "' -H \"Content-Type:application/json\" -X POST https://wolfwaka.free.beeceptor.com");

	// TODO: Handle response

	_pclose(pipe);
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