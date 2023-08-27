

#include "WaterFallCustomization.h"
#include "WaterFall.h"
#include "IDetailCustomization.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Editor.h"
#include "Engine/Selection.h"



#if WITH_EDITOR
TSharedRef<IDetailCustomization> WaterFallCustomization::MakeInstance()
{
	return MakeShareable(new WaterFallCustomization);
}


TWeakObjectPtr<AWaterFall> WaterFall;
void WaterFallCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	UE_LOG(LogTemp, Warning, TEXT("Starting to check selected actors..."));
	// 初始化为nullptr
	AWaterFall* SelectedWaterFall = nullptr;

	
	// Get the current Level Editor Selection
	USelection* SelectedActors = GEditor->GetSelectedActors();
	for (FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
	{
		AActor* Selected = Cast<AActor>(*Iter);
		if (Selected)
		{
			SelectedWaterFall = Cast<AWaterFall>(Selected);
			if (SelectedWaterFall)
			{
				UE_LOG(LogTemp, Warning, TEXT("Selected Actor %s"), *SelectedWaterFall->GetName());
				break;  // 如果找到了，退出循环
			}
		}
	}

	// 将找到的实例保存到成员变量中

	this->WaterFall = SelectedWaterFall;

	IDetailCategoryBuilder& WaterFallCategory = DetailBuilder.EditCategory("WaterFall", FText::GetEmpty(), ECategoryPriority::Important);
	WaterFallCategory.AddCustomRow(FText::GetEmpty())
	.ValueContent()
	[
		SNew(SButton)
		.Text(FText::FromString("Toggle Tick"))
		.OnClicked_Lambda([this]() -> FReply
		{
			if(this->WaterFall.IsValid()) // 确保WaterFall不是nullptr
				{
					this->WaterFall->ToggleTick(); // 调用AWaterFall内定义的ToggleTick方法
					return FReply::Handled();
				}
				return FReply::Unhandled();
			})
		];
	UE_LOG(LogTemp, Warning, TEXT("Finished checking selected actors."));
}

#endif
