// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Character.h"
#include "GGCharacter.generated.h"

/** TODO: create interface for clients to set server data from game save */
class UGGAnimatorComponent;
class UPaperFlipbookComponent;

UCLASS()
class GG_API AGGCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AGGCharacter(const FObjectInitializer& ObjectInitializer);

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// PostInitializeComponents is guaranteed to be called on all clients
	virtual void PostInitializeComponents() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

private:
	void MoveRight(float AxisValue);

	/**	=================================
	* 
	* Movement ability interface
	* 
	*/
public:
	/** Walljumping:
	* Wall jumping capability is implemented similar to normal jump as an "ability" per direction we face.
	* What exactly is a wall jump:
	* A wall jump occurs when the player presses jump button next to a wall while not on standing on ground,
	* the player is forced to travel upwards and opposite to the wall for a short while, before regaining
	* air control.
	*
	* In this implementation, a full wall jump is separated into lateral motion and Z motion.  While the 
	* execution of occurs at same time, their duration differs and lateral motion can't be controlled.
	*
	* NOTE: We MUST be able to hold jump key for longer jump in order for wall jump to make sense
	*/
	//	Flags indicating wall jump direction
	UPROPERTY(VisibleAnywhere, Category = GGCharacter)
        uint8 bPressedWallJumpRight : 1;
	UPROPERTY(VisibleAnywhere, Category = GGCharacter)
        uint8 bPressedWallJumpLeft : 1;
	UPROPERTY(VisibleAnywhere, Category = GGCharacter)
        uint8 bModeWallJump : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = GGCharacter)
		float WallJumpMaxHoldTimeVertical;
	/**
	* The defined time lateral movement is fixed in a wall jump. Different from vertical velocity
	* from jumping, the time of lateral velocity being enforced is not controllable.
	*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = GGCharacter)
        float NormalWallJumpMinHoldTimeLateral;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = GGCharacter)
		float NormalWallJumpMaxHoldTimeLateral;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = GGCharacter)
		float DashWallJumpMaxHoldTimeLateral;

	float WallJumpLateralHoldTime;
	
	/**
	* Override to check whether we want to wall jump or just jump, this merely sends the instruction to
	* the CharacterMovementComponent, all actual movement update details are done therein as an "ability"
	*/

	virtual void Jump() override;

	virtual void StopJumping() override;
	/**	Identical interface with Jump */
	UFUNCTION(BlueprintCallable, Category = "Pawn|Character|GGCharacter")
		bool CanWallJump() const;
	
protected:
	virtual bool CanJumpInternal_Implementation() const override;
public:
	virtual bool IsJumpProvidingForce() const;	// Don't like JumpKeyHoldTime > 0.f, change to >= 0.f

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Pawn|Character|GGCharacter", meta = (DisplayName = "CanWallJump"))
		bool CanWallJumpInternal() const;
	virtual bool CanWallJumpInternal_Implementation() const;

public:
	/**
	* Dashing produces 2 possible state, on ground and mid-air. While on ground, the character's collision
	* capsule is in a crouched state, we use the built-in CharacterMovementComponent function for that.
	* While in mid-air, all jump related functions turns into Dash form if the dash button is held with normal
	* collision capsule
	*/

	/** Input flag */
	uint8 bIsDashKeyDown : 1;
	/**	Replicated flag */
	uint8 bPerformDashedAction : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = GGCharacter)
		float DashMaxDuration;
	float TimeDashedFor;

	UFUNCTION(BlueprintCallable, Category = "Pawn|Character|GGCharacter")
		virtual void Dash();
	UFUNCTION(BlueprintCallable, Category = "Pawn|Character|GGCharacter")
		virtual void StopDashing();

	UFUNCTION(BlueprintCallable, Category = "Pawn|Character|GGCharacter")
		bool CanDash() const;
protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Pawn|Character|GGCharacter", meta = (DisplayName = "CanDash"))
		bool CanDashInternal() const;
	virtual bool CanDashInternal_Implementation() const;

	/**
	* Interacting functions from CharacterMovementComponent to check for ability usage
	*/
public:
	/** Super trigger jump if jump button has been pressed, override to also check other abilities */
	virtual void CheckJumpInput(float DeltaTime) override;

	/** Super reset jump input state after having checked input, override to also clear other abilities */
	virtual void ClearJumpInput() override;
	
	/**
	* Character locomotion utility
	*/
	virtual float GetWallJumpMaxHoldTimeVertical() const;	//	Full duration of wall jump
	virtual float GetNormalWallJumpMaxHoldTimeLateral() const;
	virtual float GetDashWallJumpMaxHoldTimeLateral() const;
	virtual float GetDashMaxDuration() const;

private:	// Handy for directional wall check by CharacterMovementComponent etc
	FVector Right;
	FVector Left;
protected:
	
	/**
	* Local field, we save the last non-zero player input direction so that we can tell where the 
	* Character wants to face.
	*/
	FVector LastActualMovementInput;
	//	Override to cache actual input
	virtual void AddMovementInput(FVector WorldDirection, float ScaleValue = 1.f, bool bForce= false) override;
	float AimLevel;
	virtual void AddAimInput(float ScaleValue = 0.f);

public:
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="GG|View")
	FVector GetPlanarForwardVector() const;

    /**
    * Entity damage receiving interface
    * Damage originate from damage dealing components, on damage checking they have access to the actor they are trying 
    * to deal damage to, which will be here for the case of character. Consequently, the AActor level object (Character)
    * passes on such an event to the damage handling component, networked.
    * TODO: enable information extraction from int32 DamageData, perhaps with a struct wrapper.
    */
    
    //  Server function, called by the locally controlled client who identifies himself to have taken damage
    UFUNCTION(reliable, Server, WithValidation, Category="GG|GGDamage")
        void ServerReceiveDamage(int32 DamageData);
        bool ServerReceiveDamage_Validate(int32 DamageData);
        void ServerReceiveDamage_Implementation(int32 DamageData);
    
    //  Multicast function. called by server to let simulated proxies know this client has take damage
    UFUNCTION(unreliable, NetMulticast, Category="GG|GGDamage")
        void MulticastReceiveDamage(int32 DamageData);
    void MulticastReceiveDamage_Implementation(int32 DamageData);
    
    virtual void ReceiveDamage(int32 DamageData);
    
    static FName BodyFlipbookComponentName;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GG|Animation")
        UPaperFlipbookComponent* BodyFlipbookComponent;
    static FName AnimatorComponentName;
    
    //UGGAnimatorComponent* AnimatorComponent;
};
