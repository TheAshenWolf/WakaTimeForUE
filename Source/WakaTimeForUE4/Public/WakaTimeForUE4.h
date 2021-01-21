// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FWakaTimeForUE4Module : public IModuleInterface
{
public:


	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	void OnNewActorDropped(const TArray<UObject*>& Objects, const TArray<AActor*>& Actors);
	void OnDuplicateActorsEnd();
	void OnDeleteActorsEnd();
	void OnAddLevelToWorld(ULevel* Level);
	void OnPostSaveWorld(uint32 SaveFlags, UWorld* World, bool bSucces);
	void OnPostPIEStarted(bool bIsSimulating);
	void OnPrePIEEnded(bool bIsSimulating);
	void OpenSettingsWindow();
	void AddToolbarButton(FToolBarBuilder& Builder);
	FReply SetDeveloper();
	FReply SetDesigner();
	FReply SaveData();

	TSharedPtr<FUICommandList> PluginCommands;
};

class WakaCommands : public TCommands<WakaCommands>
{
public:

	WakaCommands()
		: TCommands<WakaCommands>
		(
			TEXT("TutorialPluginEditor"),
			NSLOCTEXT("Contexts", "TutorialPluginEditor", "TutorialPluginEditor Plugin"),
			NAME_None,
			FEditorStyle::GetStyleSetName()
			) {}

	virtual void RegisterCommands() override;

public:

	TSharedPtr<FUICommandInfo> TestCommand;
};