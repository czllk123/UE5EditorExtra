#pragma once
#include "Components/SplineComponent.h"

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

class SplineProcessor
{
public:
	//对输入的spline进行筛选(删除一些过短，流向差异过大的spline)然后分簇
	static void ProcessSplines(TArray<USplineComponent*>& SplineComponents);

	//删除过短的水流和跨越X轴太远的Splines
	static void FitterSplines(TArray<USplineComponent*>& SplineComponents, const float SplineMaxLength);

	void InsertSpline(QuadTreeNode* Node, USplineComponent* Spline);
};
