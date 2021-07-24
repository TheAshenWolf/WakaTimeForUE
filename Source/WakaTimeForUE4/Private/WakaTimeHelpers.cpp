#include "WakaTimeHelpers.h"

#include <activation.h>
#include <string>

bool FWakaTimeHelpers::PathExists (const std::string& Path) {
	struct stat Buffer;   
	return (stat (Path.c_str(), &Buffer) == 0); 
}

bool FWakaTimeHelpers::RunCommand(std::string CommandToRun, bool bRequireNonZeroProcess = false,
                                  std::string ExeToRun = "C:\\Windows\\System32\\cmd.exe", int WaitMs = 0, bool bRunPure = false,
                                  std::string Directory = "")
{
	if (bRunPure)
	{
		CommandToRun = " " + CommandToRun;
	}
	else
	{
		CommandToRun = " /c start /b " + CommandToRun;
	}

	UE_LOG(LogTemp, Warning, TEXT("Running command: %s"), *FString(UTF8_TO_TCHAR(CommandToRun.c_str())));

	STARTUPINFO Startupinfo;
	PROCESS_INFORMATION Process_Information;

	ZeroMemory(&Startupinfo, sizeof(Startupinfo));
	Startupinfo.cb = sizeof(Startupinfo);
	ZeroMemory(&Process_Information, sizeof(Process_Information));

	bool bSuccess = CreateProcess(*FString(UTF8_TO_TCHAR(ExeToRun.c_str())), // use cmd or powershell
	                             UTF8_TO_TCHAR(CommandToRun.c_str()), // the command
	                             nullptr, // Process handle not inheritable
	                             nullptr, // Thread handle not inheritable
	                             FALSE, // Set handle inheritance to FALSE
	                             CREATE_NO_WINDOW, // Don't open the console window
	                             nullptr, // Use parent's environment block
	                             Directory == "" ? nullptr : *FString(UTF8_TO_TCHAR(Directory.c_str())),
	                             // Use parent's starting directory 
	                             &Startupinfo, // Pointer to STARTUPINFO structure
	                             &Process_Information); // Pointer to PROCESS_INFORMATION structure

	// Close process and thread handles.
	bool bReturnValue;

	if (bRequireNonZeroProcess)
	{
		DWORD ExitCode;
		GetExitCodeProcess(Process_Information.hProcess, &ExitCode);
		bReturnValue = (ExitCode == 0);
	}
	else
	{
		bReturnValue = true;
	}

	WaitForSingleObject(Process_Information.hProcess, WaitMs);

	CloseHandle(Process_Information.hThread);
	CloseHandle(Process_Information.hProcess);

	return bSuccess && bReturnValue;
}
