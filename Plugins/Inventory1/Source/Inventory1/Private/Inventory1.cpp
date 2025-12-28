// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inventory1.h"

#define LOCTEXT_NAMESPACE "FInventory1Module"

DEFINE_LOG_CATEGORY(LogInventory);

void FInventory1Module::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FInventory1Module::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FInventory1Module, Inventory1)