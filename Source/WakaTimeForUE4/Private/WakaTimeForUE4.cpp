// Copyright Epic Games, Inc. All Rights Reserved.

#include "WakaTimeForUE4.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Misc/MessageDialog.h"
#include <string>
#include <array>
#include <iostream>
#include <stdexcept>
#include <stdio.h>


using namespace std;
using namespace EAppMsgType;

#define LOCTEXT_NAMESPACE "FWakaTimeForUE4Module"

void CheckForPython() {
	UE_LOG(LogTemp, Warning, TEXT("Checking Python installation..."));
	char buffer[128];
	string result = "";
	string py = "Python 3";
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

void FWakaTimeForUE4Module::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	CheckForPython();
	// char* result = _popen("python --version", "r");
}

void FWakaTimeForUE4Module::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FWakaTimeForUE4Module, WakaTimeForUE4)