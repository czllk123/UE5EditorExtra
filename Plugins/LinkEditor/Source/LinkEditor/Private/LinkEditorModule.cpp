// Copyright Epic Games, Inc. All Rights Reserved.

#include "LinkEditorModule.h"
#include "LinkEditorEditorModeCommands.h"

#define LOCTEXT_NAMESPACE "LinkEditorModule"

void FLinkEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FLinkEditorEditorModeCommands::Register();
}

void FLinkEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	FLinkEditorEditorModeCommands::Unregister();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLinkEditorModule, LinkEditorEditorMode)