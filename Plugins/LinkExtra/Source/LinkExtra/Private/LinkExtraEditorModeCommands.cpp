// Copyright Epic Games, Inc. All Rights Reserved.

#include "LinkExtraEditorModeCommands.h"

#include "Framework/Commands/InputChord.h"
#include "Framework/Commands/UICommandInfo.h"
#include "Styling/AppStyle.h"
#include "UObject/NameTypes.h"
#include "UObject/UnrealNames.h"

#define LOCTEXT_NAMESPACE "LinkExtraEditorModeCommands"

FName FLinkExtraEditorModeCommands::LinkExtraContext = TEXT("LinkExtraEditorMode");

FLinkExtraEditorModeCommands::FLinkExtraEditorModeCommands()
	: TCommands<FLinkExtraEditorModeCommands>
(
	FLinkExtraEditorModeCommands::LinkExtraContext, //Context name for fast lookup
	NSLOCTEXT("Contexts", "LinkExtraEditorMode", "LinkExtra Editor Mode"), // Localized context name for displaying
	NAME_None, //"LevelEditor" // Parent
	FAppStyle::GetAppStyleSetName()// Icon Style Set
	)
{
}

void FLinkExtraEditorModeCommands::RegisterCommands()
{
	UI_COMMAND(LandscapeMode, "Mode - Landscape", "", EUserInterfaceActionType::RadioButton, FInputChord());
	NameToCommandMap.Add("ToolMode_Manage", LandscapeMode);
	UI_COMMAND(FoliageMode, "Mode - Foliage", "", EUserInterfaceActionType::RadioButton, FInputChord());
	NameToCommandMap.Add("ToolMode_Sculpt", FoliageMode);
	UI_COMMAND(WaterMode, "Mode - Water", "", EUserInterfaceActionType::RadioButton, FInputChord());
	NameToCommandMap.Add("ToolMode_Paint", WaterMode);
	
}

#undef LOCTEXT_NAMESPACE
