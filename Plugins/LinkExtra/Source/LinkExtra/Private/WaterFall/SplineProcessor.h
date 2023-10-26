#pragma once
#include "Components/SplineComponent.h"
#include "Iris/Core/IrisDebugging.h"

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




// 簇的定义
struct FCluster {
	TArray<int32> SplineIndices;  // 该簇中包含的spline的索引
	FVector Center;               // 簇的中心点位置，初始化为spline的起点
	FVector2d ClusterStartWidth; // 簇的开始宽度，用来是设置SplineMesh的开始宽度
	FVector2d ClusterEndWidth; // 簇结束的宽度，用来是设置SplineMesh的结束宽度
	
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

struct FClusterWeight
{
	float Distance = 1.0f;
	float SplineLength =1.0f;
	float Curvature = 1.0f;
};

class FSplineProcessor
{
public:
	//对输入的spline进行筛选(删除一些过短，流向差异过大的spline)然后分簇
	static void ProcessSplines(TArray<USplineComponent*>& SplineComponents);

	//删除过短的水流和跨越X轴太远的Splines
	static void FitterSplines(TArray<USplineComponent*>& SplineComponents, const float SplineMaxLength);

	//填充SplineData
	static TArray<FSplineData> FillSplineData(TArray<USplineComponent*>& SplineComponents);

	static float Distance(const FSplineData& First, const FSplineData& Second, FClusterWeight* Weights);

	void InsertSpline(QuadTreeNode* Node, USplineComponent* Spline);

	static TArray<FCluster> BuildClusters(const TArray<USplineComponent*>& InOutSplineComponents, const TArray<FSplineData>& SplineData,  FClusterWeight Weights, float Threshold);

	
};
