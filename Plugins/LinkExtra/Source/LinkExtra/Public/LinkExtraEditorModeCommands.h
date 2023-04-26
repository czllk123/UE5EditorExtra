// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"


enum class ELinkToolMode
{
	MaskPainter = 1,
	FoliagePainter = 2,
	//MaskPainterToolModeMax,
};
/**
 * This class contains info about the full set of commands used in this editor mode.
 */
class FLinkExtraEditorModeCommands : public TCommands<FLinkExtraEditorModeCommands>
{
public:
	FLinkExtraEditorModeCommands();

	virtual void RegisterCommands() override;
	static TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> GetCommands();

	TSharedPtr<FUICommandInfo> MaskPainterMode;
	TSharedPtr<FUICommandInfo> FoliagePainterMode;

protected:
	TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> Commands;
};
