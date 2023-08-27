// Copyright Epic Games, Inc. All Rights Reserved.

#include "LinkExtraModule.h"
#include "LinkExtraEditorModeCommands.h"
#include "WaterFall/WaterFallCustomization.h"
#define LOCTEXT_NAMESPACE "LinkExtraModule"

void FLinkExtraModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FLinkExtraEditorModeCommands::Register();
	RegisterDetails();

	UE_LOG(LogTemp, Warning, TEXT("FLinkExtraModule::StartupModule()"));
}

void FLinkExtraModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	FLinkExtraEditorModeCommands::Unregister();
	UnregisterDetails();
}

void FLinkExtraModule::RegisterDetails()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	
	PropertyModule.RegisterCustomClassLayout(
		TEXT("WaterFall"),
		FOnGetDetailCustomizationInstance::CreateStatic(&WaterFallCustomization::MakeInstance));
}

void FLinkExtraModule::UnregisterDetails()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule & PropertyModule =
			FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

		PropertyModule.UnregisterCustomClassLayout(TEXT("WaterFall")); 
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLinkExtraModule, LinkExtra) 