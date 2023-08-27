// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "Components/BoxComponent.h"
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
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WaterFall")
	UNiagaraComponent* Niagara;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WaterFall")
	AWaterFall* WaterFall;

	UFUNCTION(BlueprintCallable , Category = "WaterFall")
	void ToggleTick();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WaterFall")
	UBoxComponent* BoxCollision;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	bool bIsTicking = false; 




public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
