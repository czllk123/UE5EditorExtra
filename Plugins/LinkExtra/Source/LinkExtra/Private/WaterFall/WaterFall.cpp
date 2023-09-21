// Fill out your copyright notice in the Description page of Project Settings.


#include "WaterFall.h"
#include "Editor.h"

#include "NiagaraSystemInstanceController.h"
#include "NiagaraComponent.h"
#include "NiagaraSystemSimulation.h"
#include "CustomNiagaraDataSetAccessor.h"
#include "NiagaraDataSetReadback.h"

#include "NiagaraDataSet.h"


#include "NiagaraSystemInstance.h"
#include "NiagaraEmitterInstance.h"

#include "Async/TaskGraphFwd.h"
#include "Async/TaskTrace.h"
#include "Tasks/TaskPrivate.h"
#include "Async/InheritedContext.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"

#include "NiagaraComputeExecutionContext.h"
#include "NiagaraDataSetDebugAccessor.h"
#include "Selection.h"
#include "NiagaraSystem.h"


// Sets default values
AWaterFall::AWaterFall()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	//PrimaryActorTick.TickInterval = 1.0f;
	bIsEditorOnlyActor = true;



	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	Niagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Niagara"));
	Niagara->SetupAttachment(RootComponent);
	Niagara->Deactivate();

	
	//Niagara->SetNiagaraVariableObject(TEXT("User.ParticleExportObject"),this);
	Niagara->SetNiagaraVariableInt(TEXT("ParticleSpawnCount"),SplineCount);

	
	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
	BoxCollision->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void AWaterFall::BeginPlay()
{
	Super::BeginPlay();
	
}


void AWaterFall::StartSimulation()
{
	
	

	UE_LOG(LogTemp,Warning,TEXT("测试计时器是否开始！"));
	//const FString EmitterName = "Fountain002";
	bSimulateValid = false;
	SimulateState = EWaterFallButtonState::Stop;

	//每次调用函数都新生成一个Stream流
	RandomStream.Initialize(FPlatformTime::Cycles());
	
	//重新模式前，清理场景中的Spline
	ClearAllSpline();
	
	USelection* SelectedActors = GEditor->GetSelectedActors();
	for (FSelectionIterator It(*SelectedActors); It; ++It)
	{
		AWaterFall* WaterFallActor = Cast<AWaterFall>(*It);
		for(UActorComponent* AC : WaterFallActor ->GetComponents())
		{
			UNiagaraComponent* NiagaraComponent = Cast<UNiagaraComponent>(AC);
			if(NiagaraComponent== nullptr)
			{
				continue;
			}
			NiagaraComponent->bAutoActivate=true;
			NiagaraComponent->Activate(false);
			NiagaraComponent->ReregisterComponent();

			//设置Niagara模拟时的相关参数
			NiagaraComponent->SetNiagaraVariableInt(TEXT("ParticleSpawnCount"),SplineCount);
			
			check(IsInGameThread());
			FNiagaraSystemInstanceControllerPtr SystemInstanceController = NiagaraComponent->GetSystemInstanceController(); 
			FNiagaraSystemInstance* SystemInstance = SystemInstanceController.IsValid() ? SystemInstanceController->GetSystemInstance_Unsafe() : nullptr;

			check(NiagaraComponent->GetAsset() != nullptr);
			StoreSystemInstance = SystemInstance;
			
			UNiagaraSystem* NiagaraSystem = NiagaraComponent->GetAsset(); 
			const FVector ComponentLocation = NiagaraComponent->GetComponentLocation();
			const bool bIsActive = NiagaraComponent->IsActive();
			if (bIsActive)
			{
				UE_LOG(LogTemp,Warning,TEXT("Niagara is Active ！"));
			} 
			check(StoreSystemInstance);
		}
	}
	
	GetWorld()->GetTimerManager().SetTimer
	(SimulationTimerHandle, this, &AWaterFall::GenerateSplineMesh, GetDataBufferRate, true );
}


void AWaterFall::StopSimulation()
{

	bSimulateValid = true;
	SimulateState = EWaterFallButtonState::Simulate;
	
	USelection* SelectedActors = GEditor->GetSelectedActors();
	for (FSelectionIterator It(*SelectedActors); It; ++It)
	{
		AWaterFall* WaterFallActor = Cast<AWaterFall>(*It);
		for(UActorComponent* AC : WaterFallActor ->GetComponents())
		{
			UNiagaraComponent* NiagaraComponent = Cast<UNiagaraComponent>(AC);
			if(NiagaraComponent== nullptr)
			{
				continue;
			}
			NiagaraComponent->bAutoActivate=false;
			NiagaraComponent->Deactivate();
			NiagaraComponent->ReregisterComponent();
		}
	}
	UE_LOG(LogTemp, Warning ,TEXT("测试计时器是否结束！"))
	
	if(SimulationTimerHandle.IsValid())
		GetWorld()->GetTimerManager().ClearTimer(SimulationTimerHandle); 
	
}

void AWaterFall::ResetParameters()
{
	IsRaining = false;
}

void AWaterFall::GenerateSplineMesh()
{
	UE_LOG(LogTemp, Warning ,TEXT(" Start GenerateSplineMesh！"))
	
	auto SystemSimulation = StoreSystemInstance->GetSystemSimulation();
	const bool bSystemSimulationValid = SystemSimulation.IsValid() && SystemSimulation->IsValid();
	//等待异步完成再去访问资源，否则触发崩溃
	if(bSystemSimulationValid)
	{
		SystemSimulation->WaitForInstancesTickComplete();
	}
	
	TArray<FParticleData> ParticleDataArray; //储存所有发射器的粒子数据
	FCustomNiagaraDataSetAccessor templateFuncs;
	
	for (const TSharedRef<FNiagaraEmitterInstance, ESPMode::ThreadSafe>& EmitterInstance : StoreSystemInstance->GetEmitters())
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
		UE_LOG(LogTemp,Warning,TEXT("DataBuffer is Valid ！"));
		for(uint32 iInstance = 0; iInstance < DataBuffer->GetNumInstances(); ++ iInstance)
		{
			FParticleData TempParticleData;
			for(const auto& ParticleVar : ParticlesVariables)
			{
				if (ParticleVar == "Position") 
				{
					templateFuncs.GetParticleDataFromDataBuffer(CompiledData, DataBuffer, ParticleVar, iInstance, TempParticleData.Position);
				} 
				else if (ParticleVar == "Velocity") 
				{
					templateFuncs.GetParticleDataFromDataBuffer(CompiledData, DataBuffer, ParticleVar, iInstance, TempParticleData.Velocity);
				} 
				else if (ParticleVar == "Age") 
				{
					templateFuncs.GetParticleDataFromDataBuffer(CompiledData, DataBuffer, ParticleVar, iInstance, TempParticleData.Age);
				}
				else if (ParticleVar == "UniqueID") 
				{
					templateFuncs.GetParticleDataFromDataBuffer(CompiledData, DataBuffer, ParticleVar, iInstance, TempParticleData.UniqueID);
				}
			}
			ParticleDataArray.Add(TempParticleData);
			
		}
		/*
		for(const FParticleData& particle : ParticleDataArray)
		{
			UE_LOG(LogTemp, Warning, TEXT("Particle Data - UniqueID: %d, Position: %s, Velocity: %s, Age: %f"), 
				   particle.UniqueID, 
				   *particle.Position.ToString(), 
				   *particle.Velocity.ToString(), 
				   particle.Age);
		}
		UE_LOG(LogTemp, Error, TEXT("============================================================================"));
		*/
		for(const FParticleData& particle : ParticleDataArray)
		{
			UpdateSplineComponent(particle.UniqueID, particle.Position);
		}
	}






	
}

void AWaterFall::UpdateSplineComponent(int32 ParticleID, FVector ParticlePosition)
{
	USplineComponent** SplineComponentPtr = ParticleIDToSplineComponentMap.Find(ParticleID);
	if(SplineComponentPtr)
	{
		WaterFallSpline = *SplineComponentPtr;
		WaterFallSpline->AddSplineWorldPoint(ParticlePosition);
	}
	else
	{
		// 如果没找到，创建一个新的SplineComponent并添加到TMap
		WaterFallSpline = NewObject<USplineComponent>(this, USplineComponent::StaticClass()); 
		WaterFallSpline->SetHiddenInGame(true);
		WaterFallSpline->SetMobility(EComponentMobility::Movable);
		WaterFallSpline->RegisterComponent();  // 注册组件，使其成为场景的一部分
		WaterFallSpline->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepRelativeTransform);
		WaterFallSpline->ClearSplinePoints(true);
		WaterFallSpline->AddSplinePointAtIndex(ParticlePosition,0, ESplineCoordinateSpace::World,true );
		
		ParticleIDToSplineComponentMap.Add(ParticleID, WaterFallSpline);

#if WITH_EDITORONLY_DATA
		const FLinearColor RandomColor(RandomStream.FRand(), RandomStream.FRand(), RandomStream.FRand(), 1.0f);
		WaterFallSpline->EditorUnselectedSplineSegmentColor = RandomColor;
		//UE_LOG(LogTemp,Warning,TEXT("Color : %s"),*RandomColor.ToString());
		WaterFallSpline->EditorSelectedSplineSegmentColor=(FLinearColor(1.0f, 0.0f, 0.0f, 1.0f));
#endif
	}
}

void AWaterFall::ClearAllSpline()
{
	for(auto& ElementSpline: ParticleIDToSplineComponentMap)
	{
		USplineComponent* SplineComponent = ElementSpline.Value;
		if(SplineComponent)
		{
			SplineComponent->DestroyComponent();
		}
	}
	//清空TMap
	ParticleIDToSplineComponentMap.Empty();
}


const FNiagaraDataSet* AWaterFall::GetParticleDataSet(FNiagaraSystemInstance* SystemInstance, FNiagaraEmitterInstance* EmitterInstance, int32 iEmitter)
{
	
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
	return &EmitterInstance->GetData();
}


// Called every frame
void AWaterFall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
}

