// Copyright  2022 Tav Shande.All Rights Reserved.


#include "IvyGeneratorBase.h"
#include "CrazyIvy.h"
#include "Modules/ModuleManager.h"
#include <Engine/Engine.h>
#include "MeshMergeModule.h"
#include "SMeshData.h"
#include <Runtime/Engine/Classes/Components/SplineMeshComponent.h>
#include "Kismet/KismetMathLibrary.h"
#include "Components/MeshComponent.h"
#include "EditorAssetLibrary.h"
#include "Materials/Material.h"
#include "AssetRegistry/AssetRegistryModule.h"





// Sets default values

static  bool getBarycentricCoordinates(const FVector& vector1, const FVector& vector2, const FVector& vector3, const FVector& position)
{
	double area = 0.5f * (FVector::CrossProduct(vector2 - vector1, vector3 - vector1)).Length();

	double  alpha = 0.5f * (FVector::CrossProduct(vector2 - position, vector3 - position)).Length() / area;

	double  beta = 0.5f * (FVector::CrossProduct(vector1 - position, vector3 - position)).Length() / area;

	double  gamma = 0.5f *(FVector::CrossProduct(vector1 - position, vector2 - position)).Length() / area;

	
	if (abs(1.0f - alpha - beta - gamma) > 0.00001f) return false;

	return true;
}


static  FVector2D getEpsilon()
{
	return FVector2D((double)std::numeric_limits<float>::epsilon(),(double) std::numeric_limits<float>::epsilon());
}


static  float vectorToPolar(const FVector2D& vector)
{
	float phi = (vector.X == 0.0f) ? 0.0f : (float)atan(vector.Y / vector.X);

	if (vector.X < 0.0f)
	{
		phi += 3.1415926535897932384626433832795f;
	}
	else
	{
		if (vector.Y < 0.0f)
		{
			phi += 2.0f * 3.1415926535897932384626433832795f;
		}
	}

	return phi;
}

static  float getAngle(const FVector& vector1, const FVector& vector2)
{
	float length1 = vector1.Length();

	float length2 = vector2.Length();

	if ((length1 == 0) || (length2 == 0)) return 0.0f;

	return acos((float)FVector::DotProduct(vector1, vector2) / (length1 * length2));
}


AIvyGeneratorBase::AIvyGeneratorBase(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	RootComponent =
		ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("SceneRootComponent"));
	RootComponent->SetMobility(EComponentMobility::Movable);
	RootComponent->bVisualizeComponent = true;
	GrowthNode = ObjectInitializer.CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>
		(this, TEXT("UHierarchicalInstancedN"));
	GrowthNode->SetMobility(EComponentMobility::Movable);
	GrowthNode->bVisualizeComponent = true;
	GrowthNode->SetupAttachment(RootComponent);
	GrowthNode->bDisableCollision = true;

	GrowthBranch = ObjectInitializer.CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>
		(this, TEXT("UHierarchicalInstancedB"));
	GrowthBranch->SetMobility(EComponentMobility::Movable);
	GrowthBranch->bVisualizeComponent = true;
	GrowthBranch->SetupAttachment(RootComponent);
	GrowthBranch->bDisableCollision = true;

	StaticMeshEditorSubsystemObject= ObjectInitializer.CreateDefaultSubobject<UStaticMeshEditorSubsystem>
		(this, TEXT("UStaticMeshEditorSubsystemObj"));


	ResetSettings();

	SetDefaultMeshesMaterials();

	//SetFlags(RF_DuplicateTransient);

}




void AIvyGeneratorBase::StartGrowth()
{
	GrowthVal = false;
	GrowthState = EButtonState::Stop;

	
	UE_LOG(LogCrazyIvy, Log, TEXT("=========== CRAZY IVY =========="))
	UE_LOG(LogCrazyIvy, Log, TEXT("Starting Grow"))
		if (bFirstTime == true)
		{
			TotalPoints = 0;
			for (auto& Comp : BranchPrimitiveComps)
				if (Comp)
					Comp->DestroyComponent();
			BranchPrimitiveComps.Empty();
			Roots.Empty();
			NumberOfPoints.Empty();
			DeadRootsCount = 0;
			UE_LOG(LogCrazyIvy, Log, TEXT("Has Been Fired For The First Time"))
			bFirstTime = false;
		}


	if (DestinationMesh)
	{
		Triangles.Empty();
		if(!DestinationMesh->GetStaticMeshComponent()
			||!DestinationMesh->GetStaticMeshComponent()->GetStaticMesh())
		{
			UE_LOG(LogCrazyIvy, Log, TEXT("Sorry, Your Ivy Has No Destination Mesh Component"), )
			StopGrowth();
			return;
		}

		Triangles = GetTriangles(DestinationMesh->GetStaticMeshComponent());

		if (Roots.IsEmpty())

		{
			UIvyRoot* TempRoot = NewObject<UIvyRoot>(this);
			TempRoot->Parents = 0;

			FVector TempPoint;
			TempPoint = this->GetActorLocation();


			UIvyNode* TempNode = NewObject<UIvyNode>(this);

			TempNode->Pos = TempPoint;
			TempNode->Length = 0;
			TempNode->AdhesionVector = FVector(0, 0, 0);


			TempRoot->Nodes.Empty();
			TempRoot->Nodes.Add(TempNode);

			Roots.Add(TempRoot);
			NumberOfPoints.Add(0);

		}

		GetWorld()->GetTimerManager().SetTimer
		(TimeHandle, this, &AIvyGeneratorBase::SpawnPoints, FMath::Pow(10.0, -GrowthSpeed), true);
	}
	else
	{
		UE_LOG(LogCrazyIvy, Log, TEXT("Sorry, Your Ivy Has No Destination Mesh"), )
		StopGrowth();
	}
}

void AIvyGeneratorBase::StopGrowth()
{
	GrowthVal = true;
	GrowthState = EButtonState::Grow;

	UE_LOG(LogCrazyIvy, Log ,TEXT("Growth Process Has Been Stoped."))

	if (TimeHandle.IsValid())
		GetWorld()->GetTimerManager().ClearTimer(TimeHandle);
}





int AIvyGeneratorBase::ComputeRandomLeafMaterialIndex(float weight)
{
	float probability = rand() / (float)RAND_MAX;
	float leafProbability = 1 - LeafDensity;

	int Num = LeafMaterials.Num();

	for (auto i = Num; i >= 1; i--)
	{
	
		if (leafProbability > 0)
		{
			if ((probability * weight > i * leafProbability / Num))
				return i - 1;
			else
				continue;
		}
		else
			return(FMath::RandRange(0, Num - 1));
	}
	return 0;
}

void AIvyGeneratorBase::ResetSettings()
{
	UE_LOG(LogCrazyIvy, Log, TEXT("IVY Parameters Have Been Set To Default "))

	IvySize = 4;

	PrimaryWeight = 0.5f;

	RandomWeight = 0.5f;

	GravityWeight = 1.0f;

	AdhesionWeight = 0.2f;

	BranchingProbability = 0.05f;


	MaxFloatLength = 0.1;

	MaxAdhesionDistance = 3.0f;

	IvyBranchSize = 0.18;

	IvyLeafSize = 1.0f;

	LeafDensity = 1;


	MAX_NUMBER = 10000;

	DensityMultiplier = 3;

	LeafOffset = 0;
	
}



void AIvyGeneratorBase::CreateMesh(TArray<UPrimitiveComponent*>ComponentsToMerge, const FString& InBasePackageName, TArray<UObject*>& OutAssetsToSync,
	const float ScreenSize)
{
	if (ComponentsToMerge.Num()==0)
	{
		return;
	}
	
	FMeshMergingSettings TempMergeSettings;
	FMaterialProxySettings TempMaterialSettings;


	TempMaterialSettings.TextureSize = FIntPoint::TIntPoint(1024, 1024);
	TempMaterialSettings.GutterSpace = 4.0;
	TempMaterialSettings.MetallicConstant = 0.0;
	TempMaterialSettings.RoughnessConstant = 0.5000;
	TempMaterialSettings.AnisotropyConstant = 0.0000;
	TempMaterialSettings.SpecularConstant = 0.50000;
	TempMaterialSettings.OpacityConstant = 1.000000;
	TempMaterialSettings.OpacityMaskConstant = 1.000000;
	TempMaterialSettings.AmbientOcclusionConstant = 1.000000;
	TempMaterialSettings.TextureSizingType = ETextureSizingType::TextureSizingType_UseSingleTextureSize;
	TempMaterialSettings.MaterialMergeType = MaterialMergeType_Default;
	TempMaterialSettings.BlendMode = BLEND_Opaque;
	TempMaterialSettings.bAllowTwoSidedMaterial = true;
	TempMaterialSettings.bNormalMap = true;
	TempMaterialSettings.bTangentMap = false;
	TempMaterialSettings.bMetallicMap = false;
	TempMaterialSettings.bRoughnessMap = false;
	TempMaterialSettings.bAnisotropyMap = false;
	TempMaterialSettings.bSpecularMap = false;
	TempMaterialSettings.bEmissiveMap = false;
	TempMaterialSettings.bOpacityMap = false;
	TempMaterialSettings.bOpacityMaskMap = false;
	TempMaterialSettings.bAmbientOcclusionMap = false;
	TempMaterialSettings.DiffuseTextureSize = FIntPoint::TIntPoint(1024,1024);
	TempMaterialSettings.NormalTextureSize = FIntPoint::TIntPoint(1024, 1024);
	TempMaterialSettings.TangentTextureSize = FIntPoint::TIntPoint(1024, 1024);
	TempMaterialSettings.MetallicTextureSize = FIntPoint::TIntPoint(1024, 1024);
	TempMaterialSettings.RoughnessTextureSize = FIntPoint::TIntPoint(1024, 1024);
	TempMaterialSettings.AnisotropyTextureSize = FIntPoint::TIntPoint(1024, 1024);
	TempMaterialSettings.SpecularTextureSize = FIntPoint::TIntPoint(1024, 1024);
	TempMaterialSettings.EmissiveTextureSize = FIntPoint::TIntPoint(1024, 1024);
	TempMaterialSettings.OpacityTextureSize = FIntPoint::TIntPoint(1024, 1024);
	TempMaterialSettings.OpacityMaskTextureSize = FIntPoint::TIntPoint(1024, 1024);
	TempMaterialSettings.AmbientOcclusionTextureSize = FIntPoint::TIntPoint(1024, 1024);
	


	TempMergeSettings.bMergePhysicsData = false;
	TempMergeSettings.bMergeMaterials = false;
	TempMergeSettings.GutterSize = 2;
	TempMergeSettings.SpecificLOD = 0;
	TempMergeSettings.LODSelectionType = EMeshLODSelectionType::CalculateLOD;
	TempMergeSettings.bGenerateLightMapUV = true;
	TempMergeSettings.bComputedLightMapResolution = false;
	TempMergeSettings.bPivotPointAtZero = false;	
	TempMergeSettings.bAllowDistanceField = false;
	TempMergeSettings.bBakeVertexDataToMesh = true;
	TempMergeSettings.bComputedLightMapResolution = false;
	TempMergeSettings.bCreateMergedMaterial = false;
	TempMergeSettings.bGenerateLightMapUV = true;
//	TempMergeSettings.bGenerateNaniteEnabledMesh = false;
	TempMergeSettings.bIncludeImposters = true;
	TempMergeSettings.bMergeEquivalentMaterials = true;
	TempMergeSettings.bMergeMaterials = false;
	TempMergeSettings.bMergePhysicsData = false;
	TempMergeSettings.bPivotPointAtZero = false;
	TempMergeSettings.bReuseMeshLightmapUVs = true;
	TempMergeSettings.bUseLandscapeCulling = false;
	TempMergeSettings.bUseTextureBinning = false;
	TempMergeSettings.bUseVertexDataForBakingMaterial = true;
	TempMergeSettings.GutterSize = 2;
	TempMergeSettings.LODSelectionType = EMeshLODSelectionType::AllLODs;
	TempMergeSettings.MaterialSettings = TempMaterialSettings;
	TempMergeSettings.MergeType = EMeshMergeType::MeshMergeType_Default;
	TempMergeSettings.OutputUVs[8] = EUVOutput::OutputChannel;
	TempMergeSettings.SpecificLOD = 0;
	TempMergeSettings.TargetLightMapResolution = 256;




	
   FVector TempLocation = this->GetActorLocation();

	const IMeshMergeUtilities& Module = FModuleManager::Get().LoadModuleChecked<IMeshMergeModule>("MeshMergeUtilities").GetUtilities();
	Module.MergeComponentsToStaticMesh(ComponentsToMerge, this->GetWorld(), TempMergeSettings,
		nullptr, nullptr, InBasePackageName, OutAssetsToSync, TempLocation, ScreenSize, true);


}

FVector AIvyGeneratorBase::ComputeAdhesion(const FVector& pos)
{
	FVector adhesionVector = FVector(0, 0, 0);


	//define a maximum distance
	float local_maxAdhesionDistance = DestinationMesh->GetStaticMeshComponent()
		->GetStaticMesh()->GetBounds().SphereRadius * MaxAdhesionDistance;
	
	float minDistance = local_maxAdhesionDistance;


	//find nearest triangle
	for (auto& t : Triangles)

	{
	
		//scalar product projection
		float nq = FVector::DotProduct(t.Norm, pos - t.A);
		
		//continue if backside of triangle
		if (nq < 0.0f) continue;


			//project last node onto triangle plane, e.g. scalar product projection
			FVector p0 = pos - t.Norm * nq;

			//compute barycentric coordinates of p0

			
				if (getBarycentricCoordinates(t.A, t.C, t.B, p0))
					
				{
					//compute distance
					float distance = (float)(p0 - pos).Length();
					
					//find shortest distance
					if (distance < minDistance)
					{
						minDistance = distance;
					

						adhesionVector = (p0 - pos).GetSafeNormal();


							//distance dependent adhesion vector
							adhesionVector *= 1.0f - distance /
								local_maxAdhesionDistance;
							
						
					}
				}
	
	}
	
	return adhesionVector;
}

bool AIvyGeneratorBase::ComputeCollision(const FVector& oldPos, FVector& newPos, bool& climbing)
{
	//reset climbing state
	climbing = false;

	bool intersection;

	int deadlockCounter = 0;

	do
	{
		intersection = false;

		for (auto& t : Triangles)
		{
			//compute intersection with triangle plane parametrically: intersectionPoint = oldPos + (newPos - oldPos) * t0;
			float t0 = -FVector::DotProduct(t.Norm, oldPos - t.A) / 
				FVector::DotProduct(t.Norm, newPos - oldPos);

			//plane intersection
			if ((t0 >= 0.0f) && (t0 <= 1.0f))
			{
				//intersection point
				FVector intersectionPoint = oldPos + (newPos - oldPos) * t0;

				

				//triangle intersection
				if (getBarycentricCoordinates(t.A, t.C, t.B, intersectionPoint))
				{
					//test on entry or exit of the triangle mesh
					bool entry = FVector::DotProduct(t.Norm, newPos - oldPos) < 0.0f ? true : false;

					if (entry)
					{
						//project newPos to triangle plane
						FVector p0 = newPos - t.Norm * FVector::DotProduct(t.Norm, newPos - t.A);

						//mirror newPos at triangle plane
						newPos += 2.0f * (p0 - newPos);

						
						
						intersection = true;

						climbing = true;
					}
				}
			}
		}

		//abort climbing and growing if there was a collistion detection problem
		if (deadlockCounter++ > 5)
		{
			return false;
		}
	} while (intersection);

	return true;
	
}


void AIvyGeneratorBase::SetDefaultMeshesMaterials()
{	
	
	
	auto MeshAssetBranch = ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("StaticMesh'/CrazyIvy/Statics/Meshes/SM_Branch.SM_Branch'"));
	auto MeshAssetNode = ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("StaticMesh'/CrazyIvy/Statics/Meshes/SM_Sphere.SM_Sphere'"));

	auto MaterialAssetBranch = (ConstructorHelpers::FObjectFinder<UMaterial>
		(TEXT("UMaterial'/CrazyIvy/Statics/Materials/Branch/M_PreviewBranch.M_PreviewBranch'")));

	if (MeshAssetBranch.Object != nullptr && MeshAssetNode.Object != nullptr)
	{
		IvyBranchMesh = MeshAssetBranch.Object;
		if (GrowthNode)
		{
			GrowthNode->SetStaticMesh(MeshAssetNode.Object);
			if (MaterialAssetBranch.Object != nullptr)
				GrowthNode->SetMaterial(0, MaterialAssetBranch.Object);
		}
		if (GrowthBranch)
		{
			GrowthBranch->SetStaticMesh(MeshAssetBranch.Object);
			if (MaterialAssetBranch.Object != nullptr)
				GrowthBranch->SetMaterial(0, MaterialAssetBranch.Object);
		}
	}
	LeafMaterials.Empty();
	auto MaterialAsset=(ConstructorHelpers::FObjectFinder<UMaterialInstance>
		(TEXT("UMaterialInstance'/CrazyIvy/Statics/Materials/foliage/MI_ivy_HederaHelix_leaf_adult.MI_ivy_HederaHelix_leaf_adult'")));
	if (MaterialAsset.Object != nullptr)
		LeafMaterials.Add(MaterialAsset.Object);

	auto MaterialAsset1 = (ConstructorHelpers::FObjectFinder<UMaterialInstance>
		(TEXT("UMaterialInstance'/CrazyIvy/Statics/Materials/foliage/MI_ivy_HederaHelix_leaf_young.MI_ivy_HederaHelix_leaf_young'")));
	if (MaterialAsset1.Object != nullptr)
		LeafMaterials.Add(MaterialAsset1.Object);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> ObjectList;
	FName PackagePath= FName("/CrazyIvy/Statics/Meshes/Leaves");

	AssetRegistryModule.Get().GetAssetsByPath(PackagePath,ObjectList,false,false);
	for (auto ObjIter = ObjectList.CreateConstIterator(); ObjIter; ++ObjIter) {
		const FAssetData& Asset = *ObjIter;
		UObject* Object = Asset.GetAsset();
		UStaticMesh* ST = Cast<UStaticMesh>(Object);
		if(ST)
			IvyLeafMeshes.Add(ST);
	}
	
}





void AIvyGeneratorBase::ClearAll()
{
	TotalPoints = 0;
	StopGrowth();

	GrowthState = EButtonState::Reset;
	for (auto& Comp : BranchPrimitiveComps)
		if (Comp)
			Comp->DestroyComponent();
	BranchPrimitiveComps.Empty();
	if (GrowthNode)
	{
		GrowthNode->ClearInstances();	
//		GrowthNode->SetActive(false);
	}

	if (GrowthBranch)
	{
		GrowthBranch->ClearInstances();

//		GrowthBranch->SetActive(false);
	}


	Roots.Empty();
	NumberOfPoints.Empty();
	DeadRootsCount = 0;
	UE_LOG(LogCrazyIvy, Log, TEXT("Preproduction entities have been cleared"))

}

bool AIvyGeneratorBase::Birth()
{

	if (Roots.IsEmpty())
	{
		UE_LOG(LogCrazyIvy, Log, TEXT("Sorry, Your Ivy Is Empty (No roots)!"),)
		return false;
	}
	if (bFirstTime)
	{
		ClearAll();
		UE_LOG(LogCrazyIvy, Log, TEXT("Sorry, Unable to Create your ivy!"), )
		return false;
	}
	

	float gaussian[11] = { 1.0f, 2.0f, 4.0f, 7.0f, 9.0f, 10.0f, 9.0f, 7.0f, 4.0f, 2.0f, 1.0f };
	
	for (int j=0; j<Roots.Num();j++)
	{
		auto Size1 = 0;
		auto root = Roots[j];
		if (!Roots[j])
		{
			ensureMsgf(Roots[j], TEXT("%s Can not Birth root is null "), *GetName());
			continue;
		}
			
		if (!root->Nodes.IsEmpty())
			Size1 = root->Nodes.Num();
		else
			continue;


		for (int g = 0; g < 5; ++g)
		{
			for (int node = 0; node < (Size1) && Size1>0; node++)
			{
				FVector e = FVector(0, 0, 0);

				for (int i = -5; i <= 5; ++i)
				{
					FVector tmpAdhesion = FVector(0, 0, 0);

					if ((node + i) < 0)
					{
						ensureMsgf(root->Nodes[0], TEXT("%s Can not Birth Node 0 is null "), *GetName());
						tmpAdhesion = root->Nodes[0]->AdhesionVector;
					}
					if ((node + i) > Size1)
					{
						ensureMsgf(root->Nodes.Last(), TEXT("%s Can not Birth Last Node is null "), *GetName());
						tmpAdhesion = root->Nodes.Last()->AdhesionVector;
					}
					if (((node + i) >= 0) && ((node + i) < Size1 - 1))
					{
						
						ensureMsgf(root->Nodes[node + i], TEXT("%s Can not Birth Node is null "), *GetName());
						tmpAdhesion = root->Nodes[node + i]->AdhesionVector;
					}

					e += tmpAdhesion * gaussian[i + 5];
				}

				root->Nodes[node]->SmoothAdhesionVector = (e / 56.0f);

			}
			for (int node = 0; node < (Size1) && Size1>0; node++)
			{

				root->Nodes[node]->AdhesionVector = root->Nodes[node]->SmoothAdhesionVector;

			}

		}
	}



	float tempIvySize = (IvySize) * 0.005;
	//parameters that depend on the scene object bounding sphere
	float local_ivyLeafSize = DestinationMesh->GetStaticMeshComponent()
		->GetStaticMesh()->GetBounds().SphereRadius * tempIvySize * IvyLeafSize;



	//create leafs
	
	if (LeafMaterials.Num() > 0)
	{
		UE_LOG(LogCrazyIvy, Log, TEXT("The process of leafs have been started"), )
		for (int j = 0; j < Roots.Num(); j++)
		{
			auto root = Roots[j];
			if (!root)
			{
				ensureMsgf(Roots[j], TEXT("%s Can not Birth leaves root is null "), *GetName());
				continue;
			}

			for (int i = 0; i < DensityMultiplier; ++i)
			{

				for (int node = 0; node < root->Nodes.Num(); node++)
				{
					//weight depending on ratio of node length to total length
					ensureMsgf(root->Nodes[node], TEXT("%s Can not Birth leaves Node is null "), *GetName());

					float weight = pow(root->Nodes[node]->Length / root->Nodes.Last()->Length, 0.7f);
					//test: the probability of leaves on the ground is increased

					float groundIvy = std::max<float>(0.0f, -FVector::DotProduct(FVector(0.0f, 0.0f, 1.0f),
						(root->Nodes[node]->AdhesionVector).GetSafeNormal()));

					weight += groundIvy * pow(1.0f - root->Nodes[node]->Length / root->Nodes.Last()->Length, 2.0f);



					//random influence
					float probability = rand() / (float)RAND_MAX;

					float leafProbability = 1 - LeafDensity;

					if (probability * weight > leafProbability)
					{
						//alignment weight depends on the adhesion "strength"
						float alignmentWeight = root->Nodes[node]->AdhesionVector.Length();


						//horizontal angle (+ an epsilon vector, otherwise there's a problem at 0?and 90?.. mmmh)
						FVector2D temp = FVector2D(-(double)root->Nodes[node]->AdhesionVector.Y, (double)root->Nodes[node]->AdhesionVector.X);

						float phi = vectorToPolar(temp.GetSafeNormal() + getEpsilon()) - PI * 0.5f;


						//vertical angle, trimmed by 0.5
						float theta = getAngle(root->Nodes[node]->AdhesionVector, FVector(0.0f, 0.0f, -1.0f)) * 0.5f;

						//size of leaf
						float  sizeWeight = 1.5f - (cos(weight * 2.0f * PI) * 0.5f + 0.5f);





						//center of leaf quad
						FVector center = root->Nodes[node]->Pos + FMath::VRand() * local_ivyLeafSize * LeafOffset;//

						//random influence
						phi += (rand() / (float)RAND_MAX - 0.5) * (1.3f - alignmentWeight);

						theta += (rand() / (float)RAND_MAX - 0.5) * (1.1f - alignmentWeight) + PI;




						//int matIndex = ComputeRandomLeafMaterialIndex(weight);
						int MeshIndex = FMath::RandRange(0, IvyLeafMeshes.Num() - 1);


						if (IvyLeafMeshes.Num() > 0 && IvyLeafMeshes[MeshIndex] != nullptr)
						{
							float tempSize = (float)((local_ivyLeafSize * sizeWeight) /
								(IvyLeafMeshes[MeshIndex]->GetBounds().BoxExtent.GetMax() * 2));


							FVector tempSizeVector = FVector(tempSize, tempSize, tempSize);


							FRotator tempRotator;
							tempRotator.Roll = FMath::RandRange(0, 0);
							tempRotator.Yaw = FMath::RadiansToDegrees(phi);
							tempRotator.Pitch = FMath::RadiansToDegrees(theta) + FMath::RandRange(-20.0, 20.0);;

							FTransform tempTransform;
							FTransform tmpTransform = FTransform(tempRotator, center, tempSizeVector);
							IvyAddInstanceMesh(MeshIndex, tmpTransform);

						}
						else
							UE_LOG(LogCrazyIvy, Log, TEXT("Leaves Mesh Num is 0   %s "), *GetName())



					}




				}

			}


		}
		UE_LOG(LogCrazyIvy, Log, TEXT("The process of leafs have been done"), )

	}
	//Create Branches

	if (IvyBranchMesh)
	{
		float SectionLength = IvyBranchMesh->GetBounds().BoxExtent.X * 2;
		UE_LOG(LogCrazyIvy, Log, TEXT("The process of branches have been started"), )

		for (int i=0 ;i< Roots.Num() ; i++)
		{
			if (!Roots[i])
			{
				ensureMsgf(Roots[i], TEXT("%s Can not Birth Branches Due To root is null "), *GetName());
				continue;
			}
			
			if (!TempSpline)
			{
				TempSpline = NewObject<USplineComponent>(this);
				TempSpline->SetupAttachment(RootComponent);
				TempSpline->SetHiddenInGame(true);
				TempSpline->SetMobility(EComponentMobility::Movable);
				TempSpline->RegisterComponent();


			}
			TempSpline->ClearSplinePoints();
			
			
			for (auto j=0 ; j< Roots[i]->Nodes.Num();j++)
			{
				ensureMsgf(Roots[i]->Nodes[j], TEXT("%s Can not Birth Branches Due To Node is null "), *GetName());
				TempSpline->AddSplinePoint(Roots[i]->Nodes[j]->Pos, ESplineCoordinateSpace::World, true);
			}

			float Distance = TempSpline->GetSplineLength() / SectionLength;

			int LastIndex = FMath::TruncToInt(Distance);


			TempSpline->Duration = 1.0;
			float StartDistance = 0;

			for (auto j = 0; j <= LastIndex; j++)
			{
				auto* SplineMeshComp = NewObject<USplineMeshComponent>(this);
				SplineMeshComp->SetStaticMesh(IvyBranchMesh);
				SplineMeshComp->SetupAttachment(RootComponent);
				SplineMeshComp->SetMobility(EComponentMobility::Movable);
				SplineMeshComp->RegisterComponent();
				SplineMeshComp->SetVisibility(false, true);
				SplineMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				if (BranchPrimitiveComps.Num() < 50000)
					BranchPrimitiveComps.Add(SplineMeshComp);
				else
				{
						UE_LOG(LogCrazyIvy, Log, TEXT("Number of Mesh components is illegal but It has been doe with legal amount!"))
						TempSpline->ClearSplinePoints();
						return true;
				}
					


				float FloatPart = 0;
				if (j == LastIndex)
				{
					FloatPart = Distance - LastIndex;
				}
				

				FVector StartLocation = TempSpline->GetLocationAtDistanceAlongSpline
				(StartDistance, ESplineCoordinateSpace::Local);

				FVector StartTangent = TempSpline->GetTangentAtDistanceAlongSpline
				(StartDistance, ESplineCoordinateSpace::Local).GetClampedToSize(0, SectionLength);
				///--------------------------------
				float EndDistance = StartDistance +( 1 ) * SectionLength;

				

				FVector EndLocation = TempSpline->GetLocationAtDistanceAlongSpline
				(EndDistance, ESplineCoordinateSpace::Local);

				FVector EndTangent = TempSpline->GetTangentAtDistanceAlongSpline
				(EndDistance, ESplineCoordinateSpace::Local).
					GetClampedToSize(0, SectionLength * (1));


				SplineMeshComp->SetStartAndEnd(StartLocation, StartTangent, EndLocation, EndTangent, true);

	
				

				float tempScale = ComputeTrunksDiameter(Roots[i] , StartDistance);
				
				SplineMeshComp->SetStartScale(FVector2D(tempScale, tempScale), true);

				tempScale = ComputeTrunksDiameter(Roots[i], EndDistance);

				SplineMeshComp->SetEndScale(FVector2D(tempScale, tempScale), true);


				
					StartDistance = EndDistance;
			}


			TempSpline->ClearSplinePoints();
		}
	}
	UE_LOG(LogCrazyIvy, Log, TEXT("Branches process has been done completely"))
	return true;
}

void AIvyGeneratorBase::IvyDeleteAsset(FString InLog,FString InString)
{
	UE_LOG(LogCrazyIvy, Log, TEXT("%s"), *InLog)

	UEditorAssetLibrary::DeleteAsset(InString);
}

//void AIvyGeneratorBase::IvySetLodBuildSettings(UStaticMesh* StaticMesh, const int32 LodIndex, const FMeshBuildSettings& BuildOptions)
//{
//	//UClass* ClassToCreate = UStaticMeshEditorSubsystem::StaticClass();
//	UStaticMeshEditorSubsystem* ClassToCreate = NewObject<UStaticMeshEditorSubsystem>();
//	ClassToCreate->SetLodBuildSettings(StaticMesh, LodIndex,BuildOptions);
//}

float AIvyGeneratorBase::ComputeTrunksDiameter(UIvyRoot* root, float dist)
{

	float tempIvySize = (IvySize) * 0.005;
	float local_ivyBranchSize = DestinationMesh->GetStaticMeshComponent()
		->GetStaticMesh()->GetBounds().SphereRadius * tempIvySize * IvyBranchSize;

		//process only roots with more than one node
	if (root->Nodes.Num() == 1)
		return 0;
	

		//branch diameter depends on number of parents
		float local_ivyBranchDiameter = 1.0f / (float)(root->Parents + 1) + 1.0f;
	
		
		float weight = dist / TempSpline->GetSplineLength();
			
		
		return local_ivyBranchDiameter * local_ivyBranchSize * (1.3f - weight);

}


void AIvyGeneratorBase::SpawnPoints()
{
	
	if (GrowthVal == false)
	{
	
		float TempIvySize = (IvySize) * 0.005;
		Local_IvySize = DestinationMesh->GetStaticMeshComponent()
			->GetStaticMesh()->GetBounds().SphereRadius * TempIvySize;


		Local_MaxFloatLength = DestinationMesh->GetStaticMeshComponent()
			->GetStaticMesh()->GetBounds().SphereRadius * MaxFloatLength;


		//normalize weights of influence
		double sum = PrimaryWeight + RandomWeight + AdhesionWeight;


		auto local_primaryWeight = PrimaryWeight / sum;

		auto local_randomWeight = RandomWeight /sum;

		auto local_adhesionWeight = AdhesionWeight/sum;

		auto Size = Roots.Num();

		for (auto i = 0; i < Size ; i++)
		{
			ensureMsgf(Roots[i], TEXT("%s Can not grow due to root is null "), *GetName());
			
			if (Roots[i]->Nodes.Num() > 0)
			{
			
				if (TotalPoints < MAX_NUMBER && (DeadRootsCount != Roots.Num()))
				{
										
					if (!Roots[i]->Alive) continue;

				
					
						//let the ivy die, if the maximum float length is reached
					if (Roots[i]->Nodes.Last()->FloatingLength > Local_MaxFloatLength)
					{					
						Roots[i]->Alive = false;
						DeadRootsCount++;
						
					}
					
				
						//grow vectors: primary direction, random influence, and adhesion of scene objectss
					
					
						FVector RandomVector = (FMath::VRand()+ FVector(0.0f, 0.0f, 0.2f)).GetSafeNormal();
						
						FVector PrimaryVector = Roots[i]->Nodes.Last()->PrimaryDir;
						
						
						//adhesion influence to the nearest triangle = weighted sum of previous adhesion vectors
						FVector AdhesionVector = ComputeAdhesion(Roots[i]->Nodes.Last()->Pos);

						UIvyNode* TempNode= NewObject<UIvyNode>(this);

						ensureMsgf(TempNode, TEXT("%s Can not grow due to Node is null "), *GetName());
						TempNode->AdhesionVector = AdhesionVector;
						
						
					
						
						FVector GrowVector = Local_IvySize * (PrimaryVector * local_primaryWeight + RandomVector * local_randomWeight + AdhesionVector * local_adhesionWeight);
						
				
						//gravity influence

						//compute gravity vector
						FVector GravityVector = Local_IvySize * FVector(0.0f, 0.0f, -1.0f) * GravityWeight;
						//gravity depends on the floating length
						GravityVector *= pow(Roots[i]->Nodes.Last()->FloatingLength / Local_MaxFloatLength, 0.7f);
						
					
							

						//climbing state of that ivy node, will be set during collision detection
						bool Climbing;

						//compute position of next ivy node
						
						FVector NewPos = Roots[i]->Nodes.Last()->Pos+ GrowVector+ GravityVector;
							
						//combine alive state with result of the collision detection, e.g. let the ivy die in case of a collision detection problem
						Roots[i]->Alive = Roots[i]->Alive & ComputeCollision(Roots[i]->Nodes.Last()->Pos
							, NewPos, Climbing);
							

						//update grow vector due to a changed newPos
						GrowVector = NewPos - Roots[i]->Nodes.Last()->Pos- GravityVector;
											
						float Length = Roots[i]->Nodes.Last()->Length + (NewPos - Roots[i]->Nodes.Last()->Pos).Length();
					
						TempNode->Length = Length;


						TempNode->FloatingLength = Climbing ? 0.0f : Roots[i]->Nodes.Last()->FloatingLength +
								(NewPos - Roots[i]->Nodes.Last()->Pos).Length();
							

						TempNode->Pos = NewPos;
						float TempSizeX = 1;
						float TempSizeY = Local_IvySize * 0.25;
						float TempLength = 1;

						if (Roots[i]->Nodes.Num() > 1 && GrowthBranch)
						{
							
							TempLength = (NewPos - Roots[i]->Nodes.Last()->Pos).Size();

							if (GrowthBranch->GetStaticMesh())
							{
								TempSizeX = TempLength / (GrowthBranch->GetStaticMesh()->GetBounds().BoxExtent.X * 2);

							}

							FVector TempSizeVector = FVector(TempSizeX, TempSizeY, TempSizeY);

							FRotator TempRotator= UKismetMathLibrary::FindLookAtRotation( NewPos, Roots[i]->Nodes.Last()->Pos);
							
	

							FTransform TempTransform = FTransform(TempRotator, NewPos, TempSizeVector);
							GrowthBranch->AddInstance(TempTransform,true);

						}
						if (GrowthNode)
						{
							float TempS = TempSizeY * (GrowthBranch->GetStaticMesh()->GetBounds().BoxExtent.Y * 2) / (GrowthNode->GetStaticMesh()->GetBounds().BoxExtent.X * 2);
							FVector TempSizeVector = FVector(TempS, TempS, TempS);
							FRotator TempRotator(0, 0, 0);
							FTransform TempTransform = FTransform(TempRotator, NewPos, TempSizeVector);
							GrowthNode->AddInstance(TempTransform, true);
						}
						
					
					
						
					
						TotalPoints++;
					

						NumberOfPoints[i] = Roots[i]->Nodes.Num();		
					
						TempNode->PrimaryDir = (0.5f* Roots[i]->Nodes.Last()->PrimaryDir + 0.5f * GrowVector.GetSafeNormal()).GetSafeNormal();

						TempNode->Climb = Climbing;
						Roots[i]->Nodes.Emplace(TempNode);

				}
				else
				{
				Roots[i]->Alive = false;

				StopGrowth();

				UE_LOG(LogCrazyIvy, Log, TEXT("Preview Growth Has been done compeletly  %s "), *GetName())
				return;
				}
			}
		}
		
		for (int j=0;j< Roots.Num();j++)
		{
			auto root = Roots[j];
			if (!root)
			{
				ensureMsgf(Roots[j], TEXT("%s Can not add branch due to root is null "), *GetName());
				continue;
			}

			if (!root->Alive) continue;

			if (root->Parents > 3) continue;
			
			const auto Size1 = root->Nodes.Num();

			for (auto i = 0; i < Size1; i++)
			{
				//weight depending on ratio of node length to total length
				float weight = 1.0f - (cos(root->Nodes[i]->Length
					/ root->Nodes.Last()->Length* 2.0f * PI) * 0.5f + 0.5f);
				
				
				//random influence
				float probability = rand() / (float)RAND_MAX;
				

				if (probability * weight > 1-BranchingProbability )
				{
					

					UIvyRoot* TempRoot = NewObject<UIvyRoot>(this);
					ensureMsgf(TempRoot, TEXT("%s Can not add branch due to root is null "), *GetName());

					FVector TempLocation= root->Nodes[i]->Pos;

					UIvyNode* TempNode= NewObject<UIvyNode>(this);
					ensureMsgf(TempNode, TEXT("%s Can not add branch Node is null "), *GetName());
					
					TempNode->Pos = TempLocation;

					
					TempNode->Length= 0 ;
					TempNode->AdhesionVector= FVector(0, 0, 0);


					TempNode->Climb = true;
					
					TempNode->FloatingLength = root->Nodes[i]->FloatingLength;
					
					NumberOfPoints.Add(0);


					TempRoot->Alive = true;

					TempRoot->Parents = root->Parents + 1;
					TempRoot->Nodes.Add(TempNode);

					Roots.Add(TempRoot);


					
					return;
				}
			
			}

			
		}

	}
}






