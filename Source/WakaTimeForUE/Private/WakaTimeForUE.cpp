// ReSharper disable CppParameterMayBeConst
// ReSharper disable CppLocalVariableMayBeConst
#include "WakaTimeForUE.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "GeneralProjectSettings.h"
#include "LevelEditor.h"
#include "WakaTimeHelpers.h"
#include "Styling/SlateStyleRegistry.h"
#include <Editor/MainFrame/Public/Interfaces/IMainFrameModule.h>
#include <activation.h>
#include <fstream>

#include "Interfaces/IPluginManager.h"
#include "UObject/ObjectSaveContext.h"

using namespace std;

#define LOCTEXT_NAMESPACE "FWakaTimeForUEModule"

// Global variables
string GAPIKey("");
string GBaseCommand("");
char* GUserProfile;
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


DEFINE_LOG_CATEGORY(LogWakaTime);

// Module methods

void FWakaTimeForUEModule::StartupModule()
{
	AssignGlobalVariables();


	// testing for "wakatime-cli.exe" which is used by most IDEs
	if (FWakaTimeHelpers::RunCmdCommand(
		"where /r " + string(GUserProfile) + "\\.wakatime\\" + GWakaCliVersion,
		true))
	{
		UE_LOG(LogWakaTime, Log, TEXT("Found IDE wakatime-cli"));
		GBaseCommand = (string(GUserProfile) + "\\.wakatime\\" + GWakaCliVersion);
	}
	else
	{
		// neither way was found; download and install the new version
		UE_LOG(LogWakaTime, Log, TEXT("Did not find wakatime"));
		GBaseCommand = "/c start \"\" /b \"" + string(GUserProfile) + "\\.wakatime\\" +
			GWakaCliVersion + "\"";
		string FolderPath = string(GUserProfile) + "\\.wakatime";
		if (!FWakaTimeHelpers::PathExists(FolderPath))
		{
			FWakaTimeHelpers::RunCmdCommand("mkdir " + FolderPath, false, INFINITE);
		}
		DownloadWakatimeCli(string(GUserProfile) + "/.wakatime/wakatime-cli/" + GWakaCliVersion);
	}
	// TheAshenWolf(Wakatime-cli.exe is not in the path by default, which is why we have to use the user path)


	if (!StyleSetInstance.IsValid())
	{
		StyleSetInstance = CreateToolbarIcon();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleSetInstance);
	}

	string ConfigFileDir = string(GUserProfile) + "/.wakatime.cfg";
	HandleStartupApiCheck(ConfigFileDir);

	// Add Listeners
	NewActorsDroppedHandle = FEditorDelegates::OnNewActorsDropped.AddRaw(
		this, &FWakaTimeForUEModule::OnNewActorDropped);
	DeleteActorsEndHandle = FEditorDelegates::OnDeleteActorsEnd.AddRaw(this, &FWakaTimeForUEModule::OnDeleteActorsEnd);
	DuplicateActorsEndHandle = FEditorDelegates::OnDuplicateActorsEnd.AddRaw(
		this, &FWakaTimeForUEModule::OnDuplicateActorsEnd);
	AddLevelToWorldHandle = FEditorDelegates::OnAddLevelToWorld.AddRaw(this, &FWakaTimeForUEModule::OnAddLevelToWorld);

#if ENGINE_MAJOR_VERSION >= 5
	PostSaveWorldHandle = FEditorDelegates::PostSaveWorldWithContext.AddRaw(this, &FWakaTimeForUEModule::OnPostSaveWorld);
#else// TheAshenWolf(PostSaveWorld is deprecated as of UE5)
	PostSaveWorldHandle = FEditorDelegates::PostSaveWorld.AddRaw(this, &FWakaTimeForUEModule::OnPostSaveWorld);
#endif


	
	
	GPostPieStartedHandle = FEditorDelegates::PostPIEStarted.AddRaw(this, &FWakaTimeForUEModule::OnPostPieStarted);
	GPrePieEndedHandle = FEditorDelegates::PrePIEEnded.AddRaw(this, &FWakaTimeForUEModule::OnPrePieEnded);
	if (GEditor)
	{
		BlueprintCompiledHandle = GEditor->OnBlueprintCompiled().AddRaw(
			this, &FWakaTimeForUEModule::OnBlueprintCompiled);
	}
	else
	{
		UE_LOG(LogWakaTime, Error, TEXT("No GEditor present"));
	}

	FWakaCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	// This handles the WakaTime settings button in the top bar
	PluginCommands->MapAction(
		FWakaCommands::Get().WakaTimeSettingsCommand,
		FExecuteAction::CreateRaw(this, &FWakaTimeForUEModule::OpenSettingsWindow)
	);

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");


	TSharedPtr<FExtender> NewToolbarExtender = MakeShareable(new FExtender);

	NewToolbarExtender->AddToolBarExtension("Content",
	                                        EExtensionHook::Before,
	                                        PluginCommands,
	                                        FToolBarExtensionDelegate::CreateRaw(
		                                        this, &FWakaTimeForUEModule::AddToolbarButton));
	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(NewToolbarExtender);
}

void FWakaTimeForUEModule::ShutdownModule()
{
	// Removing event handles
	FEditorDelegates::OnNewActorsDropped.Remove(NewActorsDroppedHandle);
	FEditorDelegates::OnDeleteActorsEnd.Remove(DeleteActorsEndHandle);
	FEditorDelegates::OnDuplicateActorsEnd.Remove(DuplicateActorsEndHandle);
	FEditorDelegates::OnAddLevelToWorld.Remove(AddLevelToWorldHandle);
#if ENGINE_MAJOR_VERSION >= 5
	FEditorDelegates::PostSaveWorldWithContext.Remove(PostSaveWorldHandle);
#else // TheAshenWolf(PostSaveWorld is deprecated as of UE5)
	FEditorDelegates::PostSaveWorld.Remove(PostSaveWorldHandle);
#endif
	FEditorDelegates::PostPIEStarted.Remove(GPostPieStartedHandle);
	FEditorDelegates::PrePIEEnded.Remove(GPrePieEndedHandle);
	if (GEditor)
	{
		GEditor->OnBlueprintCompiled().Remove(BlueprintCompiledHandle);
	}

	free(GUserProfile);
}

void FWakaCommands::RegisterCommands()
{
	UI_COMMAND(WakaTimeSettingsCommand, "Waka Time", "Waka time settings", EUserInterfaceActionType::Button,
	           FInputChord());
}


// Initialization methods

void FWakaTimeForUEModule::AssignGlobalVariables()
{
	// use _dupenv_s instead of getenv, as it is safer
	GUserProfile = _strdup("c:");
	size_t LenDrive = NULL;
	_dupenv_s(&GUserProfile, &LenDrive, "USERPROFILE");
	
	/*string profile = GUserProfile;
	profile.insert(0, 1, '"');
	profile.append("\"");
	
	const char* tmp = profile.c_str();
	GUserProfile = (char*)tmp;*/

	WCHAR BufferW[256];
	GWakatimeArchitecture = GetSystemWow64DirectoryW(BufferW, 256) == 0 ? "386" : "amd64";
	GWakaCliVersion = "wakatime-cli-windows-" + GWakatimeArchitecture + ".exe";
}

void FWakaTimeForUEModule::HandleStartupApiCheck(string ConfigFilePath)
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

void FWakaTimeForUEModule::DownloadWakatimeCli(string CliPath)
{
	if (FWakaTimeHelpers::PathExists(CliPath))
	{
		UE_LOG(LogWakaTime, Log, TEXT("CLI found"));
		return; // if CLI exists, no need to change anything
	}

	UE_LOG(LogWakaTime, Log, TEXT("CLI not found, attempting download."));

	string URL = "https://github.com/wakatime/wakatime-cli/releases/download/v1.18.9/wakatime-cli-windows-" +
		GWakatimeArchitecture + ".zip";

	string LocalZipFilePath = string(GUserProfile) + "/.wakatime/" + "wakatime-cli.zip";

	bool bSuccessDownload = FWakaTimeHelpers::DownloadFile(URL, LocalZipFilePath);

	if (bSuccessDownload)
	{
		UE_LOG(LogWakaTime, Log, TEXT("Successfully downloaded wakatime-cli.zip"));
		bool bSuccessUnzip = FWakaTimeHelpers::UnzipArchive(LocalZipFilePath,
		                                                    string(GUserProfile) + "/.wakatime");

		if (bSuccessUnzip) UE_LOG(LogWakaTime, Log, TEXT("Successfully extracted wakatime-cli."));
	}
	else
	{
		UE_LOG(LogWakaTime, Error, TEXT("Error downloading python. Please, install it manually."));
	}
}

string FWakaTimeForUEModule::GetProjectName()
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

	return "Unreal Engine";
}


// UI methods

TSharedRef<FSlateStyleSet> FWakaTimeForUEModule::CreateToolbarIcon()
{
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet("WakaTime2DStyle"));


	FString ResourcesDirectory = IPluginManager::Get().FindPlugin(TEXT("WakaTimeForUE"))->GetBaseDir() + "/Resources";
	UE_LOG(LogWakaTime, Log, TEXT("%s"), *ResourcesDirectory);


	Style->SetContentRoot(ResourcesDirectory);
	Style->Set("mainIcon", new FSlateImageBrush(ResourcesDirectory + "/Icon128.png", FVector2D(40, 40),
	                                            FSlateColor(FLinearColor(1, 1, 1))));

	return Style;
}

void FWakaTimeForUEModule::AddToolbarButton(FToolBarBuilder& Builder)
{
	FSlateIcon Icon = FSlateIcon(TEXT("WakaTime2DStyle"), "mainIcon"); //Style.Get().GetStyleSetName(), "Icon128.png");

	Builder.AddToolBarButton(FWakaCommands::Get().WakaTimeSettingsCommand, NAME_None, FText::FromString("WakaTime"),
	                         FText::FromString("WakaTime plugin settings"),
	                         Icon, NAME_None);
}

void FWakaTimeForUEModule::OpenSettingsWindow()
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
		.OnClicked(FOnClicked::CreateRaw(this, &FWakaTimeForUEModule::SaveData))
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

FReply FWakaTimeForUEModule::SaveData()
{
	UE_LOG(LogWakaTime, Log, TEXT("Saving settings"));

	string APIKeyBase = TCHAR_TO_UTF8(*(GAPIKeyBlock.Get().GetText().ToString()));
	GAPIKey = APIKeyBase.substr(APIKeyBase.find(" = ") + 1);

	string ConfigFileDir = string(GUserProfile) + "/.wakatime.cfg";
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

void FWakaTimeForUEModule::SendHeartbeat(bool bFileSave, string FilePath, string Activity)
{
	UE_LOG(LogWakaTime, Log, TEXT("Sending Heartbeat"));

	string Command = GBaseCommand;

	Command += " --entity \"" + GetProjectName() + "\" ";

	if (GAPIKey != "")
	{
		Command += "--key " + GAPIKey + " ";
	}

	Command += "--project \"" + GetProjectName() + "\" ";
	Command += "--entity-type \"app\" ";
	Command += "--language \"Unreal Engine\" ";
	Command += "--plugin \"unreal-wakatime/1.2.2\" ";
	Command += "--category " + Activity + " ";

	if (bFileSave)
	{
		Command += "--write";
	}

	bool bSuccess = false;
	try
	{
		bSuccess = FWakaTimeHelpers::RunCmdCommand(Command, false, INFINITE, true);
	}
	catch (int Err)
	{
		UE_LOG(LogWakaTime, Warning, TEXT("%i"), Err);
	}

	//bool success = RunCommand(command, false, baseCommand,INFINITE, true);

	if (bSuccess)
	{
		UE_LOG(LogWakaTime, Log, TEXT("Heartbeat successfully sent."));
	}
	else
	{
		UE_LOG(LogWakaTime, Error, TEXT("Heartbeat couldn't be sent."));
		UE_LOG(LogWakaTime, Error, TEXT("Error code = %d"), GetLastError());
	}
}

// Event methods

void FWakaTimeForUEModule::OnNewActorDropped(const TArray<UObject*>& Objects, const TArray<AActor*>& Actors)
{
	SendHeartbeat(false, GetProjectName(), "designing");
}

void FWakaTimeForUEModule::OnDuplicateActorsEnd()
{
	SendHeartbeat(false, GetProjectName(), "designing");
}

void FWakaTimeForUEModule::OnDeleteActorsEnd()
{
	SendHeartbeat(false, GetProjectName(), "designing");
}

void FWakaTimeForUEModule::OnAddLevelToWorld(ULevel* Level)
{
	SendHeartbeat(false, GetProjectName(), "designing");
}

#if ENGINE_MAJOR_VERSION == 5
	void FWakaTimeForUEModule::OnPostSaveWorld(UWorld* World, FObjectPostSaveContext Context)
	{
		SendHeartbeat(true, GetProjectName(), "designing");
}
#else
	void FWakaTimeForUEModule::OnPostSaveWorld(uint32 SaveFlags, UWorld* World, bool bSucces)
	{
		SendHeartbeat(true, GetProjectName(), "designing");
	}
#endif

void FWakaTimeForUEModule::OnPostPieStarted(bool bIsSimulating)
{
	SendHeartbeat(false, GetProjectName(), "debugging");
}

void FWakaTimeForUEModule::OnPrePieEnded(bool bIsSimulating)
{
	SendHeartbeat(true, GetProjectName(), "debugging");
}

void FWakaTimeForUEModule::OnBlueprintCompiled()
{
	SendHeartbeat(true, GetProjectName(), "coding");
}


#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FWakaTimeForUEModule, WakaTimeForUE)

#include "Windows/HideWindowsPlatformTypes.h"
