// Copyright Epic Games, Inc. All Rights Reserved.
#include "WakaTimeForUE4.h"

#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Framework/SlateDelegates.h"
#include "Templates/SharedPointer.h"
#include "Misc/MessageDialog.h"
#include "Misc/CoreDelegates.h"
#include "LevelEditor.h"
#include "Misc/Paths.h"
#include "Editor.h"

#include <stdexcept>
#include <iostream>
#include <stdio.h>
#include <sstream>
#include <fstream>
#include <string>
#include <chrono>
#include <vector>
#include <array>
#include <Editor/MainFrame/Public/Interfaces/IMainFrameModule.h>


using namespace std;
using namespace EAppMsgType;

#define LOCTEXT_NAMESPACE "FWakaTimeForUE4Module"

bool isDeveloper(true);
bool isDesigner(false);
bool isDebugging(false);
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

// UI Elements
TSharedRef<STextBlock> positionBlock = SNew(STextBlock)
.Text(FText::FromString(FString(UTF8_TO_TCHAR(position.c_str())))).MinDesiredWidth(100).Justification(ETextJustify::Right);

TSharedRef<SEditableTextBox> apiKeyBlock = SNew(SEditableTextBox)
.Text(FText::FromString(FString(UTF8_TO_TCHAR(apiKey.c_str())))).MinDesiredWidth(500);

TSharedRef<SWindow> SettingsWindow = SNew(SWindow);


void WakaCommands::RegisterCommands()
{
	UI_COMMAND(TestCommand, "Waka Time", "Waka time settings", EUserInterfaceActionType::Button, FInputGesture());
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


FReply FWakaTimeForUE4Module::SaveData() {
	UE_LOG(LogTemp, Warning, TEXT("WakaTime: Saving settings"));
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
	return "\"Unreal Engine 4\"";
}

void SendHeartbeat(bool fileSave, std::string filePath)
{
	UE_LOG(LogTemp, Warning, TEXT("WakaTime: Sending Heartbeat"));

	string command("wakatime --entity " + filePath + " ");
	if (apiKey != "") {
		command += "--key " + apiKey + " ";
	}
	if (fileSave) {
		command += "--write ";
	}

	command += "--project " + GetProjectName() + " ";
	command += "--entity-type \"app\" ";
	command += "--plugin \"unreal-wakatime/1.0.0\" ";
	command += "--category " + (isDebugging ? "debugging" : devCategory) + " ";

	// UE_LOG(LogTemp, Log, TEXT("WakaTime cmd: %s"), *FString(command.c_str()));
	system(command.c_str()); // TODO: Rework for windows
}

void SendHeartbeat(bool fileSave, FString filepath) {
	std::string path(TCHAR_TO_UTF8(*filepath));
	SendHeartbeat(fileSave, path);
}

void FWakaTimeForUE4Module::StartupModule()
{
	std::string line;
	std::ifstream infile("wakatimeSaveData.txt");

	if (std::getline(infile, line))
	{
		if (line == "Developer")
		{
			UE_LOG(LogTemp, Warning, TEXT("WakaTime: Position set to Developer"));
			SetDeveloper();
		}
		else if (line == "Designer")
		{
			; (LogTemp, Warning, TEXT("WakaTime: Position set to Designer"));
			SetDesigner();
		}

		if (std::getline(infile, line)) {
			UE_LOG(LogTemp, Warning, TEXT("WakaTime: Api key found."));
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
	NewActorsDroppedHandle = FEditorDelegates::OnNewActorsDropped.AddRaw(this, &FWakaTimeForUE4Module::OnNewActorDropped);
	DeleteActorsEndHandle = FEditorDelegates::OnDeleteActorsEnd.AddRaw(this, &FWakaTimeForUE4Module::OnDeleteActorsEnd);
	DuplicateActorsEndHandle = FEditorDelegates::OnDuplicateActorsEnd.AddRaw(this, &FWakaTimeForUE4Module::OnDuplicateActorsEnd);
	AddLevelToWorldHandle = FEditorDelegates::OnAddLevelToWorld.AddRaw(this, &FWakaTimeForUE4Module::OnAddLevelToWorld);
	PostSaveWorldHandle = FEditorDelegates::PostSaveWorld.AddRaw(this, &FWakaTimeForUE4Module::OnPostSaveWorld);
	PostPIEStartedHandle = FEditorDelegates::PostPIEStarted.AddRaw(this, &FWakaTimeForUE4Module::OnPostPIEStarted);
	PrePIEEndedHandle = FEditorDelegates::PrePIEEnded.AddRaw(this, &FWakaTimeForUE4Module::OnPrePIEEnded);

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
			FToolBarExtensionDelegate::CreateRaw(this, &FWakaTimeForUE4Module::AddToolbarButton));
		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(NewToolbarExtender);
	}
}

void FWakaTimeForUE4Module::ShutdownModule()
{

	FEditorDelegates::OnNewActorsDropped.Remove(NewActorsDroppedHandle);
	FEditorDelegates::OnDeleteActorsEnd.Remove(DeleteActorsEndHandle);
	FEditorDelegates::OnDuplicateActorsEnd.Remove(DuplicateActorsEndHandle);
	FEditorDelegates::OnAddLevelToWorld.Remove(AddLevelToWorldHandle);
	FEditorDelegates::PostSaveWorld.Remove(PostSaveWorldHandle);
	FEditorDelegates::PostPIEStarted.Remove(PostPIEStartedHandle);
	FEditorDelegates::PrePIEEnded.Remove(PrePIEEndedHandle);
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

void FWakaTimeForUE4Module::OnNewActorDropped(const TArray<UObject*>& Objects, const TArray<AActor*>& Actors) {
	SendHeartbeat(false, GetProjectName());
}

void FWakaTimeForUE4Module::OnDuplicateActorsEnd() {
	SendHeartbeat(false, GetProjectName());
}

void FWakaTimeForUE4Module::OnDeleteActorsEnd() {
	SendHeartbeat(false, GetProjectName());
}

void FWakaTimeForUE4Module::OnAddLevelToWorld(ULevel* Level) {
	SendHeartbeat(false, GetProjectName());
}

void FWakaTimeForUE4Module::OnPostSaveWorld(uint32 SaveFlags, UWorld* World, bool bSucces)
{
	SendHeartbeat(true, GetProjectName());
}

void FWakaTimeForUE4Module::OnPostPIEStarted(bool bIsSimulating)
{
	isDebugging = true;
	SendHeartbeat(true, GetProjectName());
}

void FWakaTimeForUE4Module::OnPrePIEEnded(bool bIsSimulating)
{
	isDebugging = false;
	SendHeartbeat(true, GetProjectName());
}


void FWakaTimeForUE4Module::OpenSettingsWindow() {

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
				.Text(FText::FromString(TEXT("Developer"))).ToolTipText(FText::FromString(TEXT("You activity will be marked as \"coding\"")))
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
				.Text(FText::FromString(TEXT("Designer"))).ToolTipText(FText::FromString(TEXT("You activity will be marked as \"designing\"")))
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
	Builder.AddToolBarButton(WakaCommands::Get().TestCommand);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FWakaTimeForUE4Module, WakaTimeForUE4)