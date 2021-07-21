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
#include <Editor/MainFrame/Public/Interfaces/IMainFrameModule.h>
#include <Runtime/SlateCore/Public/Styling/SlateStyle.h>
#include <Runtime/Engine/Public/Slate/SlateGameResources.h>
#include "Styling/SlateStyleRegistry.h"
#include "Editor/EditorEngine.h"
#include "BlueprintEditor.h"

using namespace std;
using namespace EAppMsgType;

#define LOCTEXT_NAMESPACE "FWakaTimeForUE4Module"

bool isGEditorLinked(false);
string apiKey("");

string baseCommand("");

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

// Pozitrone(According to StackOverflow - PherricOxide, this is the fastest method to check if file exists)
inline bool FileExists (const std::string& name) {
	struct stat buffer;   
	return (stat (name.c_str(), &buffer) == 0); 
}

void WakaCommands::RegisterCommands()
{
	UI_COMMAND(WakaTimeSettingsCommand, "Waka Time", "Waka time settings", EUserInterfaceActionType::Button, FInputChord());
}

FReply FWakaTimeForUE4Module::SaveData()
{
	UE_LOG(LogTemp, Warning, TEXT("WakaTime: Saving settings"));

	std::string apiKeyBase = TCHAR_TO_UTF8(*(apiKeyBlock.Get().GetText().ToString()));
	apiKey = apiKeyBase.substr(apiKeyBase.find(" = ") + 3);

	const char* homedrive = getenv("HOMEDRIVE");
	const char* homepath = getenv("HOMEPATH");
	
	std::string configFileDir = std::string(homedrive) + homepath + "/.wakatime.cfg";
	std::fstream configFile(configFileDir);
	
	bool foundKey = NULL;

	if(!FileExists(configFileDir))
	{
		configFile.open(configFileDir, std::fstream::out); // Pozitrone(Create the file if it does not exist) and write the data in it
		configFile << "[settings]" << '\n';
		configFile << "api_key = " << apiKey;
		configFile.close();

		SettingsWindow.Get().RequestDestroyWindow();
		return FReply::Handled();
	}
	
	if(configFile.fail())
	{
		UE_LOG(LogTemp, Error, TEXT("WakaTime: Couldn't open the config file"));
		SettingsWindow.Get().RequestDestroyWindow();
		return FReply::Handled();
	}

	vector<string> data;
	std::string tempLine;
	while(std::getline(configFile, tempLine))
	{
		if(tempLine.find("api_key") != std::string::npos)
		{
			data.push_back("api_key = " + apiKey); // if key was found, add the rewritten value to the data set
			foundKey = true;
		}
		else
		{
			data.push_back(tempLine); // If key was not found, add the according line to the data set
			foundKey = false;
		}
	}
	
	if(!foundKey)
	{
		// There is no key present, add it
		configFile << "[settings]" << '\n';
		configFile << "api_key = " << apiKey;
		configFile.close();
	}
	else
	{
		// Rewrite the file with the new override
		int index;
		for (index = 0; index < (int) data.size(); index++) {
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

bool RunCmdCommand(string commandToRun, bool requireNonZeroProcess = false)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	LPCWSTR exe = TEXT("C:\\Windows\\System32\\cmd.exe");

	wchar_t wtext[256];
	mbstowcs(wtext, commandToRun.c_str(), strlen(commandToRun.c_str()) + 1); //Plus null
	LPWSTR cmd = wtext;

	bool success = CreateProcess(exe, // use cmd
								 cmd, // the command
								 nullptr, // Process handle not inheritable
								 nullptr, // Thread handle not inheritable
								 FALSE, // Set handle inheritance to FALSE
								 CREATE_NO_WINDOW, // Don't open the console window
								 nullptr, // Use parent's environment block
								 nullptr, // Use parent's starting directory 
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

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	
	return success && returnValue;
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
	command += "--plugin \"unreal-wakatime/1.0.1\" ";
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
	// WARN: have your wakatime-cli dir in your $PATH
	// Pozitrone(there is no scenario in which the wakatime command would stop working while the project is open, so we can cache this value)
	if(RunCmdCommand("where wakatime", true)) //if we found an external command called 'wakatime'
	{
		baseCommand = (" /c start /b wakatime --entity ");
	}
	else 
	{
		// Pozitrone(Wakatime-cli.exe is not in the path by default, which is why we have to use the user path)
		baseCommand = (" /c start /b " + std::string(homedrive) + homepath + "/.wakatime/wakatime-cli/wakatime-cli.exe"  + " --entity ");
	}

	
	if (!StyleSetInstance.IsValid())
	{
		StyleSetInstance = FWakaTimeForUE4Module::Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleSetInstance);
	}


	std::string line;
	std::string lineSettings = "[settings]";
	
	std::string configFileDir = std::string(homedrive) + homepath + "/.wakatime.cfg";
	std::fstream infile(configFileDir);

	if(infile.is_open())
	{
		UE_LOG(LogTemp, Warning, TEXT("We've opened the infile"));
	}

	if(std::getline(infile, line))
	{
		if(std::getline(infile, line))
		{
			UE_LOG(LogTemp, Warning, TEXT("WakaTime: API key found."));
				if(std::getline(infile, lineSettings))
				{
					std::fstream auxFile;
						auxFile << "[settings]" << '\n';
						auxFile << infile.rdbuf();
						infile << auxFile.rdbuf();
						infile.close();

				}
			apiKey = line.substr(line.find(" = ") + 3); // Pozitrone(Extract only the api key from the line);
			apiKeyBlock.Get().SetText(FText::FromString(FString(UTF8_TO_TCHAR(apiKey.c_str()))));

			infile.close();
		}
		else
		{
			OpenSettingsWindow();
		}
	}

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
#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FWakaTimeForUE4Module, WakaTimeForUE4)

#include "Windows/HideWindowsPlatformTypes.h"
