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

bool isDeveloper(true);
bool isDesigner(false);
bool isDebugging(false);
bool isGEditorLinked(false);
string position("Developer");
string devCategory("coding");
string apiKey("");

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
TSharedRef<STextBlock> positionBlock = SNew(STextBlock)
.Text(FText::FromString(FString(UTF8_TO_TCHAR(position.c_str())))).MinDesiredWidth(100).Justification(
	                                                       ETextJustify::Right);

TSharedRef<SEditableTextBox> apiKeyBlock = SNew(SEditableTextBox)
.Text(FText::FromString(FString(UTF8_TO_TCHAR(apiKey.c_str())))).MinDesiredWidth(500);

TSharedRef<SWindow> SettingsWindow = SNew(SWindow);

TSharedPtr<FSlateStyleSet> StyleSetInstance = NULL;


void WakaCommands::RegisterCommands()
{
	UI_COMMAND(TestCommand, "Waka Time", "Waka time settings", EUserInterfaceActionType::Button, FInputChord());
}

FReply FWakaTimeForUE4Module::SetDeveloper()
{
	isDeveloper = true;
	isDesigner = false;
	devCategory = "coding";
	position = "Developer";
	positionBlock.Get().SetText(FText::FromString(FString(UTF8_TO_TCHAR(position.c_str()))));
	return FReply::Handled();
}

FReply FWakaTimeForUE4Module::SetDesigner()
{
	isDeveloper = false;
	isDesigner = true;
	devCategory = "designing";
	position = "Designer";
	positionBlock.Get().SetText(FText::FromString(FString(UTF8_TO_TCHAR(position.c_str()))));
	return FReply::Handled();
}


FReply FWakaTimeForUE4Module::SaveData()
{
	UE_LOG(LogTemp, Display, TEXT("WakaTime: Saving settings"));
	apiKey = TCHAR_TO_UTF8(*(apiKeyBlock.Get().GetText().ToString()));
	std::ofstream saveFile;
	saveFile.open("wakatimeSaveData.txt");
	saveFile << position << '\n';
	saveFile << apiKey;
	saveFile.close();
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

void SendHeartbeat(bool fileSave, std::string filePath)
{
	UE_LOG(LogTemp, Display, TEXT("WakaTime: Sending Heartbeat"));

	string command(" /C start /B wakatime --entity \"" + filePath + "\" ");
	if (apiKey != "")
	{
		command += "--key " + apiKey + " ";
	}

	command += "--project \"" + GetProjectName() + "\" ";
	command += "--entity-type \"app\" ";
	command += "--language \"Unreal Engine\" ";
	command += "--plugin \"unreal-wakatime/1.0.1\" ";
	command += "--category " + (isDebugging ? "debugging" : devCategory) + " ";

	if (fileSave)
	{
		command += "--write ";
	}

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	LPCWSTR exe = TEXT("C:\\Windows\\System32\\cmd.exe");

	wchar_t wtext[256];
	mbstowcs(wtext, command.c_str(), strlen(command.c_str()) + 1); //Plus null
	LPWSTR cmd = wtext;

	bool success = CreateProcess(exe, // use cmd
	                             cmd, // the command
	                             nullptr, // Process handle not inheritable
	                             nullptr, // Thread handle not inheritable
	                             FALSE, // Set handle inheritance to FALSE
	                             CREATE_NO_WINDOW, // Dont open the console window
	                             nullptr, // Use parent's environment block
	                             nullptr, // Use parent's starting directory 
	                             &si, // Pointer to STARTUPINFO structure
	                             &pi); // Pointer to PROCESS_INFORMATION structure

	// Wait until process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);

	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	if (success)
	{
		UE_LOG(LogTemp, Display, TEXT("WakaTime: Heartbeat successfully sent."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("WakaTime: Heartbeat couldn't be sent."));
		UE_LOG(LogTemp, Error, TEXT("WakaTime: Error code = %d"), GetLastError());
	}
}

void SendHeartbeat(bool fileSave, FString filepath)
{
	std::string path(TCHAR_TO_UTF8(*filepath));
	SendHeartbeat(fileSave, path);
}

void FWakaTimeForUE4Module::StartupModule()
{
	if (!StyleSetInstance.IsValid())
	{
		StyleSetInstance = FWakaTimeForUE4Module::Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleSetInstance);
	}


	std::string line;
	std::ifstream infile("wakatimeSaveData.txt");

	if (std::getline(infile, line))
	{
		if (line == "Developer")
		{
			UE_LOG(LogTemp, Display, TEXT("WakaTime: Position set to Developer"));
			SetDeveloper();
		}
		else if (line == "Designer")
		{
			UE_LOG(LogTemp, Display, TEXT("WakaTime: Position set to Designer"));
			SetDesigner();
		}

		if (std::getline(infile, line))
		{
			UE_LOG(LogTemp, Display, TEXT("WakaTime: Api key found."));
			apiKey = line;
			apiKeyBlock.Get().SetText(FText::FromString(FString(UTF8_TO_TCHAR(apiKey.c_str()))));

			infile.close();
		}
		else
		{
			OpenSettingsWindow();
		}
	}
	else
	{
		OpenSettingsWindow();
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
	PluginCommands->MapAction(
		WakaCommands::Get().TestCommand,
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

	FString projectDirectory;

	if (FPaths::DirectoryExists(FPaths::ProjectPluginsDir() / "WakaTimeForUE4-main"))
	{
		projectDirectory = (FPaths::ProjectPluginsDir() / "WakaTimeForUE4-main" / "Resources");
		UE_LOG(LogTemp, Display, TEXT("WakaTime: Main detected"));
	}
	else if (FPaths::DirectoryExists(FPaths::ProjectPluginsDir() / "WakaTimeForUE4-release"))
	{
		projectDirectory = (FPaths::ProjectPluginsDir() / "WakaTimeForUE4-release" / "Resources");
		UE_LOG(LogTemp, Display, TEXT("WakaTime: Release detected"));
	}
	else
	{
		projectDirectory = (FPaths::ProjectPluginsDir() / "WakaTimeForUE4" / "Resources");
		UE_LOG(LogTemp, Warning, TEXT("WakaTime: Neither detected"));
	}


	Style->SetContentRoot(projectDirectory);
	Style->Set("mainIcon", new FSlateImageBrush(projectDirectory + "/Icon128.png", FVector2D(40, 40), FSlateColor()));
	return Style;
}

void FWakaTimeForUE4Module::OnNewActorDropped(const TArray<UObject*>& Objects, const TArray<AActor*>& Actors)
{
	SendHeartbeat(false, GetProjectName());
}

void FWakaTimeForUE4Module::OnDuplicateActorsEnd()
{
	SendHeartbeat(false, GetProjectName());
}

void FWakaTimeForUE4Module::OnDeleteActorsEnd()
{
	SendHeartbeat(false, GetProjectName());
}

void FWakaTimeForUE4Module::OnAddLevelToWorld(ULevel* Level)
{
	SendHeartbeat(false, GetProjectName());
}

void FWakaTimeForUE4Module::OnPostSaveWorld(uint32 SaveFlags, UWorld* World, bool bSucces)
{
	SendHeartbeat(false, GetProjectName());
}

void FWakaTimeForUE4Module::OnPostPIEStarted(bool bIsSimulating)
{
	isDebugging = true;
	SendHeartbeat(false, GetProjectName());
}

void FWakaTimeForUE4Module::OnPrePIEEnded(bool bIsSimulating)
{
	isDebugging = false;
	SendHeartbeat(false, GetProjectName());
}

void FWakaTimeForUE4Module::OnBlueprintCompiled()
{
	SendHeartbeat(false, GetProjectName());
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
		  .VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			  .HAlign(HAlign_Right)
			  .VAlign(VAlign_Top)
			[
				SNew(STextBlock).Justification(ETextJustify::Left)
				                .Text(FText::FromString(TEXT("I am working as a: "))).MinDesiredWidth(100)
			]
			+ SHorizontalBox::Slot()
			  .HAlign(HAlign_Left)
			  .VAlign(VAlign_Top)
			[
				positionBlock
			]
		]
		+ SVerticalBox::Slot()
		  .HAlign(HAlign_Center)
		  .VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			  .HAlign(HAlign_Left)
			  .VAlign(VAlign_Bottom)
			[
				SNew(SBox).WidthOverride(100)
				[
					SNew(SButton)
				.Text(FText::FromString(TEXT("Developer"))).ToolTipText(
						             FText::FromString(TEXT("You activity will be marked as \"coding\"")))
		.OnClicked(FOnClicked::CreateRaw(this, &FWakaTimeForUE4Module::SetDeveloper))
				]
			]
			+ SHorizontalBox::Slot()
			  .HAlign(HAlign_Left)
			  .VAlign(VAlign_Bottom)
			[
				SNew(SBox).WidthOverride(100)
				[
					SNew(SButton)
				.Text(FText::FromString(TEXT("Designer"))).ToolTipText(
						             FText::FromString(TEXT("You activity will be marked as \"designing\"")))
		.OnClicked(FOnClicked::CreateRaw(this, &FWakaTimeForUE4Module::SetDesigner))
				]

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

	Builder.AddToolBarButton(WakaCommands::Get().TestCommand, NAME_None, FText::FromString("WakaTime"),
	                         FText::FromString("WakaTime plugin settings"),
	                         icon, NAME_None);
	//Style->Set("Niagara.CompileStatus.Warning", new IMAGE_BRUSH("Icons/CompileStatus_Warning", Icon40x40));
}
#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FWakaTimeForUE4Module, WakaTimeForUE4)

#include "Windows/HideWindowsPlatformTypes.h"
