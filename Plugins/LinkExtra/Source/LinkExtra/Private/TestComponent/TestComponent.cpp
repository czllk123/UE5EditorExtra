// Fill out your copyright notice in the Description page of Project Settings.


#include "TestComponent.h"


// Sets default values for this component's properties
UTestComponent::UTestComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	
	//“允许这个组件在每一帧都被tick”。
	PrimaryComponentTick.bCanEverTick = true;

	//在构造函数中，设置头文件声明的属性的默认值
	TestBoolean = true;
	TestFloat = 1.0f;
	TestInt = 100;
}


// Called when the game starts
void UTestComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// 初始化LastFrameBoolean
	LastFrameBoolean = TestBoolean;
	
}


// Called every frame
void UTestComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	//Super代表父类，可以添加自定义行为，然后保留父类的行为
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 检查TestBoolean是否有变化
	if(TestBoolean!=LastFrameBoolean)
	{
		//如果TestBoolean改变了，打印一串字符
		UE_LOG(LogTemp,Warning,TEXT("TestBoolean has changed!"));

		//更新LastFrameBoolean
		LastFrameBoolean = TestBoolean;
	}
}

