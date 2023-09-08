//Copyright  2022 Tav Shande.All Rights Reserved.


#include "IvyCustomDetailPanel.h"
#include "IDetailsView.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "DetailCategoryBuilder.h"

#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"

#include "Internationalization/Text.h"
#include "IvyGeneratorBase.h"
#include "UObject/Class.h"

TSharedRef<IDetailCustomization> IvyCustomDetailPanel::MakeInstance()
{
	return MakeShareable(new IvyCustomDetailPanel);
}

void IvyCustomDetailPanel::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	//Edits a category. If it doesn't exist it creates a new one
	IDetailCategoryBuilder& CustomCategory = DetailBuilder.EditCategory("IvyButtons");

	//Store the currently selected objects from the viewport to the SelectedObjects array.
	DetailBuilder.GetObjectsBeingCustomized(SelectedObjects);
	CustomCategory.SetSortOrder(1);
	CustomCategory.InitiallyCollapsed(false);

	//Adding a custom row
	CustomCategory.AddCustomRow(FText::FromString("Outline Color Changing Category"))
		.ValueContent()
		.VAlign(EVerticalAlignment::VAlign_Center)
		.HAlign(EHorizontalAlignment::HAlign_Center)
		.MaxDesiredWidth(500)
		[
			SNew(SButton)
			.VAlign(EVerticalAlignment::VAlign_Fill)
			.HAlign(EHorizontalAlignment::HAlign_Center)
		.OnClicked_Lambda([this]()->FReply {	
		

		for (const TWeakObjectPtr<UObject>& Object : SelectedObjects)
		{
			AIvyGeneratorBase* IvyActor = Cast<AIvyGeneratorBase>(Object.Get());
			if(IvyActor->GrowthVal)
				IvyActor->StartGrowth();
			else
				IvyActor->StopGrowth();
		}
		return FReply::Handled();
			})
		.Content()

				[
					SNew(STextBlock)
					.Text_Raw(this, &IvyCustomDetailPanel::TestText)


				]

		];


}


FText IvyCustomDetailPanel::TestText() const
{
	FString TempCaption;

	for (const TWeakObjectPtr<UObject>& Object : SelectedObjects)
	{
		AIvyGeneratorBase* IvyActor = Cast<AIvyGeneratorBase>(Object.Get());
		if (IvyActor->GetGrowthStateValue() == EButtonState::Stop)
		{
			TempCaption = "Stop";
		}
		else if (IvyActor->GetGrowthStateValue() == EButtonState::Reset)
		{
			TempCaption = "Grow";
		}
		else
			TempCaption = "Grow";

	}
	return FText::FromString(TempCaption);
};

//FText IvyCustomDetailPanel::TestTextCopyButton() const
//{
//	FString TempCaption;
//
//	for (const TWeakObjectPtr<UObject>& Object : SelectedObjects)
//	{
//		AIvyGeneratorBase* IvyActor = Cast<AIvyGeneratorBase>(Object.Get());
//		if (IvyActor->bCanCopyGeneratedMeshes)
//		{
//			TempCaption = " BakeIvyToNewBP";
//		}
//
//
//		else
//			TempCaption = "";
//
//	}
//	return FText::FromString(TempCaption);
//};

