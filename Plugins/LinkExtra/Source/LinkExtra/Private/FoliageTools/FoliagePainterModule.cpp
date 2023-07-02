/*
#include "FoliagePainterModule.h"
#include "IAssetTools.h"
#include "SpeciesAssetTypeActions.h"

#define LOCTEXT_NAMESPACE "FoliagePainterModule"
void FFoliagePainter::RegisterAssetsAction() const
{
	IAssetTools& AssetToolModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	const auto Category = AssetToolModule.RegisterAdvancedAssetCategory(FName(TEXT("FoliagePainter")), LOCTEXT("FolaigePainter.DisplayName", "Folaige Painter"));
	const TSharedPtr<FSpeciesAssetTypeActions> AssetsTypeAction = MakeShareable(new FSpeciesAssetTypeActions);
	AssetToolModule.RegisterAssetTypeActions(AssetsTypeAction.ToSharedRef());
}

void FFoliagePainter::StartupModule()
{
	RegisterAssetsAction();
}

void FFoliagePainter::ShutdownModule()
{

}
#undef LOCTEXT_NAMESPACE
IMPLEMENT_MODULE(FFoliagePainter, FFoliagePainter)
*/