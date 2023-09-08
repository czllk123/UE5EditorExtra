//Copyright  2022 Tav Shande.All Rights Reserved.

#include"CrazyIvy.h"
#include"PropertyEditorModule.h"
#include"IvyCustomDetailPanel.h"
#include"IvyGeneratorBase.h"

DEFINE_LOG_CATEGORY(LogCrazyIvy);

#define LOCTEXT_NAMESPACE "FCrazyIvyModule"

void FCrazyIvyModule::StartupModule()
{
	UE_LOG(LogCrazyIvy, Warning, TEXT("CrazyIvyModule module has started!"));
	//Get the property module
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	//Register the custom details panel we have created
	PropertyModule.RegisterCustomClassLayout(AIvyGeneratorBase::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&IvyCustomDetailPanel::MakeInstance));
}

void FCrazyIvyModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCrazyIvyModule, CrazyIvy)