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

bool UAddToInstance::AddToFoliageInstance(const UObject* WorldContextObject,UStaticMesh* InStaticMesh, int32 StaticMeshIndex, FTransform Transform,  FString SavePath, TMap<int32, FGuid>& FoliageUUIDs)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	
	FVector StartLocation = Transform.GetLocation()+FVector(0.0f,0.0f,0.01f);
	FVector EndLocation = StartLocation - FVector(0.0f, 0.0f, 9000.0f);
	FCollisionQueryParams CollisionParams;
	CollisionParams.bTraceComplex = true;
	FHitResult Hit;

	ULevel* CurrentLevel = nullptr; // 提前声明CurrentLevel变量
	UPrimitiveComponent* BaseComp = nullptr;
	
	bool bHit = World->LineTraceSingleByChannel(Hit, StartLocation, EndLocation, ECC_Visibility, CollisionParams,FCollisionResponseParams());
	if(bHit)
	{
		CurrentLevel = Hit.GetActor()->GetLevel();
		BaseComp = Hit.Component.Get();
	}
	else
	{
		UE_LOG(LogTemp,Error,TEXT("当前射线未击中任何物体！！！"));
	}
	DrawDebugLine(World,StartLocation,EndLocation,FColor::Red,false,5.0f,0,5.0f);
	
	AInstancedFoliageActor* InstancedFoliageActor = AInstancedFoliageActor::GetInstancedFoliageActorForLevel(CurrentLevel, true);
	//如果IFA不存在或者无效，返回false
	if(!InstancedFoliageActor || !IsValid(InstancedFoliageActor))
	{
		UE_LOG(LogTemp,Error,TEXT("Can not find InstancedFoliageActor!!!"))
		return false;
	}


	FGuid FoliageInstaceGuid;
	FoliageInstaceGuid = FGuid::NewGuid();
	
	//打印当前植被的UUID
	//UE_LOG(LogTemp, Warning, TEXT("FoliageInstaceGuid is : %s"), *FoliageInstaceGuid.ToString());
	
	//获取给定static mesh 所有的Foliage Type,这里的FoliageType 类型必须是UFoliageType，不能是 UFoliageType_InstancedStaticMesh，要不然下面那个函数用不了
	TArray<const UFoliageType*> FoliageTypes;
	InstancedFoliageActor->GetAllFoliageTypesForSource(InStaticMesh, FoliageTypes);

	//如果给定的StaticMesh没已有关联的FoliageType,则新建一个。
	UFoliageType* FoliageType;
	if(FoliageTypes.Num()==0)
	{
		//获取给定的StaticMesh名称作为FoliageType名称
		const FString FoliageTypeName = FString::Format(TEXT("{0}_FoliageType"),{InStaticMesh->GetName()});
		//这个新建的FoliageType对象的Outer为InstancedFoliageActor，用于实例化植被
		FoliageType = NewObject<UFoliageType_InstancedStaticMesh>(InstancedFoliageActor, *FoliageTypeName,RF_Public | RF_Standalone);
		//这个地方别对这个foliageType做任何更改，只是将他创建出来用来撒点，最后将这个保存成foliageType文件，然后对它做相应的设置

		
		// 设置资产保存的目标路径
		//FString Path = TEXT("/Game/MyFoliageTypes/"); // 自定义保存路径
		FString AssetPath;
		//SavePath是传进来的参数
		if(SavePath.IsEmpty())
		{
			AssetPath = TEXT("/Game/FoliageTypes/");
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

		
		//创建一个新的空的资源包
		UPackage* NewPackage  = CreatePackage(nullptr, *PackagePath);
		
		// 加载资源包到内存
		NewPackage->FullyLoad();
		
		// 检查资产是否已存在，如果已存在，则什么都不做
		if (FindObject<UObject>(NewPackage, *AssetName ))
		{
			return false;
		}
		
		//DuplicateObject函数创建一个新的 UFoliageType 对象，它将复制源 FoliageType 对象的属性和值，并且将outer更改为AssetPackage用于保存上面新建的FoliageType文件，在这个地方对FoliageType进行设置
		UFoliageType* FoliageTypeAsset = DuplicateObject<UFoliageType>(FoliageType, NewPackage, *AssetName);
		
		//设置FoliageType相关属性
		FoliageTypeAsset->SetSource(InStaticMesh);
		InstancedFoliageActor->AddFoliageType(FoliageTypeAsset);

		FAssetRegistryModule::AssetCreated(FoliageTypeAsset);
		
		//标脏要设置给具体的Asset, 上面新建的NewPackage到“DuplicateObject”函数后就没用了
		if (FoliageTypeAsset != nullptr)
		{
			// 标记资源包为"脏"
			bool bIsMarkedDirty = FoliageTypeAsset->MarkPackageDirty();

			if (bIsMarkedDirty)
			{
				UE_LOG(LogTemp, Log, TEXT("FoliageTypeAsset has been successfully marked as dirty."));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to mark FoliageTypeAsset as dirty."));
			}
		}
		
		// 保存资产
		FString FilePath = FString::Printf(TEXT("%s%s%s"), *FullPath, *AssetName, *FPackageName::GetAssetPackageExtension());
		bool const bSaved = UPackage::SavePackage(NewPackage, FoliageTypeAsset, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *FilePath);
		
		/*
		const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		
		if (bSaved)
		{
			// 通知资产注册表有关新资产的信息
			AssetRegistryModule.Get().AddPath(AssetPath);
			
			// Scan the asset files in the specified path to update the registry
			TArray<FString> ScanPaths;
			ScanPaths.Add(AssetPath);
			AssetRegistryModule.Get().ScanPathsSynchronous(ScanPaths);

			AssetRegistryModule.Get().AssetCreated(FoliageTypeAsset);
		}
		// After saving the asset and updating the asset registry
		const FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToAssets(TArray<FAssetData>{ FAssetData(FoliageTypeAsset) });
		*/
	}
	else
	{
		//如果StaticMesh指向的FoliageType已经存在或者存在多个，那就用第一个。
		FoliageType = const_cast<UFoliageType*>(FoliageTypes[0]);
	}

	// 查找Foliage Type对应的Foliage Info
	FFoliageInfo* FoliageInfo = InstancedFoliageActor->FindInfo(FoliageType);

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
			
			
			//上面通过传入FoliageInstaceGuid传到ProceduralGuid
			FoliageInstance->ProceduralGuid = FoliageInstaceGuid;
			
			//Need To Be Called After Instance Is Moved
			FoliageInfo->PostMoveInstances({ FoliageInstanceIndex });
			
			//输出植被的UUID
			FoliageUUIDs.Add(StaticMeshIndex, FoliageInstaceGuid);

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

bool UAddToInstance::RemoveFoliageInstance(AInstancedFoliageActor* InstancedFoliageActor, TArray<FGuid> FoliageInstanceUUID)
{
	//如果IFA不存在或者无效，返回false
	if(!InstancedFoliageActor || !IsValid(InstancedFoliageActor))
	{
		UE_LOG(LogTemp,Error,TEXT("Can not find InstancedFoliageActor!!!"))
		return false;
	}

	//遍历所有的FoliageInfos
	for (auto& FoliagePair:InstancedFoliageActor->GetFoliageInfos())
	{
		// 获取 FoliageInfo 引用 , 将 const 引用转换为非 const 引用
		FFoliageInfo& FoliageInfo =const_cast<FFoliageInfo&>(FoliagePair.Value.Get());
		// 创建一个用于存储要删除的实例索引的数组
		TArray<int32> InstanceIndicesToRemove;

		//遍历所有的Foliage实例，检查他们的UUID是否在要删除的数组中
		for (int32 i = 0; i<FoliageInfo.Instances.Num(); i++)
		{
			// 遍历 FoliageInfo 中的所有实例
			FGuid InstanceUUID = FoliageInfo.Instances[i].ProceduralGuid;
			// 检查 UUID 是否存在于要删除的 FoliageInstanceUUID 数组中
			if(FoliageInstanceUUID.Contains(InstanceUUID))
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



