// Copyright Epic Games, Inc. All Rights Reserved.

#include "LinkEditorEditorModeToolkit.h"
#include "LinkEditorEditorMode.h"
#include "Engine/Selection.h"

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "EditorModeManager.h"

#define LOCTEXT_NAMESPACE "LinkEditorEditorModeToolkit"

FLinkEditorEditorModeToolkit::FLinkEditorEditorModeToolkit()
{
}

void FLinkEditorEditorModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
{
	FModeToolkit::Init(InitToolkitHost, InOwningMode);
}

void FLinkEditorEditorModeToolkit::GetToolPaletteNames(TArray<FName>& PaletteNames) const
{
	PaletteNames.Add(NAME_Default);
}


FName FLinkEditorEditorModeToolkit::GetToolkitFName() const
{
	return FName("LinkEditorEditorMode");
}

FText FLinkEditorEditorModeToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("DisplayName", "LinkEditorEditorMode Toolkit");
}

#undef LOCTEXT_NAMESPACE
