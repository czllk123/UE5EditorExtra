// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LinkExtraEditorModeToolkit.h"


/**
 面板组件的切换，目前想定义两个组件
 1、MaskPainter
 2、FoliagePainter
 3、River
 4、·····
 */
class LINKEXTRA_API SLinkEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLinkEditor)
		{
		}

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	//根据传入的参数和父工具包（FLinkExtraEditorModeToolkit）实例构造该控件。
	void Construct(const FArguments& InArgs, TShaderRef<FLinkExtraEditorModeToolkit> InParentToolkit);

private:
	TSharedPtr<IDetailsView> ToolDetailsView_MaskPainterComponent;//成员变量 -> 用于显示LandscapeMask相关的详细信息视图。
	TSharedPtr<IDetailsView> ToolDetailsView_FoliagePainter;//成员变量 -> 用于显示  Mask Paint Component相关的详细信息视图。

	//两个点击事件
	void OnClickMaskPainterComponentBtn();
	void ONClickFoliagePainterBtn();
};
