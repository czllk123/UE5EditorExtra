// Fill out your copyright notice in the Description page of Project Settings.


#include "SLinkEditor.h"

#include "LinkExtraEditorModeCommands.h"
#include "SlateOptMacros.h"

#define LOCTEXT_NAMESPACE "LinkEditor"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SLinkEditor::Construct(const FArguments& InArgs, TShaderRef<FLinkExtraEditorModeToolkit> InParentToolkit)
{
	TSharedRef<FUICommandList> CommandList = InParentToolkit ->GetToolkitCommands();
	FToolBarBuilder ModeToolBar(CommandList, FMultiBoxCustomization::None);
	{
		ModeToolBar.AddToolBarButton(FLinkExtraEditorModeCommands::Get().MaskPainterMode, NAME_None, LOCTEXT("Mode.MaskPainterComp", "掩码笔刷"), LOCTEXT("Mode.MaskPainterComp.Tooltip", "掩码组件绘制工具"));
		ModeToolBar.AddToolBarButton(FLinkExtraEditorModeCommands::Get().FoliagePainterMode, NAME_None, LOCTEXT("Mode.FoliagePainter", "植被笔刷"), LOCTEXT("Mode.FoliagePainter.Tooltip", "植被绘制工具"));
	}
	
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SLinkEditor::OnClickMaskPainterComponentBtn()
{
}

void SLinkEditor::ONClickFoliagePainterBtn()
{
}

#undef LOCTEXT_NAMESPACE