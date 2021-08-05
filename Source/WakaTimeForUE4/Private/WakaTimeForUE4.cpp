// Copyright Epic Games, Inc. All Rights Reserved.
// ReSharper disable CppParameterMayBeConst
// ReSharper disable CppLocalVariableMayBeConst
#include "WakaTimeForUE4.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "GeneralProjectSettings.h"
#include "LevelEditor.h"
#include "WakaTimeHelpers.h"
#include "Styling/SlateStyleRegistry.h"
#include <Editor/MainFrame/Public/Interfaces/IMainFrameModule.h>
#include <activation.h>
#include <fstream>

using namespace std;

#define LOCTEXT_NAMESPACE "FWakaTimeForUE4Module"

// Global variables
string GAPIKey("");
string GBaseCommand("");
char* GHomedrive;
char* GHomepath;
string GWakatimeArchitecture;
string GWakaCliVersion;

// Handles
FDelegateHandle NewActorsDroppedHandle;
FDelegateHandle DeleteActorsEndHandle;
FDelegateHandle DuplicateActorsEndHandle;
FDelegateHandle AddLevelToWorldHandle;
FDelegateHandle PostSaveWorldHandle;
FDelegateHandle GPostPieStartedHandle;
FDelegateHandle GPrePieEndedHandle;
FDelegateHandle BlueprintCompiledHandle;

// UI Elements
TSharedRef<SEditableTextBox> GAPIKeyBlock = SNew(SEditableTextBox)
.Text(FText::FromString(FString(UTF8_TO_TCHAR(GAPIKey.c_str())))).MinDesiredWidth(500);
TSharedRef<SWindow> SettingsWindow = SNew(SWindow);
TSharedPtr<FSlateStyleSet> StyleSetInstance = nullptr;


// Module methods

void FWakaTimeForUE4Module::StartupModule()
{
	AssignGlobalVariables();


	// testing for "wakatime-cli.exe" which is used by most IDEs
	if (FWakaTimeHelpers::RunCmdCommand(
		"where /r " + string(GHomedrive) + GHomepath + "\\.wakatime\\wakatime-cli\\ wakatime-cli.exe",
		true))
	{
		UE_LOG(LogTemp, Warning, TEXT("WakaTime: Found IDE wakatime-cli"));
		GBaseCommand = (string(GHomedrive) + GHomepath + "\\.wakatime\\wakatime-cli\\wakatime-cli.exe ");
	}
	else
	{
		// neither way was found; download and install the new version
		UE_LOG(LogTemp, Warning, TEXT("WakaTime: Did not find wakatime"));
		GBaseCommand = "/c start /b  " + string(GHomedrive) + GHomepath + "\\.wakatime\\wakatime-cli\\" +
			GWakaCliVersion;
		string FolderPath = string(GHomedrive) + GHomepath + "\\.wakatime\\wakatime-cli";
		if (!FWakaTimeHelpers::PathExists(FolderPath))
		{
			FWakaTimeHelpers::RunCmdCommand("mkdir " + FolderPath, false, INFINITE);
		}
		DownloadWakatimeCli(string(GHomedrive) + GHomepath + "/.wakatime/wakatime-cli/" + GWakaCliVersion);
	}
	// Pozitrone(Wakatime-cli.exe is not in the path by default, which is why we have to use the user path)


	if (!StyleSetInstance.IsValid())
	{
		StyleSetInstance = CreateToolbarIcon();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleSetInstance);
	}

	string ConfigFileDir = string(GHomedrive) + GHomepath + "/.wakatime.cfg";
	HandleStartupApiCheck(ConfigFileDir);

	// Add Listeners
	NewActorsDroppedHandle = FEditorDelegates::OnNewActorsDropped.AddRaw(
		this, &FWakaTimeForUE4Module::OnNewActorDropped);
	DeleteActorsEndHandle = FEditorDelegates::OnDeleteActorsEnd.AddRaw(this, &FWakaTimeForUE4Module::OnDeleteActorsEnd);
	DuplicateActorsEndHandle = FEditorDelegates::OnDuplicateActorsEnd.AddRaw(
		this, &FWakaTimeForUE4Module::OnDuplicateActorsEnd);
	AddLevelToWorldHandle = FEditorDelegates::OnAddLevelToWorld.AddRaw(this, &FWakaTimeForUE4Module::OnAddLevelToWorld);
	PostSaveWorldHandle = FEditorDelegates::PostSaveWorld.AddRaw(this, &FWakaTimeForUE4Module::OnPostSaveWorld);
	GPostPieStartedHandle = FEditorDelegates::PostPIEStarted.AddRaw(this, &FWakaTimeForUE4Module::OnPostPieStarted);
	GPrePieEndedHandle = FEditorDelegates::PrePIEEnded.AddRaw(this, &FWakaTimeForUE4Module::OnPrePieEnded);
	if (GEditor)
	{
		BlueprintCompiledHandle = GEditor->OnBlueprintCompiled().AddRaw(
			this, &FWakaTimeForUE4Module::OnBlueprintCompiled);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("WakaTime: No GEditor present"));
	}

	FWakaCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	// This handles the WakaTime settings button in the top bar
	PluginCommands->MapAction(
		FWakaCommands::Get().WakaTimeSettingsCommand,
		FExecuteAction::CreateRaw(this, &FWakaTimeForUE4Module::OpenSettingsWindow)
	);

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");


	TSharedPtr<FExtender> NewToolbarExtender = MakeShareable(new FExtender);

	NewToolbarExtender->AddToolBarExtension("Content",
	                                        EExtensionHook::Before,
	                                        PluginCommands,
	                                        FToolBarExtensionDelegate::CreateRaw(
		                                        this, &FWakaTimeForUE4Module::AddToolbarButton));
	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(NewToolbarExtender);
}

void FWakaTimeForUE4Module::ShutdownModule()
{
	// Removing event handles
	FEditorDelegates::OnNewActorsDropped.Remove(NewActorsDroppedHandle);
	FEditorDelegates::OnDeleteActorsEnd.Remove(DeleteActorsEndHandle);
	FEditorDelegates::OnDuplicateActorsEnd.Remove(DuplicateActorsEndHandle);
	FEditorDelegates::OnAddLevelToWorld.Remove(AddLevelToWorldHandle);
	FEditorDelegates::PostSaveWorld.Remove(PostSaveWorldHandle);
	FEditorDelegates::PostPIEStarted.Remove(GPostPieStartedHandle);
	FEditorDelegates::PrePIEEnded.Remove(GPrePieEndedHandle);
	if (GEditor)
	{
		GEditor->OnBlueprintCompiled().Remove(BlueprintCompiledHandle);
	}

	free(GHomedrive);
	free(GHomepath);
}

void FWakaCommands::RegisterCommands()
{
	UI_COMMAND(WakaTimeSettingsCommand, "Waka Time", "Waka time settings", EUserInterfaceActionType::Button,
	           FInputChord());
}


// Initialization methods

void FWakaTimeForUE4Module::AssignGlobalVariables()
{
	// use _dupenv_s instead of getenv, as it is safer
	GHomedrive = nullptr;
	size_t LenDrive = NULL;
	_dupenv_s(&GHomedrive, &LenDrive, "HOMEDRIVE");

	GHomepath = nullptr;
	size_t LenPath = NULL;
	_dupenv_s(&GHomepath, &LenPath, "HOMEPATH");

	WCHAR BufferW[256];
	GWakatimeArchitecture = GetSystemWow64DirectoryW(BufferW, 256) == 0 ? "386" : "amd64";
	GWakaCliVersion = "wakatime-cli-windows-" + GWakatimeArchitecture + ".exe";
}

void FWakaTimeForUE4Module::HandleStartupApiCheck(string ConfigFilePath)
{
	string Line;
	bool bFoundApiKey = false;

	if (!FWakaTimeHelpers::PathExists(ConfigFilePath))
		// if there is no .wakatime.cfg, open the settings window straight up
	{
		OpenSettingsWindow();
		return;
	}

	fstream ConfigFile(ConfigFilePath);

	while (getline(ConfigFile, Line))
	{
		if (Line.find("api_key") != string::npos)
		{
			GAPIKey = Line.substr(Line.find(" = ") + 3); // Pozitrone(Extract only the api key from the line);
			GAPIKeyBlock.Get().SetText(FText::FromString(FString(UTF8_TO_TCHAR(GAPIKey.c_str()))));
			ConfigFile.close();
			bFoundApiKey = true;
		}
	}

	if (!bFoundApiKey)
	{
		OpenSettingsWindow(); // if key was not found, open the settings
	}
}

void FWakaTimeForUE4Module::DownloadWakatimeCli(string CliPath)
{
	if (FWakaTimeHelpers::PathExists(CliPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("WakaTime: CLI found"));
		return; // if CLI exists, no need to change anything
	}

	UE_LOG(LogTemp, Warning, TEXT("WakaTime: CLI not found, attempting download."));

	string URL = "https://github.com/wakatime/wakatime-cli/releases/download/v1.18.9/wakatime-cli-windows-" +
		GWakatimeArchitecture + ".zip";

	string LocalZipFilePath = string(GHomedrive) + GHomepath + "/.wakatime/" + "wakatime-cli.zip";

	bool bSuccessDownload = FWakaTimeHelpers::DownloadFile(URL, LocalZipFilePath);

	if (bSuccessDownload)
	{
		UE_LOG(LogTemp, Warning, TEXT("WakaTime: Successfully downloaded wakatime-cli.zip"));
		bool bSuccessUnzip = FWakaTimeHelpers::UnzipArchive(LocalZipFilePath,
		                                                    string(GHomedrive) + GHomepath + "/.wakatime/wakatime-cli");

		if (bSuccessUnzip) UE_LOG(LogTemp, Warning, TEXT("WakaTime: Successfully extracted wakatime-cli."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("WakaTime: Error downloading python. Please, install it manually."));
	}
}

string FWakaTimeForUE4Module::GetProjectName()
{
	const TCHAR* ProjectName = FApp::GetProjectName();
	string MainModuleName = TCHAR_TO_UTF8(ProjectName);
	const UGeneralProjectSettings& ProjectSettings = *GetDefault<UGeneralProjectSettings>();
	if (ProjectSettings.ProjectName != "")
	{
		return TCHAR_TO_UTF8(*(ProjectSettings.ProjectName));
	}

	if (MainModuleName != "")
	{
		return TCHAR_TO_UTF8(ProjectName);
	}

	return "Unreal Engine 4";
}


// UI methods

TSharedRef<FSlateStyleSet> FWakaTimeForUE4Module::CreateToolbarIcon()
{
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet("WakaTime2DStyle"));

	FString EngineDirectory;

	if (FPaths::DirectoryExists(FPaths::EnginePluginsDir() / "WakaTimeForUE4-main"))
	{
		EngineDirectory = (FPaths::EnginePluginsDir() / "WakaTimeForUE4-main" / "Resources");
		UE_LOG(LogTemp, Warning, TEXT("WakaTime: Main detected"));
	}
	else if (FPaths::DirectoryExists(FPaths::EnginePluginsDir() / "WakaTimeForUE4-release"))
	{
		EngineDirectory = (FPaths::EnginePluginsDir() / "WakaTimeForUE4-release" / "Resources");
		UE_LOG(LogTemp, Warning, TEXT("WakaTime: Release detected"));
	}
	else
	{
		EngineDirectory = (FPaths::EnginePluginsDir() / "WakaTimeForUE4" / "Resources");
		UE_LOG(LogTemp, Warning, TEXT("WakaTime: Neither detected"));
	}


	Style->SetContentRoot(EngineDirectory);
	Style->Set("mainIcon", new FSlateImageBrush(EngineDirectory + "/Icon128.png", FVector2D(40, 40), FSlateColor()));
	return Style;
}

void FWakaTimeForUE4Module::AddToolbarButton(FToolBarBuilder& Builder)
{
	FSlateIcon Icon = FSlateIcon(TEXT("WakaTime2DStyle"), "mainIcon"); //Style.Get().GetStyleSetName(), "Icon128.png");

	Builder.AddToolBarButton(FWakaCommands::Get().WakaTimeSettingsCommand, NAME_None, FText::FromString("WakaTime"),
	                         FText::FromString("WakaTime plugin settings"),
	                         Icon, NAME_None);
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
				GAPIKeyBlock
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

FReply FWakaTimeForUE4Module::SaveData()
{
	UE_LOG(LogTemp, Warning, TEXT("WakaTime: Saving settings"));

	string APIKeyBase = TCHAR_TO_UTF8(*(GAPIKeyBlock.Get().GetText().ToString()));
	GAPIKey = APIKeyBase.substr(APIKeyBase.find(" = ") + 1);

	string ConfigFileDir = string(GHomedrive) + GHomepath + "/.wakatime.cfg";
	fstream ConfigFile(ConfigFileDir);

	if (!FWakaTimeHelpers::PathExists(ConfigFileDir) || ConfigFile.fail())
	{
		ConfigFile.open(ConfigFileDir, fstream::out);
		// Pozitrone(Create the file if it does not exist) and write the data in it
		ConfigFile << "[settings]" << '\n';
		ConfigFile << "api_key = " << GAPIKey;
		ConfigFile.close();

		SettingsWindow.Get().RequestDestroyWindow();
		return FReply::Handled();
	}

	TArray<string> Data;
	string TempLine;
	bool bFoundKey = false;
	while (getline(ConfigFile, TempLine))
	{
		if (TempLine.find("api_key") != string::npos)
		{
			Data.Add("api_key = " + GAPIKey); // if key was found, add the rewritten value to the data set
			bFoundKey = true;
		}
		else
		{
			Data.Add(TempLine); // If key was not found, add the according line to the data set
		}
	}
	ConfigFile.close();

	if (!bFoundKey)
	{
		// There is no key present, add it
		ConfigFile.open(ConfigFileDir, fstream::out);
		ConfigFile << "[settings]" << '\n';
		ConfigFile << "api_key = " << GAPIKey;
		ConfigFile.close();
	}
	else
	{
		// Rewrite the file with the new override
		ConfigFile.open(ConfigFileDir, fstream::out);
		for (int Index = 0; Index < Data.Num(); Index++)
		{
			ConfigFile << Data[Index] << endl;
		}
		ConfigFile.close();
	}

	SettingsWindow.Get().RequestDestroyWindow();
	return FReply::Handled();
}

// Lifecycle methods

void FWakaTimeForUE4Module::SendHeartbeat(bool bFileSave, string FilePath, string Activity)
{
	UE_LOG(LogTemp, Warning, TEXT("WakaTime: Sending Heartbeat"));

	string Command = GBaseCommand;

	Command += " --entity ";

	Command += " \"" + FilePath + "\" ";

	if (GAPIKey != "")
	{
		Command += "--key " + GAPIKey + " ";
	}

	Command += "--project \"" + GetProjectName() + "\" ";
	Command += "--entity-type \"app\" ";
	Command += "--language \"Unreal Engine\" ";
	Command += "--plugin \"unreal-wakatime/1.2.0\" ";
	Command += "--category " + Activity + " ";

	if (bFileSave)
	{
		Command += "--write ";
	}

	//UE_LOG(LogTemp, Warning, TEXT("%s"), *FString(UTF8_TO_TCHAR(command.c_str())));
	bool bSuccess = false;
	try
	{
		bSuccess = FWakaTimeHelpers::RunCmdCommand(Command, false, INFINITE, true);
	}
	catch (int Err)
	{
		UE_LOG(LogTemp, Warning, TEXT("%i"), Err);
	}

	//bool success = RunCommand(command, false, baseCommand,INFINITE, true);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("WakaTime: Heartbeat successfully sent."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("WakaTime: Heartbeat couldn't be sent."));
		UE_LOG(LogTemp, Error, TEXT("WakaTime: Error code = %d"), GetLastError());
	}
}

// Event methods

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
	SendHeartbeat(true, GetProjectName(), "designing");
}

void FWakaTimeForUE4Module::OnPostPieStarted(bool bIsSimulating)
{
	SendHeartbeat(false, GetProjectName(), "debugging");
}

void FWakaTimeForUE4Module::OnPrePieEnded(bool bIsSimulating)
{
	SendHeartbeat(true, GetProjectName(), "debugging");
}

void FWakaTimeForUE4Module::OnBlueprintCompiled()
{
	SendHeartbeat(true, GetProjectName(), "coding");
}


#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FWakaTimeForUE4Module, WakaTimeForUE4)

#include "Windows/HideWindowsPlatformTypes.h"
