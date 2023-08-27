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

	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
	BoxCollision->SetupAttachment(RootComponent);
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
		Niagara->SetPaused(false);
		Niagara->Activate(true);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Tick is now disabled"));
		Niagara->Deactivate();
	}
}

// Called every frame
void AWaterFall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if(bIsTicking )
	{
		UE_LOG(LogTemp, Warning, TEXT("I am here !!!"));
		if(Niagara)
		{
			UNiagaraDataInterface* DataInterface = Niagara->GetDataInterface(TEXT("Niagara"));
			
		}
	}

}


