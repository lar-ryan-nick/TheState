// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameFramework/Character.h"
#include "ThirdPersonCharacter.generated.h"

UCLASS(config = Game)
class AThirdPersonCharacter : public ACharacter
{
	GENERATED_BODY()

		UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class UCameraComponent* FirstPersonCameraComponent;

		/** Camera boom positioning the camera behind the character */
		UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class UCameraComponent* FollowCamera;
public:
	AThirdPersonCharacter();

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		float BaseLookUpRate;

protected:

	/** Resets HMD orientation in VR. */
	void OnResetVR();

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/**
	* Called via input to turn at a given rate.
	* @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	*/
	void TurnAtRate(float Rate);

	/**
	* Called via input to turn look up/down at a given rate.
	* @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	*/
	void LookUpAtRate(float Rate);

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

	void SwitchPawns();

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface
	virtual void BeginPlay();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aim", meta = (BlueprintProtected = "true"))
	TSubclassOf<class UUserWidget> HUDWidgetClass;
	UPROPERTY()
	class UUserWidget *CurrentWidget;

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UFUNCTION()
	void Sprint();

	UFUNCTION()
	void StopSprinting();

	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void GetUp();

	UFUNCTION(BlueprintPure, Category = "Shooting")
	bool GetIsShooting();

	UFUNCTION(BlueprintPure, Category = "Shooting")
	bool GetIsAiming();
	/*
	UFUNCTION(BlueprintCallable, Category = "Shooting")
	void SetIsShooting(bool NewIsShooting);
	*/
	UFUNCTION()
	void StartShooting();

	UFUNCTION()
	void StopShooting();

	//UPROPERTY(Category = "Shooting", EditAnywhere, BlueprintReadWrite)
	//FVector GunOffset;

	/* How fast the weapon will fire */
	UPROPERTY(Category = "Shooting", EditAnywhere, BlueprintReadWrite)
	float FireRate;

	UFUNCTION()
	void FireShot();

	UFUNCTION(BlueprintCallable, Category = "Camera")
	void ChangePOV();

	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetOverShoulderPOV();

	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetThirdPersonPOV();

private:
	UPROPERTY()
	class USphereComponent *Sensor;

	struct FTimerHandle GetUpHandle;

	struct FTimerHandle ShootingHandle;

	UPROPERTY(EditAnywhere, Category = "Shooting")
	bool bIsShooting;

	UPROPERTY(EditAnywhere, Category = "Shooting")
	bool bIsAiming;

	UPROPERTY()
	class UPawnSensingComponent *PawnSensingComponent;

	UFUNCTION()
	void OnSeePlayer(APawn *Pawn);

	UPROPERTY()
	bool bIsDead;
};

