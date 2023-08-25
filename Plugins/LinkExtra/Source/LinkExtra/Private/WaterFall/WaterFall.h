// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "WaterFall.generated.h"

UCLASS(ClassGroup=(Custom), HideCategories=(Tags,AssetUserData, Rendering, Physics,
	Activation, Collision, Cooking, HLOD, Networking, Input, LOD, Actor,Replication))
class LINKEXTRA_API AWaterFall : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AWaterFall();
	
	UPROPERTY(VisibleAnywhere, Category = "WaterFall")
	USceneComponent* SceneRoot;
	
	UPROPERTY(EditAnywhere, Category = "WaterFall")
	UNiagaraComponent* Niagara;

	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
