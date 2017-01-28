// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Engine.h"
#include "GameFramework/Character.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "MyProjectCharacter.generated.h"

UCLASS(config = Game)
class AMyProjectCharacter : public ACharacter
{
	GENERATED_BODY()

		/** Camera boom positioning the camera behind the character */
		UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class UCameraComponent* FollowCamera;
public:
	AMyProjectCharacter();

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

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

public:

	UPROPERTY(VisibleAnywhere, Category = "Ragdoll")
		bool bInRagdoll = false;

	UPROPERTY(VisibleAnywhere, Category = "Ragdoll")
		bool bRecovering;

	UPROPERTY(VisibleAnywhere, Category = "KnockDown")
		bool bKnocked = false;

	UPROPERTY(VisibleAnywhere, Category = "Ragdoll")
		FVector LyingLocation;

	UPROPERTY(VisibleAnywhere, Category = "Ragdoll")
		FVector CapsuleLocation;

	FTimerHandle RecoverDelay;
	FTimerHandle KnockDownTimer;

	UPROPERTY(EditAnywhere, Category = "StandUp")
		UAnimMontage* StandUp_Back_Montage;

	UPROPERTY(EditAnywhere, Category = "StandUp")
		FName SectionName;

	UPROPERTY(EditAnywhere, Category = "StandUp")
		float PhysicsAlpha;

	UPROPERTY()
		FRotator CapsuleRotation;
	FVector CapsuleVector;
	float YawFloat;
	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	//UFUNCTION()
	//void OnOverlap(class UPrimitiveComponent* HitComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
	UFUNCTION()
		void Physics();

	UFUNCTION()
		void Recovered();

	UFUNCTION()
		void OnTheGround();

	UFUNCTION()
		void KnockDown();
};
