// Copyright Epic Games, Inc. All Rights Reserved.

#include "LinkExtraEditorModeToolkit.h"
#include "LinkExtraEditorMode.h"
#include "Engine/Selection.h"

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "EditorModeManager.h"

#define LOCTEXT_NAMESPACE "LinkExtraEditorModeToolkit"

FLinkExtraEditorModeToolkit::FLinkExtraEditorModeToolkit()
{
}

void FLinkExtraEditorModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
{
	FModeToolkit::Init(InitToolkitHost, InOwningMode);
}

void FLinkExtraEditorModeToolkit::GetToolPaletteNames(TArray<FName>& PaletteNames) const
{
	PaletteNames.Add(NAME_Default);
}


FName FLinkExtraEditorModeToolkit::GetToolkitFName() const
{
	return FName("LinkExtraEditorMode");
}

FText FLinkExtraEditorModeToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("DisplayName", "LinkExtraEditorMode Toolkit");
}

#undef LOCTEXT_NAMESPACE
