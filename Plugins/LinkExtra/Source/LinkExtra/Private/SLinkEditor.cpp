// Fill out your copyright notice in the Description page of Project Settings.


#include "SLinkEditor.h"
#include "EditorModeManager.h"
#include "EditorModes.h"
#include "LinkExtraEditorModeCommands.h"
#include "SlateOptMacros.h"

#define LOCTEXT_NAMESPACE "LinkEditor"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION


END_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SLinkEditor::OnClickMaskPainterComponentBtn()
{
}

void SLinkEditor::ONClickFoliagePainterBtn()
{
}

void FLinkToolKit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost)
{
	FModeToolkit::Init(InitToolkitHost);
	
}

FName FLinkToolKit::GetToolkitFName() const
{
	return FName("LinkEditor");
}

FText FLinkToolKit::GetBaseToolkitName() const
{
	return LOCTEXT("ToolkitName", "Link");
}

FEdModeLandscape* FLinkToolKit::GetEditorMode() const
{
	return (FEdModeLandscape*)GLevelEditorModeTools().GetActiveMode(FBuiltinEditorModes::EM_Landscape);
}

TSharedPtr<SWidget> FLinkToolKit::GetInlineContent() const
{
	return LinkEditorWidgets;
}

const TArray<FName> FLinkToolKit::PaletteNames = { LinkEditorNames::Landscape, LinkEditorNames::Foliage, LinkEditorNames::Water };

void FLinkToolKit::GetToolPaletteNames(TArray<FName>& InPaletteName) const
{
	InPaletteName = PaletteNames;
}

FText FLinkToolKit::GetToolPaletteDisplayName(FName PaletteName) const
{
	if(PaletteName == LinkEditorNames::Landscape)
	{
		return  LOCTEXT("Mode.Landscape", "Landscape");
	}
	else if(PaletteName == LinkEditorNames::Foliage)
	{
		return LOCTEXT("Mode.Foliage", "Foliage");
	}
	else if(PaletteName == LinkEditorNames::Water)
	{
		return LOCTEXT("Mode.Water", "Water");
	}
	return FText();
}

void FLinkToolKit::BuildToolPalette(FName PaletteName, FToolBarBuilder& ToolbarBuilder)
{
	auto Commands = FLinkExtraEditorModeCommands::Get();
	if(PaletteName ==  LinkEditorNames::Landscape)
	{
		ToolbarBuilder.AddToolBarButton(Commands.PaintTool);
	}
	else if (PaletteName ==  LinkEditorNames::Foliage)
	{
		ToolbarBuilder.AddToolBarButton(Commands.PaintTool);
	}
	else if (PaletteName ==  LinkEditorNames::Water)
	{
		ToolbarBuilder.AddToolBarButton(Commands.PaintTool);
	}
}
/*
void FLinkToolKit::OnToolPaletteChanged(FName PaletteName)
{
	if (PaletteName == LinkEditorNames::Landscape && !IsModeActive(LinkEditorNames::Landscape ))
	{
		OnChangeMode(LinkEditorNames::Landscape);
	}
	else if (PaletteName == LinkEditorNames::Foliage && !IsModeActive(LinkEditorNames::Foliage ))
	{
		OnChangeMode(LinkEditorNames::Foliage); 
	}
	else if (PaletteName == LinkEditorNames::Water  && !IsModeActive(LinkEditorNames::Water ))
	{
		OnChangeMode(LinkEditorNames::Water); 
	}
}
*/
#undef LOCTEXT_NAMESPACE
