// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"
#include "SpeciesAssetTypeActions.h"

/**
 * 
 */

class LINKEXTRA_API FSpeciesAssetTypeActions : public FAssetTypeActions_Base
{
public:
	FSpeciesAssetTypeActions();
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual FColor  GetTypeColor() const override;
	virtual uint32 GetCategories() override;
     
};
