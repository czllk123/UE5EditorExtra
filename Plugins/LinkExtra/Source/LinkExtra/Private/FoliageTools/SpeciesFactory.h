// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Species.h"
#include "SpeciesFactory.generated.h"

/**
 * 
 */
UCLASS()
class LINKEXTRA_API USpeciesFactory : public UFactory
{
	GENERATED_BODY()
public:
	USpeciesFactory();

	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext) override;
};
