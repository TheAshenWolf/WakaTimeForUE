// Copyright Epic Games, Inc. All Rights Reserved.
#include "WakaTimeForUE4.h"

#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/GenericPlatformMisc.h"
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
string position("Developer");
string devCategory("coding");
string apiKey("");

// Handles
FDelegateHandle NewActorsDroppedHandle;
FDelegateHandle DeleteActorsEndHandle;
FDelegateHandle DuplicateActorsEndHandle;
FDelegateHandle AddLevelToWorldHandle;
FDelegateHandle PostSaveWorldHandle;

void WakaCommands::RegisterCommands()
{
	UI_COMMAND(TestCommand, "Waka Time", "Waka time settings", EUserInterfaceActionType::Button, FInputGesture());
}



FReply FWakaTimeForUE4Module::SetDeveloper()
{
	isDeveloper = true;
	isDesigner = false;
	devCategory = "coding";
	return FReply::Handled();
}

FReply FWakaTimeForUE4Module::SetDesigner()
{
	isDeveloper = false;
	isDesigner = true;
	devCategory = "designing";
	return FReply::Handled();
}

string GetProjectName()
{
	FString projectPath = FPaths::GetProjectFilePath();

	std::string segment;
	std::vector<std::string> seglist;
	size_t pos = 0;
	std::string token;
	std::string delimiter("\\");
	std::string path = std::string(TCHAR_TO_UTF8(*projectPath));

	while ((pos = path.find(delimiter)) != std::string::npos) {
		token = path.substr(0, pos);
		std::cout << token << std::endl;
		path.erase(0, pos + delimiter.length());
	
		seglist.push_back(segment);
	}

	return seglist.back();
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

#ifdef __unix__ // If Mac or Linux
		UE_LOG(LogTemp, Error, TEXT("Python not found. Please, install it and restart the editor."));
		FMessageDialog::Open(Ok, FText::FromString("Python not found. Please, install it and restart the editor."));
		FGenericPlatformMisc::RequestExit(false);															 // Quits the editor
		system("gnome-terminal -x sh -c 'echo sudo apt-get install python3; sudo apt-get install python3'"); // Starts terminal and runs a command prompting user for password
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__) // If windows
			UE_LOG(LogTemp, Error, TEXT("Python not found. Please, install it and restart the editor."));
		_pclose(_popen("python", "r")); // Opens Microsoft Store
		FMessageDialog::Open(Ok, FText::FromString("Python not found. Please, install it and restart the editor."));
		FGenericPlatformMisc::RequestExit(false); // Quits the editor
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
#ifdef __unix__ // If Mac or Linux
		UE_LOG(LogTemp, Warning, TEXT("Installing Pip..."));
		system("gnome-terminal -x sh -c 'echo sudo apt-get install python3-pip; sudo apt-get install python3-pip'");
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__) // If windows
		UE_LOG(LogTemp, Warning, TEXT("Installing Pip..."));
		_pclose(_popen("python ../../../Resources/get-pip.py", "r"));
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
	char buffer[128];
	string result = "";
	FILE* pipe;

	pipe = _popen("wakatime -h", "r");

	while (!feof(pipe))
	{

		if (fgets(buffer, 128, pipe) != NULL)
			result += buffer;
	}

	if (result.find("usage: wakatime") == std::string::npos)
	{
		UE_LOG(LogTemp, Warning, TEXT("CLI found."));
	}
	else
	{
#ifdef __unix__ // If Mac or Linux
		UE_LOG(LogTemp, Warning, TEXT("Installing WakaTimeCLI..."));
		system("gnome-terminal -x sh -c 'echo sudo pip install wakatime; sudo pip install wakatime'");
		system("gnome-terminal -x sh -c 'cd ../../../Resources/bash-wakatime");
		system("gnome-terminal -x sh -c 'source ./bash-wakatime.sh'");
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__) // If windows
		UE_LOG(LogTemp, Warning, TEXT("Installing WakaTimeCLI..."));
		_pclose(_popen("pip install wakatime", "r"));
#else // Else...
		UE_LOG(LogTemp, Warning, TEXT("Installing WakaTimeCLI..."));
		_pclose(_popen("pip install wakatime", "r"));
#endif
	}
}

void SendHeartbeat(bool fileSave, std::string filePath)
{
	UE_LOG(LogTemp, Warning, TEXT("Sending Heartbeat"));

	char buffer[128];
	string result = "";
	FILE *pipe;

	string command("wakatime --file " + filePath + " ");
	if (apiKey != "") {
		command += "--key " + apiKey + " ";
	}
	if (fileSave) {
		command += "--write ";
	}

	command += "--project " + GetProjectName() + " ";
	command += "--key aeba87d5-dfb5-4df1-9eea-653c87bb350f"; // TODO: REMOVE

	pipe = _popen(command.c_str(), "r");

	while (!feof(pipe))
	{
		if (fgets(buffer, 128, pipe) != NULL)
			result += buffer;
	}
}

void SendHeartbeat(bool fileSave, FString filepath) {
	std::string path(TCHAR_TO_UTF8(*filepath));
	SendHeartbeat(fileSave, path);
}


void FWakaTimeForUE4Module::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	CheckForPython();
	CheckForPip();
	CheckForWakaTimeCLI();

	// Add Listeners
	NewActorsDroppedHandle = FEditorDelegates::OnNewActorsDropped.AddRaw(this, &FWakaTimeForUE4Module::OnNewActorDropped);
	DeleteActorsEndHandle = FEditorDelegates::OnDeleteActorsEnd.AddRaw(this, &FWakaTimeForUE4Module::OnDeleteActorsEnd);
	DuplicateActorsEndHandle = FEditorDelegates::OnDuplicateActorsEnd.AddRaw(this, &FWakaTimeForUE4Module::OnDuplicateActorsEnd);
	AddLevelToWorldHandle = FEditorDelegates::OnAddLevelToWorld.AddRaw(this, &FWakaTimeForUE4Module::OnAddLevelToWorld);
	PostSaveWorldHandle = FEditorDelegates::PostSaveWorld.AddRaw(this, &FWakaTimeForUE4Module::OnPostSaveWorld);

	WakaCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);
	PluginCommands->MapAction(
		WakaCommands::Get().TestCommand,
		FExecuteAction::CreateRaw(this, &FWakaTimeForUE4Module::TestAction)
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
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

void FWakaTimeForUE4Module::OnNewActorDropped(const TArray<UObject*>& Objects, const TArray<AActor*>& Actors) {
	UE_LOG(LogTemp, Warning, TEXT("Actor Dropped."));
}

void FWakaTimeForUE4Module::OnDuplicateActorsEnd() {
	UE_LOG(LogTemp, Warning, TEXT("Actor duplicated."));
}

void FWakaTimeForUE4Module::OnDeleteActorsEnd() {
	UE_LOG(LogTemp, Warning, TEXT("Actor deleted."));
}

void FWakaTimeForUE4Module::OnAddLevelToWorld(ULevel* Level) {
	UE_LOG(LogTemp, Warning, TEXT("Added level to world."));
}

void FWakaTimeForUE4Module::OnPostSaveWorld(uint32 SaveFlags, UWorld* World, bool bSucces)
{
	UE_LOG(LogTemp, Warning, TEXT("Save world."));
	/*FString fileName(World->GetFName().ToString());
	SendHeartbeat(true, fileName);*/
}

void FWakaTimeForUE4Module::TestAction() {
	UE_LOG(LogTemp, Warning, TEXT("Waka Waka, Eh eh!"));

	FMargin baseMargin;
	baseMargin.Left = 100;
	baseMargin.Right = 100;

	TSharedRef<SWindow> SettingsWindow = SNew(SWindow)
		.Title(FText::FromString(TEXT("WakaTime Settings")))
		.ClientSize(FVector2D(800, 400))
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center).Padding(baseMargin)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Top)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Your api key:")))
					]
					+ SVerticalBox::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SEditableTextBox)
						.Text(FText::FromString(FString(UTF8_TO_TCHAR(apiKey.c_str()))))
					]
					
					
				]
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center).Padding(baseMargin)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Top)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("I am working as a:")))
					]
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Top)
					[
						SNew(STextBlock)
						.Text(FText::FromString(FString(UTF8_TO_TCHAR(position.c_str()))))
					]
				]
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center).Padding(baseMargin)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Bottom)
						[
							SNew(SButton)
							.Text(FText::FromString(TEXT("Developer")))
							.OnClicked(this, &FWakaTimeForUE4Module::SetDeveloper)
						]
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Bottom)
						[
							SNew(SButton)
							.Text(FText::FromString(TEXT("Designer")))
							.OnClicked(this, &FWakaTimeForUE4Module::SetDesigner)
						]
				]
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Top).Padding(baseMargin)
				[
				SNew(SEditableTextBox)
					.Text(FText::FromString(FString(UTF8_TO_TCHAR(apiKey.c_str()))))
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