#pragma once
#include "CoreMinimal.h"
#include "Components/SplineComponent.h"

#include "SplineProcessor.generated.h"


/*
struct QuadTreeNode
{
	FBox2D Bounds;
	TArray<USplineComponent*> Splines;
	QuadTreeNode* Children[4];

	QuadTreeNode(const FBox2D& Bounds) : Bounds(Bounds) {
		for (int i = 0; i < 4; ++i) {
			Children[i] = nullptr;
		}
	}
};

*/


// 簇的定义
USTRUCT()
struct FCluster {
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere)
	TArray<int32> SplineIndices;  // 该簇中包含的spline的索引
	UPROPERTY(EditAnywhere)
	FVector Center;               // 簇的中心点位置，初始化为spline的起点
	UPROPERTY(EditAnywhere)
	FVector2D ClusterStartWidth; // 簇的开始宽度，用来是设置SplineMesh的开始宽度
	UPROPERTY(EditAnywhere)
	FVector2D ClusterEndWidth; // 簇结束的宽度，用来是设置SplineMesh的结束宽度
	USplineComponent* RepresentativeSpline = nullptr;
	FCluster()
		:Center(FVector::ZeroVector),
		ClusterStartWidth(FVector2d::ZeroVector),
		ClusterEndWidth(FVector2d::ZeroVector)
	{}
};

// 影响分簇的因素定义
struct FSplineData {
	FVector StartPosition;
	FVector EndPosition;
	float Length;
	float Curvature;
	
	FSplineData()
		:StartPosition(FVector::ZeroVector),
		EndPosition(FVector::ZeroVector),
		Length(0.0f),
		Curvature(0.0f)
	{}
	
};


USTRUCT()
struct FClusterWeight
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "Spline|Cluster", DisplayName="分簇距离")
	float Distance = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Spline|Cluster", DisplayName="Spline长度")
	float SplineLength =1.0f;
	UPROPERTY(EditAnywhere, Category = "Spline|Cluster", DisplayName="Spline曲率")
	float Curvature = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Spline|Cluster", DisplayName="分簇阈值")
	float ClusterThreshold = 1000.0f;
};

UCLASS()
class USplineProcessor : public UObject
{
	GENERATED_BODY()

public:

	USplineProcessor();
	
	UPROPERTY(EditAnywhere, Category = "ClusterSpline", DisplayName="分簇参数")
	FClusterWeight WeightData;
	
	UPROPERTY(VisibleAnywhere, Category = "ClusterSpline")
	TArray<FCluster> Clusters;
	

	
	const TArray<FCluster>& USplineProcessor::GetClusters() const
	{
		return Clusters;
	}
	
	//对输入的spline进行筛选(删除一些过短，流向差异过大的spline)然后分簇,这是一个总的函数，包含了下面几个函数。
	void ProcessSplines(TArray<USplineComponent*>& SplineComponents);

	//删除过短的水流和跨越X轴太远的Splines
	//FitteredSplines是WaterFall的成员，用在这个函数里面
	void FitterSplines(TArray<USplineComponent*>& SplineComponents, const float SplineMaxLength);

	//填充SplineData
	TArray<FSplineData> FillSplineData(TArray<USplineComponent*>& SplineComponents);

	float Distance(const FSplineData& First, const FSplineData& Second, FClusterWeight* Weights);



	TArray<FCluster> BuildClusters(const TArray<USplineComponent*>& InOutSplineComponents, const TArray<FSplineData>& SplineData,  FClusterWeight Weights);


};
