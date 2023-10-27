#pragma once

#if WITH_EDITOR
#include "PropertyEditorModule.h"
#include "IDetailCustomization.h"
#include "WaterFall.h"


class FWaterFallCustomization :public IDetailCustomization
{
public:
	/* Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();
	
	/* IDetalCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	FText ButtonText() const;
private:
	/* Contains references to all selected objects inside in the viewport */
	TArray<TWeakObjectPtr<UObject>> SelectedObjects;
	
};
#endif