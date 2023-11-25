

#include "WaterFallCustomization.h"
#include "WaterFall.h"
#include "IDetailCustomization.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Editor.h"
#include "Engine/Selection.h"
#include "NiagaraSystemInstanceController.h"
#include "Editor/PropertyEditor/Public/DetailLayoutBuilder.h" // IDetailLayoutBuilder
#include "Editor/PropertyEditor/Public/DetailCategoryBuilder.h" // IDetailCategoryBuilder
#include "Editor/PropertyEditor/Public/DetailWidgetRow.h" // FDetailWidgetRow
#include "Slate/Public/Widgets/Input/SButton.h" // SButton


#if WITH_EDITORONLY_DATA
#define LOCTEXT_NAMESPACE "WaterFallCustomization"
TSharedRef<IDetailCustomization> FWaterFallCustomization::MakeInstance()
{
	return MakeShareable(new FWaterFallCustomization);
}



void FWaterFallCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	//TargetActor可以看作是在场景中选中的Actor
	TArray<TWeakObjectPtr<UObject>> CustomizedObjects;
	DetailBuilder.GetObjectsBeingCustomized(CustomizedObjects);
	for (auto Object : CustomizedObjects)
	{
		if (Object.IsValid())
		{
			if (AWaterFall* Actor = Cast<AWaterFall>(Object))
				TargetActor = Actor;
			break;
		}
	}
	check(TargetActor.Get())

	//重置参数
	{
		IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("RestParameters", FText::FromString(TEXT("重置所有参数")), ECategoryPriority::Important);
		Category.InitiallyCollapsed(false);
		Category.AddCustomRow(LOCTEXT("", ""), false)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.Text(LOCTEXT("RestParameters", "重置所有参数"))
			.OnClicked_Lambda([this]
						 {
							 if (TargetActor.IsValid()) TargetActor->ResetParameters();
							 return FReply::Handled();
						 })
		];
	}

	//发射源设置
	{
		IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("EmissionSourceSetup", FText::FromString(TEXT("发射源设置")), ECategoryPriority::Important);
		Category.InitiallyCollapsed(false);
	}

	//Niagara解算
	{
		IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Simulation", FText::FromString(TEXT("Niagara解算")), ECategoryPriority::Important);
		Category.InitiallyCollapsed(false);
		Category.AddCustomRow(LOCTEXT("", ""), false)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.ButtonColorAndOpacity_Lambda([this]() -> FSlateColor
			{
				if (TargetActor.IsValid() && TargetActor->bSimulateValid)
				{
					// 蓝色按钮
					return FSlateColor(FLinearColor(0.0f, 112.0f, 224.0f));
				}
				else
				{
					// 黄色按钮
					return FSlateColor(FLinearColor(200.0f, 0.0f, 150.0f));
				}
			})
			.OnClicked_Lambda([this]() -> FReply
			 {
				 if (TargetActor.IsValid() && TargetActor->bSimulateValid)
				 {
					TargetActor->StartSimulation();
				 }
				 else
				 {
					 TargetActor->StopSimulation();
				 }
				 return FReply::Handled();
			 })
			 .Content()
				[
					SNew(STextBlock)
					.Text_Raw(this, &FWaterFallCustomization::ButtonText)
				]
		];
		
		Category.AddCustomRow(LOCTEXT("", ""), false)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.Text(LOCTEXT("ClearAllResource", "清除所有资源"))
			.OnClicked_Lambda([this]
						 {
							 if (TargetActor.IsValid()) TargetActor->ClearAllResource();
							 return FReply::Handled();
						 })
		];

	}
	
	//重采样曲线

	{
		IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("ResampleSpline", FText::FromString(TEXT("重采样曲线")), ECategoryPriority::Important);
		Category.InitiallyCollapsed(false);
		Category.AddCustomRow(LOCTEXT("", ""), false)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.Text(LOCTEXT("ResampleSpline", "重采样曲线"))
			.OnClicked_Lambda([this]
						 {
							 if (TargetActor.IsValid()) TargetActor->ReGenerateSplineAfterResample();
							 return FReply::Handled();
						 })
		];
	}

	//筛选曲线
	{
		IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("FitterSpline", FText::FromString(TEXT("筛选曲线")), ECategoryPriority::Important);
		Category.InitiallyCollapsed(false);
		Category.AddCustomRow(LOCTEXT("", ""), false)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.Text(LOCTEXT("FitterSpline", "筛选曲线"))
			.OnClicked_Lambda([this]
						 {
							 if (TargetActor.IsValid()) TargetActor->FitterSplines(TargetActor->Percent,TargetActor->CrossYAxisDistance,TargetActor->AngleRange);
							 return FReply::Handled();
						 })
		];
	}
	
	//为曲线分簇

	{
		IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("ClusterSpline", FText::FromString(TEXT("为曲线分簇")), ECategoryPriority::Important);
		Category.InitiallyCollapsed(false);
		Category.AddCustomRow(LOCTEXT("", ""), false)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.Text(LOCTEXT("ClusterSpline", "为曲线分簇"))
			.OnClicked_Lambda([this]
						 {
							 if (TargetActor.IsValid()) TargetActor->ClusterSplines();
							 return FReply::Handled();
						 })
		];
	}
	
	//生成清除预览面片

	{
		IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("SplineMesh", FText::FromString(TEXT("生成预览面片")), ECategoryPriority::Important);
		Category.InitiallyCollapsed(false);
		Category.AddCustomRow(LOCTEXT("", ""), false)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.Text(LOCTEXT("SplineMesh", "生成预览面片"))
			.OnClicked_Lambda([this]
						 {
							 if (TargetActor.IsValid()) TargetActor->GenerateSplineMesh();
							 return FReply::Handled();
						 })
		];
		Category.AddCustomRow(LOCTEXT("", ""), false)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.Text(LOCTEXT("SplineMesh", "清除预览面片"))
			.OnClicked_Lambda([this]
						 {
							 if (TargetActor.IsValid()) TargetActor->ClearAllSplineMesh();
							 return FReply::Handled();
						 })
		];
	}

	//生成瀑布面片

	{
		IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("StaticMesh", FText::FromString(TEXT("生成瀑布面片")), ECategoryPriority::Important);
		Category.InitiallyCollapsed(false);
		Category.AddCustomRow(LOCTEXT("", ""), false)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.Text(LOCTEXT("StaticMesh", "生成瀑布面片"))
			.OnClicked_Lambda([this]
						 {
							 if (TargetActor.IsValid()) TargetActor->RebuildWaterFallMesh();
							 return FReply::Handled();
						 })
		];
	}

	//粒子设置
	{
		IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("ParticleSettings", FText::FromString(TEXT("生成粒子")), ECategoryPriority::Important);
		Category.InitiallyCollapsed(false);
		Category.AddCustomRow(LOCTEXT("", ""), false)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.Text(LOCTEXT("ParticleSettings", "生成粒子"))
			.OnClicked_Lambda([this]
						 {
							 if (TargetActor.IsValid()) TargetActor->SpawnParticles();
							 return FReply::Handled();
						 })
		];
		Category.AddCustomRow(LOCTEXT("", ""), false)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.Text(LOCTEXT("ParticleSettings", "清除粒子"))
			.OnClicked_Lambda([this]
						 {
							 if (TargetActor.IsValid()) TargetActor->ClearSpawnedParticles();
							 return FReply::Handled();
						 })
		];
	}

	//保存为资产
	{
		IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("SaveToDisk", FText::FromString(TEXT("保存为资产")), ECategoryPriority::Important);
		Category.InitiallyCollapsed(false);
	}
	
}









FText FWaterFallCustomization::ButtonText() const 
{
	FString TempCaption;
	if(TargetActor->GetSimulateStateValue() == EWaterFallButtonState::Stop)
	{
		TempCaption = "Stop";
	}
	else
	{
		TempCaption = "Simulate";
	}
	
	return FText::FromString(TempCaption);
}
#undef LOCTEXT_NAMESPACE
#endif
