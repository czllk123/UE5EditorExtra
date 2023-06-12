// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InstancedFoliage.h"
#include "LandscapeInfo.h"
#include "LandscapeLayerInfoObject.h"
#include "AddToInstance.generated.h"

/**
 * @李林科
 * 蓝图函数头文件
 * 
 */
UCLASS()
class  UAddToInstance : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category=AddToFoliage)
		static bool AddToFoliageInstance(const UObject* WorldContextObject, TArray<AActor*> ActorsToIgnore, FGuid FoliageInstanceGuid, UStaticMesh *InStaticMesh, FTransform Transform,  FString SavePath, TMap<TSoftObjectPtr<AInstancedFoliageActor>, FGuid>& FoliageUUIDs);

	UFUNCTION(BlueprintCallable, Category=AddToFoliage)
		static bool RemoveFoliageInstance(TMap<TSoftObjectPtr<AInstancedFoliageActor>, FGuid> FoliageUUIDs);

	UFUNCTION(BlueprintCallable, Category=AddToFoliage)
		static TArray<int32> CalculateWeightAverage(const TArray<float>& Weights, int32 OutputSize);

	static bool CheckInstanceLocationOverlap( FFoliageInfo* FoliageInfo,  FVector Location, float Tolerance = 0.1f);
	

};
