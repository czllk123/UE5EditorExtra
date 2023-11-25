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

	UPROPERTY(BlueprintReadOnly)
	float CollisionDelayTimer;

	FParticleData()
		: UniqueID(0), Position(FVector::ZeroVector), Velocity(FVector::ZeroVector), Age(0.0f), CollisionDelayTimer(0.0f)
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

//储存buffer数据
struct FLODData
{
	TArray<FVector3f> Vertices;
	TArray<FVector3f> Normals;
	TArray<FVector2f> UVs;
	TArray<int32> Triangles;
	int32 VertexCount; // 用于追踪当前LOD的顶点数量
};

UCLASS(hidecategories=(Tags, AssetUserData, Rendering, Physics,
	Activation, Collision, Cooking, HLOD, Networking, Input, LOD, Actor,Replication, Navigation, PathTracing, DataLayers, WorldPartition))
class LINKEXTRA_API AWaterFall : public AActor
{
	GENERATED_BODY()
	friend class FWaterFallCustomization;
	friend class USplineProcessor;

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

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "WaterFall")
	USceneComponent* SceneRoot;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "WaterFall")
	UNiagaraComponent* Niagara;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "WaterFall")
	UInstancedStaticMeshComponent* InstancedStaticMeshComponent;

	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "WaterFall")
	UBoxComponent* KillBox;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "WaterFall")
	USplineComponent* EmitterSpline;
	const USplineComponent* GetEmitterSpline() const  {return EmitterSpline;}

	UPROPERTY(Category="Simulate",BlueprintReadWrite)
	bool bSimulateValid = true;

	UPROPERTY(VisibleDefaultsOnly, Category = "Spline Clustering")
	USplineProcessor* SplineProcessorInstance; 
	
	//要获取的粒子的信息
	TArray<FName> ParticlesVariables = {"UniqueID","Position",  "Velocity", "Age", "CollisionDelayTimer"};

	FNiagaraSystemInstance* StoreSystemInstance;
	
	EWaterFallButtonState GetSimulateStateValue(){ return SimulateState;}



	/////////////////////////////////////////////DetailParameters//////////////////////////////////////

	// 资产名称
	UPROPERTY(EditAnywhere, Category = "SaveToDisk",DisplayName="资产名称")
	FString MeshName = "WaterFallMesh";

	//保存路径
	UPROPERTY(EditAnywhere, Category = "SaveToDisk",DisplayName="保存路径")
	FString SavePath = "/Game/BP/";

	
	UPROPERTY(EditDefaultsOnly, Category = "EmissionSourceSetup", DisplayName="是否贴地")
	bool bSnapToGround = false;

	UPROPERTY(EditAnywhere, Category = "EmissionSourceSetup", DisplayName="随机数")
	int32 Seed = 666;
	
	UPROPERTY(EditDefaultsOnly, Category = "EmissionSourceSetup", DisplayName="初始位置偏移")
	float Disturb = 0.0f;

	UPROPERTY(EditAnywhere, Category = "EmissionSourceSetup", DisplayName="发射粒子大小", meta = (ClampMin = " 0.2"), meta = (ClampMax = "2"))
	float ISMScale = 1.0f;
	

	UPROPERTY(EditDefaultsOnly, Category= "EmissionSourceSetup", DisplayName="坡度剔除")
	FVector2D SlopeRange = {0, 90};
	
	UPROPERTY(EditAnywhere, Category = "EmissionSourceSetup", DisplayName="粒子模拟数量", BlueprintReadWrite, meta = (ClampMin = " 1"), meta = (ClampMax = "200"))
	int32 SplineCount = 50;
	

	UPROPERTY(EditAnywhere, Category = "Simulation", DisplayName="粒子生命周期")
	float ParticleLife = 7.0f;

	UPROPERTY(EditAnywhere, Category = "Simulation", DisplayName="粒子初始速度大小")
	float ParticleVelocity = 1.0f;

	//获取粒子buffer的时间间隔
	UPROPERTY(EditAnywhere, Category = "Simulation", BlueprintReadWrite, meta = (ClampMin = " 0.1"), meta = (ClampMax = "2"), DisplayName="获取Buffer间隔时间")
	float GetDataBufferRate = 0.1f;
	

	//Spline重采样间距
	UPROPERTY(EditAnywhere, Category = "ResampleSpline", BlueprintReadWrite, meta = (ClampMin = "1"), meta = (ClampMax = "10"), DisplayName="重采样分段长度")
	float  RestLength = 2.0f;

	//Spline重采样点数量
	//UPROPERTY(EditAnywhere, Category = "WaterFall|Spline", BlueprintReadWrite, meta = (ClampMin = "1"), meta = (ClampMax = "100")，DisplayName="重采样点数量")
	int32 SampleNumber = 10;

	//筛选曲线
	UPROPERTY(EditAnywhere, Category = "FitterSpline", BlueprintReadWrite, meta = (ClampMin = "0"), meta = (ClampMax = "1"), DisplayName="筛选太短的Spline")
	float Percent = 0.5f;
	
	UPROPERTY(EditDefaultsOnly, Category = "FitterSpline", BlueprintReadWrite, meta = (ClampMin = "0"), meta = (ClampMax = "180"), DisplayName="与粒子发射方向的夹角")
	float AngleRange = 30.0f;
	
	UPROPERTY(EditAnywhere, Category = "FitterSpline", BlueprintReadWrite, meta = (ClampMin = "0"), meta = (ClampMax = "1"), DisplayName="筛选横跨Y轴的Spline")
	float CrossYAxisDistance = 0.5f;
	
	//分簇
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ClusterSpline", DisplayName="分簇数量")
	int32 ClusterNumber;
	
	UPROPERTY(EditAnywhere, Category = "ClusterSpline", DisplayName="分簇参数")
	FClusterWeight ClusterParameters;


	
	//面片开始宽度
	UPROPERTY(EditAnywhere, Category = "SplineMesh", BlueprintReadWrite, meta = (ClampMin = " 0"), meta = (ClampMax = "10"),DisplayName="首部宽度范围")
	FVector2D StartWidthRange  = FVector2D(2,5);
	
	// 面片结束宽度范围
	UPROPERTY(EditAnywhere, Category = "SplineMesh", BlueprintReadWrite, meta = (ClampMin = "5", ClampMax = "20"),DisplayName="尾部宽度范围")
	FVector2D EndWidthRange = FVector2D(5.0, 7.0);

	//瀑布SplineMesh引用的StaticMesh
	UPROPERTY(EditAnywhere, Category = "StaticMesh", DisplayName="输入生成瀑布的静态网格")
	UStaticMesh* SourceStaticMesh;
	

	//粒子
	UPROPERTY(EditAnywhere, Category = "ParticleSettings",DisplayName="是否生成粒子")
	bool bSpawnParticles = true;

	UPROPERTY(EditAnywhere, Category = "ParticleSettings", meta = (ClampMin = "0.2", ClampMax = "2"),DisplayName="例子缩放比例")
	FVector2D ParticleScaleRange = FVector2D(0.5, 1.5); // 粒子的最小缩放比例
	
	UPROPERTY(EditAnywhere, Category = "ParticleSettings",DisplayName="粒子之间的最小距离")
	float MinDistanceBetweenParticles = 1000.0f; // 粒子之间的最小距离

	//粒子引用
	UPROPERTY(EditDefaultsOnly, Category = "ParticleSettings", DisplayName="中上部粒子资产引用")
	TObjectPtr<UParticleSystem> CenterParticle;

	UPROPERTY(EditAnywhere, Category = "ParticleSettings", DisplayName="底部粒子资产引用")
	TObjectPtr<UParticleSystem> BottomParticle;
	
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

	UFUNCTION(BlueprintCallable, Category ="RestParameters")
	void ResetParameters();
	
	UFUNCTION(BlueprintCallable,Category="WaterFall", DisplayName="收集粒子Buffer")
	void CollectionParticleDataBuffer();

	// 声明绑定Niagara事件的函数
	//void BindNiagaraEvents(UNiagaraComponent* NiagaraComponent);

	// 声明处理Niagara碰撞事件的函数
	//UFUNCTION()
	//void OnCollisionEvent(const FNiagaraCollisionEventPayload& CollisionEventPayload);
	

	UFUNCTION(BlueprintCallable,Category="WaterFall|Spline", DisplayName="生成瀑布面片")
	void GenerateWaterFallSpline();

	UFUNCTION(BlueprintCallable,Category="WaterFall", DisplayName="生成SplineMesh")
	void GenerateSplineMesh();
	
	UFUNCTION(BlueprintCallable,Category="WaterFall", DisplayName="生成Spline曲线")
	void UpdateSplineComponent(int32 ParticleID, FVector ParticlePosition);
	
	UFUNCTION(BlueprintCallable,Category="WaterFall", DisplayName="生成SplineMesh")
	TArray<USplineMeshComponent*> UpdateSplineMeshComponent(USplineComponent* Spline);

	UFUNCTION(BlueprintCallable, Category="WaterFall|Simulation", DisplayName="清理资源")
	void ClearAllResource();

	UFUNCTION(BlueprintCallable, Category="WaterFall", DisplayName="清理所有Mesh")
	void ClearAllSplineMesh();

	UFUNCTION(Category="Mesh",DisplayName="销毁StaticMeshActor")
	void DestroyWaterFallMeshActor();
	
	UFUNCTION( Category="WaterFall", DisplayName="重采样Spline")
	static TArray<FVector> ResampleSplinePoints( const USplineComponent* InSpline, float ResetLength, float SplineLength);

	UFUNCTION( Category="WaterFall", DisplayName="重采样Spline")
	static TArray<FVector> ResampleSplinePointsWithNumber(const USplineComponent* InSpline, int32 SampleNum);

	UFUNCTION(BlueprintCallable, Category="WaterFall", DisplayName="重新生成重采样后的曲线")
	void ReGenerateSplineAfterResample();

	UFUNCTION(BlueprintCallable, Category="WaterFall", DisplayName="重新生成重采样后的曲线")
	void ReGenerateSplineAfterResampleWithNumber();
	
	UFUNCTION(BlueprintCallable, Category="WaterFall", DisplayName="计算两个向量之间的夹角")
	static float GetAngleBetweenVectors(const FVector& A, const FVector& B);
	
	UFUNCTION(BlueprintCallable,Category="WaterFall", DisplayName="筛选曲线")
	void FitterSplines(const float LengthPercent, const float DistancePercent, const float& Angle);
	
	UFUNCTION(BlueprintCallable,Category="WaterFall", DisplayName="曲线分簇")
	void ClusterSplines();
	
	
	UFUNCTION(BlueprintCallable,CallInEditor, Category="WaterFall", DisplayName="SplineMeshToStaticMesh")
	UStaticMesh* RebuildStaticMeshFromSplineMesh();
	
	static void FillMeshDescription(FMeshDescription& MeshDescription, const TArray<FVector3f>& Positions, const TArray<FVector3f>& Normals, TArray<FVector2f>& UVs,  const TArray<int32>& Triangles);
	
	UFUNCTION(BlueprintCallable,Category="WaterFall", DisplayName="生成新的StaticMesh")
	void RebuildWaterFallMesh();

	UFUNCTION(Category = "WaterFall|Save", DisplayName="保存StaticMesh到磁盘")
	static UStaticMesh* SaveAssetToDisk(const UStaticMesh* InStaticMesh,  const FString& StaticMeshName, const FString& SaveRelativePath);

	//偏移SplineMesh的UV,让UV可以连续，而不是重复，可能还需要旋转90度，来适应shader
	UFUNCTION(Category= "WaterFall|Spline", DisplayName="计算SplinMesh的UV偏移")
	FVector2f CalculateUVOffsetBasedOnSpline(const USplineComponent* SplineComponent,
	const USplineMeshComponent* CurrentSplineMeshComponent,
	const TArray<USplineMeshComponent*>& AllSplineMeshComponents, float SegmentLength);


	void SpawnParticles();
	void ClearSpawnedParticles();
	

	// 计算变换后的顶点位置和法线的辅助函数
	static void CalculateDeformedPositionAndNormal(const USplineMeshComponent* SplineMesh, const FVector3f& LocalPosition, const FVector3f& LocalNormal, FVector& OutPosition, FVector& OutNormal);

	// 收集来自SplineMeshes的LOD数据
	void CollectLODDataFromSplineMeshes(TMap<int32, FLODData>& LODDataMap);

	// 为每个LOD创建MeshDescription
	void CreateMeshDescriptionsForLODs(const TMap<int32, FLODData>& LODDataMap, TArray<FMeshDescription>& OutMeshDescriptions);

	// 从MeshDescriptions创建StaticMesh
	UStaticMesh* CreateStaticMeshFromLODMeshDescriptions(const TArray<FMeshDescription>& MeshDescriptions);
	
	void SpawnStaticMesh(UStaticMesh* StaticMesh);

	UFUNCTION(CallInEditor, Category="WaterFall")
	void BuildStaticMeshFromSplineMesh();

	
	

	
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
	//USplineComponent* SelectedSplineComponent = nullptr;
	
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

	//筛选后的Spline
	TArray<USplineComponent*>VaildSplines;
	

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

	//生成的瀑布StaticMesh
	UStaticMesh* GeneratedWaterFallMesh = nullptr;

	//先在RebuildStaticMeshFromSplineMesh函数中设置StaticMesh的默认材质,再保存资产
	UMaterialInterface* DefaultMaterial = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, TEXT("/Game/BP/Materials/Checker_Mat.Checker_Mat")));

	UPROPERTY()
	TArray<UParticleSystemComponent*> SpawnedParticles;
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	

};

