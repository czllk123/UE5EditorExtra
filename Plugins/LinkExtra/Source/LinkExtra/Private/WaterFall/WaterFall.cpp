﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "WaterFall.h"

#include <Windows.Data.Text.h>

#include "AssetToolsModule.h"
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

#include "Selection.h"
#include "NiagaraSystem.h"
#include "Components/SplineMeshComponent.h"
#include "Components/SplineComponent.h"

#include "StaticMeshAttributes.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMeshActor.h"


#include "PhysicsEngine/BodySetup.h"

#include "UObject/SavePackage.h"
#include "Components/BrushComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "StaticMeshOperations.h"
#include "Async/ParallelFor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
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
	Niagara->bAutoActivate=false;
	Niagara->Deactivate();

	
	//Niagara->SetNiagaraVariableObject(TEXT("User.ParticleExportObject"),this);
	Niagara->SetNiagaraVariableInt(TEXT("ParticleSpawnCount"),SplineCount);

	InstancedStaticMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("InstancedStaticMesh"));
	InstancedStaticMeshComponent->SetupAttachment(RootComponent);
	InstancedStaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	//InstancedStaticMeshComponent->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Game/BP/StaticMesh/Sphere.Sphere")));
	InstancedStaticMeshComponent->SetStaticMesh(Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, TEXT("/Game/BP/StaticMesh/Sphere.Sphere"))));

	
	KillBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision")); 
	KillBox->SetupAttachment(RootComponent);
	KillBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	KillBox->SetBoxExtent(FVector(32,32,32));

	//在构造函数中创建一个实例，那么在AWaterFall对象整个生命周期中都有效，后面多次调用ClusterSpline函数就不会重复创建实例
	SplineProcessorInstance = CreateDefaultSubobject<USplineProcessor>(TEXT("SplineProcessorInstance"));

	
	EmitterSpline = CreateDefaultSubobject<USplineComponent>(TEXT("EmitterSpline"));
	EmitterSpline->SetupAttachment(RootComponent);
	EmitterSpline->bShouldVisualizeScale = true;
	EmitterSpline->ScaleVisualizationWidth = 100.0f;
	EmitterSpline->EditorUnselectedSplineSegmentColor = FLinearColor::Red;


	ConstructorHelpers::FObjectFinder<UParticleSystem> ParticleAsset1(TEXT("/Game/AncientTempleRuins/Effects/Waterfall_01/ps_Waterfall_Splash_01_05_Foam.ps_Waterfall_Splash_01_05_Foam"));
	if (ParticleAsset1.Succeeded())
	{
		BottomParticles.Add(ParticleAsset1.Object);
	}

	ConstructorHelpers::FObjectFinder<UParticleSystem> ParticleAsset2(TEXT("/Game/AncientTempleRuins/Effects/Waterfall_01/ps_Waterfall_Splash_01_01_Bottom.ps_Waterfall_Splash_01_01_Bottom"));
	if (ParticleAsset2.Succeeded())
	{
		BottomParticles.Add(ParticleAsset2.Object);
	}


}
// Called when the game starts or when spawned
void AWaterFall::BeginPlay()
{
	Super::BeginPlay();
	
}

void AWaterFall::ComputeEmitterPoints(USplineComponent* EmitterSplineComponent, int32 EmitterPointsCount, bool bSnapToGrid)
{
	InstancedStaticMeshComponent->ClearInstances();

	TArray<bool> EliminationMaskForHit;
	EliminationMaskForHit.Init(false, EmitterPointsCount);

	this->SourceEmitterAttributes.Locations.Init(FVector::ZeroVector, EmitterPointsCount);
	this->SourceEmitterAttributes.Velocities.Init(FVector::ZeroVector, EmitterPointsCount);
	
	this->EmitterPoints.Empty();
	this->EmitterPoints.Init(FEmitterPoints(), EmitterPointsCount);

	const float SplineLength = EmitterSplineComponent->GetSplineLength();
	const float PointSpacing = SplineLength / (EmitterPointsCount - 1);
	
	ParallelFor(EmitterPointsCount, [this, &EmitterSplineComponent, &SplineLength, &PointSpacing, &EliminationMaskForHit](int32 i)
	{
		const uint32 InitialSeed = GenHash(Seed, i, 456);
		
		const FRandomStream Rs(InitialSeed);
		//const float SplineDist = UKismetMathLibrary::RandomFloatInRangeFromStream( Rs, 0, SplineLength);
		const float SplineDist = PointSpacing * i;

		const FVector Location = EmitterSplineComponent->GetWorldLocationAtDistanceAlongSpline(SplineDist);
		const FVector RightVector = EmitterSplineComponent->GetRightVectorAtDistanceAlongSpline(SplineDist, ESplineCoordinateSpace::World);
		const FVector ScaleAlongSpline = EmitterSplineComponent->GetScaleAtDistanceAlongSpline(SplineDist);
		const float Range = ScaleAlongSpline.Y * 500;
		float RandHori = 0;
		RandHori = UKismetMathLibrary::RandomFloatInRangeFromStream(-Range, Range, Rs);

		FVector RayStart = Location + RightVector * RandHori;

		const float DisturbX = UKismetMathLibrary::RandomFloatInRangeFromStream(-Disturb, Disturb, Rs);
		const float DisturbY = UKismetMathLibrary::RandomFloatInRangeFromStream(-Disturb, Disturb, Rs);

		RayStart.X += DisturbX;
		RayStart.Y += DisturbY;

		const FVector RayEnd = RayStart + FVector(0, 0, -100000000);

		// Do Line Trace
		FHitResult OutHit;
		bool bHit = UKismetSystemLibrary::LineTraceSingle(this, RayStart, RayEnd, UEngineTypes::ConvertToTraceType(ECC_Visibility), true, {}, EDrawDebugTrace::None, OutHit, true, FLinearColor::Red, FLinearColor::Green, 5.0f);
		bHit = bHit && OutHit.bBlockingHit && !OutHit.bStartPenetrating;

		if (!bHit)
		{
			EliminationMaskForHit[i] = true;
		}
		/*
		else
		{
			if (bUseSlopeDel)
			{
				const float Angle = UKismetMathLibrary::DegAcos(OutHit.ImpactNormal.Z);
				if (Angle < SlopeRange.X && Angle > SlopeRange.Y)
				{
					EliminationMaskForHit[i] = true;
				}
			}
	
			if (bSpawnOnLandscapeOnly)
			{
				// if (OutHit.Component->GetClass() != ULandscapeHeightfieldCollisionComponent::StaticClass() && OutHit.Component->GetClass() != ULandscapeMeshCollisionComponent::StaticClass())
				const UClass* HitClass = OutHit.Component->GetClass();
				if (HitClass != ULandscapeHeightfieldCollisionComponent::StaticClass())
				{
					EliminationMaskForHit[i] = true;
				}
			}

		}
		*/
		if (!EliminationMaskForHit[i])
		{
			this->SourceEmitterAttributes.Locations[i] = OutHit.ImpactPoint;
			
			this->EmitterPoints[i].Location = OutHit.ImpactPoint;
			//this->EmitterPoints[i].Transform.SetScale3D(FVector(0.2,0.2,0.2));
			
			this->EmitterPoints[i].Normal = OutHit.ImpactNormal;
			//InstancedStaticMeshComponent->AddInstance(this->EmitterPoints[i].Transform,  true);
		}
	});

	// Eliminate Points based on hit or not

	for (int32 i = EmitterPointsCount - 1; i >= 0; i--)
	{
		if (EliminationMaskForHit[i])
		{
			this->EmitterPoints.RemoveAtSwap(i);
			this->SourceEmitterAttributes.Locations.RemoveAtSwap(i);
		}

	}

	//EliminateClosePts();
	TArray<FTransform> ValidTransforms;
	for (const FEmitterPoints& EmitterPoint : this->EmitterPoints)
	{
		FTransform Transform(EmitterPoint.Location);
		FVector ScaleVector(ISMScale, ISMScale, ISMScale);
		Transform.SetScale3D(ScaleVector);
		
		ValidTransforms.Add(Transform);
	}

	InstancedStaticMeshComponent->AddInstances(ValidTransforms, false, true);

}

void AWaterFall::UpdateEmitterPoints()
{
	
	if (EmitterSpline)
	{
		//对EmitterSpline重采样，使用重采样后的位置作为粒子发射源
		//const TArray<FVector> SampleEmitterPoints = ResampleSplinePointsWithNumber(SplineComponent, SplineCount);
		ComputeEmitterPoints(EmitterSpline, SplineCount, true);
	}
	
}


void AWaterFall::StartSimulation()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AAAAAAA)
	UE_LOG(LogTemp,Warning,TEXT("测试计时器是否开始！"));
	//const FString EmitterName = "Fountain002";
	bSimulateValid = false;
	SimulateState = EWaterFallButtonState::Stop;

	//每次调用函数都新生成一个Stream流
	RandomStream.Initialize(FPlatformTime::Cycles());
	
	//重新模式前，清理场景中的Spline
	ClearAllResource();
	
	USelection* SelectedActors = GEditor->GetSelectedActors();
	for (FSelectionIterator It(*SelectedActors); It; ++It)
	{
		AWaterFall* WaterFallActor = Cast<AWaterFall>(*It);
		if (WaterFallActor == nullptr)
		{
			continue; // 如果当前选中的对象不是 AWaterFall 类型，则跳过
		}

		UNiagaraComponent* NiagaraComponent = WaterFallActor->FindComponentByClass<UNiagaraComponent>();
		USplineComponent* SplineComponent = WaterFallActor->FindComponentByClass<USplineComponent>();
		UBoxComponent* BoxComponent = WaterFallActor->FindComponentByClass<UBoxComponent>(); 
		
		if (NiagaraComponent && SplineComponent && BoxComponent)
		{
			//NiagaraComponent->bAutoActivate=false;
			NiagaraComponent->Activate(false);
			//NiagaraComponent->ReregisterComponent();

			NiagaraComponent->SetNiagaraVariableFloat(TEXT("ParticleLifeTime"), ParticleLife);
			NiagaraComponent->SetNiagaraVariableInt(TEXT("ParticleSpawnCount"),SplineCount);

			//TODO 以后把速度方向也传进来
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(NiagaraComponent, TEXT("SplinePointsArray"), SourceEmitterAttributes.GetLocations());

			//获取场景中KillBox的大小和位置传入Niagara
			const FVector KillBoxLocation = BoxComponent->GetComponentLocation();
			const FVector KillBoxBounds = BoxComponent->GetScaledBoxExtent()*2.0f;
			NiagaraComponent->SetNiagaraVariableVec3(TEXT("BoxPosition"), KillBoxLocation);
			NiagaraComponent->SetNiagaraVariableVec3(TEXT("BoxSize"), KillBoxBounds);


		}

			
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
	
	GetWorld()->GetTimerManager().SetTimer
	(SimulationTimerHandle, this, &AWaterFall::GenerateWaterFallSpline, GetDataBufferRate, true );
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
			//NiagaraComponent->bAutoActivate=false;
			NiagaraComponent->Deactivate();
			//NiagaraComponent->ReregisterComponent();
		}
	}
		GetWorld()->GetTimerManager().ClearTimer(SimulationTimerHandle); 
	
}



void AWaterFall::CollectionParticleDataBuffer()
{
	UE_LOG(LogTemp, Warning ,TEXT(" Start GenerateSplineMesh！"))
	
	if(StoreSystemInstance == nullptr)
		return;
	
	auto SystemSimulation = StoreSystemInstance->GetSystemSimulation();
	
	const bool bSystemSimulationValid = SystemSimulation.IsValid() && SystemSimulation->IsValid();
	//等待异步完成再去访问资源，否则触发崩溃
	if(bSystemSimulationValid)
	{
		SystemSimulation->WaitForInstancesTickComplete();
	}
	//TArray<FParticleData> ParticleDataArray; //储存所有发射器的粒子数据
	FCustomNiagaraDataSetAccessor DataSetAccessor;
	
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
			UE_LOG(LogTemp,Warning,TEXT("DataBuffer is Not Valid ！"));

			bSimulateValid = true;
			SimulateState = EWaterFallButtonState::Simulate;
			GetWorld()->GetTimerManager().ClearTimer(SimulationTimerHandle);

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
					DataSetAccessor.GetParticleDataFromDataBuffer(CompiledData, DataBuffer, ParticleVar, iInstance, TempParticleData.Position);
				} 
				else if (ParticleVar == "Velocity") 
				{
					DataSetAccessor.GetParticleDataFromDataBuffer(CompiledData, DataBuffer, ParticleVar, iInstance, TempParticleData.Velocity);
				} 
				else if (ParticleVar == "Age") 
				{
					DataSetAccessor.GetParticleDataFromDataBuffer(CompiledData, DataBuffer, ParticleVar, iInstance, TempParticleData.Age);
				}
				else if (ParticleVar == "UniqueID") 
				{
					DataSetAccessor.GetParticleDataFromDataBuffer(CompiledData, DataBuffer, ParticleVar, iInstance, TempParticleData.UniqueID);
				}
				else if(ParticleVar == "CollisionDelayTimer")
				{
					DataSetAccessor.GetParticleDataFromDataBuffer(CompiledData, DataBuffer, ParticleVar, iInstance, TempParticleData.CollisionDelayTimer);
				
				}
			}
			ParticleDataArray.Add(TempParticleData);
			
		}

		/*
		for(const FParticleData& particle : ParticleDataArray)
		{
			UE_LOG(LogTemp, Warning, TEXT("Particle Data - UniqueID: %d, Position: %s, Velocity: %s, Age: %f, CollisionDelayTimer: %f"), 
				   particle.UniqueID, 
				   *particle.Position.ToString(), 
				   *particle.Velocity.ToString(), 
				   particle.Age,
				   particle.CollisionDelayTimer);
		}
		UE_LOG(LogTemp, Error, TEXT("============================================================================"));
		
		for(const FParticleData& Particle : ParticleDataArray)
		{
			UpdateSplineComponent(Particle.UniqueID, Particle.Position);
		}*/
	}
	
}


void AWaterFall::GenerateWaterFallSpline()
{
	//由于DataBuffer是在间隔时间内传输多个粒子数据，所以绘制spline的时候不需要之前的数据，要清空ParticleDataArray
	ParticleDataArray.Empty();

	//间隔时间内收集DataBuffer
	CollectionParticleDataBuffer();


	//绘制spline
	for(const FParticleData& Particle : ParticleDataArray)
	{
		UpdateSplineComponent(Particle.UniqueID, Particle.Position);
		
	}
}



void AWaterFall::ClusterSplines()
{

	//TArray<USplineComponent*> SplineComponents;
	//CachedSplineOriginalLengths.GetKeys(SplineComponents);
	
	USplineProcessor* TempProcessor = NewObject<USplineProcessor>();
	TempProcessor->WeightData = this->ClusterParameters;
	TempProcessor->ProcessSplines(VaildSplines);
	ClustersToUse = TempProcessor->GetClusters();
	ClusterNumber = ClustersToUse.Num();
	//SplineProcessorInstance->ProcessSplines(SplineComponents);

}


void AWaterFall::GenerateSplineMesh()
{
	ClearAllSplineMesh();
	//暂且在这个地方调用,因为不需要每帧绘制，所以不能Spline那样

	for (FCluster& Cluster : ClustersToUse)
	{
		USplineComponent* InSpline = Cluster.RepresentativeSpline;
		TArray<USplineMeshComponent*> SplineMeshes = UpdateSplineMeshComponent(InSpline);
		//将splineMesh和对应的Spline 存储起来，重建Mesh用
		CachedSplineAndSplineMeshes.Add(InSpline, SplineMeshes);
	}
	/*
	//这里调用分簇算法。先Resample， 再分簇， 分簇完最好返回一个簇的开始和结尾宽度，
	for(auto& SplinePair : CachedSplineOriginalLengths)
	{
		USplineComponent* InSpline = SplinePair.Key;


		//这里


		//
		TArray<USplineMeshComponent*> SplineMeshes = UpdateSplineMeshComponent(InSpline);
		//将splineMesh和对应的Spline 存储起来，重建Mesh用
		CachedSplineAndSplineMeshes.Add(InSpline, SplineMeshes);
	}
	*/
}




void AWaterFall::UpdateSplineComponent(int32 ParticleID, FVector ParticlePosition)
{

	USplineComponent** SplineComponentPtr = ParticleIDToSplineComponentMap.Find(ParticleID);
	if(SplineComponentPtr)
	{
		WaterFallSpline = *SplineComponentPtr;
		WaterFallSpline->AddSplineWorldPoint(ParticlePosition);
		WaterFallSpline->MarkRenderStateDirty();

	}
	else
	{
		// 如果没找到，创建一个新的SplineComponent并添加到TMap
		WaterFallSpline = NewObject<USplineComponent>(this, USplineComponent::StaticClass()); 
		WaterFallSpline->SetHiddenInGame(true);
		WaterFallSpline->SetMobility(EComponentMobility::Movable);
		WaterFallSpline->RegisterComponent();  // 注册组件，使其成为场景的一部分
		WaterFallSpline->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepWorldTransform);
		WaterFallSpline->SetVisibility(true, true);
		WaterFallSpline->ClearSplinePoints(true);
		WaterFallSpline->AddSplinePointAtIndex(ParticlePosition,0, ESplineCoordinateSpace::World,true );
		WaterFallSpline->SetDrawDebug(true);
		//这个Map是粒子ID和SplineComponent的一个映射，判断接下来收集到的Buffer该绘制那根曲线
		ParticleIDToSplineComponentMap.Add(ParticleID, WaterFallSpline);

#if WITH_EDITORONLY_DATA
		//const FLinearColor RandomColor(RandomStream.FRand(), RandomStream.FRand(), RandomStream.FRand(), 1.0f);
		//WaterFallSpline->EditorUnselectedSplineSegmentColor = FColor(1.0f, 1.0f, 1.0f, 1.0f);
		//UE_LOG(LogTemp,Warning,TEXT("Color : %s"),*RandomColor.ToString());
		//WaterFallSpline->EditorSelectedSplineSegmentColor=(FColor(1.0f, 0.0f, 0.0f, 1.0f));
		WaterFallSpline->MarkRenderStateDirty();
#endif
		
	}

	//储存SplineComponent和长度 Resample用
	CachedSplineOriginalLengths.Add(WaterFallSpline, WaterFallSpline->GetSplineLength());
	BackupSplineData.Add(WaterFallSpline, WaterFallSpline);
	GEditor->RedrawLevelEditingViewports(true);
}


//TODO 后期考虑缓存一个spline,进行分簇筛选和重采样后再生成Mesh
TArray<USplineMeshComponent*> AWaterFall::UpdateSplineMeshComponent(USplineComponent* Spline)
{
	
	//重置之前存的结束宽度，否则生成的曲线会越来越宽
	LastSegmentEndWidth = 0.0f;
	int32 NumSplineSegments = Spline->GetNumberOfSplineSegments();

	// 生成每条Spline Mesh的随机首尾宽度
	const  float RandomStartWidth = RandomStream.FRandRange(StartWidthRange.X, StartWidthRange.Y);  
	const  float RandomEndWidth = RandomStream.FRandRange(EndWidthRange.X, EndWidthRange.Y);  
	const  float Step = (RandomEndWidth - RandomStartWidth)/NumSplineSegments;

	// 初始宽度设为随机开始宽度或上一段的结束宽度
	float SplineMeshStartWidth = (LastSegmentEndWidth != 0.0f) ? LastSegmentEndWidth : RandomStartWidth;
	float SplineMeshEndWidth = SplineMeshStartWidth + Step;

	//把生成的SplineMesh返回出去，然后用Map根据SplineMesh属于哪条Spline存储起来，重建Mesh用。
	TArray<USplineMeshComponent*> TempSplineMesh;
	TempSplineMesh.Empty();
	for(int32 i = 0; i < NumSplineSegments; ++i)
	{
		FVector StartPos, StartTangent, EndPos, EndTangent;
		Spline->GetLocationAndTangentAtSplinePoint(i, StartPos, StartTangent, ESplineCoordinateSpace::Local);
		Spline->GetLocationAndTangentAtSplinePoint(i + 1, EndPos, EndTangent, ESplineCoordinateSpace::Local);
		//Spline->bShouldVisualizeScale = true;
		
		USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(this, USplineMeshComponent::StaticClass());
		if(SourceStaticMesh == nullptr)
		{
			SplineMesh->SetStaticMesh(Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(),nullptr,TEXT("/Engine/EditorLandscapeResources/SplineEditorMesh"))));
		}
		else
		{
			SplineMesh->SetStaticMesh(SourceStaticMesh);

		}
		SplineMesh->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);

		//设置SplineMesh宽度
		SplineMesh->SetStartScale(FVector2d(SplineMeshStartWidth,SplineMeshStartWidth),true);
		SplineMesh->SetEndScale(FVector2d(SplineMeshEndWidth,SplineMeshEndWidth),true);

		// 存储这一段的结束宽度，以便下一段使用，上一段结束的宽度就是下一段开始的宽度
		LastSegmentEndWidth = SplineMeshEndWidth;
		
		// 更新下一段的开始和结束宽度
		SplineMeshStartWidth = SplineMeshEndWidth;
		SplineMeshEndWidth += Step;
		
		SplineMesh->SetMobility(EComponentMobility::Movable);
		SplineMesh->AttachToComponent(Spline, FAttachmentTransformRules::KeepWorldTransform);  // 将Spline Mesh附加到Spline上
		SplineMesh->RegisterComponent();
		SplineMesh->SetVisibility(true, true);

		TempSplineMesh.Add(SplineMesh);
	}
	return TempSplineMesh;
}


void AWaterFall::ClearAllResource()
{
	check(IsInGameThread());
	DestroyWaterFallMeshActor();
	/*
	for(const auto& ElementSpline: ParticleIDToSplineComponentMap)
	{
		USplineComponent* SplineComponent = ElementSpline.Value;
		if(SplineComponent)
		{
			SplineComponent->DestroyComponent();
		}
	}
	*/
	ClearAllSpline();
	//清理SplineMesh和TMap
	ClearAllSplineMesh();
	ClearSpawnedParticles();
	CachedSplineAndSplineMeshes.Empty();
	
	//清空Spline相关的TMap
	ParticleIDToSplineComponentMap.Empty();
	CachedSplineOriginalLengths.Empty();
	BackupSplineData.Empty();
	EmitterPoints.Empty();
	VaildSplines.Empty();
	UE_LOG(LogTemp, Warning, TEXT("Cleared all Spline and SplineMesh components"));
}

void AWaterFall::ClearAllSpline()
{
	for(const auto& ElementSpline: ParticleIDToSplineComponentMap)
	{
		USplineComponent* SplineComponent = ElementSpline.Value;
		if(SplineComponent)
		{
			SplineComponent->DestroyComponent();
		}
	}
}

void AWaterFall::ClearAllSplineMesh()
{
	check(IsInGameThread());
	for(const auto& ElementSplineMesh: CachedSplineAndSplineMeshes)
	{
		TArray<USplineMeshComponent* >SplineMeshComponents = ElementSplineMesh.Value;
		for(USplineMeshComponent* SplineMeshComponent : SplineMeshComponents)
		{
			if(SplineMeshComponent)
			{
				SplineMeshComponent->DestroyComponent();

			}
		}
	}
	CachedSplineAndSplineMeshes.Empty();	

}

void AWaterFall::DestroyWaterFallMeshActor()
{
	if(RebuildedStaticMeshActor)
	{
		RebuildedStaticMeshActor->Destroy();
		RebuildedStaticMeshActor = nullptr;
	} 
}

void AWaterFall::ReGenerateSplineAfterResample()
{
	

	for(const auto& SplineLengthPair : CachedSplineOriginalLengths)
	{
		USplineComponent* InSpline = SplineLengthPair.Key;
		const float SplineLength = SplineLengthPair.Value;
		const USplineComponent* BackupSpline = BackupSplineData[InSpline];
		
		TArray<FVector> PerSplineLocation = ResampleSplinePoints(BackupSpline, RestLength, SplineLength);
		InSpline->ClearSplinePoints();
		InSpline->SetSplinePoints(PerSplineLocation, ESplineCoordinateSpace::World,true);
		InSpline->SetHiddenInGame(true);
		InSpline->SetMobility(EComponentMobility::Movable);
		InSpline->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepWorldTransform);
		InSpline->SetVisibility(true, true);
		PerSplineLocation.Empty();
	}
}

void AWaterFall::ReGenerateSplineAfterResampleWithNumber()
{
	for(const auto& SplineLengthPair : CachedSplineOriginalLengths)
	{
		
		USplineComponent* InSpline = SplineLengthPair.Key;
		const USplineComponent* BackupSpline = BackupSplineData[InSpline];
		float Ssegment = InSpline->GetNumberOfSplineSegments();

		UE_LOG(LogTemp, Warning, TEXT("Before Resampling - InSpline Segments: %d, BackupSpline Segments: %d"), InSpline->GetNumberOfSplineSegments(), BackupSpline->GetNumberOfSplineSegments());

		float Bsegemnt = BackupSpline->GetNumberOfSplineSegments();
		TArray<FVector> PerSplineLocation = ResampleSplinePointsWithNumber(BackupSpline, SampleNumber);
		InSpline->ClearSplinePoints();
		InSpline->SetSplinePoints(PerSplineLocation, ESplineCoordinateSpace::World,true);
		InSpline->SetHiddenInGame(true);
		InSpline->SetMobility(EComponentMobility::Movable);
		InSpline->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepWorldTransform);
		InSpline->SetVisibility(true, true);
		PerSplineLocation.Empty();
		UE_LOG(LogTemp, Warning, TEXT("After Resampling - InSpline Segments: %d , BackupSpline Segments: %d"), InSpline->GetNumberOfSplineSegments(), BackupSpline->GetNumberOfSplineSegments());
	}
}

float AWaterFall::GetAngleBetweenVectors(const FVector& A, const FVector& B)
{
	// 计算两个向量的点积
	const float DotProduct = FVector::DotProduct(A.GetSafeNormal(), B.GetSafeNormal());
	// 通过点积计算夹角的余弦值
	const float CosAngle = FMath::Clamp(DotProduct, -1.0f, 1.0f);
	// 计算并返回夹角的度数
	return FMath::RadiansToDegrees(acosf(CosAngle));
}

void AWaterFall::FitterSplines(const float LengthPercent, const float DistancePercent, const float& Angle)
{
	VaildSplines.Empty();
	
	// 首先找到最长的样条线长度
	float SplineMaxLength = 0.0f;
	for(const auto& Elem : ParticleIDToSplineComponentMap)
	{
		USplineComponent* SplineComponent = Elem.Value;
		SplineMaxLength = FMath::Max(SplineMaxLength, SplineComponent->GetSplineLength());
	}
	//粒子的发射方向
	//TODO 以后换成粒子点带的方向
	const FVector ParticleEmissionDirection = FVector(0.0f, -1.0f, 0.0f);
	
	for( const auto& Elem : ParticleIDToSplineComponentMap)
	{
		USplineComponent* SplineComponent = Elem.Value;
		const float CurrentSplineLength = SplineComponent->GetSplineLength();
		const float EmitterSplineLength = EmitterSpline->GetSplineLength()/2;
		
		const FVector StartPosition = SplineComponent->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
		const FVector EndPosition = SplineComponent->GetLocationAtSplinePoint(SplineComponent->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World);
		const float Distance = FMath::Abs(EndPosition.Y - StartPosition.Y);
		
		// 计算样条线起点的切线方向
		FVector SplineTangent = SplineComponent->GetTangentAtSplinePoint(0, ESplineCoordinateSpace::World).GetSafeNormal();
		
		// 计算切线方向与粒子发射方向之间的夹角
		 const float VectorAngle = GetAngleBetweenVectors(SplineTangent, ParticleEmissionDirection);

		if(CurrentSplineLength / SplineMaxLength >= LengthPercent /*&& VectorAngle <= Angle*/ && Distance/EmitterSplineLength <= DistancePercent)
		{
			VaildSplines.Add(SplineComponent);
			SplineComponent->EditorUnselectedSplineSegmentColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
			
			SplineComponent->MarkRenderStateDirty();
		}
		else
		{
			SplineComponent->EditorUnselectedSplineSegmentColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
		
			SplineComponent->MarkRenderStateDirty();
		}

	}
	
}


TArray<FVector> AWaterFall::ResampleSplinePointsWithNumber(const USplineComponent* InSpline, int32 SampleNum)
{
	
	TArray<FVector> Result;

	if(InSpline)
	{
		bool bIsLoop = InSpline->IsClosedLoop();
		float Duration = InSpline->Duration;

		int32 UseSamples = FMath::Max(2, SampleNum);
		float DivNum = float(UseSamples - (int32)!bIsLoop);
		Result.Reserve(UseSamples);

		for (int32 Idx = 0; Idx < UseSamples; Idx++)
		{
			float Time = Duration * ((float)Idx / DivNum);
			FTransform Transform = InSpline->GetTransformAtTime(Time, ESplineCoordinateSpace::World, true, true);
			//UE_LOG(LogTemp, Warning, TEXT("LOcation : %s"), *Transform.GetLocation().ToString());
			Result.Add(Transform.GetLocation());
		}
	}
	return Result;
}


TArray<FVector> AWaterFall::ResampleSplinePoints( const USplineComponent* InSpline, float ResetLength, float SplineLength)
{
	ResetLength *= 100.0f; // Convert meters to centimeters

	TArray<FVector> Result;

	if (!InSpline || ResetLength <= SMALL_NUMBER)
	{
		return Result; 
	}

	bool bIsLoop = InSpline->IsClosedLoop();
	float Duration = InSpline->Duration;


	//UE_LOG(LogTemp, Warning, TEXT("Length : %f "), SplineLength);
	int32 Segments = FMath::FloorToInt(SplineLength / ResetLength);
	float DiffLength = SplineLength - ResetLength * Segments; // Remaining length

	
	// Adjust ResetLength based on remaining length
	if (DiffLength > ResetLength / 2)
	{
		Segments += 1;
		ResetLength += DiffLength / Segments;
	}
	else
	{
		ResetLength -= DiffLength / Segments;
	}

	float DivNum = float(Segments - (int32)!bIsLoop);
	
	for (int32 Idx = 0; Idx < Segments; Idx++)
	{
		float Time = Duration * ((float)Idx / DivNum);
		const float Distance= Time/  Duration * InSpline->GetSplineLength();
		//const float Distance = FMath::Min(SampleIdx * ResetLength, OriginalSplineLength);
		
		FTransform SampleTransform = InSpline->GetTransformAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World,true);
		Result.Add(SampleTransform.GetLocation());
		//UE_LOG(LogTemp, Warning, TEXT("Distance ： %f"), Distance);
	}

	//UE_LOG(LogTemp, Warning, TEXT("Generated %d sample points."), Result.Num());

	return Result;
}

void AWaterFall::FillMeshDescription(FMeshDescription& MeshDescription, const TArray<FVector3f>& Positions, const TArray<FVector3f>& Normals,TArray<FVector2f>& UVs,  const TArray<int32>& Triangles)
{
	FStaticMeshAttributes Attributes(MeshDescription);
	Attributes.Register();

	int32 VertexNUmber= Positions.Num();
	MeshDescription.ReserveNewVertices(Positions.Num());
	MeshDescription.ReserveNewVertexInstances(Positions.Num());

	FPolygonGroupID PolygonGroupID = MeshDescription.CreatePolygonGroup();

	MeshDescription.ReserveNewPolygons(Triangles.Num() / 3);
	MeshDescription.ReserveNewTriangles(Triangles.Num() / 3);
	MeshDescription.ReserveNewEdges(Triangles.Num());

	// Populate Vertex, VertexInstance Data
	TArray<FVertexID> CreatedVertexIDs;
	CreatedVertexIDs.Reserve(Positions.Num());
	TArray<FVertexInstanceID> CreatedVertexInstanceIDs;
	CreatedVertexInstanceIDs.Reserve(Positions.Num());

	for (int32 VertexIndex = 0; VertexIndex < Positions.Num(); ++VertexIndex)
	{
		FVertexID VertexID = MeshDescription.CreateVertex();
		CreatedVertexIDs.Add(VertexID);
		Attributes.GetVertexPositions()[VertexID] = Positions[VertexIndex];

		FVertexInstanceID VertexInstanceID = MeshDescription.CreateVertexInstance(VertexID);
		CreatedVertexInstanceIDs.Add(VertexInstanceID);
		Attributes.GetVertexInstanceUVs().Set(VertexInstanceID, 0, UVs[VertexIndex]);
		Attributes.GetVertexInstanceNormals().Set(VertexInstanceID, Normals[VertexIndex]);
	}

	// Populate Triangle Data
	for (int32 TriangleIndex = 0; TriangleIndex < Triangles.Num(); TriangleIndex += 3)
	{
		TArray<FVertexInstanceID> TriangleVertexInstances = {
			CreatedVertexInstanceIDs[Triangles[TriangleIndex]],
			CreatedVertexInstanceIDs[Triangles[TriangleIndex + 1]],
			CreatedVertexInstanceIDs[Triangles[TriangleIndex + 2]]
		};
		MeshDescription.CreateTriangle(PolygonGroupID, TriangleVertexInstances);
	}
}


UStaticMesh* AWaterFall::RebuildStaticMeshFromSplineMesh()
{
	
	TMap<int32, TArray<FVector3f>> LODCombinedVertices;
	TMap<int32, TArray<FVector3f>> LODCombinedNormals;
	TMap<int32, TArray<FVector2f>> LODCombinedUVs;
	TMap<int32, TArray<int32>> LODCombinedTriangles;
	TMap<int32, int32> LODVertexCounts; //记录每个LOD的顶点数量

	TArray<TUniquePtr<FMeshDescription>> AllLODMeshDescriptions;
	
	//遍历每条Spline
	for(const auto& SplineMeshPair : CachedSplineAndSplineMeshes)
	{
		
		const USplineComponent* SplineComponent = SplineMeshPair.Key;
		if(!SplineComponent)continue; 

		//获取每条Spline的长度,来计算UV的V值
		const float OriginalSplineLength = CachedSplineOriginalLengths[SplineComponent];
		
		//获取Spline上对应的多个SplineMesh
		const TArray<USplineMeshComponent*> SplineMeshComponents = SplineMeshPair.Value;
		//int32 VertexIdOffset = 0; // 记录当前的顶点索引偏移，这是为了更新三角形的索引，假设第一个SplineMesh索引为0，1，2， 第二个就得是3，4，5
		
		//遍历每条Spline中所有的SplineMesh,获取顶点Buffer
		for(USplineMeshComponent* SplineMesh : SplineMeshComponents)
		{
			if(!SplineMesh)continue; 
			UStaticMesh* SourceMesh = SplineMesh->GetStaticMesh();//拿到SplineMesh引用的StaticMesh
			if(!SourceMesh)continue;
			
			FVector2f UVOffset = CalculateUVOffsetBasedOnSpline(SplineComponent, SplineMesh, SplineMeshComponents, 1);
			//遍历每个SplineMesh的每级LOD,提取vertexBuffer
			for(int32 LODIndex = 0; LODIndex < SourceMesh->GetNumLODs(); LODIndex++)
			{
				const FStaticMeshLODResources& LODResource = SourceMesh->GetLODForExport(LODIndex);
				
				int32& CurrentVertexCount = LODVertexCounts.FindOrAdd(LODIndex);

				TArray<FVector3f>& CurrentLODVertices = LODCombinedVertices.FindOrAdd(LODIndex);
				TArray<FVector3f>& CurrentLODNormals = LODCombinedNormals.FindOrAdd(LODIndex);
				TArray<FVector2f>& CurrentLODUVs = LODCombinedUVs.FindOrAdd(LODIndex);
				TArray<int32>& CurrentLODTriangles = LODCombinedTriangles.FindOrAdd(LODIndex);
				
				// 添加顶点和UVs
				for(uint32 i = 0; i < LODResource.VertexBuffers.PositionVertexBuffer.GetNumVertices(); i++)
				{
					FVector3f LocalPosition = LODResource.VertexBuffers.PositionVertexBuffer.VertexPosition(i);

					//求一个切变变换应用到顶点上
					float& AxisValue = USplineMeshComponent::GetAxisValueRef(LocalPosition, SplineMesh->ForwardAxis);
					const FTransform SliceTransform = SplineMesh->CalcSliceTransform(AxisValue);
					AxisValue = 0.0f;

					// Apply spline deformation for  vertex position
					FVector DeformedPosition = SliceTransform.TransformPosition(static_cast<FVector>(LocalPosition));
					const FVector WorldPosition = SplineMesh->GetComponentToWorld().TransformPosition(DeformedPosition);
					CurrentLODVertices.Add(static_cast<FVector3f>(WorldPosition));
					
					// Apply spline deformation for  vertex Normal
					FVector3f LocalNormal = LODResource.VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(i);
					FVector WorldNormal = SplineMesh->GetComponentToWorld().TransformVectorNoScale(static_cast<FVector>(LocalNormal));
					FVector DeformedNormal = SliceTransform.TransformVector(WorldNormal).GetSafeNormal();
					CurrentLODNormals.Add(static_cast<FVector3f>(DeformedNormal));
					
					FVector2f OriginalUV = LODResource.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(i, 0);
					// 根据UV偏移量调整UV
					OriginalUV += UVOffset;
					CurrentLODUVs.Add(OriginalUV);
				}
				// 添加三角形并更新索引
				FIndexArrayView Indices = LODResource.IndexBuffer.GetArrayView();
				for(int32 i = 0; i < Indices.Num(); i++)
				{
					CurrentLODTriangles.Add(Indices[i] + CurrentVertexCount);
				}
				// 更新这个LOD的顶点计数
				CurrentVertexCount += LODResource.VertexBuffers.PositionVertexBuffer.GetNumVertices();
			}
		}
	}

	// 为每个LOD创建一个FMeshDescription
	for(auto& PerLODPair : LODCombinedVertices)
	{
		auto LODIndex = PerLODPair.Key;
		TUniquePtr<FMeshDescription> MeshDescription = MakeUnique<FMeshDescription>();
		FillMeshDescription(*MeshDescription, LODCombinedVertices[LODIndex],LODCombinedNormals[LODIndex],LODCombinedUVs[LODIndex],LODCombinedTriangles[LODIndex]);
		FStaticMeshOperations::ComputeTriangleTangentsAndNormals(*MeshDescription, 0.0f);
		FStaticMeshOperations::RecomputeNormalsAndTangentsIfNeeded(*MeshDescription, EComputeNTBsFlags::BlendOverlappingNormals);
		AllLODMeshDescriptions.Add(MoveTemp(MeshDescription));
	}
	
	// 创建一个新的StaticMesh实例
	UStaticMesh* NewStaticMesh = NewObject<UStaticMesh>(this, FName("WaterFallMesh"), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
	NewStaticMesh->InitResources();

	//设置默认材质
	FStaticMaterial NewMaterialSlot(DefaultMaterial);
	NewStaticMesh->GetStaticMaterials().Add(NewMaterialSlot);
	NewStaticMesh->PostEditChange();
	
	NewStaticMesh->SetRenderData(MakeUnique<FStaticMeshRenderData>());
	NewStaticMesh->CreateBodySetup();
	NewStaticMesh->bAllowCPUAccess = true;
	NewStaticMesh->GetBodySetup()->CollisionTraceFlag = ECollisionTraceFlag::CTF_UseComplexAsSimple;
	
	UStaticMesh::FBuildMeshDescriptionsParams MeshDescriptionParams;
	MeshDescriptionParams.bBuildSimpleCollision = true;
	MeshDescriptionParams.bFastBuild =true;
	
	TArray<const FMeshDescription*> ConstMeshDescriptions;
	for(const TUniquePtr<FMeshDescription>& Desc : AllLODMeshDescriptions)
	{

		ConstMeshDescriptions.Add(Desc.Get());//使用Desc.Get()从TUniquePtr中获取FMeshDescription的原始指针，并将其添加到ConstMeshDescriptions中
		
	}
	NewStaticMesh->BuildFromMeshDescriptions(ConstMeshDescriptions, MeshDescriptionParams);

	//设置各个LOD相关属性,与SplineMesh 保持一致
	for(int32 LODIndex = 0; LODIndex < NewStaticMesh->GetNumSourceModels(); LODIndex++)
	{
		//这里WaterFallMesh就是SplineMesh引用的StaticMesh
		if(SourceStaticMesh->GetNumSourceModels() > LODIndex)
		{
			NewStaticMesh->GetSourceModel(LODIndex).ScreenSize = SourceStaticMesh->GetSourceModel(LODIndex).ScreenSize;
			NewStaticMesh->GetSourceModel(LODIndex).BuildSettings.bRecomputeNormals = true;
			NewStaticMesh->GetSourceModel(LODIndex).BuildSettings.bUseFullPrecisionUVs = true;
			NewStaticMesh->GetSourceModel(LODIndex).BuildSettings.bRecomputeTangents = true;
			NewStaticMesh->GetSourceModel(LODIndex).BuildSettings.bGenerateLightmapUVs = true;
		}
	}
	NewStaticMesh->PostEditChange();
	return  NewStaticMesh;
}

void AWaterFall::RebuildWaterFallMesh()
{
	
	DestroyWaterFallMeshActor();
	
	//1.重建StaticMesh
	UStaticMesh* RebuildStaticMesh  = RebuildStaticMeshFromSplineMesh();

	if (!RebuildStaticMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to rebuild StaticMesh."));
		return;
	}

	//2.保存该StaticMesh为资产
	UStaticMesh* SavedStaticMesh = SaveAssetToDisk(RebuildStaticMesh, MeshName, SavePath);

	// 3. 使用已保存的资产生成或更新场景中的AStaticMeshActor
	if (!RebuildedStaticMeshActor)
	{
		RebuildedStaticMeshActor = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), SceneRoot->GetComponentLocation(), SceneRoot->GetComponentRotation());
	}

	//给RebuildedStaticMeshActor新建Component
	UStaticMeshComponent* WaterFallMeshComponent = NewObject<UStaticMeshComponent>(RebuildedStaticMeshActor);
	if(WaterFallMeshComponent)
	{
		const FStaticMeshLODResources& LODResource  = RebuildStaticMesh->GetLODForExport(0);
		//UE_LOG(LogTemp, Error, TEXT("NewStaticMesh Vertex Count: %d"), LODResource.VertexBuffers.PositionVertexBuffer.GetNumVertices());
		WaterFallMeshComponent->SetStaticMesh(SavedStaticMesh);
		WaterFallMeshComponent->AttachToComponent(RebuildedStaticMeshActor->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
		
		
		// 标记演员的静态网格组件已更改
		WaterFallMeshComponent->MarkPackageDirty();
		WaterFallMeshComponent->RegisterComponent();
		
		WaterFallMeshComponent->PostEditChange();

		
		RebuildedStaticMeshActor->SetActorLabel(SavedStaticMesh->GetName());
		RebuildedStaticMeshActor->SetFolderPath(FName(TEXT("WaterFall")));
		RebuildedStaticMeshActor->InvalidateLightingCache();
		RebuildedStaticMeshActor->PostEditMove(true);
		RebuildedStaticMeshActor->PostEditChange();
		RebuildedStaticMeshActor->MarkPackageDirty();
		
		UE_LOG(LogTemp, Error, TEXT("StaticMeshComponent's StaticMesh is: %s"),*WaterFallMeshComponent->GetStaticMesh().GetName());
	}

	//TODO:最后一定要清空这个Map,如果在这里清空上一次生成的SplineMesh还是追踪不到
	//CachedSplineAndSplineMeshes.Empty();
}

UStaticMesh* AWaterFall::SaveAssetToDisk(const UStaticMesh* InStaticMesh, const FString& StaticMeshName, const FString& SaveRelativePath)
{
	// 获取AssetTools模块。
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		
	const FString AssetName = StaticMeshName;
	const FString AssetPath = SaveRelativePath + StaticMeshName;
	//AssetToolsModule.Get().CreateUniqueAssetName(*SavePath, *MeshName, AssetPath, AssetName);

	//创建一个新的资产包
	UPackage* StaticMeshPackage = CreatePackage(*AssetPath);
		
	// 加载资源包到内存
	StaticMeshPackage->FullyLoad();

	// 复制对象到这个新的包
	UStaticMesh* SavedStaticMesh = DuplicateObject<UStaticMesh>(InStaticMesh, StaticMeshPackage, *AssetName);

	SavedStaticMesh->MarkPackageDirty();

	//通知资产注册模块资产已创建
	FAssetRegistryModule::AssetCreated(SavedStaticMesh);

	// 保存资产
	//将相对路径转换为工程的磁盘绝对路径
	FString FilePath = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = EObjectFlags::RF_Public | EObjectFlags::RF_Standalone;
	SaveArgs.Error = GLog;
	UPackage::SavePackage(StaticMeshPackage, SavedStaticMesh, *FilePath, SaveArgs);

	return SavedStaticMesh;
}

FVector2f AWaterFall::CalculateUVOffsetBasedOnSpline (const USplineComponent* SplineComponent,
	const USplineMeshComponent* CurrentSplineMeshComponent,
	const TArray<USplineMeshComponent*>& AllSplineMeshComponents, float SegmentLength)
{
	// 使用索引来计算当前SplineMesh的V值偏移
	int32 CurrentIndex = AllSplineMeshComponents.IndexOfByKey(CurrentSplineMeshComponent);
	float UVOffsetValue = SegmentLength * CurrentIndex;

	return FVector2f(0, UVOffsetValue);
}

void AWaterFall::SpawnParticles()
{
	ClearSpawnedParticles();
    
    if(bSpawnParticles)
    {
        // 遍历所有有效的样条线
        for(USplineComponent* Spline : VaildSplines)
        {
            if(Spline != nullptr)
            {
                // 获取样条线上的点的数量
                int32 NumberOfPoints = Spline->GetNumberOfSplinePoints();

            	// 获取样条线的最后一个点的切线方向
            	FVector EndTangent = Spline->GetTangentAtSplinePoint(NumberOfPoints - 1, ESplineCoordinateSpace::World);
            	// 将切线方向转换为旋转
            	FRotator EndRotation = EndTangent.Rotation();
                if(NumberOfPoints > 3)
                {
                    // 获取样条线的第一个点的位置
                    FVector StartPointLocation = Spline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
                    // 获取样条线的最后一个点的位置
                    FVector EndPointLocation = Spline->GetLocationAtSplinePoint(NumberOfPoints - 1, ESplineCoordinateSpace::World);

                    // 随机选择一个大小
                    float RandomScale = FMath::RandRange(ParticleScaleRange.X, ParticleScaleRange.Y);
                	
                	// 随机选择一个粒子系统
                	UParticleSystem* SelectedParticleSystem = BottomParticles[FMath::RandRange(0, BottomParticles.Num() - 1)];
					/*
                    // 在样条线的起始点生成粒子1
                    UParticleSystemComponent* SpawnedParticle1 = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), CenterParticle, StartPointLocation, FRotator::ZeroRotator, FVector(RandomScale), true);
                    if (IsValid(SpawnedParticle1))
                    {
                        SpawnedParticles.Add(SpawnedParticle1);
                    }
*/
                    // 在样条线的终点生成粒子2
                    UParticleSystemComponent* SpawnedParticle = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedParticleSystem, EndPointLocation, FRotator::ZeroRotator, FVector(RandomScale), true);
                    if (IsValid(SpawnedParticle))
                    {
                        SpawnedParticles.Add(SpawnedParticle);
                    }
                }
            }
        }

        // 删除距离过近的粒子
        for (int32 i = 0; i < SpawnedParticles.Num(); ++i)
        {
            if (!IsValid(SpawnedParticles[i]))
                continue;

            for (int32 j = i + 1; j < SpawnedParticles.Num(); ++j)
            {
                if (!IsValid(SpawnedParticles[j]))
                    continue;

                if (FVector::Dist(SpawnedParticles[i]->GetComponentLocation(), SpawnedParticles[j]->GetComponentLocation()) < MinDistanceBetweenParticles)
                {
                    SpawnedParticles[j]->DestroyComponent();
                    SpawnedParticles.RemoveAt(j);
                    --j; // Make sure to decrement j after removing an element to keep the index valid.
                }
            }
        }
    }
}

void AWaterFall::ClearSpawnedParticles()
{
	// 清除所有生成的粒子
	for (UParticleSystemComponent* ParticleComp : SpawnedParticles)
	{
		if (IsValid(ParticleComp))
		{
			ParticleComp->DestroyComponent(); 
		}
	}
	SpawnedParticles.Empty(); // 清空数组
}


// 计算变换后的顶点位置和法线的辅助函数
void AWaterFall::CalculateDeformedPositionAndNormal(const USplineMeshComponent* SplineMesh, const FVector3f& LocalPosition, const FVector3f& LocalNormal, FVector& OutPosition, FVector& OutNormal)
{
	// 根据SplineMesh的ForwardAxis求切变变换应用到顶点上
	float& AxisValue = USplineMeshComponent::GetAxisValueRef(const_cast<FVector3f&>(LocalPosition), SplineMesh->ForwardAxis);
	const FTransform SliceTransform = SplineMesh->CalcSliceTransform(AxisValue);
	AxisValue = 0.0f;

	// Apply spline deformation for vertex position
	const FVector DeformedPosition = SliceTransform.TransformPosition(static_cast<FVector>(LocalPosition));
	OutPosition = SplineMesh->GetComponentToWorld().TransformPosition(DeformedPosition);

	// Apply spline deformation for vertex normal
	const FVector WorldNormal = SplineMesh->GetComponentToWorld().TransformVectorNoScale(static_cast<FVector>(LocalNormal));
	OutNormal = SliceTransform.TransformVector(WorldNormal).GetSafeNormal();
}

void AWaterFall::CollectLODDataFromSplineMeshes(TMap<int32, FLODData>& LODDataMap)
{
    // 遍历每条Spline
    for (const auto& SplineMeshPair : CachedSplineAndSplineMeshes)
    {
        const USplineComponent* SplineComponent = SplineMeshPair.Key;
        if (!SplineComponent) continue; 

        // 获取每条Spline的长度,来计算UV的V值
        const float OriginalSplineLength = CachedSplineOriginalLengths[SplineComponent];

        // 获取Spline上对应的多个SplineMesh
        const TArray<USplineMeshComponent*>& SplineMeshComponents = SplineMeshPair.Value;

        // 遍历每条Spline中所有的SplineMesh,获取顶点Buffer
        for (USplineMeshComponent* SplineMesh : SplineMeshComponents)
        {
            if (!SplineMesh) continue;
            UStaticMesh* SourceMesh = SplineMesh->GetStaticMesh(); // 拿到SplineMesh引用的StaticMesh
            if (!SourceMesh) continue;

            FVector2f UVOffset = CalculateUVOffsetBasedOnSpline(SplineComponent, SplineMesh, SplineMeshComponents, 1);

            // 遍历每个SplineMesh的每级LOD,提取vertexBuffer
            for (int32 LODIndex = 0; LODIndex < SourceMesh->GetNumLODs(); LODIndex++)
            {
            	
            	FLODData& CurrentLODData = LODDataMap.FindOrAdd(LODIndex);
            	//CurrentLODData.VertexCount = 0; // 重置顶点计数
            	
                const FStaticMeshLODResources& LODResource = SourceMesh->GetLODForExport(LODIndex);
            	
                // 添加顶点和UVs
                for (uint32 VertexIndex = 0; VertexIndex < LODResource.VertexBuffers.PositionVertexBuffer.GetNumVertices(); VertexIndex++)
                {
                    FVector3f LocalPosition = LODResource.VertexBuffers.PositionVertexBuffer.VertexPosition(VertexIndex);
                    FVector3f LocalNormal = LODResource.VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(VertexIndex);
                    FVector2f OriginalUV = LODResource.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(VertexIndex, 0);

                    // 计算变换后的顶点位置和法线
                    FVector DeformedPosition, DeformedNormal;
                    CalculateDeformedPositionAndNormal(SplineMesh, LocalPosition, LocalNormal, DeformedPosition, DeformedNormal);

                    // 根据UV偏移量调整UV
                    FVector2f AdjustedUV = OriginalUV + UVOffset;

                    // 存储变换后的顶点、法线和UV
                    CurrentLODData.Vertices.Add(static_cast<FVector3f>(DeformedPosition));
                    CurrentLODData.Normals.Add(static_cast<FVector3f>(DeformedNormal));
                    CurrentLODData.UVs.Add(AdjustedUV);
                }

                // 添加三角形并更新索引
                FIndexArrayView Indices = LODResource.IndexBuffer.GetArrayView();
                for (int32 Index = 0; Index < Indices.Num(); Index++)
                {
                    // 索引需要加上当前LOD的顶点偏移量
                    CurrentLODData.Triangles.Add(Indices[Index] + CurrentLODData.VertexCount);
                }

                // 更新这个LOD的顶点计数
                CurrentLODData.VertexCount += LODResource.VertexBuffers.PositionVertexBuffer.GetNumVertices();
            }
        }
    }
}

void AWaterFall::CreateMeshDescriptionsForLODs(const TMap<int32, FLODData>& LODDataMap, TArray<FMeshDescription>& OutMeshDescriptions)
{
    OutMeshDescriptions.Empty(LODDataMap.Num());

    for (const TPair<int32, FLODData>& LODPair : LODDataMap)
    {
        const FLODData& LODData = LODPair.Value;

        FMeshDescription MeshDescription;
        MeshDescription.Empty();

        // 注册MeshDescription的属性
        FStaticMeshAttributes Attributes(MeshDescription);
        Attributes.Register();
    	

        // 预留空间
    	int32 VertexNUmber=LODData.VertexCount;
        MeshDescription.ReserveNewVertices(LODData.VertexCount);
        MeshDescription.ReserveNewVertexInstances(LODData.VertexCount); // 根据三角形数量预留顶点实例 
        MeshDescription.ReserveNewTriangles(LODData.Triangles.Num()/3);
        MeshDescription.ReserveNewPolygons(LODData.Triangles.Num()/3);

    	// 创建一个多边形组
    	FPolygonGroupID PolygonGroupID = MeshDescription.CreatePolygonGroup();
    	
        // 添加顶点
        for (const FVector3f& Vertex : LODData.Vertices)
        {
            const FVertexID VertexID = MeshDescription.CreateVertex();
            Attributes.GetVertexPositions()[VertexID] = Vertex;
        }
    	
        // 添加三角形和顶点实例
        for (int32 TriangleIndex = 0; TriangleIndex < LODData.Triangles.Num(); TriangleIndex += 3)
        {
            FVertexID VertexIDs[3];
            FVertexInstanceID VertexInstanceIDs[3];

            for (int32 Corner = 0; Corner < 3; ++Corner)
            {
                int32 VertexIndex = LODData.Triangles[TriangleIndex + Corner];
                VertexIDs[Corner] = FVertexID(VertexIndex);
                VertexInstanceIDs[Corner] = MeshDescription.CreateVertexInstance(VertexIDs[Corner]);
            	
            	// 确保顶点实例ID是有效的
            	check(MeshDescription.IsVertexInstanceValid(VertexInstanceIDs[Corner]));

            	// 添加日志输出来检查法线值是否有效
            	//FVector3f Normal = LODData.Normals[TriangleIndex];
            	FVector3f Normal = LODData.Normals[VertexIndex];
            	UE_LOG(LogTemp, Log, TEXT("Normal: %s"), *Normal.ToString());
            	/*
                Attributes.GetVertexInstanceNormals()[VertexInstanceIDs[Corner]] = LODData.Normals[TriangleIndex + Corner];
                Attributes.GetVertexInstanceUVs().Set(VertexInstanceIDs[Corner], 0, LODData.UVs[TriangleIndex + Corner]);
                */
            	// 为每个顶点实例设置法线和UV
            	Attributes.GetVertexInstanceNormals()[VertexInstanceIDs[Corner]] = LODData.Normals[VertexIndex];
            	Attributes.GetVertexInstanceUVs().Set(VertexInstanceIDs[Corner], 0, LODData.UVs[VertexIndex]);
            }

            // 创建多边形
        	const TArray<FVertexInstanceID> PerimeterVertexInstances(VertexInstanceIDs, UE_ARRAY_COUNT(VertexInstanceIDs));
        	MeshDescription.CreatePolygon(PolygonGroupID, PerimeterVertexInstances);
        }

        // 将构建的MeshDescription添加到输出数组
    	//构建StaticMesh 设置里面要把bRecomputeNormals设置为True
    	FStaticMeshOperations::ComputeTriangleTangentsAndNormals(MeshDescription, 0.0f);
    	FStaticMeshOperations::RecomputeNormalsAndTangentsIfNeeded(MeshDescription, EComputeNTBsFlags::BlendOverlappingNormals);
        OutMeshDescriptions.Add(MoveTemp(MeshDescription));
    }
}

UStaticMesh* AWaterFall::CreateStaticMeshFromLODMeshDescriptions(const TArray<FMeshDescription>& MeshDescriptions)
{
	// 创建一个新的StaticMesh实例
	UStaticMesh* StaticMesh = NewObject<UStaticMesh>(this, FName("WaterFallMesh"), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
	if (!StaticMesh)
	{
		return nullptr;
	}

	StaticMesh->InitResources();
	//设置默认材质
	FStaticMaterial MaterialSlot(DefaultMaterial);
	StaticMesh->GetStaticMaterials().Add(MaterialSlot);
	//StaticMesh->PostEditChange();
	
	// 设置渲染数据
	StaticMesh->SetRenderData(MakeUnique<FStaticMeshRenderData>());
	
	// 创建碰撞体
	StaticMesh->CreateBodySetup();
	StaticMesh->bAllowCPUAccess = true;
	StaticMesh->GetBodySetup()->CollisionTraceFlag = ECollisionTraceFlag::CTF_UseComplexAsSimple;

	/*
	UStaticMesh::FBuildMeshDescriptionsParams MeshDescriptionParams;
	MeshDescriptionParams.bBuildSimpleCollision = true;
	MeshDescriptionParams.bFastBuild =true;
	*/

	// 遍历所有的MeshDescription并提交到StaticMesh
	for (int32 LODIndex = 0; LODIndex < MeshDescriptions.Num(); ++LODIndex)
	{
		// 确保StaticMesh有足够的LOD slots
		if (StaticMesh->GetNumSourceModels() <= LODIndex)
		{
			StaticMesh->AddSourceModel();
		}

		// 获取当前LOD的模型
		FStaticMeshSourceModel& LODModel = StaticMesh->GetSourceModel(LODIndex);
        
		// 设置LOD的构建设置
		LODModel.BuildSettings.bRecomputeNormals = true;
		LODModel.BuildSettings.bRecomputeTangents = true;
		LODModel.BuildSettings.bRemoveDegenerates = true;
		LODModel.BuildSettings.bUseHighPrecisionTangentBasis = false;
		LODModel.BuildSettings.bUseFullPrecisionUVs = false;
		LODModel.BuildSettings.bGenerateLightmapUVs = true;
        
		// 设置LOD的屏幕尺寸
		LODModel.ScreenSize.Default = 0.1f / FMath::Pow(2.0f, LODIndex);

		// 获取MeshDescription的引用
		FMeshDescription* MeshDescription = StaticMesh->GetMeshDescription(LODIndex);
		if (!MeshDescription)
		{
			// 如果MeshDescription不存在，则创建一个新的
			MeshDescription = StaticMesh->CreateMeshDescription(LODIndex);
			check(MeshDescription); // 确保MeshDescription已创建
		}
		// 修改MeshDescription
		*MeshDescription = MeshDescriptions[LODIndex];
		
		// 提交MeshDescription到StaticMesh
		UStaticMesh::FCommitMeshDescriptionParams Params;
		Params.bMarkPackageDirty = false;
		Params.bUseHashAsGuid = true;
		StaticMesh->CommitMeshDescription(LODIndex, Params);
	}

	// 构建StaticMesh
	StaticMesh->Build(false);
	StaticMesh->MarkPackageDirty();
	StaticMesh->PostEditChange();

	// 返回创建的StaticMesh
	return StaticMesh;
	
}

void AWaterFall::SpawnStaticMesh(UStaticMesh* StaticMesh)
{
	DestroyWaterFallMeshActor();
	
	if (StaticMesh == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid static mesh."));
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	FTransform SceneRootTransform = SceneRoot->GetComponentTransform();
	// 如果 RebuildedStaticMeshActor 不存在，则创建它
	if (!RebuildedStaticMeshActor)
	{
		RebuildedStaticMeshActor = GetWorld()->SpawnActor<AStaticMeshActor>(
			AStaticMeshActor::StaticClass(), 
			SceneRootTransform
		);
	}
	else
	{
		// 如果 RebuildedStaticMeshActor 已存在，更新它的变换以匹配 SceneRoot
		RebuildedStaticMeshActor->SetActorTransform(SceneRootTransform);
	}

	// 获取静态网格组件并设置静态网格
	UStaticMeshComponent* StaticMeshComponent = RebuildedStaticMeshActor->GetStaticMeshComponent();
	if (StaticMeshComponent)
	{
		StaticMeshComponent->SetStaticMesh(StaticMesh);
		
		// 重置 StaticMeshComponent 的相对变换
		StaticMeshComponent->SetRelativeLocationAndRotation(FVector::ZeroVector, FQuat::Identity);

		StaticMeshComponent->RegisterComponent();
		StaticMeshComponent->MarkPackageDirty();
		StaticMeshComponent->PostEditChange();
	}

	RebuildedStaticMeshActor->SetActorLabel(StaticMesh->GetName());
	RebuildedStaticMeshActor->SetFolderPath(FName(TEXT("WaterFall")));
	RebuildedStaticMeshActor->InvalidateLightingCache();
	RebuildedStaticMeshActor->PostEditMove(true);
	RebuildedStaticMeshActor->PostEditChange();
	RebuildedStaticMeshActor->MarkPackageDirty();
}
void AWaterFall::BuildStaticMeshFromSplineMesh()
{
	GeneratedWaterFallMesh = nullptr;
	
	TMap<int32, FLODData> LODDataMap;
	TArray<FMeshDescription> MeshDescriptions;
	CollectLODDataFromSplineMeshes(LODDataMap);

	CreateMeshDescriptionsForLODs(LODDataMap, MeshDescriptions);

	UStaticMesh*StaticMesh = CreateStaticMeshFromLODMeshDescriptions(MeshDescriptions);
	
	SpawnStaticMesh(StaticMesh);
	
	//SaveAssetToDisk(StaticMesh, MeshName, SavePath);
	
	GeneratedWaterFallMesh = StaticMesh;
}
 

/*
TArray<FVector> AWaterFall::CalculateParticleLocation()
{
	for(const auto& SplineLengthPair : CachedSplineOriginalLengths)
	{
		USplineComponent* InSpline = SplineLengthPair.Key;
		
		
	}
}
*/

void AWaterFall::ResetParameters()
{
	bSnapToGround = false;
	ISMScale = 1.0f;
	Seed = 666;
	Disturb = 0.0f;
	SlopeRange = {0, 90};
	SplineCount = 50;
	ParticleLife = 7.0f;
	ParticleVelocity = 1.0f;
	GetDataBufferRate = 0.1f;
	RestLength = 2.0f;
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


void AWaterFall::PostEditUndo()
{
	Super::PostEditUndo();
	SplineDataChangedEvent.Broadcast();
}

//PostEditChangeChainProperty可以检测整个属性链上的多个属性，PostEditChangeProperty 通常用于响应单一属性的更改
void AWaterFall::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) 
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	FProperty* PropertyThatChanged = PropertyChangedEvent.MemberProperty;
	const FName PropertyName = PropertyThatChanged ? PropertyThatChanged->GetFName() : NAME_None;

	if(PropertyChangedEvent.Property != nullptr)
	{
		//if(!(PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive))
			
		if(PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive)
			return;
		if(PropertyName == GET_MEMBER_NAME_CHECKED(AWaterFall, SplineCount))
		{
			//ReGenerateSplineAfterResample();
			//ReGenerateSplineAfterResampleWithNumber();
			UpdateEmitterPoints();
		}

		else if (PropertyName == GET_MEMBER_NAME_CHECKED(AWaterFall, Percent) || PropertyName == GET_MEMBER_NAME_CHECKED(AWaterFall, CrossYAxisDistance))
		{
			FitterSplines(Percent, CrossYAxisDistance, NULL);
		}
		else if(PropertyName == GET_MEMBER_NAME_CHECKED(AWaterFall, RestLength))
		{
			ReGenerateSplineAfterResample();
		}
		else if(PropertyName == GET_MEMBER_NAME_CHECKED(FClusterWeight, Distance)
			||PropertyName == GET_MEMBER_NAME_CHECKED(FClusterWeight, SplineLength)
			||PropertyName == GET_MEMBER_NAME_CHECKED(FClusterWeight, Curvature)
			||PropertyName == GET_MEMBER_NAME_CHECKED(FClusterWeight, ClusterThreshold))
			
		{
			ClusterSplines();
		}
	}
}

void AWaterFall::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if(EmitterSpline->bSplineHasBeenEdited)
	{
		UpdateEmitterPoints();

	}

}


// Called every frame
void AWaterFall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
}

