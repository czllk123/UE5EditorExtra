// Copyright Epic Games, Inc. All Rights Reserved.

#include "LinkExtraModule.h"
#include "LinkExtraEditorModeCommands.h"

#define LOCTEXT_NAMESPACE "LinkExtraModule"

void FLinkExtraModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FLinkExtraEditorModeCommands::Register();
}

void FLinkExtraModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	FLinkExtraEditorModeCommands::Unregister();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLinkExtraModule, LinkExtraEditorMode)