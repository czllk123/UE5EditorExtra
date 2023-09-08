//Copyright  2022 Tav Shande.All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "IvyGeneratorBase.h"
#include "DetailLayoutBuilder.h"
#include "IDetailCustomization.h"



class CRAZYIVY_API IvyCustomDetailPanel: public IDetailCustomization
{
private:

	/* Contains references to all selected objects inside in the viewport */
	TArray<TWeakObjectPtr<UObject>> SelectedObjects;

public:

	/* Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/* IDetalCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	FString _ButtText = "Grow";

	FText TestText() const;





};
