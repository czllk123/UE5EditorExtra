#pragma once

#if WITH_EDITOR
#include "PropertyEditorModule.h"
#include "IDetailCustomization.h"
#include "WaterFall.h"


class WaterFallCustomization :public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	//FReply OnClickTick();
private:
	TWeakObjectPtr<AWaterFall> WaterFall;
};
#endif