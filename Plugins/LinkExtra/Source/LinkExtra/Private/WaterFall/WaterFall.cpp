// Fill out your copyright notice in the Description page of Project Settings.


#include "WaterFall.h"

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
			//NiagaraComponent->bAutoActivate=true;
			NiagaraComponent->Activate(false);
			NiagaraComponent->ReregisterComponent();

			NiagaraComponent->SetNiagaraVariableInt(TEXT("ParticleSpawnCount"),SplineCount);
			
			//对EmitterSpline重采样，使用重采样后的位置作为粒子发射源
			const TArray<FVector> SampleEmitterPoints = ResampleSplinePointsWithNumber(SplineComponent, SplineCount);
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(NiagaraComponent, TEXT("SplinePointsArray"), SampleEmitterPoints);

			//获取场景中KillBox的大小和位置传入Niagara
			FVector KillBoxLocation = BoxComponent->GetComponentLocation();
			FVector KillBoxBounds = BoxComponent->GetScaledBoxExtent()*2.0f;
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
			NiagaraComponent->ReregisterComponent();
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

	TArray<USplineComponent*> SplineComponents;
	CachedSplineOriginalLengths.GetKeys(SplineComponents);
	
	USplineProcessor* TempProcessor = NewObject<USplineProcessor>();
	TempProcessor->WeightData = this->ClusterParameters;
	TempProcessor->ProcessSplines(SplineComponents);
	ClustersToUse = TempProcessor->GetClusters();
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

	}
	else
	{
		// 如果没找到，创建一个新的SplineComponent并添加到TMap
		WaterFallSpline = NewObject<USplineComponent>(this, USplineComponent::StaticClass()); 
		WaterFallSpline->SetHiddenInGame(true);
		WaterFallSpline->SetMobility(EComponentMobility::Movable);
		WaterFallSpline->RegisterComponent();  // 注册组件，使其成为场景的一部分
		WaterFallSpline->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepRelativeTransform);
		WaterFallSpline->SetVisibility(true, true);
		WaterFallSpline->ClearSplinePoints(true);
		WaterFallSpline->AddSplinePointAtIndex(ParticlePosition,0, ESplineCoordinateSpace::World,true );

		//这个Map是粒子ID和SplineComponent的一个映射，判断接下来收集到的Buffer该绘制那根曲线
		ParticleIDToSplineComponentMap.Add(ParticleID, WaterFallSpline);

#if WITH_EDITORONLY_DATA
		const FLinearColor RandomColor(RandomStream.FRand(), RandomStream.FRand(), RandomStream.FRand(), 1.0f);
		WaterFallSpline->EditorUnselectedSplineSegmentColor = RandomColor;
		//UE_LOG(LogTemp,Warning,TEXT("Color : %s"),*RandomColor.ToString());
		WaterFallSpline->EditorSelectedSplineSegmentColor=(FLinearColor(1.0f, 0.0f, 0.0f, 1.0f));
#endif
		
	}

	//储存SplineComponent和长度 Resample用
	CachedSplineOriginalLengths.Add(WaterFallSpline, WaterFallSpline->GetSplineLength());
	BackupSplineData.Add(WaterFallSpline, WaterFallSpline);

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
		if(WaterFallMesh == nullptr)
		{
			SplineMesh->SetStaticMesh(Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(),nullptr,TEXT("/Engine/EditorLandscapeResources/SplineEditorMesh"))));
		}
		else
		{
			SplineMesh->SetStaticMesh(WaterFallMesh);

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
		SplineMesh->AttachToComponent(Spline, FAttachmentTransformRules::KeepRelativeTransform);  // 将Spline Mesh附加到Spline上
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
	for(const auto& ElementSpline: ParticleIDToSplineComponentMap)
	{
		USplineComponent* SplineComponent = ElementSpline.Value;
		if(SplineComponent)
		{
			SplineComponent->DestroyComponent();
		}
	}

	//清理SplineMesh和TMap
	ClearAllSplineMesh();
	CachedSplineAndSplineMeshes.Empty();
	
	//清空Spline相关的TMap
	ParticleIDToSplineComponentMap.Empty();
	CachedSplineOriginalLengths.Empty();
	BackupSplineData.Empty();
	UE_LOG(LogTemp, Warning, TEXT("Cleared all Spline and SplineMesh components"));
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
		InSpline->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepRelativeTransform);
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
		InSpline->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepRelativeTransform);
		InSpline->SetVisibility(true, true);
		PerSplineLocation.Empty();
		UE_LOG(LogTemp, Warning, TEXT("After Resampling - InSpline Segments: %d , BackupSpline Segments: %d"), InSpline->GetNumberOfSplineSegments(), BackupSpline->GetNumberOfSplineSegments());
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


	UE_LOG(LogTemp, Warning, TEXT("Length : %f "), SplineLength);
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
		FillMeshDescription(*MeshDescription, LODCombinedVertices[LODIndex],
			LODCombinedNormals[LODIndex],
			LODCombinedUVs[LODIndex],
			LODCombinedTriangles[LODIndex]);
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
		if(WaterFallMesh->GetNumSourceModels() > LODIndex)
		{
			NewStaticMesh->GetSourceModel(LODIndex).ScreenSize = WaterFallMesh->GetSourceModel(LODIndex).ScreenSize;
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
		
		WaterFallMeshComponent->AttachToComponent(RebuildedStaticMeshActor->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
		
		WaterFallMeshComponent->SetStaticMesh(SavedStaticMesh);
		
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


void AWaterFall::ResetParameters()
{
	return;
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

void AWaterFall::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if(PropertyChangedEvent.Property != nullptr)
	{
		const FName PropertyName = PropertyChangedEvent.Property->GetFName();
		if(PropertyName == GET_MEMBER_NAME_CHECKED(AWaterFall, SampleNumber))
		{
			//ReGenerateSplineAfterResample();
			ReGenerateSplineAfterResampleWithNumber();
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(AWaterFall, StartWidthRange) || PropertyName == GET_MEMBER_NAME_CHECKED(AWaterFall, EndWidthRange))
		{
			GenerateSplineMesh();
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


// Called every frame
void AWaterFall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
}

