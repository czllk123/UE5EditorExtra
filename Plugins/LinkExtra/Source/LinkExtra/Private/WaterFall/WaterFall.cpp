// Fill out your copyright notice in the Description page of Project Settings.


#include "WaterFall.h"
#include "Editor.h"
#include "Engine/Selection.h"

#include "NiagaraComponent.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraSystem.h"
#include "NiagaraSystemInstanceController.h"



// Sets default values
AWaterFall::AWaterFall()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	//PrimaryActorTick.TickInterval = 1.0f;
	bIsEditorOnlyActor = true;

	bIsTicking = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	Niagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Niagara"));
	Niagara->SetupAttachment(RootComponent);
	

	Niagara->SetNiagaraVariableObject(TEXT("User.ParticleExportObject"),this);

	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
	BoxCollision->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void AWaterFall::BeginPlay()
{
	Super::BeginPlay();
	
}

void AWaterFall::StartGenerateSpline()
{
	bSimulateValid = false;
	SimulateState = EWaterFallButtonState::Stop;
	const FString EmitterName = "Fountain002";
	UE_LOG(LogTemp,Warning,TEXT("测试计时器是否开始！"));

	if(!Niagara)
	{
		UE_LOG(LogTemp, Warning, TEXT("Niagara组件为空！"));
		return;
	}

	if(!Niagara->IsActive())
	{
		UE_LOG(LogTemp, Warning, TEXT("Niagara组件没有激活！"));
		return;
	}

	FNiagaraSystemInstanceControllerPtr SystemInstanceController = Niagara->GetSystemInstanceController();
    
	if(SystemInstanceController.IsValid() == false )
	{
		UE_LOG(LogTemp, Warning, TEXT("SystemInstanceController无效！"));
		return ;
	}
	
	FNiagaraSystemInstance* SystemInstance = SystemInstanceController->GetSystemInstance_Unsafe();

	for(const auto& EmitterInstance : SystemInstance->GetEmitters())
	{
		UNiagaraEmitter* NiagaraEmitter = EmitterInstance->GetCachedEmitter().Emitter;
		if(!NiagaraEmitter || (NiagaraEmitter->GetUniqueEmitterName() != EmitterName))
		{
			return;
		}

		if(EmitterInstance->GetGPUContext() != nullptr) 
		{
			return;
		}

		const FNiagaraDataSet& ParticleDataSet = EmitterInstance->GetData();
		const FNiagaraDataBuffer* ParticleDataBuffer = ParticleDataSet.GetCurrentData();
		for(uint32 ParticleInstance = 0; ParticleInstance < ParticleDataBuffer->GetNumInstances(); ++ParticleInstance)
		{
			UE_LOG(LogTemp,Warning,TEXT("aaaaaa"));
			FParticleData ParticleData;
		}
	}	

	
}

void AWaterFall::StopGenerateSpline()
{

	bSimulateValid = true;
	SimulateState = EWaterFallButtonState::Simulate;

	UE_LOG(LogTemp, Warning ,TEXT("测试计时器是否结束！"))
}

void AWaterFall::ResetParmaters()
{
}



// Called every frame
void AWaterFall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	/*
	FNiagaraSystemInstance* CurrentSystemInstance = nullptr;
	bIsTicking = !bIsTicking;
	SetActorTickEnabled(bIsTicking);.bIsTicking
	CurrentSystemInstance = GetNiagaraSystemInstance();
	bool bSelected = IsSelectedInEditor();
	if(bSelected)
	{
		CurrentSystemInstance = GetNiagaraSystemInstance();
	}
	
	if(bIsTicking && CurrentSystemInstance!=nullptr)
	{
		Niagara->SetPaused(false);
		Niagara->Activate(true);
		Niagara->ReregisterComponent();
		TArray<FParticleData> TempData;
		TempData = CollectParticleData(CurrentSystemInstance);
		
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Tick is now disabled"));

		// Deactivate Niagara if it's active
		if (Niagara && Niagara->IsActive())
		{
			Niagara->Deactivate();
		}
	}
*/
}


