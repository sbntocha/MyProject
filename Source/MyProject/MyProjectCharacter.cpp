// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MyProject.h"
#include "Kismet/HeadMountedDisplayFunctionLibrary.h"
#include "MyProjectCharacter.h"

//////////////////////////////////////////////////////////////////////////
// AMyProjectCharacter

AMyProjectCharacter::AMyProjectCharacter()
{
	// Replicates
	bReplicates = true;
	bReplicateMovement = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 90.0f);

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

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 200.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

												// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

												   // Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
												   // are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void AMyProjectCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &AMyProjectCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMyProjectCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AMyProjectCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AMyProjectCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AMyProjectCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AMyProjectCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AMyProjectCharacter::OnResetVR);

	// Handle Simulated Physics
	PlayerInputComponent->BindAction("Physics", IE_Pressed, this, &AMyProjectCharacter::Physics);
}


void AMyProjectCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AMyProjectCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void AMyProjectCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void AMyProjectCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AMyProjectCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AMyProjectCharacter::MoveForward(float Value)
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

void AMyProjectCharacter::MoveRight(float Value)
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

void AMyProjectCharacter::Tick(float DeltaTime)
{
	if (bInRagdoll)
	{
		if (!bRecovering)
		{
			//Lying Trace (LT)
			FCollisionQueryParams LT_TraceParams = FCollisionQueryParams(FName(TEXT("LT_Trace")), true, this);
			LT_TraceParams.bTraceComplex = true;
			LT_TraceParams.bTraceAsyncScene = true;
			LT_TraceParams.bReturnPhysicalMaterial = false;

			//Re-initialize hit info
			FHitResult LT_Hit(ForceInit);

			//call GetWorld() from within an actor extending class
			if (GetWorld()->LineTraceSingleByChannel(
				LT_Hit,        //result
				GetMesh()->GetSocketLocation("Hips"),    //start
				(GetMesh()->GetSocketLocation("Hips") + (FVector(0.f, 0.f, -1.f) * 50.f)), //end
				ECC_Visibility,//ECC_Pawn, //collision channel
				LT_TraceParams)
				)
			{
				CapsuleLocation = LT_Hit.Location + FVector(0.f, 0.f, 90.f);
				//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("True"));
			}

			else
			{
				CapsuleLocation = GetMesh()->GetSocketLocation("Hips") + FVector(0.f, 0.f, 90.f);
				//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("False"));
			}

			LyingLocation = FMath::VInterpTo(LyingLocation, CapsuleLocation, GetWorld()->DeltaTimeSeconds, 30.f);
			GetCapsuleComponent()->SetWorldLocation(LyingLocation, false, nullptr, ETeleportType::None);

			//Forward Trace (FT)
			FCollisionQueryParams FT_TraceParams = FCollisionQueryParams(FName(TEXT("FT_Trace")), true, this);
			FT_TraceParams.bTraceComplex = true;
			FT_TraceParams.bTraceAsyncScene = true;
			FT_TraceParams.bReturnPhysicalMaterial = false;

			//Re-initialize hit info
			FHitResult FT_Hit(ForceInit);
			CapsuleRotation = FRotator(0.f, 0.f, 0.f);
			//call GetWorld() from within an actor extending class
			if (GetWorld()->LineTraceSingleByChannel(
				FT_Hit,        //result
				GetMesh()->GetSocketLocation("Hips"),    //start
				GetMesh()->GetSocketLocation("Hips") + (FRotationMatrix(GetMesh()->GetSocketRotation("Hips")).GetUnitAxis(EAxis::Z) * 50), //end
				ECC_Visibility, //collision channel
				FT_TraceParams)
				)
			{
				SectionName = FName("StandUp_Forward");
				CapsuleRotation = FRotator(0, 0, 0);
				CapsuleRotation.Yaw = GetMesh()->GetBoneAxis("Hips", EAxis::Y).Rotation().Yaw + 180;

				GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Blue, TEXT("Forward"));

			}
			else
			{
				SectionName = FName("Default");
				CapsuleRotation = FRotator(0, 0, 0);
				CapsuleRotation.Yaw = GetMesh()->GetBoneAxis("Hips", EAxis::Y).Rotation().Yaw;

				GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::White, TEXT("Back"));
				//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, CapsuleRotation.ToString());
				//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, (FRotationMatrix(GetMesh()->GetSocketRotation("Hips")).GetUnitAxis(EAxis::Z) * 50).ToString());
				//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::White, (FRotationMatrix(GetMesh()->GetSocketRotation("Hips")).GetScaledAxis(EAxis::Z) * 50).ToString());
			}
			GetCapsuleComponent()->SetWorldRotation(CapsuleRotation);
			//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::White, (FRotationMatrix(GetMesh()->GetSocketRotation("Hips")).GetScaledAxis(EAxis::Y) ).ToString());
		}
		else
		{
			PhysicsAlpha = FMath::FInterpTo(PhysicsAlpha, 0.f, GetWorld()->DeltaTimeSeconds, 5.f);
			GetMesh()->SetAllBodiesBelowPhysicsBlendWeight("Hips", PhysicsAlpha, false, true);
		}
	}

	else
	{
		LyingLocation = (GetMesh()->GetSocketLocation("Hips") + FVector(0.f, 0.f, 90.f));
	}


}

void AMyProjectCharacter::Physics()
{

	if (!bInRagdoll)
	{
		GetMesh()->SetAllBodiesBelowSimulatePhysics("Hips", true);
		GetMesh()->SetAllBodiesBelowPhysicsBlendWeight("Hips", 1.f, false, true);
		bInRagdoll = true;
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
	}
	else if (!bRecovering)
	{
		PhysicsAlpha = 1.f;
		bRecovering = true;
		GetMesh()->AnimScriptInstance->Montage_Play(StandUp_Back_Montage, 1.f);
		GetMesh()->AnimScriptInstance->Montage_JumpToSection(SectionName);
		int32 SectionIndex = StandUp_Back_Montage->GetSectionIndex(GetMesh()->AnimScriptInstance->Montage_GetCurrentSection());
		GetWorldTimerManager().SetTimer(RecoverDelay, this, &AMyProjectCharacter::Recovered, StandUp_Back_Montage->GetSectionLength(SectionIndex), false);
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, FString::FromInt(SectionIndex));
	}

}

void AMyProjectCharacter::OnTheGround()
{
	if (!bKnocked && !bInRagdoll)
	{
		GetMesh()->SetAllBodiesBelowSimulatePhysics("Hips", true);
		GetMesh()->AnimScriptInstance->Montage_Play(StandUp_Back_Montage, 1.f);
		GetMesh()->AnimScriptInstance->Montage_JumpToSection("KnockDown");
		bKnocked = true;
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
		GetWorldTimerManager().SetTimer(KnockDownTimer, this, &AMyProjectCharacter::KnockDown, StandUp_Back_Montage->GetSectionLength(2), false);
	}
	else if (!bRecovering)
	{

	}
}

void AMyProjectCharacter::KnockDown()
{
	GetWorldTimerManager().ClearTimer(KnockDownTimer);
	//GetMesh()->AnimScriptInstance->Montage_Stop(.2f, StandUp_Back_Montage);
	//GetMesh()->SetAllBodiesBelowSimulatePhysics("Hips", true);
	//GetMesh()->SetAllBodiesBelowPhysicsBlendWeight("Hips", 1.f, false, true);
	//bInRagdoll = true;
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::White, TEXT("Caido"));
}

void AMyProjectCharacter::Recovered()
{
	GetWorldTimerManager().ClearTimer(RecoverDelay);
	bRecovering = false;
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
	bInRagdoll = false;
	GetMesh()->SetAllBodiesBelowSimulatePhysics("Hips", false);
	//GetMesh()->AnimScriptInstance->Montage_Stop(0.2f);

	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Recovered"));
	//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, bRecovering ? TEXT("true") : TEXT("false"));
}
