// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "FirstAttempt.h"
#include "Kismet/HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/PawnSensingComponent.h"
#include "Blueprint/UserWidget.h"
#include "ThirdPersonCharacter.h"
#include "ThirdPersonVehicle.h"
#include "Projectile.h"
#include "EnemyController.h"
#include "FirstAttemptGameModeBase.h"

//////////////////////////////////////////////////////////////////////////
// AThirdPersonCharacter

AThirdPersonCharacter::AThirdPersonCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> CarMesh(TEXT("/Game/Mannequin/Character/Mesh/SK_Mannequin.SK_Mannequin"));
	GetMesh()->SetSkeletalMesh(CarMesh.Object);

	static ConstructorHelpers::FClassFinder<UObject> AnimBPClass(TEXT("/Game/Mannequin/Animations/ThirdPerson_AnimBP"));
	GetMesh()->SetAnimInstanceClass(AnimBPClass.Class);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(RootComponent);
	FirstPersonCameraComponent->RelativeLocation = FVector(-40,-15, 70); // Position the camera
	//FirstPersonCameraComponent->SetupAttachment(GetMesh(), FName("head"));
	//FirstPersonCameraComponent->RelativeLocation = FVector(25, 0, 0);
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

												// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

												   // Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
												   // are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)

	Sensor = CreateDefaultSubobject<USphereComponent>(TEXT("Sensor"));
	Sensor->SetSphereRadius(0, true);
	Sensor->SetupAttachment(GetMesh());

	//GetMesh()->AttachTo(RootComponent);
	
	//GetMesh()->OnComponentHit.AddDynamic(this, &AThirdPersonCharacter::OnHit);
	GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &AThirdPersonCharacter::OnOverlap);

	bIsShooting = false;
	bIsDead = false;
	bIsAiming = false;
	//GunOffset = FVector(150.f, -50.f, 50.f);
	FireRate = 0.3f;

	PawnSensingComponent = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("PawnSensingComponent"));
	//Set the peripheral vision angle to 90 degrees
	PawnSensingComponent->SetPeripheralVisionAngle(90.f);

	GetCharacterMovement()->MaxWalkSpeed = 1200;
}

//////////////////////////////////////////////////////////////////////////
// Input

void AThirdPersonCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);

	PlayerInputComponent->BindAction("SwitchCamera", IE_Pressed, this, &AThirdPersonCharacter::ChangePOV);

	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &AThirdPersonCharacter::SetOverShoulderPOV);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &AThirdPersonCharacter::SetThirdPersonPOV);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AThirdPersonCharacter::Sprint);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AThirdPersonCharacter::StopSprinting);

	PlayerInputComponent->BindAction("Shoot", IE_Pressed, this, &AThirdPersonCharacter::StartShooting);
	PlayerInputComponent->BindAction("Shoot", IE_Released, this, &AThirdPersonCharacter::StopShooting);

	PlayerInputComponent->BindAxis("HumanMoveForward", this, &AThirdPersonCharacter::MoveForward);
	PlayerInputComponent->BindAxis("HumanMoveRight", this, &AThirdPersonCharacter::MoveRight);

	PlayerInputComponent->BindAction("SwitchPawns", IE_Pressed, this, &AThirdPersonCharacter::SwitchPawns);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AThirdPersonCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AThirdPersonCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AThirdPersonCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AThirdPersonCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AThirdPersonCharacter::OnResetVR);
}

void AThirdPersonCharacter::BeginPlay()
{
	Super::BeginPlay();
	SetThirdPersonPOV();
	//Register the function that is going to fire when the character sees a Pawn
	if (PawnSensingComponent)
	{
		PawnSensingComponent->OnSeePawn.AddDynamic(this, &AThirdPersonCharacter::OnSeePlayer);
	}
	/*
	if (HUDWidgetClass != nullptr)
	{
		CurrentWidget = CreateWidget<UUserWidget>(GetWorld(), HUDWidgetClass);
		if (CurrentWidget != nullptr)
		{
			CurrentWidget->AddToViewport();
		}
	}
	*/
}


void AThirdPersonCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AThirdPersonCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void AThirdPersonCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void AThirdPersonCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AThirdPersonCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AThirdPersonCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AThirdPersonCharacter::MoveRight(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void AThirdPersonCharacter::SwitchPawns()
{
	TArray<AActor*> OverlappingActors;
	Sensor->GetOverlappingActors(OverlappingActors);
	for (int i = 0; i < OverlappingActors.Num(); i++)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Fuck"));
		APawn *Pawn = Cast<APawn>(OverlappingActors[i]);
		if (Pawn && Pawn != this)
		{
			//UE_LOG(LogTemp, Warning, TEXT("Ass"));
			AController* TempController = GetController();
			if (TempController)
			{
				//UE_LOG(LogTemp, Warning, TEXT("Bitch"));
				TempController->UnPossess();
				TempController->Possess(Pawn);
				AThirdPersonCharacter *Character = Cast<AThirdPersonCharacter>(Pawn);
				if (Character)
				{
					Character->GetUp();
				}
				/*
				else
				{
					AThirdPersonVehicle *Vehicle = Cast<AThirdPersonVehicle>(Pawn);
					if (Vehicle)
					{
						GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Ignore);
						GetMesh()->AttachTo(Vehicle->GetMesh());
						GetMesh()->RelativeLocation.Set(50, 50, 200);
						GetMesh()->SetSimulatePhysics(true);
					}
				}
				*/
			}
			break;
		}
	}
}

void AThirdPersonCharacter::Sprint()
{
	GetCharacterMovement()->MaxWalkSpeed = 2400;
}

void AThirdPersonCharacter::StopSprinting()
{
	GetCharacterMovement()->MaxWalkSpeed = 1200;
}

void AThirdPersonCharacter::OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//UE_LOG(LogTemp, Warning, TEXT("Your message"));
	if ((OverlappedComp != NULL) && (OtherActor != NULL) && (OtherActor != this) && (OtherComp != NULL) && OtherActor->GetVelocity().Size() > 30.0f)
	{
		StopShooting();
		GetCharacterMovement()->DisableMovement();
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Ignore);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		GetMesh()->SetSimulatePhysics(true);
		//GetMesh()->SetAllBodiesBelowSimulatePhysics(GetMesh()->GetBoneName(1), true);
		AFirstAttemptGameModeBase *GameMode = Cast<AFirstAttemptGameModeBase>(GetWorld()->GetAuthGameMode());
		if (this == UGameplayStatics::GetPlayerPawn(this, 0))
		{
			GetWorld()->GetTimerManager().SetTimer(GetUpHandle, this, &AThirdPersonCharacter::GetUp, 4, true);
			if (GameMode)
			{
				GameMode->EndGame();
			}
		}
		else if (!this->bIsDead)
		{
			if (GameMode)
			{
				GameMode->IncrementKillCount(1);
			}
		}
		bIsDead = true;
	}
}

void AThirdPersonCharacter::GetUp()
{
	if (!bIsDead && GetMesh()->IsSimulatingPhysics() && GetMesh()->GetPhysicsLinearVelocity().Size() == 0)
	{
		FRotator Rotation;
		Rotation.Yaw = -90;
		Rotation.Pitch = 0;
		Rotation.Roll = 0;
		GetMesh()->SetSimulatePhysics(false);
		GetMesh()->SetRelativeRotation(Rotation);
		//GetMesh()->RemoveFromRoot();
		//GetCapsuleComponent()->SetWorldLocation(FVector(GetCapsuleComponent()->RelativeLocation.X + GetMesh()->RelativeLocation.X, GetCapsuleComponent()->RelativeLocation.Y + GetMesh()->RelativeLocation.Y, GetCapsuleComponent()->RelativeLocation.Z + GetMesh()->RelativeLocation.Z));
		GetCapsuleComponent()->SetWorldLocation(FVector(GetMesh()->RelativeLocation.X, GetMesh()->RelativeLocation.Y, GetMesh()->RelativeLocation.Z + 90));
		GetMesh()->RelativeLocation.Set(0, 0, -90);
		GetMesh()->AttachTo(RootComponent);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Block);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		GetWorld()->GetTimerManager().ClearTimer(GetUpHandle);
		/*
		static ConstructorHelpers::FObjectFinder<UAnimSequence> anim(TEXT("AnimSequence'/Game/Mannequin/Animations/ThirdPerson_GetUp.ThirdPerson_GetUp'"));
		static UAnimSequence *Anim = anim.Object;
		GetMesh()->PlayAnimation(Anim, false);
		*/
	}
}

bool AThirdPersonCharacter::GetIsShooting()
{
	return bIsShooting;
}

bool AThirdPersonCharacter::GetIsAiming()
{
	return bIsAiming;
}
/*
void AThirdPersonCharacter::SetIsShooting(bool NewIsShooting)
{
	bIsShooting = NewIsShooting;
}
*/
void AThirdPersonCharacter::StartShooting()
{
	bIsShooting = true;
	//CameraBoom->TargetArmLength = 0;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetWorld()->GetTimerManager().SetTimer(ShootingHandle, this, &AThirdPersonCharacter::FireShot, FireRate, true);
}

void AThirdPersonCharacter::StopShooting()
{
	bIsShooting = false;
	//CameraBoom->TargetArmLength = 300.0f;
	/*
	if (FollowCamera->IsActive())
	{
		GetCharacterMovement()->bUseControllerDesiredRotation = false;
		GetCharacterMovement()->bOrientRotationToMovement = true;
	}
	*/
	//GetWorld()->GetTimerManager().ClearTimer(ShootingHandle);
}



void AThirdPersonCharacter::FireShot()
{
	//UE_LOG(LogTemp, Warning, TEXT("%d"), bIsShooting);
	// If it's ok to fire again
	if (!bIsDead)
	{
		FRotator FireRotation = GetControlRotation();

		FRotator ProjectileRotation(0, FireRotation.Yaw, 0);

		FVector SpawnLocation = GetMesh()->GetBoneLocation(FName("hand_l")) + ProjectileRotation.RotateVector(FVector(50, 0, 0));

		//FVector SpawnLocation = GetActorLocation() + ProjectileRotation.RotateVector(GunOffset);

		UWorld* World = GetWorld();
		if (World != NULL)
		{
			// spawn the projectile
			AProjectile *Projectile = World->SpawnActor<AProjectile>(SpawnLocation, FireRotation);
			/*
			Projectile->GetProjectileMesh()->SetupAttachment(GetMesh(), FName("hand_l"));
			Projectile->GetProjectileMesh()->RelativeLocation.Set(10, 0, 0);
			Projectile->GetProjectileMesh()->DetachFromParent();
			*/
		}
		/*
		// try and play the sound if specified
		if (FireSound != nullptr)
		{
			UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
		}
		*/
		if (!bIsShooting)
		{
			GetWorld()->GetTimerManager().ClearTimer(ShootingHandle);
			if (FollowCamera->IsActive())
			{
				GetCharacterMovement()->bUseControllerDesiredRotation = false;
				GetCharacterMovement()->bOrientRotationToMovement = true;
			}
		}
	}
}

void AThirdPersonCharacter::ChangePOV()
{
	if (FirstPersonCameraComponent->IsActive())
	{
		FirstPersonCameraComponent->Deactivate();
		FollowCamera->Activate();
		if (!bIsShooting)
		{
			GetCharacterMovement()->bUseControllerDesiredRotation = false;
			GetCharacterMovement()->bOrientRotationToMovement = true;
		}
		/*
		if (CurrentWidget)
		{
			CurrentWidget->RemoveFromParent();
		}
		*/
	}
	else
	{
		FirstPersonCameraComponent->Activate();
		FollowCamera->Deactivate();
		GetCharacterMovement()->bUseControllerDesiredRotation = true;
		GetCharacterMovement()->bOrientRotationToMovement = false;
		/*
		if (HUDWidgetClass != nullptr)
		{
			CurrentWidget = CreateWidget<UUserWidget>(GetWorld(), HUDWidgetClass);
			if (CurrentWidget != nullptr && !CurrentWidget->IsVisible())
			{
				CurrentWidget->AddToViewport();
			}
		}
		*/
	}
}

void AThirdPersonCharacter::SetOverShoulderPOV()
{
	if (FollowCamera->IsActive())
	{
		bIsAiming = true;
		ChangePOV();
	}
}

void AThirdPersonCharacter::SetThirdPersonPOV()
{
	if (FirstPersonCameraComponent->IsActive())
	{
		bIsAiming = false;
		ChangePOV();
	}
}

void AThirdPersonCharacter::OnSeePlayer(APawn* Pawn)
{
	AEnemyController *EnemyController = Cast<AEnemyController>(GetController());
	if (!bIsDead && EnemyController && Pawn == UGameplayStatics::GetPlayerPawn(this, 0))
	{
		EnemyController->SetFocus(Pawn);
		EnemyController->MoveToActor(Pawn);
		StartShooting();
	}
}