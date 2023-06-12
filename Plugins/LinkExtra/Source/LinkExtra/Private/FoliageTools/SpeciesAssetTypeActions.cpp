// Fill out your copyright notice in the Description page of Project Settings.


#include "SpeciesAssetTypeActions.h"

#include "Species.h"

FSpeciesAssetTypeActions::FSpeciesAssetTypeActions()
{
}

FText FSpeciesAssetTypeActions::GetName() const
{
	return  NSLOCTEXT("AssetTypeActions", "SpeciesAssetTypeActions", "Species");
}

UClass* FSpeciesAssetTypeActions::GetSupportedClass() const
{
	return USpecies::StaticClass();
}

FColor FSpeciesAssetTypeActions::GetTypeColor() const
{
	return FColor::Blue;
}

uint32 FSpeciesAssetTypeActions::GetCategories()
{
	return 12;
}
