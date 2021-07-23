// Copyright Epic Games, Inc. All Rights Reserved.
#include "WakaTimeForUE4.h"

#include "Framework/Application/SlateApplication.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Framework/SlateDelegates.h"
#include "GeneralProjectSettings.h"
#include "LevelEditor.h"
#include "Editor.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <Editor/MainFrame/Public/Interfaces/IMainFrameModule.h>
#include <Runtime/SlateCore/Public/Styling/SlateStyle.h>
#include <Runtime/Engine/Public/Slate/SlateGameResources.h>
#include "Styling/SlateStyleRegistry.h"
#include "Editor/EditorEngine.h"
#include "BlueprintEditor.h"
#include <wow64apiset.h>
#include <direct.h>

using namespace std;
using namespace EAppMsgType;

#define LOCTEXT_NAMESPACE "FWakaTimeForUE4Module"

bool isGEditorLinked(false);
std::string apiKey("");

string baseCommand("");

const char* homedrive = getenv("HOMEDRIVE");
const char* homepath = getenv("HOMEPATH");

// Handles
FDelegateHandle NewActorsDroppedHandle;
FDelegateHandle DeleteActorsEndHandle;
FDelegateHandle DuplicateActorsEndHandle;
FDelegateHandle AddLevelToWorldHandle;
FDelegateHandle PostSaveWorldHandle;
FDelegateHandle PostPIEStartedHandle;
FDelegateHandle PrePIEEndedHandle;
FDelegateHandle BlueprintCompiledHandle;

// UI Elements
TSharedRef<SEditableTextBox> apiKeyBlock = SNew(SEditableTextBox)
.Text(FText::FromString(FString(UTF8_TO_TCHAR(apiKey.c_str())))).MinDesiredWidth(500);

TSharedRef<SWindow> SettingsWindow = SNew(SWindow);

TSharedPtr<FSlateStyleSet> StyleSetInstance = NULL;


void WakaCommands::RegisterCommands()
{
	UI_COMMAND(WakaTimeSettingsCommand, "Waka Time", "Waka time settings", EUserInterfaceActionType::Button, FInputChord());
}

FReply FWakaTimeForUE4Module::SaveData()
{
	UE_LOG(LogTemp, Warning, TEXT("WakaTime: Saving settings"));

	std::string apiKeyBase = TCHAR_TO_UTF8(*(apiKeyBlock.Get().GetText().ToString()));
	apiKey = apiKeyBase.substr(apiKeyBase.find(" = ") + 1);

	const char* homedrive = getenv("HOMEDRIVE");
	const char* homepath = getenv("HOMEPATH");
	
	std::string configFileDir = std::string(homedrive) + homepath + "/.wakatime.cfg";
	std::fstream configFile(configFileDir);

	if(!FWakaTimeHelpers::FileExists(configFileDir) || configFile.fail())
	{
		configFile.open(configFileDir, std::fstream::out); // Pozitrone(Create the file if it does not exist) and write the data in it
		configFile << "[settings]" << '\n';
		configFile << "api_key = " << apiKey;
		configFile.close();

		SettingsWindow.Get().RequestDestroyWindow();
		return FReply::Handled();
	}

	TArray<string> data;
	std::string tempLine;
	bool foundKey = false;
	while(std::getline(configFile, tempLine))
	{
		if(tempLine.find("api_key") != std::string::npos)
		{
			data.Add("api_key = " + apiKey); // if key was found, add the rewritten value to the data set
			foundKey = true;
		}
		else
		{
			data.Add(tempLine); // If key was not found, add the according line to the data set
		}
	}
	configFile.close();
	
	if(!foundKey)
	{
		// There is no key present, add it
		configFile.open(configFileDir, std::fstream::out);
		configFile << "[settings]" << '\n';
		configFile << "api_key = " << apiKey;
		configFile.close();
	}
	else
	{
		// Rewrite the file with the new override
		configFile.open(configFileDir, std::fstream::out);
		int index;
		for (index = 0; index < (int) data.Num(); index++) {
			configFile << data[index] << endl;
		}
		configFile.close();
	}
	
	SettingsWindow.Get().RequestDestroyWindow();
	return FReply::Handled();
}

string GetProjectName()
{
	const TCHAR* projectName = FApp::GetProjectName();
	std::string mainModuleName = TCHAR_TO_UTF8(projectName);
	const UGeneralProjectSettings& ProjectSettings = *GetDefault<UGeneralProjectSettings>();
	if (ProjectSettings.ProjectName != "")
	{
		return TCHAR_TO_UTF8(*(ProjectSettings.ProjectName));
	}
	else if (mainModuleName != "")
	{
		return TCHAR_TO_UTF8(projectName);
	}

	return "Unreal Engine 4";
}

bool RunCmdCommand(string commandToRun, bool requireNonZeroProcess = false, bool usePowershell = false, int waitMs = 0, bool runPure = false, string directory = "")
{
	if (runPure)
	{
		commandToRun = " " + commandToRun;
	}
	else
	{
		commandToRun = " /c start /b " + commandToRun;
	}

	UE_LOG(LogTemp, Warning, TEXT("Running command: %s"), *FString(UTF8_TO_TCHAR(commandToRun.c_str())));
	
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	wchar_t wtext[256];
	mbstowcs(wtext, commandToRun.c_str(), strlen(commandToRun.c_str()) + 1); //Plus null
	LPWSTR cmd = wtext;

	bool success = CreateProcess(*FString(UTF8_TO_TCHAR(exeToRun.c_str())), // use cmd or powershell
								 cmd, // the command
								 nullptr, // Process handle not inheritable
								 nullptr, // Thread handle not inheritable
								 FALSE, // Set handle inheritance to FALSE
								 CREATE_NO_WINDOW, // Don't open the console window
								 nullptr, // Use parent's environment block
								 directory == "" ? nullptr : *FString(UTF8_TO_TCHAR(directory.c_str())), // Use parent's starting directory 
								 &si, // Pointer to STARTUPINFO structure
								 &pi); // Pointer to PROCESS_INFORMATION structure

	// Close process and thread handles.
	bool returnValue;
	
	if (requireNonZeroProcess)
	{
		DWORD ec;
		GetExitCodeProcess(pi.hProcess, &ec);
		returnValue = (ec == 0);
	}
	else
	{
		returnValue = true;
	}

	WaitForSingleObject(pi.hProcess, waitMs);
	
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	
	return success && returnValue;
}

bool RunPowershellCommand(string commandToRun, bool requireNonZeroProcess = false, int waitMs = 0, bool runPure = false, string directory = "")
{
	return RunCommand(commandToRun, requireNonZeroProcess, "C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe", waitMs, runPure, directory);
}

bool RunCmdCommand(string commandToRun, bool requireNonZeroProcess = false, int waitMs = 0, bool runPure = false, string directory = "")
{
	return RunCommand(commandToRun, requireNonZeroProcess, "C:\\Windows\\System32\\cmd.exe", waitMs, runPure, directory);
}

void SendHeartbeat(bool fileSave, std::string filePath, std::string activity)
{
	UE_LOG(LogTemp, Warning, TEXT("WakaTime: Sending Heartbeat"));

	string command = baseCommand;

	command += " \"" + filePath + "\" ";
	
	if (apiKey != "")
	{
		command += "--key " + apiKey + " ";
	}

	command += "--project \"" + GetProjectName() + "\" ";
	command += "--entity-type \"app\" ";
	command += "--language \"Unreal Engine\" ";
	command += "--plugin \"unreal-wakatime/1.2.0\" ";
	command += "--category " + activity + " ";

	if (fileSave)
	{
		command += "--write ";
	}

	//UE_LOG(LogTemp, Warning, TEXT("%s"), *FString(UTF8_TO_TCHAR(command.c_str())));
	
	bool success = RunCmdCommand(command);

	if (success)
	{
		UE_LOG(LogTemp, Warning, TEXT("WakaTime: Heartbeat successfully sent."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("WakaTime: Heartbeat couldn't be sent."));
		UE_LOG(LogTemp, Error, TEXT("WakaTime: Error code = %d"), GetLastError());
	}
}

void SendHeartbeat(bool fileSave, FString filepath, std::string activity)
{
	std::string path(TCHAR_TO_UTF8(*filepath));
	SendHeartbeat(fileSave, path, activity);
}

void FWakaTimeForUE4Module::StartupModule()
{
	const char* homedrive = getenv("HOMEDRIVE");
	const char* homepath = getenv("HOMEPATH");

	// look for an external command called 'wakatime'. if it exists, use the pure `wakatime` command, but if it doesn't, call the executable by its full name
	// Pozitrone(there is no scenario in which the wakatime command would stop working while the project is open, so we can cache this value)
	if(RunCmdCommand("where wakatime", true)) //if we found an external command called 'wakatime'
	{
		baseCommand = ("wakatime --entity ");
	}
	else 
	{
		// Pozitrone(Wakatime-cli.exe is not in the path by default, which is why we have to use the user path)
		baseCommand = (std::string(homedrive) + homepath + "/.wakatime/wakatime-cli/wakatime-cli.exe"  + " --entity ");

		string folderPath = string(homedrive) + homepath + "\\.wakatime";
		
		RunCmdCommand("mkdir " + folderPath, false,  INFINITE);

		DownloadWakatimeCLI(std::string(homedrive) + homepath + "/.wakatime/wakatime-cli/wakatime-cli.exe");
		
	}

	
	if (!StyleSetInstance.IsValid())
	{
		StyleSetInstance = FWakaTimeForUE4Module::Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleSetInstance);
	}
	
	std::string configFileDir = std::string(homedrive) + homepath + "/.wakatime.cfg";
	HandleStartupApiCheck(configFileDir);

	// Add Listeners
	NewActorsDroppedHandle = FEditorDelegates::OnNewActorsDropped.AddRaw(
		this, &FWakaTimeForUE4Module::OnNewActorDropped);
	DeleteActorsEndHandle = FEditorDelegates::OnDeleteActorsEnd.AddRaw(this, &FWakaTimeForUE4Module::OnDeleteActorsEnd);
	DuplicateActorsEndHandle = FEditorDelegates::OnDuplicateActorsEnd.AddRaw(
		this, &FWakaTimeForUE4Module::OnDuplicateActorsEnd);
	AddLevelToWorldHandle = FEditorDelegates::OnAddLevelToWorld.AddRaw(this, &FWakaTimeForUE4Module::OnAddLevelToWorld);
	PostSaveWorldHandle = FEditorDelegates::PostSaveWorld.AddRaw(this, &FWakaTimeForUE4Module::OnPostSaveWorld);
	PostPIEStartedHandle = FEditorDelegates::PostPIEStarted.AddRaw(this, &FWakaTimeForUE4Module::OnPostPIEStarted);
	PrePIEEndedHandle = FEditorDelegates::PrePIEEnded.AddRaw(this, &FWakaTimeForUE4Module::OnPrePIEEnded);
	if (GEditor)
	{
		BlueprintCompiledHandle = GEditor->OnBlueprintCompiled().AddRaw(
			this, &FWakaTimeForUE4Module::OnBlueprintCompiled);
		isGEditorLinked = true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("WakaTime: No GEditor present"));
	}

	WakaCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	// This handles the WakaTime settings button in the top bar
	PluginCommands->MapAction(
		WakaCommands::Get().WakaTimeSettingsCommand,
		FExecuteAction::CreateRaw(this, &FWakaTimeForUE4Module::OpenSettingsWindow)
	);

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	{
		TSharedPtr<FExtender> NewToolbarExtender = MakeShareable(new FExtender);

		NewToolbarExtender->AddToolBarExtension("Content",
												EExtensionHook::Before,
												PluginCommands,
												FToolBarExtensionDelegate::CreateRaw(
													this, &FWakaTimeForUE4Module::AddToolbarButton));
		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(NewToolbarExtender);
	}
}

void FWakaTimeForUE4Module::ShutdownModule()
{
	// Removing event handles
	FEditorDelegates::OnNewActorsDropped.Remove(NewActorsDroppedHandle);
	FEditorDelegates::OnDeleteActorsEnd.Remove(DeleteActorsEndHandle);
	FEditorDelegates::OnDuplicateActorsEnd.Remove(DuplicateActorsEndHandle);
	FEditorDelegates::OnAddLevelToWorld.Remove(AddLevelToWorldHandle);
	FEditorDelegates::PostSaveWorld.Remove(PostSaveWorldHandle);
	FEditorDelegates::PostPIEStarted.Remove(PostPIEStartedHandle);
	FEditorDelegates::PrePIEEnded.Remove(PrePIEEndedHandle);
	if (GEditor)
	{
		GEditor->OnBlueprintCompiled().Remove(BlueprintCompiledHandle);
	}
}

TSharedRef<FSlateStyleSet> FWakaTimeForUE4Module::Create()
{
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet("WakaTime2DStyle"));

	FString engineDirectory;

	if (FPaths::DirectoryExists(FPaths::EnginePluginsDir() / "WakaTimeForUE4-main"))
	{
		engineDirectory = (FPaths::EnginePluginsDir() / "WakaTimeForUE4-main" / "Resources");
		UE_LOG(LogTemp, Warning, TEXT("WakaTime: Main detected"));
	}
	else if (FPaths::DirectoryExists(FPaths::EnginePluginsDir() / "WakaTimeForUE4-release"))
	{
		engineDirectory = (FPaths::EnginePluginsDir() / "WakaTimeForUE4-release" / "Resources");
		UE_LOG(LogTemp, Warning, TEXT("WakaTime: Release detected"));
	}
	else
	{
		engineDirectory = (FPaths::EnginePluginsDir() / "WakaTimeForUE4" / "Resources");
		UE_LOG(LogTemp, Warning, TEXT("WakaTime: Neither detected"));
	}


	Style->SetContentRoot(engineDirectory);
	Style->Set("mainIcon", new FSlateImageBrush(engineDirectory + "/Icon128.png", FVector2D(40, 40), FSlateColor()));
	return Style;
}

void FWakaTimeForUE4Module::OnNewActorDropped(const TArray<UObject*>& Objects, const TArray<AActor*>& Actors)
{
	SendHeartbeat(false, GetProjectName(), "designing");
}

void FWakaTimeForUE4Module::OnDuplicateActorsEnd()
{
	SendHeartbeat(false, GetProjectName(), "designing");
}

void FWakaTimeForUE4Module::OnDeleteActorsEnd()
{
	SendHeartbeat(false, GetProjectName(), "designing");
}

void FWakaTimeForUE4Module::OnAddLevelToWorld(ULevel* Level)
{
	SendHeartbeat(false, GetProjectName(), "designing");
}

void FWakaTimeForUE4Module::OnPostSaveWorld(uint32 SaveFlags, UWorld* World, bool bSucces)
{
	SendHeartbeat(false, GetProjectName(), "designing");
}

void FWakaTimeForUE4Module::OnPostPIEStarted(bool bIsSimulating)
{
	SendHeartbeat(false, GetProjectName(), "debugging");
}

void FWakaTimeForUE4Module::OnPrePIEEnded(bool bIsSimulating)
{
	SendHeartbeat(false, GetProjectName(), "debugging");
}

void FWakaTimeForUE4Module::OnBlueprintCompiled()
{
	SendHeartbeat(false, GetProjectName(), "coding");
}


void FWakaTimeForUE4Module::OpenSettingsWindow()
{
	SettingsWindow = SNew(SWindow)
		.Title(FText::FromString(TEXT("WakaTime Settings")))
		.ClientSize(FVector2D(800, 400))
		.SupportsMaximize(false)
		.SupportsMinimize(false).IsTopmostWindow(true)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		  .HAlign(HAlign_Center)
		  .VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			  .HAlign(HAlign_Left)
			  .VAlign(VAlign_Top)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Your api key:"))).MinDesiredWidth(500)
			]
			+ SVerticalBox::Slot()
			  .HAlign(HAlign_Center)
			  .VAlign(VAlign_Center)
			[
				apiKeyBlock
			]
		]
		+ SVerticalBox::Slot()
		  .HAlign(HAlign_Center)
		  .VAlign(VAlign_Bottom)
		[
			SNew(SBox).WidthOverride(100)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Save")))
		.OnClicked(FOnClicked::CreateRaw(this, &FWakaTimeForUE4Module::SaveData))
			]
		]
	];
	IMainFrameModule& MainFrameModule =
		FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT
			("MainFrame"));
	if (MainFrameModule.GetParentWindow().IsValid())
	{
		FSlateApplication::Get().AddWindowAsNativeChild
		(SettingsWindow, MainFrameModule.GetParentWindow()
										.ToSharedRef());
	}
	else
	{
		FSlateApplication::Get().AddWindow(SettingsWindow);
	}
}

void FWakaTimeForUE4Module::AddToolbarButton(FToolBarBuilder& Builder)
{
	FSlateIcon icon = FSlateIcon(TEXT("WakaTime2DStyle"), "mainIcon"); //Style.Get().GetStyleSetName(), "Icon128.png");

	Builder.AddToolBarButton(WakaCommands::Get().WakaTimeSettingsCommand, NAME_None, FText::FromString("WakaTime"),
							 FText::FromString("WakaTime plugin settings"),
							 icon, NAME_None);
	//Style->Set("Niagara.CompileStatus.Warning", new IMAGE_BRUSH("Icons/CompileStatus_Warning", Icon40x40));
}


bool UnzipArchive(std::string zipFile, std::string savePath)
{
	if (!FWakaTimeHelpers::FileExists(zipFile)) return false;

	string extractCommand = "powershell -command \"Expand-Archive -Force \"" + zipFile + "\" \"" + savePath + "\"";
	return RunPowershellCommand(extractCommand, false,  INFINITE, true);
}

bool DownloadFile(std::string url, std::string saveTo)
{
	string downloadCommand = "powershell -command \"(new-object System.Net.WebClient).DownloadFile('" + url + "','" + saveTo + "')\"";

	UE_LOG(LogTemp, Warning, TEXT("%s"), *FString(UTF8_TO_TCHAR(downloadCommand.c_str())));
	return RunPowershellCommand(downloadCommand, false,  INFINITE, true);
}

void FWakaTimeForUE4Module::DownloadPython()
{
	/*if (RunCommand("where python"))
	{
		UE_LOG(LogTemp, Warning, TEXT("WakaTime: Python found"));
		return; // if Python exists, no need to change anything
	}*/
	UE_LOG(LogTemp, Warning, TEXT("WakaTime: Python not found, attempting download."));

	string pythonVersion = "3.9.0";

	WCHAR BufferW[256];	
	string architecture = GetSystemWow64DirectoryW(BufferW, 256) == 0 ? "win32" : "amd64"; // Checks whether the computer is running 32bit or 64bit

	string url = "https://www.python.org/ftp/python/" + pythonVersion + "/python-" + pythonVersion + "-embed-" + architecture + ".zip"; // location of embed python
	string savePath = std::string(homedrive) + homepath + "/.wakatime/wakatime-cli/python.zip";

	bool successDownload = DownloadFile(url, savePath);

	if (successDownload)
	{
		UE_LOG(LogTemp, Warning, TEXT("WakaTime: Successfully downloaded Python.zip"));
		bool successUnzip = UnzipArchive(savePath, std::string(homedrive) + homepath + "/.wakatime/wakatime-cli/");

		if (successUnzip) UE_LOG(LogTemp, Warning, TEXT("WakaTime: Successfully extracted Python."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("WakaTime: Error downloading python. Please, install it manually."));
	}
}

void FWakaTimeForUE4Module::DownloadWakatimeCLI(std::string cliPath)
{
	if (FWakaTimeHelpers::FileExists(cliPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("WakaTime: CLI found"));
		return; // if CLI exists, no need to change anything
	}
	UE_LOG(LogTemp, Warning, TEXT("WakaTime: CLI not found, attempting download."));
	
	string url = "https://codeload.github.com/wakatime/wakatime/zip/master";
	string localZipFilePath = std::string(homedrive) + homepath + "/.wakatime/" + "wakatime-cli.zip";

	bool successDownload = DownloadFile(url, localZipFilePath);

	if (successDownload)
	{
		UE_LOG(LogTemp, Warning, TEXT("WakaTime: Successfully downloaded wakatime-cli.zip"));
		bool successUnzip = UnzipArchive(localZipFilePath, std::string(homedrive) + homepath + "/.wakatime/");

		string pathBase = string(homedrive) + homepath + "\\.wakatime";
		
		bool successMove = RunCmdCommand("ren " + pathBase + "\\wakatime-master wakatime-cli", false,  INFINITE);

		if (successUnzip && successMove) UE_LOG(LogTemp, Warning, TEXT("WakaTime: Successfully extracted wakatime-cli."));
		DownloadPython();
		InstallWakatimeCli();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("WakaTime: Error downloading python. Please, install it manually."));
	}	
}

void FWakaTimeForUE4Module::InstallWakatimeCli()
{
	string FolderPath = string(homedrive) + homepath + "/.wakatime/wakatime-cli/";
	bool successInstall;
	
	
	/*if (RunCommand("where python")) // if python found...
	{
		// ... perform install using "$ python ..."
		InstallCommand += " & python ./setup.py install";
	}
	else
	{*/
		// ... else use the extracted python.exe
		successInstall = RunCommand(" ./setup.py install", false, FolderPath + "\\python.exe", INFINITE, true);
	//}

	if (successInstall)
	{
		UE_LOG(LogTemp, Error, TEXT("WakaTime: Successfully installed Wakatime-cli"));
	}
}

void FWakaTimeForUE4Module::HandleStartupApiCheck(std::string configFileDir)
{	
	std::string line;
	bool foundApiKey = false;

	if(!FWakaTimeHelpers::FileExists(configFileDir)) // if there is no .wakatime.cfg, open the settings window straight up
	{
		OpenSettingsWindow();
		return;
	}

	std::fstream configFile(configFileDir);
	
	while(std::getline(configFile, line))
	{
		if(line.find("api_key") != std::string::npos)
		{
			apiKey = line.substr(line.find(" = ") + 3); // Pozitrone(Extract only the api key from the line);
			apiKeyBlock.Get().SetText(FText::FromString(FString(UTF8_TO_TCHAR(apiKey.c_str()))));
			configFile.close();
			foundApiKey = true;
		}
	}

	if (!foundApiKey)
	{
		OpenSettingsWindow(); // if key was not found, open the settings
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FWakaTimeForUE4Module, WakaTimeForUE4)

#include "Windows/HideWindowsPlatformTypes.h"
