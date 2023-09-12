// Fill out your copyright notice in the Description page of Project Settings.


#include "WaterFall.h"
#include "Editor.h"

#include "NiagaraSystemInstanceController.h"
#include "NiagaraComponent.h"
#include "NiagaraSystemSimulation.h"
#include "NiagaraDataSetReadback.h"

#include "NiagaraDataSet.h"
#include "NiagaraDataSetDebugAccessor.h"

#include "NiagaraSystemInstance.h"
#include "NiagaraEmitterInstance.h"
#include "NiagaraGPUSystemTick.h"



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




TArray<TArray<FNiagaraDataSetDebugAccessor>>		ParticleVariables;	
ParticleVariables.Add(TArray<FNiagaraDataSetDebugAccessor>{ "UniqueID"});
ParticleVariables.Add(TArray<FNiagaraDataSetDebugAccessor>{ /* 初始化 Position 相关的 Debug Accessors */ });
ParticleVariables.Add(TArray<FNiagaraDataSetDebugAccessor>{ /* 初始化 Velocity 相关的 Debug Accessors */ });
ParticleVariables.Add(TArray<FNiagaraDataSetDebugAccessor>{ /* 初始化 Age 相关的 Debug Accessors */ });



void AWaterFall::StartGenerateSpline()
{
	bSimulateValid = false;
	SimulateState = EWaterFallButtonState::Stop;
	const FString EmitterName = "Fountain002";
	
	UE_LOG(LogTemp,Warning,TEXT("测试计时器是否开始！"));
	
	if(!Niagara->IsActive())
	{
		UE_LOG(LogTemp, Warning, TEXT("Niagara组件没有激活！"));
		return;
	}
	
	UNiagaraSystem* NiagaraSystem = Niagara->GetAsset();
	if(NiagaraSystem == nullptr)
	{
		return;
	}
	FNiagaraSystemInstanceControllerPtr SystemInstanceController = Niagara->GetSystemInstanceController();
    
	if(SystemInstanceController.IsValid() == false )
	{
		UE_LOG(LogTemp, Warning, TEXT("SystemInstanceController无效！"));
		return ;
	}
	
	FNiagaraSystemInstance* SystemInstance = SystemInstanceController->GetSystemInstance_Unsafe();


	// Get system simulation
	auto SystemSimulation = SystemInstance->GetSystemSimulation();
	const bool bSystemSimulationValid = SystemSimulation.IsValid() && SystemSimulation->IsValid();
	/*
	if (bSystemSimulationValid)
	{
		SystemSimulation->WaitForInstancesTickComplete();
	}
	*/

	const TArray<TSharedRef<const FNiagaraEmitterCompiledData>>& AllEmittersCompiledData = NiagaraSystem->GetEmitterCompiledData();
	TArray<FParticleData> ParticleDataArray; //储存所有发射器的粒子数据

	//遍历每个发射器
	for(int iEmitter = 0; iEmitter < AllEmittersCompiledData.Num(); ++iEmitter)
	{

		FNiagaraEmitterInstance* EmitterInstance = &SystemInstance->GetEmitters()[iEmitter].Get();
		const FNiagaraDataSet* ParticleDataSet = GetParticleDataSet(SystemInstance, EmitterInstance, iEmitter);
		
		if(ParticleDataSet == nullptr)
		{
			continue;
		}

		const FNiagaraDataBuffer* DataBuffer = ParticleDataSet->GetCurrentData();
		if(!DataBuffer || !DataBuffer->GetNumInstances())
		{
			continue;
		}
		
		for(uint32 iInstance = 0; iInstance < DataBuffer->GetNumInstances(); ++ iInstance)
		{
			TStringBuilder<1024> StringBuilder;
			TArray<FParticleData> ParticleDataArray;
			for(const auto& ParticleVariables: ParticlesVariables)
			{
				FParticleData ParticleData;
				ParticleVariables.StringAppend(StringBuilder, DataBuffer, iInstance);
				//ParticleData.UniqueID = SomeFunctionToGetUniqueID(iInstance, DataBuffer);
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


