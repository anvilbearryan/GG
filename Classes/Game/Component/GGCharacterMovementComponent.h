// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/CharacterMovementComponent.h"
#include "GGCharacterMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class GG_API UGGCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:

	/**
	* Wall jumping implementation
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGCharacterMovementComponent|WallJump", meta = (DisplayName = "Wall-Jumpable distance", ClampMin = "0", UIMin = "0"))
		float MaxGapForWallJump;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGCharacterMovementComponent|WallJump", meta = (DisplayName = "Wall Jump Y Speed", ClampMin = "0", UIMin = "0"))
		float WallJumpLateralSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGCharacterMovementComponent|WallJump", meta = (DisplayName = "Dash Wall Jump Y Speed", ClampMin = "0", UIMin = "0"))
		float DashedWallJumpLateralSpeed;
	UPROPERTY(Category = "GGCharacterMovementComponent|WallJump", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Wall-Jump Z Velocity", ClampMin = "0", UIMin = "0"))
		float WallJumpZVelocity;
	
	// Utility check	
	virtual bool IsTouchingWall(const FVector &InDirection);
	virtual bool IsTouchingWall(const FVector &InDirection, const float Distance);

	virtual bool DoJump(bool bReplayingMoves) override;
	// Modified logic update called by CheckJumpInput, overrides the Y component of velocity
	virtual bool DoWallJump();

	/**
	* Wall sliding implementation
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GGCharacterMovementComponent|WallSlide")
		float WallSlideSpeed;
	/** NOT an ability repliacted flag, set using HandleImpact when acceleration bumps to wall */
	uint32 bWantsWallSlide : 1;

	/**	Modifies the terminal velocity when falling to incorporate wall sliding */
	virtual FVector NewFallVelocity(const FVector& InitialVelocity, const FVector& Gravity, float DeltaTime) const override;

	/** Override to clear bWantsWallSlide flag after update */
	virtual void PhysFalling(float deltaTime, int32 Iterations) override;

	/** Override to handle wall sliding */
	virtual void HandleImpact(const FHitResult& Hit, float TimeSlice = 0.f, const FVector& MoveDelta = FVector::ZeroVector) override;

	/** Override to return Crouch walk speed even mid-air */
	virtual float GetMaxSpeed() const override;

	/**
	* Replication consideration
	*/
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

	virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;
};

class FSavedMove_GGCharacter : public FSavedMove_Character
{
	typedef FSavedMove_Character Super;
public:
	uint32 bPressedWallJumpRight : 1;
	uint32 bPressedWallJumpLeft : 1;
	uint32 bPerformDashedAction : 1;
	
	///@brief Resets all saved variables.
	virtual void Clear() override;

	///@brief Store input commands in the compressed flags.
	virtual uint8 GetCompressedFlags() const override;

	///@brief This is used to check whether or not two moves can be combined into one.
	///Basically you just check to make sure that the saved variables are the same.
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const override;

	///@brief Sets up the move before sending it to the server. 
	virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const &NewAccel, class FNetworkPredictionData_Client_Character &ClientData) override;
	///@brief Sets variables on character movement component before making a predictive correction.
	virtual void PrepMoveFor(class ACharacter* Character) override;
};

class FNetworkPredictionData_Client_GGCharacter : public FNetworkPredictionData_Client_Character
{
	typedef FNetworkPredictionData_Client_Character Super;
public:
	FNetworkPredictionData_Client_GGCharacter(const UCharacterMovementComponent& ClientMovement)
		: FNetworkPredictionData_Client_Character(ClientMovement)
	{}

	/**	Causes custom FSavedMove object be allocated instead of default */
	virtual FSavedMovePtr AllocateNewMove() override;
};
