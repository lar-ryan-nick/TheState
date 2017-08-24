// Fill out your copyright notice in the Description page of Project Settings.

#include "FirstAttempt.h"
#include "FirstAttemptGameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "ThirdPersonCharacter.h"
#include "EnemySpawner.h"

AFirstAttemptGameModeBase::AFirstAttemptGameModeBase()
{
	DefaultPawnClass = AThirdPersonCharacter::StaticClass();
}

void AFirstAttemptGameModeBase::BeginPlay()
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemySpawner::StaticClass(), FoundActors);
	for (int i = 0; i < FoundActors.Num(); i++)
	{
		AEnemySpawner *EnemyMaker = Cast<AEnemySpawner>(FoundActors[i]);
		if (EnemyMaker)
		{
			//SpawnVolumeActors.AddUnique(SpawnVolumeActor);
			EnemyMaker->SetSpawningActive(true);
		}
	}
	if (HUDWidgetClass != nullptr)
	{
		CurrentWidget = CreateWidget<UUserWidget>(GetWorld(), HUDWidgetClass);
		if (CurrentWidget != nullptr)
		{
			CurrentWidget->AddToViewport();
		}
	}
	GetWorldTimerManager().SetTimer(TimeElapsedHandle, this, &AFirstAttemptGameModeBase::IncrementTimeElapsed, 1, true);
}

int AFirstAttemptGameModeBase::GetKillCount()
{
	return KillCount;
}

FString AFirstAttemptGameModeBase::GetTimeElapsed()
{
	FString TimeElapsedString;
	int Minutes = TimeElapsed / 60;
	if (Minutes < 10)
	{
		TimeElapsedString.Append(TEXT("0"));
	}
	TimeElapsedString.Append(FString::FromInt(Minutes) + TEXT(":"));
	int Seconds = TimeElapsed % 60;
	if (Seconds < 10)
	{
		TimeElapsedString.Append(TEXT("0"));
	}
	TimeElapsedString.Append(FString::FromInt(Seconds));
	return TimeElapsedString;
}

void AFirstAttemptGameModeBase::EndGame()
{
	GetWorldTimerManager().ClearTimer(TimeElapsedHandle);
}
