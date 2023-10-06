// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "NiagaraSystemSimulation.h"
#include "NiagaraSystemInstance.h"

#include "Components/BoxComponent.h"

#include "NiagaraDataSetDebugAccessor.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"

#include "MeshDescription.h"
#include "StaticMeshAttributes.h"


#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"


#include "WaterFall.generated.h"


UENUM()
enum EWaterFallButtonState
{
    Simulate    UMETA(DisplayName = "Simulate"),
    Stop     UMETA(DisplayName = "Stop"),
};

//建立结构体用来储存获取到的粒子信息，里面的每一项都要和下面ParticlesVariables相对应
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

	UPROPERTY(BlueprintReadOnly)
	float Age;

	FParticleData()
		: UniqueID(0), Position(FVector::ZeroVector), Velocity(FVector::ZeroVector), Age(0.0f)
	{
	}
};





UCLASS(hidecategories=(Tags, AssetUserData, Rendering, Physics,
	Activation, Collision, Cooking, HLOD, Networking, Input, LOD, Actor,Replication, Navigation, PathTracing, DataLayers, WorldPartition))
class LINKEXTRA_API AWaterFall : public AActor
{
	GENERATED_BODY()
	friend class WaterFallCustomization;


	typedef TSharedPtr<class FNiagaraDataSetReadback, ESPMode::ThreadSafe> FGpuDataSetPtr;
	struct FGpuEmitterCache
	{
		uint64					LastAccessedCycles;
		TArray<FGpuDataSetPtr>	CurrentEmitterData;
		TArray<FGpuDataSetPtr>	PendingEmitterData;
	};

public:
	// Sets default values for this actor's properties
	AWaterFall();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WaterFall")
	USceneComponent* SceneRoot;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WaterFall")
	UNiagaraComponent* Niagara;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WaterFall")
	UBoxComponent* BoxCollision;

	UPROPERTY(Category="Simulate",BlueprintReadWrite)
	bool bSimulateValid = true;


	
	//要获取的粒子的信息
	TArray<FName> ParticlesVariables = {"UniqueID","Position",  "Velocity", "Age"};

	FNiagaraSystemInstance* StoreSystemInstance;
	
	EWaterFallButtonState GetSimulateStateValue(){ return SimulateState;}

	//瀑布Mesh
	UPROPERTY(EditAnywhere, Category = "WaterFall|Mesh")
	UStaticMesh* WaterFallMesh;

	//生成瀑布面片数量
	UPROPERTY(EditAnywhere, Category = "WaterFall|Spline", BlueprintReadWrite, meta = (ClampMin = " 1"), meta = (ClampMax = "100"))
	int32 SplineCount = 10;

	//获取粒子buffer的时间间隔
	UPROPERTY(EditAnywhere, Category = "WaterFall|Spline", BlueprintReadWrite, meta = (ClampMin = " 0.25"), meta = (ClampMax = "2"))
	float GetDataBufferRate = 0.5f;
	
	//面片开始宽度
	UPROPERTY(EditAnywhere, Category = "WaterFall|Mesh", BlueprintReadWrite, meta = (ClampMin = " 0"), meta = (ClampMax = "5"))
	FVector2D StartWidthRange  = FVector2D(2,5);
	
	// 面片结束宽度范围
	UPROPERTY(EditAnywhere, Category = "WaterFall|Mesh", BlueprintReadWrite, meta = (ClampMin = "5", ClampMax = "10"))
	FVector2D EndWidthRange = FVector2D(5.0, 7.0);

	//Spline重采样间距
	UPROPERTY(EditAnywhere, Category = "WaterFall|Spline", BlueprintReadWrite, meta = (ClampMin = "1"), meta = (ClampMax = "10"))
	float  RestLength = 2;

	//Spline重采样点数量
	UPROPERTY(EditAnywhere, Category = "WaterFall|Spline", BlueprintReadWrite, meta = (ClampMin = "1"), meta = (ClampMax = "100"))
	int32 SampleNumber = 10;


	/////////////////////////////////////////////////StaticMesh//////////////////////////////////////////////////////////////////////////

	//将Spline上的SplineMesh转换成StaticMesh
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	
	UFUNCTION(BlueprintCallable,Category="WaterFall|Spline", DisplayName="开始模拟")
	void StartSimulation();

	UFUNCTION(BlueprintCallable,Category="WaterFall|Spline", DisplayName="停止模拟")
	void StopSimulation();
	
	FTimerHandle SimulationTimerHandle;

	UFUNCTION(BlueprintCallable,CallInEditor, Category ="WaterFall")
	void ResetParameters();
	
	UFUNCTION(BlueprintCallable,Category="WaterFall", DisplayName="收集粒子Buffer")
	void CollectionParticleDataBuffer();

	UFUNCTION(BlueprintCallable,Category="WaterFall|Spline", DisplayName="生成瀑布面片")
	void GenerateWaterFallSpline();

	UFUNCTION(BlueprintCallable,CallInEditor,Category="WaterFall", DisplayName="生成SplineMesh")
	void GenerateWaterFallMesh();
	
	UFUNCTION(BlueprintCallable,Category="WaterFall", DisplayName="生成Spline曲线")
	void UpdateSplineComponent(int32 ParticleID, FVector ParticlePosition);
	
	UFUNCTION(BlueprintCallable,Category="WaterFall", DisplayName="生成SplineMesh")
	void UpdateSplineMeshComponent(USplineComponent* Spline);

	UFUNCTION(BlueprintCallable,CallInEditor, Category="WaterFall", DisplayName="清理资源")
	void ClearAllResource();

	UFUNCTION(BlueprintCallable,CallInEditor, Category="WaterFall", DisplayName="清理所有Mesh")
	void ClearAllSplineMesh();

	UFUNCTION(BlueprintCallable, Category="WaterFall", DisplayName="重采样SplineTransform")
	TArray<FVector> ResampleSplinePoints(USplineComponent* InSpline, float ResetLength);

	UFUNCTION(BlueprintCallable, Category="WaterFall", DisplayName="重采样Spline")
	TArray<FVector> ResampleSplinePointsWithNumber(USplineComponent* InSpline, int32 SampleNum);

	UFUNCTION(BlueprintCallable,CallInEditor, Category="WaterFall", DisplayName="重新生成重采样后的曲线")
	void ReGenerateSplineAfterResample();

	UFUNCTION(BlueprintCallable,CallInEditor, Category="WaterFall", DisplayName="重新生成重采样后的曲线")
	void ReGenerateSplineAfterResampleWithNumber();

	UFUNCTION(BlueprintCallable,CallInEditor, Category="WaterFall", DisplayName="SplineMeshToStaticMesh")
	UStaticMesh* ConvertSplineMeshToStaticMesh(TArray<USplineMeshComponent* >InSplineMeshComponents);
	
	void FillMeshDescription(FMeshDescription& MeshDescription, const TArray<FVector3f>& Positions, const TArray<FVector3f>& Normals, TArray<FVector2f>& UVs,  const TArray<int32>& Triangles);

	UFUNCTION(BlueprintCallable,CallInEditor, Category="WaterFall", DisplayName="生成新的StaticMesh")
	void GenerateWaterMesh();
#if WITH_EDITOR
	DECLARE_EVENT(AWaterFall, FOnSplineDataChanged);
	virtual void PostEditUndo() override;
	//监视特定参数被更改后调用调用相应的函数更新结果
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
private:
#if WITH_EDITOR
	FOnSplineDataChanged SplineDataChangedEvent;
#endif

	
	/** Index of this instance in the system simulation. */
	int32 SystemInstanceIndex;

	//设置按钮默认值
	EWaterFallButtonState SimulateState = EWaterFallButtonState :: Simulate;
	
	TMap<FNiagaraSystemInstanceID, FGpuEmitterCache> GpuEmitterData;
	
	
	const FNiagaraDataSet* GetParticleDataSet(class FNiagaraSystemInstance* SystemInstance, class FNiagaraEmitterInstance* EmitterInstance, int32 iEmitter);
	
	USplineComponent* WaterFallSpline;
	FRandomStream RandomStream;

	//粒子ID和SplineComponent映射map
	TMap<int32, USplineComponent*> ParticleIDToSplineComponentMap;

	// 存储SplineMeshComponent的数组
	TArray<USplineMeshComponent*> CachedSplineMeshComponents;

	//存储每条Spline上对应的所有SplineMeshComponent, 重建StaticMesh用
	TMap<USplineComponent*, TArray<USplineMeshComponent*>> CachedSplineToSplineMesh;;
	
	//存储Spline原始长度，Resample时候用，每次计算样条点的时候都用原始长度算
	TMap<USplineComponent*, float> CachedSplineOriginalLengths;
	
	// 储存所有发射器的粒子数据
	TArray<FParticleData> ParticleDataArray;

	// 最近一个段落的结束宽度
	float LastSegmentEndWidth = 0.0f; 
	
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	

};

