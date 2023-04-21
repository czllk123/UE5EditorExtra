// Copyright Epic Games, Inc. All Rights Reserved.

#include "LinkEditorEditorMode.h"
#include "LinkEditorEditorModeToolkit.h"
#include "EdModeInteractiveToolsContext.h"
#include "InteractiveToolManager.h"
#include "LinkEditorEditorModeCommands.h"


//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 
// AddYourTool Step 1 - include the header file for your Tools here
//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 
#include "Tools/LinkEditorSimpleTool.h"
#include "Tools/LinkEditorInteractiveTool.h"

// step 2: register a ToolBuilder in FLinkEditorEditorMode::Enter() below


#define LOCTEXT_NAMESPACE "LinkEditorEditorMode"

const FEditorModeID ULinkEditorEditorMode::EM_LinkEditorEditorModeId = TEXT("EM_LinkEditorEditorMode");

FString ULinkEditorEditorMode::SimpleToolName = TEXT("LinkEditor_ActorInfoTool");
FString ULinkEditorEditorMode::InteractiveToolName = TEXT("LinkEditor_MeasureDistanceTool");


ULinkEditorEditorMode::ULinkEditorEditorMode()
{
	FModuleManager::Get().LoadModule("EditorStyle");

	// appearance and icon in the editing mode ribbon can be customized here
	Info = FEditorModeInfo(ULinkEditorEditorMode::EM_LinkEditorEditorModeId,
		LOCTEXT("ModeName", "LinkEditor"),
		FSlateIcon(),
		true);
}


ULinkEditorEditorMode::~ULinkEditorEditorMode()
{
}


void ULinkEditorEditorMode::ActorSelectionChangeNotify()
{
}

void ULinkEditorEditorMode::Enter()
{
	UEdMode::Enter();

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	// AddYourTool Step 2 - register the ToolBuilders for your Tools here.
	// The string name you pass to the ToolManager is used to select/activate your ToolBuilder later.
	//////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////// 
	const FLinkEditorEditorModeCommands& SampleToolCommands = FLinkEditorEditorModeCommands::Get();

	RegisterTool(SampleToolCommands.SimpleTool, SimpleToolName, NewObject<ULinkEditorSimpleToolBuilder>(this));
	RegisterTool(SampleToolCommands.InteractiveTool, InteractiveToolName, NewObject<ULinkEditorInteractiveToolBuilder>(this));

	// active tool type is not relevant here, we just set to default
	GetToolManager()->SelectActiveToolType(EToolSide::Left, SimpleToolName);
}

void ULinkEditorEditorMode::CreateToolkit()
{
	Toolkit = MakeShareable(new FLinkEditorEditorModeToolkit);
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> ULinkEditorEditorMode::GetModeCommands() const
{
	return FLinkEditorEditorModeCommands::Get().GetCommands();
}

#undef LOCTEXT_NAMESPACE
