// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/GameModeBase.h"
#include "FirstAttemptGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class FIRSTATTEMPT_API AFirstAttemptGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
	
public:
	AFirstAttemptGameModeBase();

	FORCEINLINE void IncrementKillCount(int Amount) { KillCount += Amount; }
	UFUNCTION(BlueprintPure, Category = "Score")
	int GetKillCount();

	FORCEINLINE void IncrementTimeElapsed() { TimeElapsed++; }
	UFUNCTION(BlueprintPure, Category = "Score")
	FString GetTimeElapsed();
	UFUNCTION(BlueprintCallable, Category = "GameEnd")
	void EndGame();
protected:
	virtual void BeginPlay();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD", meta = (BlueprintProtected = "true"))
	TSubclassOf<class UUserWidget> HUDWidgetClass;
	UPROPERTY()
	class UUserWidget *CurrentWidget;

private:
	int KillCount;
	int TimeElapsed;
	FTimerHandle TimeElapsedHandle;
};
