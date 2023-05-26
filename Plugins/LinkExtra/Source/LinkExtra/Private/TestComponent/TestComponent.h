// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TestComponent.generated.h"

//@作者：李林科
//@用途：測試
UCLASS(ClassGroup=(Custom),
	HideCategories=(Tags, Activation, Cooking, AssetUserData,Collision),meta=(BlueprintSpawnableComponent))
class LINKEXTRA_API UTestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UTestComponent();
	//EditAnywhere允许你在编辑器中任何地方查看和编辑这个属性。
	//BlueprintReadWrite允许你在蓝图中获取和设置这个属性。
	//Category = "My Category"在编辑器的详细面板中将这个属性归类到"My Category"分类中。
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Parameters",DisplayName="布尔值")
	bool TestBoolean;

	//这是一个浮点数
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Parameters",DisplayName="浮点数")
	float TestFloat;

	//这是一个整形
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parameters",DisplayName="整形")
	int TestInt;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	bool LastFrameBoolean;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
