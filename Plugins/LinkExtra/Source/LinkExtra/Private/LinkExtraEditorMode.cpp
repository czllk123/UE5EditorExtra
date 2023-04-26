// Copyright Epic Games, Inc. All Rights Reserved.

#include "LinkExtraEditorMode.h"
#include "LinkExtraEditorModeToolkit.h"
#include "EdModeInteractiveToolsContext.h"
#include "InteractiveToolManager.h"
#include "LinkExtraEditorModeCommands.h"


//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 
// AddYourTool Step 1 - include the header file for your Tools here
//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 
#include "Tools/LinkExtraSimpleTool.h"
#include "Tools/LinkExtraInteractiveTool.h"

// step 2: register a ToolBuilder in FLinkExtraEditorMode::Enter() below


#define LOCTEXT_NAMESPACE "LinkExtraEditorMode"

const FEditorModeID ULinkExtraEditorMode::EM_LinkExtraEditorModeId = TEXT("EM_LinkExtraEditorMode");

FString ULinkExtraEditorMode::SimpleToolName = TEXT("LinkExtra_ActorInfoTool");
FString ULinkExtraEditorMode::InteractiveToolName = TEXT("LinkExtra_MeasureDistanceTool");


ULinkExtraEditorMode::ULinkExtraEditorMode()
{
	FModuleManager::Get().LoadModule("EditorStyle");

	// appearance and icon in the editing mode ribbon can be customized here
	Info = FEditorModeInfo(ULinkExtraEditorMode::EM_LinkExtraEditorModeId,
		LOCTEXT("ModeName", "LinkExtra"),
		FSlateIcon(),
		true);
}


ULinkExtraEditorMode::~ULinkExtraEditorMode()
{
}


void ULinkExtraEditorMode::ActorSelectionChangeNotify()
{
}

void ULinkExtraEditorMode::Enter()
{
	UEdMode::Enter();

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	// AddYourTool Step 2 - register the ToolBuilders for your Tools here.
	// The string name you pass to the ToolManager is used to select/activate your ToolBuilder later.
	//////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////// 
	const FLinkExtraEditorModeCommands& SampleToolCommands = FLinkExtraEditorModeCommands::Get();

	//RegisterTool(SampleToolCommands.SimpleTool, SimpleToolName, NewObject<ULinkExtraSimpleToolBuilder>(this));
	//RegisterTool(SampleToolCommands.InteractiveTool, InteractiveToolName, NewObject<ULinkExtraInteractiveToolBuilder>(this));

	// active tool type is not relevant here, we just set to default
	GetToolManager()->SelectActiveToolType(EToolSide::Left, SimpleToolName);
}

void ULinkExtraEditorMode::CreateToolkit()
{
	Toolkit = MakeShareable(new FLinkExtraEditorModeToolkit);
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> ULinkExtraEditorMode::GetModeCommands() const
{
	return FLinkExtraEditorModeCommands::Get().GetCommands();
}

#undef LOCTEXT_NAMESPACE
