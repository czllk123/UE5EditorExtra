// Fill out your copyright notice in the Description page of Project Settings.


#include "WaterFall.h"
#include "Editor.h"

#include "NiagaraSystemInstanceController.h"
#include "NiagaraComponent.h"
#include "NiagaraSystemSimulation.h"
#include "CustomNiagaraDataSetAccessor.h"


#include "NiagaraDataSet.h"


#include "NiagaraSystemInstance.h"
#include "NiagaraEmitterInstance.h"

#include "Async/TaskGraphFwd.h"
#include "Async/TaskTrace.h"
#include "Tasks/TaskPrivate.h"
#include "Async/InheritedContext.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "NiagaraComponent.h"
#include "NiagaraComputeExecutionContext.h"
#include "NiagaraDataSetDebugAccessor.h"

#include "NiagaraSystem.h"


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

struct FCachedVariables
{
	TArray<TArray<FNiagaraDataSetDebugAccessor>> ParticlesVariables;
	
};

FCachedVariables CachedVariables;









void AWaterFall::StartGenerateSpline()
{
	bSimulateValid = false;
	SimulateState = EWaterFallButtonState::Stop;
	const FString EmitterName = "Fountain002";

	
	UE_LOG(LogTemp,Warning,TEXT("测试计时器是否开始！"));

	if(Niagara == nullptr)
	{
		return;
	}
	Niagara->Activate(true);
	//Niagara->GetSystemInstance();
	Niagara->ReregisterComponent();
	
	Niagara->ActivateSystem();
	Niagara->AdvanceSimulation(100,2.0);
	check(IsInGameThread());
	UNiagaraSystem* NiagaraSystem = Niagara->GetAsset();
	FNiagaraSystemInstanceControllerPtr SystemInstanceController = Niagara->GetSystemInstanceController();
	FNiagaraSystemInstance* SystemInstance = SystemInstanceController.IsValid() ? SystemInstanceController->GetSystemInstance_Unsafe() : nullptr;
	if (NiagaraSystem == nullptr || SystemInstance == nullptr)
	{
		return;
	}
	const FVector ComponentLocation = Niagara->GetComponentLocation();
	const bool bIsActive = Niagara->IsActive();
	if (bIsActive)
	{
		const FBox Bounds = Niagara->CalcBounds(Niagara->GetComponentTransform()).GetBox();
		if (Bounds.IsValid)
		{
			
			UE_LOG(LogTemp,Warning,TEXT("Niagara is Active ！"));
		}
	}
	
	check(SystemInstance);
	SystemInstance->Activate();
	SystemInstance->Init(true);
	auto SystemSimulation = SystemInstance->GetSystemSimulation();
	if (SystemInstance)
	{
		ENiagaraSystemInstanceState CurrentState = SystemInstance->SystemInstanceState;

		if(CurrentState == ENiagaraSystemInstanceState::PendingSpawn)
		{
			
			SystemSimulation->SetInstanceState(SystemInstance, ENiagaraSystemInstanceState::Running);
		}
	}
	/*
	if(SystemSimulation)
	{
		SystemSimulation->WaitForInstancesTickComplete();
	}

	if ( SystemInstance->SystemInstanceState == ENiagaraSystemInstanceState::PendingSpawn )
	{
		SystemSimulation->FNiagaraSystemSimulation::SetInstanceState(SystemInstance, ENiagaraSystemInstanceState::Running);
	}
	*/
	const TArray<TSharedRef<const FNiagaraEmitterCompiledData>>& AllEmittersCompiledData = NiagaraSystem->GetEmitterCompiledData();
	
	TArray<FParticleData> ParticleDataArray; //储存所有发射器的粒子数据
	FCustomNiagaraDataSetAccessor templateFuncs;

	/*
	auto& EmitterHandles = SystemInstance->GetEmitters();
	
	for(int iEmitter = 0; iEmitter < EmitterHandles.Num(); ++iEmitter)
	{

		FNiagaraEmitterInstance* EmitterInstance = &EmitterHandles[iEmitter].Get();
		if(!EmitterInstance)
		{
			continue;
		}
		UE_LOG(LogTemp,Warning,TEXT("EmitterInstance is Active ！"));
		//const FNiagaraDataSet* ParticleDataSet = GetParticleDataSet(SystemInstance, EmitterInstance, iEmitter);
	*/
	
	Niagara->InitializeSystem();
	for (const TSharedRef<FNiagaraEmitterInstance, ESPMode::ThreadSafe>& EmitterInstance : SystemInstance->GetEmitters())
	{
		UNiagaraEmitter* NiagaraEmitter = EmitterInstance->GetCachedEmitter().Emitter;
		if (NiagaraEmitter == nullptr)
		{
			continue;
		}
		if(EmitterInstance->IsInactive()==true)
		{
			UE_LOG(LogTemp,Warning,TEXT("EmitterInstance is inactive"));
		}
		
		const int64 BytesUsed = EmitterInstance->GetTotalBytesUsed();
		UE_LOG(LogTemp, Error, TEXT("Used :%lld"), BytesUsed);
		const FNiagaraDataSet* ParticleDataSet = &EmitterInstance->GetData();
		if(ParticleDataSet == nullptr)
		{
			continue;
		}

		const FNiagaraDataBuffer* DataBuffer = ParticleDataSet->GetCurrentData();
		const FNiagaraDataSetCompiledData& CompiledData = ParticleDataSet->GetCompiledData();
		
		if(!DataBuffer || !DataBuffer->GetNumInstances())
		{
			continue;
		}
		UE_LOG(LogTemp,Warning,TEXT("DataBuffer is Active ！"));
		for(uint32 iInstance = 0; iInstance < DataBuffer->GetNumInstances(); ++ iInstance)
		{
			FParticleData TempParticleData;
			for(const auto& ParticleVar : ParticlesVariables)
			{
				templateFuncs.GetParticleDataFromDataBuffer(CompiledData, DataBuffer, ParticleVar, iInstance, TempParticleData.UniqueID);
				templateFuncs.GetParticleDataFromDataBuffer(CompiledData, DataBuffer, ParticleVar, iInstance, TempParticleData.Position);
				templateFuncs.GetParticleDataFromDataBuffer(CompiledData, DataBuffer, ParticleVar, iInstance, TempParticleData.Velocity);
				templateFuncs.GetParticleDataFromDataBuffer(CompiledData, DataBuffer, ParticleVar, iInstance, TempParticleData.Age);
				ParticleDataArray.Add(TempParticleData);
				UE_LOG(LogTemp,Warning,TEXT("ParticleDataArray"));
			}
			
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


const FNiagaraDataSet* AWaterFall::GetParticleDataSet(FNiagaraSystemInstance* SystemInstance, FNiagaraEmitterInstance* EmitterInstance, int32 iEmitter)
{
	/*
	// For GPU context we need to readback and cache the data
	if (EmitterInstance->GetGPUContext())
	{

		FNiagaraComputeExecutionContext* GPUExecContext = EmitterInstance->GetGPUContext();
		FGpuEmitterCache* GpuCachedData = GpuEmitterData.Find(SystemInstance->GetId());
		if ( GpuCachedData == nullptr )
		{
			const int32 NumEmitters = SystemInstance->GetEmitters().Num();
			GpuCachedData = &GpuEmitterData.Emplace(SystemInstance->GetId());
			GpuCachedData->CurrentEmitterData.AddDefaulted(NumEmitters);
			GpuCachedData->PendingEmitterData.AddDefaulted(NumEmitters);
		}
		GpuCachedData->LastAccessedCycles = FPlatformTime::Cycles64();

		// Pending readback complete?
		if (GpuCachedData->PendingEmitterData[iEmitter] && GpuCachedData->PendingEmitterData[iEmitter]->IsReady())
		{
			GpuCachedData->CurrentEmitterData[iEmitter] = GpuCachedData->PendingEmitterData[iEmitter];
			GpuCachedData->PendingEmitterData[iEmitter] = nullptr;
		}

		// Enqueue a readback?
		if ( GpuCachedData->PendingEmitterData[iEmitter] == nullptr )
		{
			GpuCachedData->PendingEmitterData[iEmitter] = MakeShared<FNiagaraDataSetReadback, ESPMode::ThreadSafe>();
			GpuCachedData->PendingEmitterData[iEmitter]->EnqueueReadback(EmitterInstance);
		}

		// Pull current data if we have one
		if ( GpuCachedData->CurrentEmitterData[iEmitter] )
		{
			return &GpuCachedData->CurrentEmitterData[iEmitter]->GetDataSet();
		}
		return nullptr;
	}
	*/
	return &EmitterInstance->GetData();
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


