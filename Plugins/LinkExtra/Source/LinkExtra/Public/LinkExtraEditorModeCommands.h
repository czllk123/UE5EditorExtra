// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Map.h"
#include "Framework/Commands/Commands.h"
#include "Templates/SharedPointer.h"

class FName;
class FUICommandInfo;


/**
 * This class contains info about the full set of commands used in this editor mode.
 */
class FLinkExtraEditorModeCommands : public TCommands<FLinkExtraEditorModeCommands>
{
public:
	FLinkExtraEditorModeCommands();

	/**
	* Initialize commands
	*/
	virtual void RegisterCommands() override;
	
public:
	static FName LinkExtraContext;
	
	// Mode Switch
	TSharedPtr<FUICommandInfo> LandscapeMode;
	TSharedPtr<FUICommandInfo> FoliageMode;
	TSharedPtr<FUICommandInfo> WaterMode;

	// Map
	TMap<FName, TSharedPtr<FUICommandInfo>> NameToCommandMap;


};
