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

	UPROPERTY()
	bool IsRaining;
	
	//要获取的粒子的信息
	TArray<FName> ParticlesVariables = {"UniqueID","Position",  "Velocity", "Age"};

	FNiagaraSystemInstance* StoreSystemInstance;
	
	EWaterFallButtonState GetSimulateStateValue(){ return SimulateState;}

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
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	
	UFUNCTION(BlueprintCallable,Category="WaterFall|Spline", DisplayName="开始模拟")
	void  StartSimulation();

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
	void UpdateSplineMeshComponent(int32 ParticleID);

	UFUNCTION(BlueprintCallable,CallInEditor, Category="WaterFall", DisplayName="清理资源")
	void ClearAllResource();

	UFUNCTION(BlueprintCallable,CallInEditor, Category="WaterFall", DisplayName="清理所有Mesh")
	void ClearAllSplineMesh();
private:
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
	
	// 储存所有发射器的粒子数据
	TArray<FParticleData> ParticleDataArray;

	// 最近一个段落的结束宽度
	float LastSegmentEndWidth = 0.0f; 
	
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	

};

