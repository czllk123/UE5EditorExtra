#include "SplineProcessor.h"

void SplineProcessor::ProcessSplines(TArray<USplineComponent*>& SplineComponents)
{

	FitterSplines(SplineComponents, 20.0f);
	
	for(USplineComponent* SplineComponent : SplineComponents)
	{
		ensure(SplineComponent);
		const float SplineLength = SplineComponent->GetSplineLength();
		
		//获取曲线首尾切线向量，以确定其整体方向
		FVector StartTangent  = SplineComponent->GetTangentAtDistanceAlongSpline(0, ESplineCoordinateSpace::World);
		FVector EndTangent = SplineComponent->GetTangentAtDistanceAlongSpline(SplineLength, ESplineCoordinateSpace::World);
		FVector SplineDirection = (EndTangent - StartTangent).GetSafeNormal();

		// 获取spline首尾location,相连接生成一条直线段，然后采样曲线上的若干点，找到离若干点最近的Spline点，算spline点到直线段的距离
		// 如果距离越近说明曲线越直。
		// 这只是一个估算的算法，并不是严格意思上的数学算法。
		FVector StartLocation = SplineComponent->GetLocationAtDistanceAlongSpline(0, ESplineCoordinateSpace::World);
		FVector EndLocation = SplineComponent->GetLocationAtDistanceAlongSpline(SplineLength, ESplineCoordinateSpace::World);
		float TotalDeviation = 0.0f;
		constexpr int SamplePoints = 5; // 你可以根据需要增加或减少
		//沿着Spline曲线采样，但不包括起始和终点
		for(int i = 1; i < SamplePoints - 1; i++)
		{
			float Distance = i * (SplineLength / (SamplePoints - 1));
			FVector SampleLocation = SplineComponent->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
			FVector ClosestPointOnStraightLine = FMath::ClosestPointOnSegment(SampleLocation, StartLocation, EndLocation);
			TotalDeviation += FVector::Distance(SampleLocation, ClosestPointOnStraightLine);
		}
		float AverageDeviation = TotalDeviation / (SamplePoints - 2); // 这给了我们一个曲率的简单测量
	}
}

void SplineProcessor::FitterSplines(TArray<USplineComponent*>& InOutSplineComponents, const float SplineMaxLength)
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
