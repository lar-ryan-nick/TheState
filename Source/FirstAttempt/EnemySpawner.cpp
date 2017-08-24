// Fill out your copyright notice in the Description page of Project Settings.

#include "FirstAttempt.h"
#include "EnemySpawner.h"
#include "Kismet/KismetMathLibrary.h"
#include "ThirdPersonCharacter.h"


// Sets default values
AEnemySpawner::AEnemySpawner()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	WhereToSpawn = CreateDefaultSubobject<UBoxComponent>(TEXT("WhereToSpawn"));
	RootComponent = WhereToSpawn;
	MinSpawnDelay = 2.5;
	MaxSpawnDelay = 5;
}

// Called when the game starts or when spawned
void AEnemySpawner::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AEnemySpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

FVector AEnemySpawner::GetRandomPointInVolume()
{
	FVector SpawnOrigin = WhereToSpawn->Bounds.Origin;
	FVector SpawnExtent = WhereToSpawn->Bounds.BoxExtent;
	return UKismetMathLibrary::RandomPointInBoundingBox(SpawnOrigin, SpawnExtent);
}

void AEnemySpawner::SpawnPickup()
{
	if (WhatToSpawn != NULL)
	{
		UWorld *World = GetWorld();
		if (World)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = Instigator;
			FVector SpawnLocation = GetRandomPointInVolume();
			
			FRotator SpawnRotation;
			SpawnRotation.Yaw = FMath::FRand() * 360;
			SpawnRotation.Roll = 0;
			SpawnRotation.Pitch = 0;
			const AThirdPersonCharacter *SpawnedPickup = World->SpawnActor<AThirdPersonCharacter>(WhatToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
			SpawnDelay = FMath::FRandRange(MinSpawnDelay, MaxSpawnDelay);
			GetWorldTimerManager().SetTimer(SpawnTimer, this, &AEnemySpawner::SpawnPickup, SpawnDelay, false);
		}
	}
}

void AEnemySpawner::SetSpawningActive(bool bShouldSpawn)
{
	if (bShouldSpawn)
	{
		SpawnDelay = FMath::FRandRange(MinSpawnDelay, MaxSpawnDelay);
		GetWorldTimerManager().SetTimer(SpawnTimer, this, &AEnemySpawner::SpawnPickup, SpawnDelay, false);
	}
	else
	{
		GetWorldTimerManager().ClearTimer(SpawnTimer);
	}
}
