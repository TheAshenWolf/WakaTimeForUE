// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include <string>
#include <Runtime/SlateCore/Public/Styling/SlateStyle.h>

class FWakaTimeForUE4Module : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	TSharedRef<FSlateStyleSet> CreateUI();
	void OnNewActorDropped(const TArray<UObject*>& Objects, const TArray<AActor*>& Actors);
	void OnDuplicateActorsEnd();
	void OnDeleteActorsEnd();
	void OnAddLevelToWorld(ULevel* Level);
	void OnPostSaveWorld(uint32 SaveFlags, UWorld* World, bool bSucces);
	void OnPostPieStarted(bool bIsSimulating);
	void OnPrePieEnded(bool bIsSimulating);
	void OpenSettingsWindow();
	void AddToolbarButton(FToolBarBuilder& Builder);
	void OnBlueprintCompiled();
	FReply SaveData();
	std::string GetProjectName();
	void SendHeartbeat(bool bFileSave, std::string FilePath, std::string Activity);
	void DownloadWakatimeCli(std::string CliPath);
	void HandleStartupApiCheck(std::string ConfigFileDir);
	void AssignGlobalVariables();

	TSharedPtr<FUICommandList> PluginCommands;
};

class FWakaCommands : public TCommands<FWakaCommands>
{
public:

	FWakaCommands()
		: TCommands<FWakaCommands>
		(
			TEXT("WakatimeEditor"),
			NSLOCTEXT("Wakatime", "WakatimeEditor", "Wakatime Plugin"),
			NAME_None,
			FEditorStyle::GetStyleSetName()
			)	{}

	virtual void RegisterCommands() override;

public:

	TSharedPtr<FUICommandInfo> WakaTimeSettingsCommand;
};