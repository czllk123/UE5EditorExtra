// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraDataInterface.h"
#include "NiagaraDataInterfaceExport.h"
#include "Components/BoxComponent.h"
#include "WaterFall.generated.h"

class FNiagaraSystemInstance;


UENUM()
enum EWaterFallButtonState
{
    Simulate    UMETA(DisplayName = "Simulate"),
    Stop     UMETA(DisplayName = "Stop"),
};




USTRUCT(BlueprintType)
struct FParticleData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 UniqueID;

	UPROPERTY(BlueprintReadOnly)
	FVector Position;

	UPROPERTY(BlueprintReadOnly)
	FVector Velocity;

	FParticleData()
		: UniqueID(0), Position(FVector::ZeroVector), Velocity(FVector::ZeroVector)
	{
	}
};

UCLASS(ClassGroup=(Custom), HideCategories=(Tags,AssetUserData, Rendering, Physics,
	Activation, Collision, Cooking, HLOD, Networking, Input, LOD, Actor,Replication))
class LINKEXTRA_API AWaterFall : public AActor, public INiagaraParticleCallbackHandler
{
	GENERATED_BODY()
	friend class WaterFallCustomization;

public:
	// Sets default values for this actor's properties
	AWaterFall();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WaterFall")
	USceneComponent* SceneRoot;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WaterFall")
	UNiagaraComponent* Niagara;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WaterFall")
	AWaterFall* WaterFall;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WaterFall")
	UBoxComponent* BoxCollision;

	UPROPERTY(Category="Simulate",BlueprintReadWrite)
	bool bSimulateValid = true;

	EWaterFallButtonState GetSimulateStateValue(){ return SimulateState;}
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	bool bIsTicking = false;



	UFUNCTION(BlueprintCallable,Category="WaterFall", DisplayName="开始收集粒子数据")
	void StartGenerateSpline();

	UFUNCTION(BlueprintCallable,Category="WaterFall", DisplayName="停止收集粒子数据")
	void StopGenerateSpline();
	
	FTimerHandle ParticleDataTimerHandle;

	UFUNCTION(BlueprintCallable,CallInEditor, Category ="WaterFall")
	void ResetParmaters();

private:
	EWaterFallButtonState SimulateState = EWaterFallButtonState :: Simulate;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};

