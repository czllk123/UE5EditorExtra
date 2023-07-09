// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InstancedFoliage.h"
#include "Materials/Material.h"
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
		static bool AddToFoliageInstance(const UObject* WorldContextObject, TArray<AActor*> ActorsToIgnore, FGuid FoliageInstanceGuid, UFoliageType* InFoliageType, FTransform Transform, TMap<TSoftObjectPtr<AInstancedFoliageActor>, FGuid>& FoliageUUIDs);

	UFUNCTION(BlueprintCallable, Category=AddToFoliage)
		static bool RemoveFoliageInstance(TMap<TSoftObjectPtr<AInstancedFoliageActor>, FGuid> FoliageUUIDs);

	UFUNCTION(BlueprintCallable, Category=AddToFoliage)
		static TArray<int32> CalculateWeightAverage(const TArray<float>& Weights, int32 MeshCount);

	UFUNCTION(BlueprintCallable, Category=AddToFoliage)
		static TArray<int32> CalculateWeightAverageWithStream(const TArray<float>& Weights, int32 MeshCount, const FRandomStream& Stream);

	static bool CheckInstanceLocationOverlap( FFoliageInfo* FoliageInfo,  FVector Location, float Tolerance = 0.1f);

	UFUNCTION(BlueprintCallable, Category=AddToFoliage)
		static UStaticMesh* OverrideMaterialsFromFT(UFoliageType* InputFoliageType);
	
	UFUNCTION(BlueprintCallable, Category=AddToFoliage)
	static TMap< UMaterialInterface*, UStaticMesh*> GetOverrideResourceFromFT(UFoliageType* InputFoliageType);



};
