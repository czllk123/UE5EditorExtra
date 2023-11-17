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
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "NiagaraDataInterfaceExport.h"
#include "SplineProcessor.h"
#include "NiagaraEvents.h"
#include "WaterFall.generated.h"


FORCEINLINE uint32 hash_bit_rotate(uint32 x, uint32 k)
{
	return (x << k) | (x >> (32 - k));
}

FORCEINLINE void hash_bit_final(uint32 &a, uint32 &b, uint32 &c)
{
	c ^= b;
	c -= hash_bit_rotate(b, 14);
	a ^= c;
	a -= hash_bit_rotate(c, 11);
	b ^= a;
	b -= hash_bit_rotate(a, 25);
	c ^= b;
	c -= hash_bit_rotate(b, 16);
	a ^= c;
	a -= hash_bit_rotate(c, 4);
	b ^= a;
	b -= hash_bit_rotate(a, 14);
	c ^= b;
	c -= hash_bit_rotate(b, 24);
}

FORCEINLINE uint32 GenHash(uint32 kx, uint32 ky)
{
	uint32 a, b, c;
	a = b = c = 0xdeadbeef + (2 << 2) + 13;

	b += ky;
	a += kx;
	hash_bit_final(a, b, c);

	return c;
}

FORCEINLINE uint32 GenHash(uint32 kx, uint32 ky, uint32 kz)
{
	uint32 a, b, c;
	a = b = c = 0xdeadbeef + (3 << 2) + 13;

	c += kz;
	b += ky;
	a += kx;
	hash_bit_final(a, b, c);

	return c;
}

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

USTRUCT(BlueprintType)
struct FEmitterAttributes
{
	GENERATED_BODY()
	TArray<FVector> Locations;
	TArray<FVector> Velocities;
	
	const TArray<FVector>& GetLocations() const { return Locations; }
	const TArray<FVector>& GetVelocities() const { return Velocities; }
};


USTRUCT(BlueprintType)
struct FEmitterPoints
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite)
	FVector Location;

	UPROPERTY(BlueprintReadWrite)
	FVector Normal;

	UPROPERTY(BlueprintReadWrite)
	FVector Velocity;
	
	FEmitterPoints()
		: Location(FVector::ZeroVector), Normal(FVector::ZeroVector), Velocity(FVector::ZeroVector)
	{}
	
};


UCLASS(hidecategories=(Tags, AssetUserData, Rendering, Physics,
	Activation, Collision, Cooking, HLOD, Networking, Input, LOD, Actor,Replication, Navigation, PathTracing, DataLayers, WorldPartition))
class LINKEXTRA_API AWaterFall : public AActor
{
	GENERATED_BODY()
	friend class FWaterFallCustomization;


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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WaterFall")
	UInstancedStaticMeshComponent* InstancedStaticMeshComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WaterFall")
	UBoxComponent* KillBox;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WaterFall")
	USplineComponent* EmitterSpline;
	const USplineComponent* GetEmitterSpline() const  {return EmitterSpline;}

	UPROPERTY(Category="Simulate",BlueprintReadWrite)
	bool bSimulateValid = true;

	UPROPERTY(EditAnywhere, Category = "Spline Clustering")
	USplineProcessor* SplineProcessorInstance; 
	
	//要获取的粒子的信息
	TArray<FName> ParticlesVariables = {"UniqueID","Position",  "Velocity", "Age"};

	FNiagaraSystemInstance* StoreSystemInstance;
	
	EWaterFallButtonState GetSimulateStateValue(){ return SimulateState;}



	/////////////////////////////////////////////DetailParameters//////////////////////////////////////

	UPROPERTY(EditAnywhere, Category = "WaterFall|EmitterSource", DisplayName="是否贴地")
	bool bSnapToGround = false;

	UPROPERTY(EditAnywhere, Category = "WaterFall|EmitterSource", DisplayName="随机数")
	int32 Seed = 666;

	UPROPERTY(EditAnywhere, Category= "WaterFall|EmitterSource", DisplayName="坡度剔除")
	FVector2D SlopeRange = {0, 90};
	
	UPROPERTY(EditAnywhere, Category = "WaterFall|EmitterSource", DisplayName="粒子模拟数量", BlueprintReadWrite, meta = (ClampMin = " 1"), meta = (ClampMax = "200"))
	int32 SplineCount = 10;
	
	UPROPERTY(EditAnywhere, Category = "WaterFall|Simulation", DisplayName="初始位置偏移")
	float Disturb = 0.0f;
	
	UPROPERTY(EditAnywhere, Category = "WaterFall|Simulation", DisplayName="粒子生命周期")
	float ParticleLife = 7.0f;
	


	//瀑布SplineMesh引用的StaticMesh
	UPROPERTY(EditAnywhere, Category = "WaterFall|Mesh", DisplayName="输入生成瀑布的静态网格", meta = (ClampMin = " 1"), meta = (ClampMax = "20"))
	UStaticMesh* WaterFallMesh;
	
	//获取粒子buffer的时间间隔
	UPROPERTY(EditAnywhere, Category = "WaterFall|Spline", BlueprintReadWrite, meta = (ClampMin = " 0.1"), meta = (ClampMax = "2"))
	float GetDataBufferRate = 0.1f;
	
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

	//分簇
	UPROPERTY(EditAnywhere, Category = "WaterFall|Cluster", DisplayName="分簇参数")
	FClusterWeight ClusterParameters;
	
	
	//资产名称
	UPROPERTY(EditAnywhere, Category = "WaterFall|Save",DisplayName="资产名称")
	FString MeshName = "WaterFallMesh";

	//保存路径
	UPROPERTY(EditAnywhere, Category = "WaterFall|Save",DisplayName="保存路径")
	FString SavePath = "/Game/BP/";

	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable,Category="WaterFall|Emitter", DisplayName="Spline发射源")
	void ComputeEmitterPoints(USplineComponent* EmitterSplineComponent, int32 EmitterPointsCount, bool bSnapToGrid = false);

	UFUNCTION(BlueprintCallable,Category="WaterFall|Emitter", DisplayName="Spline发射源")
	void UpdateEmitterPoints();

	
	UFUNCTION(BlueprintCallable,Category="WaterFall|Spline", DisplayName="开始模拟")
	void StartSimulation();

	UFUNCTION(BlueprintCallable,Category="WaterFall|Spline", DisplayName="停止模拟")
	void StopSimulation();
	
	FTimerHandle SimulationTimerHandle;

	UFUNCTION(BlueprintCallable,CallInEditor, Category ="WaterFall")
	void ResetParameters();
	
	UFUNCTION(BlueprintCallable,Category="WaterFall", DisplayName="收集粒子Buffer")
	void CollectionParticleDataBuffer();

	// 声明绑定Niagara事件的函数
	void BindNiagaraEvents(UNiagaraComponent* NiagaraComponent);

	// 声明处理Niagara碰撞事件的函数
	UFUNCTION()
	void OnCollisionEvent(const FNiagaraCollisionEventPayload& CollisionEventPayload);
	

	UFUNCTION(BlueprintCallable,Category="WaterFall|Spline", DisplayName="生成瀑布面片")
	void GenerateWaterFallSpline();

	UFUNCTION(BlueprintCallable,CallInEditor,Category="WaterFall", DisplayName="生成SplineMesh")
	void GenerateSplineMesh();
	
	UFUNCTION(BlueprintCallable,Category="WaterFall", DisplayName="生成Spline曲线")
	void UpdateSplineComponent(int32 ParticleID, FVector ParticlePosition);
	
	UFUNCTION(BlueprintCallable,Category="WaterFall", DisplayName="生成SplineMesh")
	TArray<USplineMeshComponent*> UpdateSplineMeshComponent(USplineComponent* Spline);

	UFUNCTION(BlueprintCallable,CallInEditor, Category="WaterFall", DisplayName="清理资源")
	void ClearAllResource();

	UFUNCTION(BlueprintCallable,CallInEditor, Category="WaterFall", DisplayName="清理所有Mesh")
	void ClearAllSplineMesh();

	UFUNCTION(Category="Mesh",DisplayName="销毁StaticMeshActor")
	void DestroyWaterFallMeshActor();
	
	UFUNCTION(BlueprintCallable, Category="WaterFall", DisplayName="重采样SplineTransform")
	static TArray<FVector> ResampleSplinePoints( const USplineComponent* InSpline, float ResetLength, float SplineLength);

	UFUNCTION(BlueprintCallable, Category="WaterFall", DisplayName="重采样Spline")
	static TArray<FVector> ResampleSplinePointsWithNumber(const USplineComponent* InSpline, int32 SampleNum);

	UFUNCTION(BlueprintCallable,CallInEditor, Category="WaterFall", DisplayName="重新生成重采样后的曲线")
	void ReGenerateSplineAfterResample();

	UFUNCTION(BlueprintCallable,CallInEditor, Category="WaterFall", DisplayName="重新生成重采样后的曲线")
	void ReGenerateSplineAfterResampleWithNumber();

	UFUNCTION(BlueprintCallable,CallInEditor, Category="WaterFall", DisplayName="曲线分簇")
	void ClusterSplines();
	
	
	UFUNCTION(BlueprintCallable,CallInEditor, Category="WaterFall", DisplayName="SplineMeshToStaticMesh")
	UStaticMesh* RebuildStaticMeshFromSplineMesh();
	
	static void FillMeshDescription(FMeshDescription& MeshDescription, const TArray<FVector3f>& Positions, const TArray<FVector3f>& Normals, TArray<FVector2f>& UVs,  const TArray<int32>& Triangles);
	
	UFUNCTION(BlueprintCallable,CallInEditor, Category="WaterFall", DisplayName="生成新的StaticMesh")
	void RebuildWaterFallMesh();

	UFUNCTION(Category = "WaterFall|Save", DisplayName="保存StaticMesh到磁盘")
	static UStaticMesh* SaveAssetToDisk(const UStaticMesh* InStaticMesh,  const FString& StaticMeshName, const FString& SaveRelativePath);

	//偏移SplineMesh的UV,让UV可以连续，而不是重复，可能还需要旋转90度，来适应shader
	UFUNCTION(Category= "WaterFall|Spline", DisplayName="计算SplinMesh的UV偏移")
	FVector2f CalculateUVOffsetBasedOnSpline(const USplineComponent* SplineComponent,
	const USplineMeshComponent* CurrentSplineMeshComponent,
	const TArray<USplineMeshComponent*>& AllSplineMeshComponents, float SegmentLength);
	

	//virtual void PerformAction(FNiagaraEmitterInstance& OwningSim, const FNiagaraEventReceiverProperties& OwningEventReceiver) override;

	
#if WITH_EDITOR
	DECLARE_EVENT(AWaterFall, FOnSplineDataChanged);
	virtual void PostEditUndo() override;
	
	//监视特定参数被更改后调用调用相应的函数更新结果
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void OnConstruction(const FTransform& Transform) override;
#endif
	
private:
#if WITH_EDITOR
	FOnSplineDataChanged SplineDataChangedEvent;
#endif
	
	//用Spline撒的点作为发射源
	TArray<FEmitterPoints> EmitterPoints;

	//用Spline撒的点作为发射源
	FEmitterAttributes SourceEmitterAttributes;

	//在场景中被选择的Spline粒子发射器
	USplineComponent* SelectedSplineComponent = nullptr;
	
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
	//TArray<USplineMeshComponent*> CachedSplineMeshComponents;
	

	//存储每条Spline上对应的所有SplineMeshComponent, 重建StaticMesh用
	TMap<USplineComponent*, TArray<USplineMeshComponent*>> CachedSplineAndSplineMeshes;
	
	//存储Spline原始长度，Resample时候用，用于Resample后别的函数调用
	TMap<USplineComponent*, float> CachedSplineOriginalLengths;
	//复制一份Tmap,存储Spline原始长度，Resample时候用，每次计算样条点的时候都用原始长度算
	TMap<USplineComponent*, USplineComponent*> BackupSplineData;
	
	//返回一个Cluster包，里面包含cluster后的曲线和首尾宽度
	TArray<FCluster> ClustersToUse;
	
	// 储存所有发射器的粒子数据
	TArray<FParticleData> ParticleDataArray;

	// 最近一个段落的结束宽度
	float LastSegmentEndWidth = 0.0f;
	

	//重建StaticMesh的Actor,用来存储和销毁
	AStaticMeshActor* RebuildedStaticMeshActor = nullptr;

	//先在RebuildStaticMeshFromSplineMesh函数中设置StaticMesh的默认材质,再保存资产
	UMaterialInterface* DefaultMaterial = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, TEXT("/Game/BP/Materials/Checker_Mat.Checker_Mat")));
	
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	

};

