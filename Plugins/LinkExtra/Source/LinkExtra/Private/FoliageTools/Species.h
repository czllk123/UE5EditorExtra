// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Species.generated.h"

/**
 * 
 */
UCLASS()
class LINKEXTRA_API USpecies : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Parameters", DisplayName="FoliageTypes")
	TArray<UFoliageType*>FoliageTypes;

	UPROPERTY(EditAnywhere, Category="Parameters", DisplayName="权重")
	float Weight = 1.0f;
};

