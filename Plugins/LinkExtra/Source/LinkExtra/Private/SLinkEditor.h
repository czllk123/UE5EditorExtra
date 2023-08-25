// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LinkExtraEditorModeToolkit.h"





class LINKEXTRA_API SLinkEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLinkEditor)
		{
		}

	SLATE_END_ARGS()

	
	/** Constructs this widget with InArgs */
	//根据传入的参数和父工具包（FLinkExtraEditorModeToolkit）实例构造该控件。
	//void Construct(const FArguments& InArgs, TShaderRef<FLinkExtraEditorModeToolkit> InParentToolkit);
	
private:
	//<IDetailsView> ToolDetailsView_MaskPainterComponent;//成员变量 -> 用于显示LandscapeMask相关的详细信息视图。
	//TSharedPtr<IDetailsView> ToolDetailsView_FoliagePainter;//成员变量 -> 用于显示  Mask Paint Component相关的详细信息视图。

	//两个点击事件
	void OnClickMaskPainterComponentBtn();
	void ONClickFoliagePainterBtn();
};

namespace LinkEditorNames
{
	static const FName Landscape(TEXT("ToolMode_Landscape")); 
	static const FName Foliage(TEXT("ToolMode_Foliage")); 
	static const FName Water(TEXT("ToolMode_Water"));
}

/**
 * Mode Toolkit for the Landscape Editor Mode
 */
class LINKEXTRA_API FLinkToolKit : public  FModeToolkit
{
public:
	/** Initializes the geometry mode toolkit */
	virtual void Init(const TSharedPtr< class IToolkitHost >& InitToolkitHost) override;

	/**IToolkit interface*/
	virtual FName GetToolkitFName() const override;;
	virtual FText GetBaseToolkitName() const override;
	virtual FEdModeLandscape* GetEditorMode() const override;
	virtual TSharedPtr<SWidget> GetInlineContent() const override;

	void NotifyToolChanged();
	void NotifyBrushChanged();
	void RefreshDetailPanel();

	/**Mode Toolbar Palettes **/
	virtual void GetToolPaletteNames(TArray<FName>& InPaletteName) const;
	virtual FText GetToolPaletteDisplayName(FName PaletteName) const; 
	virtual void BuildToolPalette(FName PaletteName, class FToolBarBuilder& ToolbarBuilder);
	//virtual void OnToolPaletteChanged(FName PaletteName) override;

protected:
	void OnChangeMode(FName ModeName);
	bool IsModeEnabled(FName ModeName) const;
	bool IsModeActive(FName ModeName) const;

private:
	TSharedPtr<SLinkEditor> LinkEditorWidgets;
	const static TArray<FName> PaletteNames;

};
