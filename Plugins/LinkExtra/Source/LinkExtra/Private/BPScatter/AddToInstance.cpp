// Fill out your copyright notice in the Description page of Project Settings.


#include "AddToInstance.h"

#include "UObject/UObjectGlobals.h"
#include "UObject/Package.h"
#include "Misc/Guid.h"
#include "InstancedFoliageActor.h"
#include "Engine/StaticMesh.h"
#include "Components/PrimitiveComponent.h"



bool UAddToInstance::AddToFoliageInstance(const UObject* WorldContextObject, TArray<AActor*> ActorsToIgnore, FGuid FoliageInstanceGuid,  UFoliageType* InFoliageType, FTransform Transform, TMap<TSoftObjectPtr<AInstancedFoliageActor>, FGuid>& FoliageUUIDs)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	//如果有物体全部陷入地表，是无法转foliage成功的
	UStaticMesh* InStaticMesh = Cast<UStaticMesh>(InFoliageType->GetSource());
	float TreeBoundsZ = InStaticMesh->GetBoundingBox().Max.Z * Transform.GetScale3D().Z;
	//UE_LOG(LogTemp,Warning,TEXT("TreeHeight是 ： %f"),TreeBoundsZ)
	FVector StartLocation = Transform.GetLocation()+FVector(0.0f,0.0f,TreeBoundsZ);
	FVector EndLocation = StartLocation - FVector(0.0f, 0.0f, 90000.0f);
	FCollisionQueryParams CollisionParams;
	CollisionParams.bTraceComplex = true;
	CollisionParams.AddIgnoredActors(ActorsToIgnore);
	
	FHitResult Hit;

	ULevel* CurrentLevel = nullptr; 
	UPrimitiveComponent* BaseComp = nullptr;

	
	bool bHit = World->LineTraceSingleByChannel(Hit, StartLocation, EndLocation, ECC_WorldStatic, CollisionParams,FCollisionResponseParams());
	if(bHit)
	{
		BaseComp = Hit.Component.Get();
		CurrentLevel = Hit.GetActor()->GetLevel();
	}
	else
	{
		UE_LOG(LogTemp,Error,TEXT("当前射线未击中任何物体！！！"));
	}
	//DrawDebugLine(World,StartLocation,EndLocation,FColor::Red,false,5.0f,0,5.0f);
	
	AInstancedFoliageActor* InstancedFoliageActor = AInstancedFoliageActor::GetInstancedFoliageActorForLevel(CurrentLevel, true);
	
	//如果IFA不存在或者无效，返回false
	if(!InstancedFoliageActor || !IsValid(InstancedFoliageActor))
	{
		UE_LOG(LogTemp,Error,TEXT("Can not find InstancedFoliageActor!!!"))
		return false;
	}
/*
///////////////////////////////////////////////////////////////////
///首先先去StaticMesh对应的文件夹里面找是否存在之前创建的FoliageType文件，如果存在就直接用，
///对应的路径有两种，一种是自定义路径，第二种是默认的路径为StaticMesh所在路径
///如果两个文件夹都没有对应StaticMesh_FoliageType文件，就使用InstancedFoliageActor->GetAllFoliageTypesForSource查找
///如果还是没找到就新建一个FoliageType
////////////////////////////////////////////////////////////////////////////////////

	//FGuid FoliageInstanceGuid;
	//FoliageInstanceGuid = FGuid::NewGuid();

	//打印当前植被的UUID
	//UE_LOG(LogTemp, Warning, TEXT("FoliageInstaceGuid is : %s"), *FoliageInstaceGuid.ToString());

	//获取给定的StaticMesh名称作为FoliageType名称
	const FString FoliageTypeName = FString::Format(TEXT("{0}_FoliageType"),{InStaticMesh->GetName()});
	
	// 设置资产保存的目标路径
	//FString Path = TEXT("/Game/MyFoliageTypes/"); // 自定义保存路径
	FString AssetPath;
	//SavePath是传进来的参数,如果为空，直接使用static mesh 的路径
	if(SavePath.IsEmpty())
	{
		//AssetPath = TEXT("/Game/FoliageTypes/");
		FString MeshPath = FPaths::GetPath(InStaticMesh->GetPathName())+"/";
		AssetPath = MeshPath;
	}
	else
	{
		AssetPath = SavePath;
	}
	FString AssetName = FoliageTypeName;
	//FoliageType的相对路径,这个只用来新建Object,保存FolaigeType还是得用绝对路径
	FString PackagePath = AssetPath + AssetName;
		
	//当前项目Content目录的绝对路径
	FString ProjectContentPath = FPaths::ProjectContentDir();
		
	//减去“/Game/”后的AssetPath剩余路径
	FString SubPath = AssetPath.RightChop(6); // 6 is the length of "/Game/"

	//拼接完成后储存FoliageType的绝对路径
	FString FullPath = FPaths::Combine(ProjectContentPath, SubPath);
		
	//删除多于斜杠 //content->/content
	FPaths::RemoveDuplicateSlashes(FullPath);
	

	
	//获取给定static mesh 所有的Foliage Type,这里的FoliageType 类型必须是UFoliageType，不能是 UFoliageType_InstancedStaticMesh，要不然下面那个函数用不了
	TArray<const UFoliageType*> FoliageTypes;
	//通过ifa来查询StaticMesh对应的FoliageType
	InstancedFoliageActor->GetAllFoliageTypesForSource(InStaticMesh, FoliageTypes);

	UFoliageType* FoliageType;

	// 检查 AssetPath 下是否存在以 InStaticMesh 命名的 FoliageType
	FString FullFoliageTypePath = AssetPath + FoliageTypeName;
	UFoliageType* FoundFoliageType = LoadObject<UFoliageType>(nullptr, *FullFoliageTypePath);

	if (FoundFoliageType)
	{
		// 存在以 InStaticMesh 命名的 FoliageType，直接使用它
		FoliageType = FoundFoliageType;
	}
	
	//如果给定的StaticMesh没已有关联的FoliageType,则新建一个。
	else if (FoliageTypes.Num()==0)
	{
		//这个新建的FoliageType对象的Outer为InstancedFoliageActor，用于实例化植被
		FoliageType = NewObject<UFoliageType_InstancedStaticMesh>(InstancedFoliageActor, *FoliageTypeName,RF_Public | RF_Standalone);
		//这个地方别对这个foliageType做任何更改，只是将他创建出来用来撒点，最后将这个保存成foliageType文件，然后对它做相应的设置
		
		//创建一个新的空的资源包
		UPackage* NewPackage  = CreatePackage(*PackagePath);
		
		// 加载资源包到内存
		NewPackage->FullyLoad();
		
		if (!FindObject<UObject>(NewPackage, *AssetName ))
		{
			//DuplicateObject函数创建一个新的 UFoliageType 对象，它将复制源 FoliageType 对象的属性和值，并且将outer更改为AssetPackage用于保存上面新建的FoliageType文件，在这个地方对FoliageType进行设置
			UFoliageType* FoliageTypeAsset = DuplicateObject<UFoliageType>(FoliageType, NewPackage, *AssetName);
			//设置FoliageType相关属性
			FoliageTypeAsset->SetSource(InStaticMesh);
			InstancedFoliageActor->AddFoliageType(FoliageTypeAsset);

			FAssetRegistryModule::AssetCreated(FoliageTypeAsset);
			if (FoliageTypeAsset)
			{
				bool bIsMarkedDirty = FoliageTypeAsset->MarkPackageDirty();

				if (!bIsMarkedDirty)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to mark FoliageTypeAsset as dirty."));
				}
			}
			// 保存资产
			FString FilePath = FString::Printf(TEXT("%s%s%s"), *FullPath, *AssetName, *FPackageName::GetAssetPackageExtension());
			FSavePackageArgs SaveArgs;
			SaveArgs.TopLevelFlags = EObjectFlags::RF_Public | EObjectFlags::RF_Standalone;
			SaveArgs.Error = GLog;
			UPackage::SavePackage(NewPackage, FoliageTypeAsset, *FilePath, SaveArgs);

		}
	}
	
	else
	{
		//如果StaticMesh指向的FoliageType已经存在或者存在多个，那就用第一个。
		FoliageType = const_cast<UFoliageType*>(FoliageTypes[0]);
	}
*/



	
	//设置FoliageType相关属性
	InFoliageType->SetSource(InStaticMesh);
	InstancedFoliageActor->AddFoliageType(InFoliageType);
	FFoliageInfo* FoliageInfo = InstancedFoliageActor->FindInfo(InFoliageType);


	//如果找到了FoliageInfo
	if(FoliageInfo)
	{
		
		FFoliageInstance* FoliageInstance = new FFoliageInstance;
		FoliageInfo->AddInstance(InFoliageType, *FoliageInstance);

		if(FoliageInstance)
		{
			//Foliage Instance Index
			const int32 FoliageInstanceIndex = FoliageInfo->Instances.Num() - 1;

			//Get The New Foliage Instance
			FoliageInstance = &FoliageInfo->Instances[FoliageInstanceIndex];


			FoliageInfo->PreMoveInstances({FoliageInstanceIndex});

			
			// 设置Foliage Instance的位置、旋转和缩放
			FoliageInstance->Location = Transform.GetLocation();
			FoliageInstance->Rotation = Transform.GetRotation().Rotator();
			const FVector Scale = Transform.GetScale3D();
			FoliageInstance->DrawScale3D = FVector3f(Scale.X,Scale.Y,Scale.Z);

			FoliageInstance->BaseComponent = BaseComp;

			// Set new base
			auto NewBaseId = InstancedFoliageActor->InstanceBaseCache.AddInstanceBaseId(FoliageInfo->ShouldAttachToBaseComponent() ? BaseComp : nullptr);
			FoliageInfo->RemoveFromBaseHash(FoliageInstanceIndex);
			FoliageInstance->BaseId = NewBaseId;
			if (FoliageInstance->BaseId == FFoliageInstanceBaseCache::InvalidBaseId)
			{
				FoliageInstance->BaseComponent = nullptr;
			}
			FoliageInfo->AddToBaseHash(FoliageInstanceIndex);
			
			
			//上面通过传入FoliageInstanceGuid传到ProceduralGuid
			FoliageInstance->ProceduralGuid = FoliageInstanceGuid;
			
			//Need To Be Called After Instance Is Moved
			FoliageInfo->PostMoveInstances({ FoliageInstanceIndex });
			
			//输出植被的UUID
			FoliageUUIDs.Add(InstancedFoliageActor, FoliageInstanceGuid);

			//刷新新指定的 InstancedFoliageActor 中的植被信息。
			FoliageInfo->Refresh(true, true);
			FoliageInfo->ClearSelection();

			return true;
			
		}
		
	}
	return false;

}


bool UAddToInstance::RemoveFoliageInstance(TMap<TSoftObjectPtr<AInstancedFoliageActor>, FGuid> FoliageUUIDs)
{
    //如果没有任何FoliageUUIDs，返回false
    if(FoliageUUIDs.Num() == 0)
    {
        UE_LOG(LogTemp,Error,TEXT("FoliageUUIDs is empty!!!"))
        return false;
    }
    
    //遍历所有的FoliageUUIDs
    for(auto& FoliagePair : FoliageUUIDs)
    {
    	TSoftObjectPtr<AInstancedFoliageActor> SoftIFA = FoliagePair.Key;
        FGuid FoliageInstanceUUID = FoliagePair.Value;

        //如果IFA不存在或者无效，返回false
        if(!SoftIFA.IsValid() || !SoftIFA.Get())
        {
            UE_LOG(LogTemp,Error,TEXT("Can not find InstancedFoliageActor!!!"))
            continue;
        }

    	AInstancedFoliageActor* InstancedFoliageActor = SoftIFA.Get();
        //遍历所有的FoliageInfos
        for (auto& FoliageInfoPair:InstancedFoliageActor->GetFoliageInfos())
        {
            // 获取 FoliageInfo 引用 , 将 const 引用转换为非 const 引用
            FFoliageInfo& FoliageInfo =const_cast<FFoliageInfo&>(FoliageInfoPair.Value.Get());
            // 创建一个用于存储要删除的实例索引的数组
            TArray<int32> InstanceIndicesToRemove;

            //遍历所有的Foliage实例，检查他们的UUID是否等于要删除的UUID
            for (int32 i = 0; i<FoliageInfo.Instances.Num(); i++)
            {
                // 遍历 FoliageInfo 中的所有实例
                FGuid InstanceUUID = FoliageInfo.Instances[i].ProceduralGuid;
                // 检查 UUID 是否等于要删除的 FoliageInstanceUUID 
                if(InstanceUUID == FoliageInstanceUUID)
                {
                    //如果找到要删除的实例，将其索引添加到要删除的实例索引数组中
                    InstanceIndicesToRemove.Add(i);
                }
            }
            //对找到要删除的索引，按逆序对他们进行排序，以便在删除时不会影响其他实例的索引
            InstanceIndicesToRemove.Sort([](const int32& A, const int32& B) {return A > B; });

            //遍历要删除实例的索引数组，从FoliageInfo中删除他们
            for (int32 IndexToRemove : InstanceIndicesToRemove )
            {
                FoliageInfo.RemoveInstances(TArray<int32> {IndexToRemove} ,true);
            }
            FoliageInfo.Refresh(true, true);
        }
    }

    return true;
}

TArray<int32> UAddToInstance::CalculateWeightAverage(const TArray<float>& Weights, int32 MeshCount)
{
	TArray<int32> Result;

	// 计算权重总和
	float Sum_Weights = 0.0f;
	for (const float Weight : Weights) {
		Sum_Weights += Weight;
	}

	// 根据权重随机生成索引
	for (int32 i = 0; i < MeshCount; i++) {
		float const Rand_Weight = FMath::RandRange(0.0f, Sum_Weights);
		float Accum_Weight = 0.0f;

		for (int32 j = 0; j < Weights.Num(); j++) {
			Accum_Weight += Weights[j];

			if (Rand_Weight <= Accum_Weight) {
				Result.Add(j);
				break;
			}
		}
	}

	return Result;
}

TArray<int32> UAddToInstance::CalculateWeightAverageWithStream(const TArray<float>& Weights, int32 MeshCount, const FRandomStream& Stream)
{
	TArray<int32> Result;

	// 计算权重总和
	float Sum_Weights = 0.0f;
	for (const float Weight : Weights) {
		Sum_Weights += Weight;
	}

	// 根据权重随机生成索引
	for (int32 i = 0; i < MeshCount; i++) {
		float const Rand_Weight = Stream.FRandRange(0.0f, Sum_Weights);
		float Accum_Weight = 0.0f;

		for (int32 j = 0; j < Weights.Num(); j++) {
			Accum_Weight += Weights[j];

			if (Rand_Weight <= Accum_Weight) {
				Result.Add(j);
				break;
			}
		}
	}

	return Result;
}

// 检查是否已经存在具有相同位置的实例
bool UAddToInstance::CheckInstanceLocationOverlap( FFoliageInfo* FoliageInfo,  FVector Location, float Tolerance)
{
	for ( FFoliageInstance& Instance : FoliageInfo->Instances)
	{
		if (FVector::DistSquared(Instance.Location, Location) <= Tolerance * Tolerance)
		{
			return true;
		}
	}
	return false;
}

//从FoliageType中拿到StaticMesh 和 重载的材质
UStaticMesh* UAddToInstance::OverrideMaterialsFromFT(UFoliageType* InputFoliageType)
{
	if(InputFoliageType == nullptr)
	{
		return nullptr;
	}

	//类型强转
	UFoliageType_InstancedStaticMesh* FTISM = Cast<UFoliageType_InstancedStaticMesh>(InputFoliageType);
	
	if (FTISM == nullptr)
	{
		return nullptr;
	}

	//获取StaticMesh
	 UStaticMesh* Mesh = FTISM->GetStaticMesh();
	
	if (Mesh == nullptr)
	{
		return nullptr;
	}

	//申明空指针
	UStaticMesh *NewMesh = nullptr;

	// 判断 OverrideMaterials 数组是否包含至少一个非 nullptr 元素。
	if(FTISM->OverrideMaterials.Num()>0)
	{
		bool bHasNonNullOverrideMaterial = false;
		for(auto& material : FTISM -> OverrideMaterials)
		{
			if( material != nullptr)
			{
				bHasNonNullOverrideMaterial = true;
				break;
			}
		}
		// 如果有非 nullptr 的重载材质，则复制 Mesh 并应用新材质。
		if(bHasNonNullOverrideMaterial)
		{
			NewMesh = DuplicateObject<UStaticMesh>(Mesh, GetTransientPackage());
			for(int32 i = 0; i < FTISM->OverrideMaterials.Num(); i++)
			{
				if(FTISM->OverrideMaterials[i] != nullptr)
				{
					NewMesh->GetStaticMaterials()[i].MaterialInterface = FTISM->OverrideMaterials[i];
				}
			}
		}
	}
	// 如果 NewMesh 不是 nullptr，则返回 NewMesh，否则返回原始 Mesh。
	return NewMesh != nullptr ? NewMesh : Mesh;
}

TMap<UMaterialInterface*, UStaticMesh*> UAddToInstance::GetOverrideResourceFromFT(UFoliageType* InputFoliageType)
{
	TMap<UMaterialInterface*, UStaticMesh*> OverrideResources;
	
	if(InputFoliageType == nullptr)
	{
		return OverrideResources;
	}

	UFoliageType_InstancedStaticMesh* FTISM = Cast<UFoliageType_InstancedStaticMesh>(InputFoliageType);
	if(FTISM == nullptr)
	{
		return OverrideResources;
	}

	//获取FoliageType中的StaticMesh
	const UStaticMesh* Mesh = FTISM->GetStaticMesh();

	if(Mesh == nullptr)
	{
		return OverrideResources;
	}

	if(FTISM->OverrideMaterials.Num()> 0)
	{
		for(int32 i=0; i<FTISM->OverrideMaterials.Num() && i<Mesh->GetStaticMaterials().Num(); i++)
		{
			UMaterialInterface* OverrideMaterial = FTISM->OverrideMaterials[i];
			// 检查OverrideMaterial是否为空 同时确保遍历material时不会造成数组越界
			if(OverrideMaterial == nullptr && Mesh->GetStaticMaterials().Num()>i)
			{
				// 使用StaticMesh的默认材质
				OverrideMaterial = Mesh->GetStaticMaterials()[i].MaterialInterface;
			}
			// 用选定的材质（覆盖或默认）更新TMap
			if(OverrideMaterial != nullptr)
			{
				OverrideResources.Add(OverrideMaterial, const_cast<UStaticMesh*>(Mesh));
			}
		}
			
	}
	return OverrideResources;
}





