//Copyright  2022 Tav Shande.All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CrazyIvy.h"
#include "SMeshData.h"
#include "Components/SplineComponent.h"
#include <StaticMeshEditorSubsystem.h>
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInterface.h"
#include "IMeshMergeUtilities.h"
#include <Runtime/Engine/Classes/Components/HierarchicalInstancedStaticMeshComponent.h>
#include "IvyGeneratorBase.generated.h"




UENUM()
enum EButtonState
{
	Reset    UMETA(DisplayName = "Reset"),
	Grow     UMETA(DisplayName = "Grow"),
	Stop     UMETA(DisplayName = "Stop"),
	   
};


/** an ivy node */
UCLASS()
class UIvyNode : public UObject
{
	GENERATED_BODY()
public:

	UIvyNode() : Length(0.0f), FloatingLength(0.0f),Climb(false) {};

	/** node position */
	FVector Pos;

	/** primary grow direction, a weighted sum of the previous directions */
	FVector PrimaryDir= FVector(0, 0, 1);

	/** adhesion vector as a result from other scene objects */
	FVector AdhesionVector= FVector(0, 0, 0);

	/** a smoothed adhesion vector computed and used during the birth phase,
	   since the ivy leaves are align by the adhesion vector, this smoothed vector
	   allows for smooth transitions of leaf alignment */
	FVector SmoothAdhesionVector= FVector(0, 0, 0);

	/** length of the associated ivy branch at this node */
	float Length;

	/** length at the last node that was climbing */
	float FloatingLength;

	/** climbing state */
	bool Climb;
};


/** an ivy root point */
UCLASS()
class UIvyRoot : public UObject
{
	GENERATED_BODY()
public:

	/** a number of nodes */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default")
	TArray<UIvyNode*> Nodes;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default")
	/** alive state */
	bool Alive = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default")
	/** number of parents, represents the level in the root hierarchy */
	int Parents = 0;
};


UCLASS(AutoExpandCategories = ("IvyProperties","IvySettings", "IvySettings|Birth", "IvySettings|Grow", "IvySettings|Misc","IvyLeafSpecies"),
	HideCategories = ("ZDefault"))

class CRAZYIVY_API  AIvyGeneratorBase : public AActor
{
	GENERATED_BODY()
	

public:	
	// Sets default values for this actor's properties
	AIvyGeneratorBase(const FObjectInitializer& ObjectInitializer);
	

	
	void SpawnPoints();



	/** compute the adhesion of scene objects at a point pos*/
	FVector ComputeAdhesion(const FVector& pos);

	bool ComputeCollision(const FVector& oldPos, FVector& newPos, bool& climbing);


	EButtonState GetGrowthStateValue() { return GrowthState; };

	void SetDefaultMeshesMaterials();

	float ComputeTrunksDiameter(UIvyRoot* root, float dist);
	



public:

	UFUNCTION(BlueprintCallable, Category = Properties)
		void SetGrowthStateValue(EButtonState _GrowthState) { GrowthState = _GrowthState; };

	UFUNCTION(BlueprintCallable, Category = Properties)
		void ClearAll();

	UFUNCTION(BlueprintCallable, Category = Properties)
		void IvySuccessfulDone() { UE_LOG(LogCrazyIvy, Log, TEXT("Crazy Ivy Generated Successfully!")); UE_LOG(LogCrazyIvy, Log, TEXT("============================")) };

	UFUNCTION(BlueprintCallable, Category = Properties)
		bool Birth();

	UFUNCTION(BlueprintCallable, Category = Properties)
		void IvyDeleteAsset(FString InLog,FString InString);

	//UFUNCTION(BlueprintCallable, Category = Properties)
	//	void IvySetLodBuildSettings(UStaticMesh* StaticMesh,
	//	const int32 LodIndex,
	//	const FMeshBuildSettings& BuildOptions);

	UFUNCTION(BlueprintCallable, Category =Properties)
		void StartGrowth();

	UFUNCTION(BlueprintCallable, Category =Properties)

		void StopGrowth();

	UFUNCTION(BlueprintCallable, Category = Properties)

		int  ComputeRandomLeafMaterialIndex(float weight);

	UFUNCTION(BlueprintCallable,CallInEditor, Category =IvyButtons)
		void ResetSettings();


	UFUNCTION(BlueprintImplementableEvent, Category = Properties)
		void IvyAddInstanceMesh(int index, FTransform Transform);



	UFUNCTION(BlueprintCallable, Category = Properties)
		void CreateMesh(TArray<UPrimitiveComponent*>ComponentsToMerge, const FString& InBasePackageName,
			TArray<UObject*>& OutAssetsToSync, const float ScreenSize);


public:


	/**Select the mesh that the ivy will grow on.*/
	UPROPERTY(BlueprintReadWrite, Category = "IvyProperties", EditAnywhere)
		AStaticMeshActor* DestinationMesh;



	UPROPERTY(VisibleAnywhere, Category = "IvyProperties", BlueprintReadWrite)
		int TotalPoints = 0;

	UPROPERTY(EditAnywhere, Category = "IvyProperties", BlueprintReadWrite)
		UStaticMesh* IvyBranchMesh;

	UPROPERTY(EditAnywhere, Category = "ZDefault", BlueprintReadWrite, meta = (EditCondition = "bSeperateBranchesLeavesMesh"))
		bool  bCanClearGeneratedMeshes = false;

	UPROPERTY(EditAnywhere, Category = "ZDefault", BlueprintReadWrite, meta = (EditCondition = "bSeperateBranchesLeavesMesh"))
		bool  bCanCopyGeneratedMeshes = false;


	/** Influences the growth behavior by tuning the ivy step size. Ivy will cover a larger area..[1..10].  */
	UPROPERTY(EditAnywhere, Category = "IvySettings|Grow", BlueprintReadWrite, meta = (ClampMin = " 1"), meta = (ClampMax = "10"))
		float IvySize;


	/** A larger value means ivy has more of a tendency to grow towards the sky [0..1] */
	UPROPERTY(EditAnywhere, Category = "IvySettings|Grow", BlueprintReadWrite, meta = (ClampMin = " 0"), meta = (ClampMax = "1"))
		double PrimaryWeight;

	/** How much random weight influence to add to growth vector [0..1] */
	UPROPERTY(EditAnywhere, Category = "IvySettings|Grow", BlueprintReadWrite, meta = (ClampMin = " 0"), meta = (ClampMax = "1"))
		double RandomWeight;

	/** A bigger value means a higher tendency to grow towards the ground.[0..2] */
	UPROPERTY(EditAnywhere, Category = "IvySettings|Grow", BlueprintReadWrite, meta = (ClampMin = " 0"), meta = (ClampMax = "2"))
		double GravityWeight;

	/** A bigger value means strong adhesion to the mesh surface. [0..1] */
	UPROPERTY(EditAnywhere, Category = "IvySettings|Grow", BlueprintReadWrite, meta = (ClampMin = " 0"), meta = (ClampMax = "1"))
		double AdhesionWeight;

	/** The probability of producing a new ivy branch. A larger value means any branch can split into new branches earlier. [0..1].*/
	UPROPERTY(EditAnywhere, Category = "IvySettings|Grow", BlueprintReadWrite, meta = (ClampMin = " 0"), meta = (ClampMax = "1"))
		
		float BranchingProbability;


	/**Maximum length of a free-floating ivy branch segment.
	A bigger value means the branch will float away form the surface longer and may grow towards the sky with low gravity.[0..1]*/
	UPROPERTY(EditAnywhere, Category = "IvySettings|Grow", BlueprintReadWrite, meta = (ClampMin = " 0"), meta = (ClampMax = "1"))
		double MaxFloatLength;

	/** Maximum distance of adhesion for the destination mesh. 
	A bigger value means the adhesion is less accurate. [0..4]*/
	UPROPERTY(EditAnywhere, Category = "IvySettings|Grow", BlueprintReadWrite, meta = (ClampMin = " 0"), meta = (ClampMax = "4"))
		double MaxAdhesionDistance;

	/** Scale branch diameter [0..0.5]*/
	UPROPERTY(EditAnywhere, Category = "IvySettings|Birth", BlueprintReadWrite, meta = (ClampMin = " 0"), meta = (ClampMax = "0.5"))
		float IvyBranchSize;

	/**Scale multiplier to all leaf sizes [0..2]*/
	UPROPERTY(EditAnywhere, Category = "IvySettings|Birth", BlueprintReadWrite, meta = (ClampMin = " 0"), meta = (ClampMax = "2"))
		float IvyLeafSize;

	/**Increase or decrease the probability of how many ivy leaves are generated [0..1] */
	UPROPERTY(EditAnywhere, Category = "IvySettings|Birth", BlueprintReadWrite, meta = (ClampMin = " 0"), meta = (ClampMax = "1"))		
		float LeafDensity;
	/**How fast the Ivy preview grows. [1..3]*/
	UPROPERTY(EditAnywhere, Category = "IvyProperties", BlueprintReadWrite, meta = (ClampMin = " 1"), meta = (ClampMax = "3"))
		int  GrowthSpeed = 3;
	/**Maximum Number of ivy points [100..10000]*/
	UPROPERTY(EditAnywhere, Category = "IvySettings|Misc", BlueprintReadWrite, meta = (ClampMin = " 100"), meta = (ClampMax = "10000"))
		int MAX_NUMBER = 10000;

	/**Multiplies the overall density of leaves [1..10]*/
	UPROPERTY(EditAnywhere, Category = "IvySettings|Misc", BlueprintReadWrite, meta = (ClampMin = " 1"), meta = (ClampMax = "10"))
		int DensityMultiplier = 3;

	/** Creates an offset between branch and leaves. Higher numbers create a more bushy looking ivy plant but can leave larger gaps between branches and leaves.
	At 0, the leaves are attached directly to the branches. [0..5] */
	UPROPERTY(EditAnywhere, Category = "IvySettings|Misc", BlueprintReadWrite, meta = (ClampMin = " 0"), meta = (ClampMax = "5"))
		float LeafOffset = 0;


	UPROPERTY(EditAnywhere, Category = "ZDefault", BlueprintReadWrite)

		TArray<UMaterialInterface*> LeafMaterials;


	UPROPERTY( BlueprintReadWrite,Category = "ZDefault")
		TArray<UStaticMesh*> IvyLeafMeshes;



	UPROPERTY(EditAnywhere,Category = "ZDefault", BlueprintReadWrite)

	   TArray<UPrimitiveComponent*> BranchPrimitiveComps;

	UPROPERTY(Category = "ZDefault", EditAnywhere)
		TArray<UIvyRoot*>Roots;


	UPROPERTY(VisibleAnywhere, Category = "ZDefault")
		TArray<int>NumberOfPoints;

	UPROPERTY(Category = "ZDefault", BlueprintReadWrite)
		bool GrowthVal = true;


	UPROPERTY(Category = "ZDefault", BlueprintReadWrite)
	UHierarchicalInstancedStaticMeshComponent* GrowthNode;

	UPROPERTY(Category = "ZDefault", BlueprintReadWrite)
	UHierarchicalInstancedStaticMeshComponent* GrowthBranch;

	UPROPERTY(Category = "ZDefault", BlueprintReadWrite)
	UStaticMeshEditorSubsystem * StaticMeshEditorSubsystemObject;
private:


	double Local_MaxFloatLength;

	double Local_IvySize;



	FTimerHandle TimeHandle;

	TArray<FTriangle> Triangles;

	USplineComponent* TempSpline;
	
	EButtonState GrowthState = EButtonState::Grow;

	int DeadRootsCount = 0;

	bool bFirstTime = true;

};
