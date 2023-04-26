// Copyright Epic Games, Inc. All Rights Reserved.

#include "LinkExtraEditorModeCommands.h"
#include "LinkExtraEditorMode.h"
#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "LinkExtraEditorModeCommands"

FLinkExtraEditorModeCommands::FLinkExtraEditorModeCommands()
	: TCommands<FLinkExtraEditorModeCommands>("LinkExtraEditorMode",
		NSLOCTEXT("LinkExtraEditorMode", "LinkExtraEditorModeCommands", "LinkExtra Editor Mode"),
		NAME_None,
		FEditorStyle::GetStyleSetName())
{
}

void FLinkExtraEditorModeCommands::RegisterCommands()
{
	TArray <TSharedPtr<FUICommandInfo>>& ToolCommands = Commands.FindOrAdd(NAME_Default);

	UI_COMMAND(MaskPainterMode, "Mode - PaintCompMask", "", EUserInterfaceActionType::RadioButton, FInputChord());
	ToolCommands.Add(MaskPainterMode);

	UI_COMMAND(FoliagePainterMode, "Mode - PaintFoliage", "", EUserInterfaceActionType::RadioButton, FInputChord());
	ToolCommands.Add(FoliagePainterMode);

	//UI_COMMAND(InteractiveTool, "Measure Distance", "Measures distance between 2 points (click to set origin, shift-click to set end point)", EUserInterfaceActionType::ToggleButton, FInputChord());
	//ToolCommands.Add(InteractiveTool);
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> FLinkExtraEditorModeCommands::GetCommands()
{
	return FLinkExtraEditorModeCommands::Get().Commands;
}

#undef LOCTEXT_NAMESPACE
