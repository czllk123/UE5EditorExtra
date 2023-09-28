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

#include "Selection.h"
#include "NiagaraSystem.h"
#include "Components/SplineMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
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
	ClearAllResource();
	
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
			NiagaraComponent->bAutoActivate=false;
			NiagaraComponent->Deactivate();
			NiagaraComponent->ReregisterComponent();
		}
	}
	UE_LOG(LogTemp, Warning ,TEXT("测试计时器是否结束！"))
	
	if(SimulationTimerHandle.IsValid())
		GetWorld()->GetTimerManager().ClearTimer(SimulationTimerHandle); 
	
}



void AWaterFall::CollectionParticleDataBuffer()
{
	UE_LOG(LogTemp, Warning ,TEXT(" Start GenerateSplineMesh！"))
	
	auto SystemSimulation = StoreSystemInstance->GetSystemSimulation();
	const bool bSystemSimulationValid = SystemSimulation.IsValid() && SystemSimulation->IsValid();
	//等待异步完成再去访问资源，否则触发崩溃
	if(bSystemSimulationValid)
	{
		SystemSimulation->WaitForInstancesTickComplete();
	}
	
	//TArray<FParticleData> ParticleDataArray; //储存所有发射器的粒子数据
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
	CachedSplineMeshComponents.Empty();
	//间隔时间内收集DataBuffer
	CollectionParticleDataBuffer();
	//绘制spline
	for(const FParticleData& Particle : ParticleDataArray)
	{
		UpdateSplineComponent(Particle.UniqueID, Particle.Position);
		
	}
}
void AWaterFall::GenerateWaterFallMesh()
{
	ClearAllSplineMesh();
	//暂且在这个地方调用,因为不需要每帧绘制，所以不能像spline那样
	for(auto& SplinePair : CachedSplineOriginalLengths)
	{
		USplineComponent* Spline = SplinePair.Key;
		UpdateSplineMeshComponent(Spline);
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
}


//TODO 后期考虑缓存一个spline,进行分簇筛选和重采样后再生成Mesh
void AWaterFall::UpdateSplineMeshComponent(USplineComponent* Spline)
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
		SplineMesh->RegisterComponent();
		SplineMesh->AttachToComponent(Spline, FAttachmentTransformRules::KeepRelativeTransform);  // 将Spline Mesh附加到Spline上
		SplineMesh->SetVisibility(true, true);
		//存储到内存，方便追踪和销毁
		CachedSplineMeshComponents.Add(SplineMesh);
	}
	
}


void AWaterFall::ClearAllResource()
{
	check(IsInGameThread());
	
	for(auto& ElementSpline: ParticleIDToSplineComponentMap)
	{
		USplineComponent* SplineComponent = ElementSpline.Value;
		if(SplineComponent)
		{
			SplineComponent->DestroyComponent();
		}
	}
	for(USplineMeshComponent* SplineMeshComponent : CachedSplineMeshComponents)
	{
		if(SplineMeshComponent)
		{
			SplineMeshComponent->DestroyComponent();
		}
		
	}
	//清空TMap
	ParticleIDToSplineComponentMap.Empty();
	CachedSplineMeshComponents.Empty();
	CachedSplineOriginalLengths.Empty();
	UE_LOG(LogTemp, Warning, TEXT("Cleared all Spline and SplineMesh components"));
}

void AWaterFall::ClearAllSplineMesh()
{
	check(IsInGameThread());
	for(USplineMeshComponent* SplineMeshComponent : CachedSplineMeshComponents)
	{
		if(SplineMeshComponent)
		{
			SplineMeshComponent->DestroyComponent();
		}
	}
	CachedSplineMeshComponents.Empty();
}

void AWaterFall::ReGenerateSplineAfterResample()
{
	

	for(auto& SplineLengthPair : CachedSplineOriginalLengths)
	{
		USplineComponent* InSpline = SplineLengthPair.Key;
		
		TArray<FVector> PerSplineLocation = ResampleSplinePoints(InSpline, RestLength);
		InSpline->ClearSplinePoints();
		InSpline->SetSplinePoints(PerSplineLocation, ESplineCoordinateSpace::World,true);
		InSpline->SetHiddenInGame(true);
		InSpline->SetMobility(EComponentMobility::Movable);
		InSpline->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepRelativeTransform);
		InSpline->SetVisibility(true, true);
		UE_LOG(LogTemp, Error, TEXT("============================================"));
		PerSplineLocation.Empty();
	}
}

void AWaterFall::ReGenerateSplineAfterResampleWithNumber()
{
	for(auto& SplineLengthPair : CachedSplineOriginalLengths)
	{
		USplineComponent* InSpline = SplineLengthPair.Key;
		
		TArray<FVector> PerSplineLocation = ResampleSplinePointsWithNumber(InSpline, SampleNumber);
		InSpline->ClearSplinePoints();
		InSpline->SetSplinePoints(PerSplineLocation, ESplineCoordinateSpace::World,true);
		InSpline->SetHiddenInGame(true);
		InSpline->SetMobility(EComponentMobility::Movable);
		InSpline->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepRelativeTransform);
		InSpline->SetVisibility(true, true);
		UE_LOG(LogTemp, Error, TEXT("============================================"));
		PerSplineLocation.Empty();
	}
}



TArray<FVector> AWaterFall::ResampleSplinePointsWithNumber(USplineComponent* InSpline, int32 SampleNum)
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
			Result.Add(Transform.GetLocation());
		}
	}
	return Result;
}


TArray<FVector> AWaterFall::ResampleSplinePoints(USplineComponent* InSpline, float ResetLength)
{
	ResetLength *= 100.0f; // Convert meters to centimeters

	TArray<FVector> Result;

	if (!InSpline || !CachedSplineOriginalLengths.Contains(InSpline) || ResetLength <= SMALL_NUMBER)
	{
		return Result; 
	}

	bool bIsLoop = InSpline->IsClosedLoop();
	float Duration = InSpline->Duration;

	const float OriginalSplineLength = CachedSplineOriginalLengths[InSpline];
	UE_LOG(LogTemp, Warning, TEXT("Length : %f "), OriginalSplineLength);
	int32 Segments = FMath::FloorToInt(OriginalSplineLength / ResetLength);
	float DiffLength = OriginalSplineLength - ResetLength * Segments; // Remaining length

	
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

void AWaterFall::FillMeshDescription(FMeshDescription& MeshDescription, FVector3f& Position, FVector3f& Normal,TArrayView<FVector2f>& UV, FVector& Triangles)
{
	FStaticMeshAttributes Attributes(MeshDescription);
	Attributes.Register();

	//Reserve Stuff
	MeshDescription.ReserveNewVertices(Position.Size());
	MeshDescription.ReserveNewVertexInstances(Position.Size());

	MeshDescription.CreatePolygonGroup();
	MeshDescription.ReserveNewPolygons(Triangles.Size() / 3);
	MeshDescription.ReserveNewTriangles(Triangles.Size() / 3);
	MeshDescription.ReserveNewEdges(Triangles.Size());

	for(int v = 0; v < Position.Size(); v++)
	{
		MeshDescription.CreateVertex();
		MeshDescription.CreateVertexInstance(v);
	}

	//Fill Triangles Data
	for(int i = 0; i < Triangles.Size(); i+= 3)
	{
		MeshDescription.CreateTriangle(0, { Triangles[i],Triangles[i + 1] ,Triangles[i + 2] });
	}

	//Fill Vertex Data
	auto Positions = MeshDescription.GetVertexPositions().GetRawArray();
	auto UVs = MeshDescription.VertexInstanceAttributes().GetAttributesRef<FVector2f>(MeshAttribute::VertexInstance::TextureCoordinate).GetRawArray();
	auto Normals = MeshDescription.VertexInstanceAttributes().GetAttributesRef<FVector3f>(MeshAttribute::VertexInstance::Normal).GetRawArray();

	for(int v = 0; v < Position.Size(); ++v)
	{
		UVs[v] = UV[v];
		Positions[v] = Position[v];
		Normals[v] = Normal[v];
	}
}


UStaticMesh* AWaterFall::ConvertSplineMeshToStaticMesh(USplineMeshComponent* SplineMesh)
{
	if(!SplineMesh)return nullptr;

	UStaticMesh* SourceMesh = SplineMesh->GetStaticMesh();
	if(!SourceMesh)return nullptr;
	
	// 获取SplineMesh的原始数据
	FStaticMeshSourceModel& SrcModel = SourceMesh->GetSourceModel(0);
	FStaticMeshSourceModel NewModel;

	//目前只处理LOD0
	const FStaticMeshLODResources& SrcLOD = SourceMesh->GetLODForExport(0);

	// Vertices and UVs
	TArray<FVector3f> Vertices;
	TArray<FVector2f> UVs;
	for(int32 i = 0; i < SrcLOD.VertexBuffers.PositionVertexBuffer.GetNumVertices(); i++)
	{
		Vertices.Add(SrcLOD.VertexBuffers.PositionVertexBuffer.VertexPosition(i));
		UVs.Add(SrcLOD.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(i, 0));
	}

	// Triangles
	TArray<int32> Triangles;
	FIndexArrayView Indices = SrcLOD.IndexBuffer.GetArrayView();
	for(int32 i = 0; i < Indices.Num(); i++)
	{
		Triangles.Add(Indices[i]);
	}


	//创建一个新的MeshDescription
	FMeshDescription MeshDescription;
	FillMeshDescription(MeshDescription, Vertices, Vertices, UVs, Triangles);

	// 创建一个新的StaticMesh实例
	UStaticMesh* NewStaticMesh = NewObject<UStaticMesh>(this, FName("WaterFallMesh"));
	NewStaticMesh->SetRenderData(MakeUnique<FStaticMeshRenderData>());
	NewStaticMesh->CreateBodySetup();
	NewStaticMesh->bAllowCPUAccess = true;
	NewStaticMesh->GetBodySetup()->CollisionTraceFlag = ECollisionTraceFlag::CTF_UseComplexAsSimple;
	UStaticMesh::FBuildMeshDescriptionsParams MeshDescriptionParams;
	NewStaticMesh->BuildFromMeshDescriptions({ &MeshDescription }, MeshDescriptionParams);

	return NewStaticMesh;
	/*
	int32 VertexIndex = 0;
	TArrayView<FVertexInstanceID> VertexInstanceIDs;
	for(const FVector3f& Vertex : Vertices)
	{
		const FVertexInstanceID VertexInstanceID = MeshDescription.CreateVertexInstance(MeshDescription.CreateVertex());
		VertexInstanceIDs
		Attributes.GetVertexInstanceUVs().Set(VertexInstanceID, UV0[VertexIndex]);
		VertexIndex++;
	}

	for (int32 i = 0; i < Triangles.Num(); i += 3)
	{
		FTriangleID TriangleID = MeshDescription.CreateTriangle(
		VertexInstanceIDs[Triangles[i]],
		VertexInstanceIDs[Triangles[i + 1]],
		VertexInstanceIDs[Triangles[i + 2]]
		);
	}


	NewModel.BuildSettings.bRecomputeNormals = true;
	NewModel.BuildSettings.bRecomputeTangents = true;
	NewModel.BuildSettings.bUseHighPrecisionTangentBasis = true;
	NewModel.BuildSettings.bUseFullPrecisionUVs = true;
	NewModel.BuildSettings.bGenerateLightmapUVs = true;
	NewModel.ScreenSize = SrcModel.ScreenSize;

	NewStaticMesh->AddSourceModel(NewModel);
	NewStaticMesh->CreateMeshDescription(0, MoveTemp(MeshDescription));
	NewStaticMesh->PostEditChange();
*/

	
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
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(AWaterFall, StartWidthRange.XY) || PropertyName == GET_MEMBER_NAME_CHECKED(AWaterFall, EndWidthRange.XY))
		{
			GenerateWaterFallMesh();
		}
		else if(PropertyName == GET_MEMBER_NAME_CHECKED(AWaterFall, RestLength))
		{
			ReGenerateSplineAfterResample();
		}
	}
}


// Called every frame
void AWaterFall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
}

