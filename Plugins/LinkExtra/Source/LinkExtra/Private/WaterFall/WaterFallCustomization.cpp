

#include "WaterFallCustomization.h"
#include "WaterFall.h"
#include "IDetailCustomization.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Editor.h"
#include "Engine/Selection.h"
#include "NiagaraSystemInstanceController.h"



#if WITH_EDITORONLY_DATA
TSharedRef<IDetailCustomization> WaterFallCustomization::MakeInstance()
{
	return MakeShareable(new WaterFallCustomization);
}



void WaterFallCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	
	//Edits a category. If it doesn't exist it creates a new one
	IDetailCategoryBuilder& WaterFallCategory = DetailBuilder.EditCategory("WaterFall", FText::GetEmpty(), ECategoryPriority::Important);

	//Store the currently selected objects from the viewport to the SelectedObjects array.
	DetailBuilder.GetObjectsBeingCustomized(SelectedObjects);
	WaterFallCategory.SetSortOrder(1);
	WaterFallCategory.InitiallyCollapsed(false);
	
	//Adding a custom row
	WaterFallCategory.AddCustomRow(FText::GetEmpty())
	.ValueContent()
	.VAlign(EVerticalAlignment::VAlign_Center)
	.HAlign(EHorizontalAlignment::HAlign_Center)
	.MaxDesiredWidth(500)
	[
		SNew(SButton)
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Center)
		//.Text(FText::FromString("Simulate"))
		.OnClicked_Lambda([this]() -> FReply
		{
			
			for(const TWeakObjectPtr<UObject>& Object : SelectedObjects)
			{
				AWaterFall* WaterFallActor = Cast<AWaterFall>(Object.Get());
				/*
				if(WaterFallActor->bSimulateValid)
				{
					WaterFallActor->StartGenerateSpline();
				}
				
				else
				{
					WaterFallActor->StopGenerateSpline();
				}
				*/
				for(UActorComponent* AC : WaterFallActor ->GetComponents())
				{
					UNiagaraComponent* NiagaraComponent = Cast<UNiagaraComponent>(AC);
					if(NiagaraComponent)
						if(WaterFallActor->bSimulateValid)
						{
							NiagaraComponent->Activate(true);
							NiagaraComponent->ReregisterComponent();
							
							WaterFallActor->StartGenerateSpline();	
						}
						else 
						{
							NiagaraComponent->SetPaused(true);
							NiagaraComponent->ReregisterComponent();
							WaterFallActor->StopGenerateSpline();
						}
				}
				
			}
			return FReply::Handled();
		})
			.Content()
				[
					SNew(STextBlock)
					.Text_Raw(this, &WaterFallCustomization::ButtonText)
				]
	];

}

FText WaterFallCustomization::ButtonText() const 
{
	FString TempCaption;
	for(const TWeakObjectPtr<UObject>& Object : SelectedObjects)
	{
		AWaterFall* WaterFallActor = Cast<AWaterFall>(Object.Get());
		if(WaterFallActor->GetSimulateStateValue() == EWaterFallButtonState::Stop)
		{
			TempCaption = "Stop";
		}
		else
		{
			TempCaption = "Simulate";
		}
		
	}
	return FText::FromString(TempCaption);
}

#endif
