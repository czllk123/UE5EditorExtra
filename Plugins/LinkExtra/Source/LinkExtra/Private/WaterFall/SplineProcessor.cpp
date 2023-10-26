#include "SplineProcessor.h"
#include "Components/SplineComponent.h"
void FSplineProcessor::ProcessSplines(TArray<USplineComponent*>& SplineComponents)
{

	FClusterWeight WeightData;
	
	FitterSplines(SplineComponents, 20.0f);
	TArray<FSplineData> SplineData = FillSplineData(SplineComponents);
	TArray<FCluster> Cluster = BuildClusters(SplineComponents, SplineData, WeightData, 100); 
	
}

void FSplineProcessor::FitterSplines(TArray<USplineComponent*>& InOutSplineComponents, const float SplineMaxLength)
{
	for(int32 i = 0; i < InOutSplineComponents.Num();)
	{
		const USplineComponent* SplineComponent = InOutSplineComponents[i];
		ensure(SplineComponent);

		const float SplineLength = SplineComponent->GetSplineLength();
		const FVector StartPosition = SplineComponent->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
		const FVector EndPosition = SplineComponent->GetLocationAtSplinePoint(SplineComponent->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World);

		if(SplineLength < SplineMaxLength/2 || FMath::Abs(EndPosition.X - StartPosition.X) > SplineMaxLength / 3)
		{
			InOutSplineComponents.RemoveAtSwap(i);
		}
		else
		{
			i++; 
		}
	}
}

TArray<FSplineData> FSplineProcessor::FillSplineData(TArray<USplineComponent*>& SplineComponents)
{
	TArray<FSplineData> SplineDataArray;
	for(const USplineComponent* SplineComponent: SplineComponents)
	{
		FSplineData Data;
		Data.StartPosition = SplineComponent->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
		Data.EndPosition = SplineComponent->GetLocationAtSplinePoint(SplineComponent->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World);
		Data.Length = SplineComponent->GetSplineLength();
		
		// 获取spline首尾location,相连接生成一条直线段，然后采样曲线上的若干点，找到离若干点最近的Spline点，算spline点到直线段的距离
		// 如果距离越近说明曲线越直。
		// 这只是一个估算的算法，并不是严格意思上的数学算法。
		float TotalDeviation = 0.0f;
		constexpr int SamplePoints = 5; // 你可以根据需要增加或减少
		//沿着Spline曲线采样，但不包括起始和终点
		for(int i = 1; i < SamplePoints - 1; i++)
		{
			float Distance = i * (Data.Length / (SamplePoints - 1));
			FVector SampleLocation = SplineComponent->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
			FVector ClosestPointOnStraightLine = FMath::ClosestPointOnSegment(SampleLocation, Data.StartPosition, Data.EndPosition);
			TotalDeviation += FVector::Distance(SampleLocation, ClosestPointOnStraightLine);
		}
		const float AverageDeviation = TotalDeviation / (SamplePoints - 2); // 这给了我们一个曲率的简单测量
		Data.Curvature = AverageDeviation;

		SplineDataArray.Add(Data);
	}
	return SplineDataArray;
} 

float FSplineProcessor::Distance(const FSplineData& First, const FSplineData& Second, FClusterWeight* Weights)
{
	FClusterWeight Weight;
	
	float Spatial_distance = (First.StartPosition - Second.StartPosition).Size() + (First.EndPosition - Second.EndPosition).Size();
	float LengthDiffence = FMath::Abs(First.Length - Second.Length);
	float CurvatureDiffence = FMath::Abs(First.Curvature - Second.Curvature);
	return  Weight.Distance*Spatial_distance + Weight.SplineLength*LengthDiffence + Weight.Curvature*CurvatureDiffence;
}

void FSplineProcessor::InsertSpline(QuadTreeNode* Node, USplineComponent* Spline)
{
}

TArray<FCluster> FSplineProcessor::BuildClusters(const TArray<USplineComponent*>& InOutSplineComponents, const TArray<FSplineData>& SplineData, FClusterWeight Weights, float Threshold)
{
	TArray<FCluster> Clusters;

	if (SplineData.Num() == 0 || InOutSplineComponents.Num() != SplineData.Num())
	{
		return Clusters; // Return an empty list if there are no splines or if there is a mismatch in count.
	}
	
	for (int32 i = 0; i < SplineData.Num(); i++)
	{
		const FSplineData& CurrentSpline = SplineData[i];
		bool bAddedToCluster = false;

		//对于每个spline，我们查看它是否可以添加到现有的cluster中
		for (FCluster& Cluster : Clusters)
		{
			const FSplineData& RepresentativeSpline = SplineData[Cluster.SplineIndices[0]];

			// 计算曲率、长度和距离的差异权重
			float CurvatureDifference = FMath::Abs(CurrentSpline.Curvature - RepresentativeSpline.Curvature) * Weights.Curvature;
			float LengthDifference = FMath::Abs(CurrentSpline.Length - RepresentativeSpline.Length) * Weights.SplineLength;
			float Distance = FVector::Dist(CurrentSpline.StartPosition, RepresentativeSpline.StartPosition) * Weights.Distance;

			// 如果所有权重之和低于阈值，则将其添加到该簇中
			if (CurvatureDifference + LengthDifference + Distance < Threshold)
			{
				Cluster.SplineIndices.Add(i);
				Cluster.Center = (Cluster.Center * (Cluster.SplineIndices.Num() - 1) + CurrentSpline.StartPosition) / Cluster.SplineIndices.Num();  // 更新簇中心
				bAddedToCluster = true;
				break; 
			}
		}
		
		if (!bAddedToCluster)
		{
			// 如果当前Spline未添加到现有簇中，创建一个新簇
			FCluster NewCluster;
			NewCluster.SplineIndices.Add(i);
			NewCluster.Center = CurrentSpline.StartPosition;
			Clusters.Add(NewCluster);
		}
	}
	
	// Visualization: Assign a random color to each cluster and update the spline components.
	for (FCluster& Cluster : Clusters)
	{
		FLinearColor RandomColor = FLinearColor::MakeRandomColor();
		for (int32 SplineIndex : Cluster.SplineIndices)
		{
			USplineComponent* SplineComponent = InOutSplineComponents[SplineIndex];
			SplineComponent->EditorUnselectedSplineSegmentColor = RandomColor;
			SplineComponent->UpdateSpline();
			SplineComponent->PostEditChange();
		}
	}
	return Clusters;
}





/*
TArray<FCluster> SplineProcessor::BuildClusters(const TArray<FSplineData>& Splines, float MaxDistance)
{
	TArray<FCluster> Clusters;
	for (int32 i = 0; i < Splines.Num(); ++i) {
		const FSplineData& Spline = Splines[i];

		bool bInserted = false;
		for (FCluster& Cluster : Clusters) {
			// 如果spline的起点与簇的中心点的距离小于阈值，则将spline加入该簇
			if (FVector::Dist(Spline.StartPoint, Cluster.Center) < MaxDistance) {
				Cluster.SplineIndices.Add(i);
				// 更新簇的中心点为簇中所有spline的平均位置
				Cluster.Center = FVector(0, 0, 0);
				for (int32 Index : Cluster.SplineIndices) {
					Cluster.Center += Splines[Index].StartPoint;
				}
				Cluster.Center /= Cluster.SplineIndices.Num();
				bInserted = true;
				break;
			}
		}

		// 如果该spline没有被插入任何簇，创建一个新簇
		if (!bInserted) {
			FCluster NewCluster;
			NewCluster.Center = Spline.StartPoint;
			NewCluster.SplineIndices.Add(i);
			Clusters.Add(NewCluster);
		}
	}

	return Clusters;
}
*/
