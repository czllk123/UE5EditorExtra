// Fill out your copyright notice in the Description page of Project Settings.


#include "AddToInstance.h"
#include "AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "UObject/UObjectGlobals.h"
#include "FoliageEditModule.h"
#include "UObject/Package.h"
#include "Misc/Guid.h"
#include "InstancedFoliageActor.h"
#include "IContentBrowserSingleton.h"
#include "Engine/StaticMesh.h"
#include "InstancedFoliageActor.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "Components/ModelComponent.h"
#include "Engine/Brush.h"
#include "Components/PrimitiveComponent.h"
#include "Components/BrushComponent.h"
#include "Engine/LevelStreamingVolume.h"
#include "Engine/LevelBounds.h"



/*
AInstancedFoliageActor* UAddToInstance::GetOrCreateIFA()
{
	//获取当前关卡
	ULevel* CurrentLevel = GWorld->GetCurrentLevel();

	//获取当前关卡中的IFA, 如果没有则自动创建一个
	AInstancedFoliageActor* InstancedFoliageActor = AInstancedFoliageActor::GetInstancedFoliageActorForLevel(CurrentLevel, true);
	return InstancedFoliageActor;
	
}
*/



bool UAddToInstance::AddToFoliageInstance(const UObject* WorldContextObject,UStaticMesh* InStaticMesh, int32 StaticMeshIndex, FTransform Transform,  FString SavePath, TMap<AInstancedFoliageActor*, FGuid>& FoliageUUIDs)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	
	FVector StartLocation = Transform.GetLocation()+FVector(0.0f,0.0f,0.01f);
	FVector EndLocation = StartLocation - FVector(0.0f, 0.0f, 90000.0f);
	FCollisionQueryParams CollisionParams;
	CollisionParams.bTraceComplex = true;
	FHitResult Hit;

	ULevel* CurrentLevel = nullptr; // 提前声明CurrentLevel变量
	UPrimitiveComponent* BaseComp = nullptr;
	FVector HitLocation;
	
	bool bHit = World->LineTraceSingleByChannel(Hit, StartLocation, EndLocation, ECC_Visibility, CollisionParams,FCollisionResponseParams());
	if(bHit)
	{
		BaseComp = Hit.Component.Get();
		//CurrentLevel = Hit.GetActor()->GetLevel();
		CurrentLevel = BaseComp->GetComponentLevel();
		
		HitLocation = Hit.ImpactPoint;
		//UE_LOG(LogTemp,Warning,TEXT("当前关卡为：%s"),*CurrentLevel->GetName());
	}
	else
	{
		UE_LOG(LogTemp,Error,TEXT("当前射线未击中任何物体！！！"));
	}
	DrawDebugLine(World,StartLocation,EndLocation,FColor::Red,false,5.0f,0,5.0f);
	
	const TArray<ULevelStreaming*>& StreamedLevels = World->GetStreamingLevels();


	ULevel* LevelOfHitActor = nullptr;

	for (const ULevelStreaming* EachLevelStreaming : StreamedLevels)
	{
		if(!EachLevelStreaming) 
		{
			continue;
		}
	
		ULevel* EachLevel =  EachLevelStreaming->GetLoadedLevel();
	
		//Is This Level Valid and Visible?
		if(!EachLevel || !EachLevel->bIsVisible) 
		{
			continue;
		}
		 
		//Is the Hit Location Within this Level's Bounds?
		if(ALevelBounds::CalculateLevelBounds(EachLevel).IsInside(HitLocation))
		{
			LevelOfHitActor = EachLevel;
			CurrentLevel = EachLevel;
			UE_LOG(LogTemp,Warning,TEXT("当前关卡为：%s"),*CurrentLevel->GetName());
			break;
		}
	}

	
	
	AInstancedFoliageActor* InstancedFoliageActor = AInstancedFoliageActor::GetInstancedFoliageActorForLevel(CurrentLevel, true);
	//如果IFA不存在或者无效，返回false
	if(!InstancedFoliageActor || !IsValid(InstancedFoliageActor))
	{
		UE_LOG(LogTemp,Error,TEXT("Can not find InstancedFoliageActor!!!"))
		return false;
	}

///////////////////////////这一块写的比较乱////////////////////////////////////////
///首先先去StaticMesh对应的文件夹里面找是否存在之前创建的FoliageType文件，如果存在就直接用，
///对应的路径有两种，一种是自定义路径，第二种是默认的路径为StaticMesh所在路径
///如果两个文件夹都没有对应StaticMesh_FoliageType文件，就使用InstancedFoliageActor->GetAllFoliageTypesForSource查找
///如果还是没找到就新建一个FoliageType
////////////////////////////////////////////////////////////////////////////////////

	FGuid FoliageInstanceGuid;
	FoliageInstanceGuid = FGuid::NewGuid();

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
		UPackage* NewPackage  = CreatePackage(nullptr, *PackagePath);
		
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
			bool const bSaved = UPackage::SavePackage(NewPackage, FoliageTypeAsset, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *FilePath);

		}
	}
	
	else
	{
		//如果StaticMesh指向的FoliageType已经存在或者存在多个，那就用第一个。
		FoliageType = const_cast<UFoliageType*>(FoliageTypes[0]);
	}

	//设置FoliageType相关属性
	//FoliageType->SetSource(InStaticMesh);
	//InstancedFoliageActor->AddFoliageType(FoliageType);
	FFoliageInfo* FoliageInfo = InstancedFoliageActor->FindInfo(FoliageType);

	
	if (!FoliageInfo)
	{
		// 添加新的FoliageType到InstancedFoliageActor中，并返回对应的FoliageInfo

		FoliageType->SetSource(InStaticMesh);
		InstancedFoliageActor->AddFoliageType(FoliageType);
		FoliageInfo = InstancedFoliageActor->FindInfo(FoliageType);
		UE_LOG(LogTemp, Warning, TEXT("No FFoliageInfo found for this UFoliageType."));
		
	}

	//如果找到了FoliageInfo
	if(FoliageInfo)
	{
		
		FFoliageInstance* FoliageInstance = new FFoliageInstance;
		FoliageInfo->AddInstance(FoliageType, *FoliageInstance);

		if(FoliageInstance)
		{
			//Foliage Instance Index
			const int32 FoliageInstanceIndex = FoliageInfo->Instances.Num() - 1;

			//Get The New Foliage Instance
			FoliageInstance = &FoliageInfo->Instances[FoliageInstanceIndex];


			FoliageInfo->PreMoveInstances({FoliageInstanceIndex});

			
			//FoliageInstance->Location = Transform.GetLocation();

			//此处调用CheckInstanceLocationOverlap函数，检查在容差范围内是否有相同的实例要放置
			//现在先用这种方法检测，之后用SphereOverlapActors()
			/*
			FVector InstanceLocation = Transform.GetLocation();
			
			//设置位置检测容差值
			float Tolerance = 10.0f;
			if(CheckInstanceLocationOverlap(FoliageInfo, InstanceLocation, Tolerance))
			{
				// 位置已经存在实例，不要添加新实例
				UE_LOG(LogTemp,Error,TEXT("Instance already exists at the current location, No need to add !!!"))
				return false;
			}
			*/

			
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

			return true;
			
			
		}
		//Refresh Foliage Editor
		//IFoliageEditModule& FoliageEditModule = FModuleManager::LoadModuleChecked<IFoliageEditModule>("FoliageEdit");
		//FoliageEditModule.UpdateMeshList();
		
	
		
	}
	return false;

}


bool UAddToInstance::RemoveFoliageInstance(TMap<AInstancedFoliageActor*, FGuid> FoliageUUIDs)
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
        AInstancedFoliageActor* InstancedFoliageActor = FoliagePair.Key;
        FGuid FoliageInstanceUUID = FoliagePair.Value;

        //如果IFA不存在或者无效，返回false
        if(!InstancedFoliageActor || !IsValid(InstancedFoliageActor))
        {
            UE_LOG(LogTemp,Error,TEXT("Can not find InstancedFoliageActor!!!"))
            continue;
        }

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

TArray<int32> UAddToInstance::CalculateWeightAverage(const TArray<float>& Weights, int32 OutputSize)
{
	TArray<int32> result;

	// 计算权重总和
	float sum_weights = 0.0f;
	for (const float weight : Weights) {
		sum_weights += weight;
	}

	// 根据权重随机生成索引
	for (int32 i = 0; i < OutputSize; i++) {
		float rand_weight = FMath::RandRange(0.0f, sum_weights);
		float accum_weight = 0.0f;

		for (int32 j = 0; j < Weights.Num(); j++) {
			accum_weight += Weights[j];

			if (rand_weight <= accum_weight) {
				result.Add(j);
				break;
			}
		}
	}

	return result;
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



