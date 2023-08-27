// Fill out your copyright notice in the Description page of Project Settings.


#include "WaterFall.h"


// Sets default values
AWaterFall::AWaterFall()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bIsTicking = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	Niagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Niagara"));
	Niagara->SetupAttachment(RootComponent);

}

// Called when the game starts or when spawned
void AWaterFall::BeginPlay()
{
	Super::BeginPlay();
	
}

void AWaterFall::ToggleTick()
{
	bIsTicking = !bIsTicking;
	SetActorTickEnabled(bIsTicking);
	if(bIsTicking)
	{
		UE_LOG(LogTemp, Warning, TEXT("Tick is now enabled"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Tick is now disabled"));
	}
}

// Called every frame
void AWaterFall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if(bIsTicking || Niagara->IsActive())
	{
		if(Niagara)
		{
			UNiagaraDataInterface* DataInterface = Niagara->GetDataInterface(TEXT("Niagara"));
			UE_LOG(LogTemp, Warning, TEXT("I am here !!!"));
		}
	}

}


